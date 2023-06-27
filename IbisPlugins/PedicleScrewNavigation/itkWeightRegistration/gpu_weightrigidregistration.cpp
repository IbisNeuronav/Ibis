/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "gpu_weightrigidregistration.h"

#include <itkTimeProbesCollectorBase.h>
#include <vnl/algo/vnl_real_eigensystem.h>
#include <vnl/algo/vnl_symmetric_eigensystem.h>

class CommandIterationUpdateWeightOpenCL : public itk::Command
{
public:
    typedef CommandIterationUpdateWeightOpenCL Self;
    typedef itk::Command Superclass;
    typedef itk::SmartPointer<Self> Pointer;
    itkNewMacro( Self );

protected:
    CommandIterationUpdateWeightOpenCL(){};

public:
    typedef const GPU_WeightRigidRegistration::OptimizerType * OptimizerPointer;
    typedef const GPU_WeightRigidRegistration::GPUCostFunctionType * GPUConstCostFunctionPointer;
    typedef GPU_WeightRigidRegistration::ItkRigidTransformType::Pointer ItkRigidTransformPointer;
    vtkTransform * m_targetImageVtkTransform;
    vtkTransform * m_vtktransform;
    vtkTransform * m_parentTransform;
    bool m_Debug;

    void SetDebug( bool debug ) { m_Debug = debug; }

    void SetVtkTransform( vtkTransform * vtktransform ) { m_vtktransform = vtktransform; }

    void SetTargetImageVtkTransform( vtkTransform * targetImageVtkTransform )
    {
        m_targetImageVtkTransform = targetImageVtkTransform;
    }

    void SetParentTransform( vtkTransform * transform ) { m_parentTransform = transform; }

    void Execute( itk::Object * caller, const itk::EventObject & event )
    {
        Execute( (const itk::Object *)caller, event );
    }

    void Execute( const itk::Object * object, const itk::EventObject & event )
    {
        OptimizerPointer optimizer         = dynamic_cast<OptimizerPointer>( object );
        GPUConstCostFunctionPointer metric = dynamic_cast<GPUConstCostFunctionPointer>( optimizer->GetCostFunction() );

        if( !itk::IterationEvent().CheckEvent( &event ) )
        {
            return;
        }

        if( m_Debug )
            std::cout << "Step " << optimizer->GetCurrentIteration() << " - Optimizer Value:\t"
                      << optimizer->GetCurrentValue() << " [ Intensity: " << metric->GetCurrentIntensityMetricValue()
                      << " + Gradient: " << metric->GetCurrentGradientMetricValue() << " ]" << std::endl;

        ItkRigidTransformPointer itkTransform = GPU_WeightRigidRegistration::ItkRigidTransformType::New();

        GPU_WeightRigidRegistration::ItkRigidTransformType::ParametersType params = optimizer->GetCurrentPosition();
        GPU_WeightRigidRegistration::ItkRigidTransformType::CenterType center     = metric->GetCenter();
        itkTransform->SetCenter( center );
        itkTransform->SetParameters( params );

        vtkTransform * vtktransform = m_vtktransform;

        GPU_WeightRigidRegistration::ItkRigidTransformType::MatrixType matrix = itkTransform->GetMatrix();

        GPU_WeightRigidRegistration::ItkRigidTransformType::OffsetType offset = itkTransform->GetOffset();

        vtkSmartPointer<vtkMatrix4x4> localMatrix_inv = vtkSmartPointer<vtkMatrix4x4>::New();

        for( unsigned int i = 0; i < 3; i++ )
        {
            for( unsigned int j = 0; j < 3; j++ )
            {
                localMatrix_inv->SetElement( i, j, matrix.GetVnlMatrix().get( i, j ) );
            }

            localMatrix_inv->SetElement( i, 3, offset[i] );
        }
        localMatrix_inv->Invert();

        vtkSmartPointer<vtkMatrix4x4> finalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();

        if( m_parentTransform != 0 )
        {
            vtkSmartPointer<vtkMatrix4x4> parentWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
            m_parentTransform->GetInverse( parentWorldMatrix );
            finalMatrix->Multiply4x4( parentWorldMatrix, localMatrix_inv, localMatrix_inv );
        }

        vtktransform->SetMatrix( localMatrix_inv );
        vtktransform->Modified();
    }
};

GPU_WeightRigidRegistration::GPU_WeightRigidRegistration()
    : m_OptimizationRunning( false ),
      m_debug( false ),
      m_initialSigma( 1.0 ),
      m_populationSize( 0 ),
      m_parentVtkTransform( 0 ),
      m_sourceVtkTransform( 0 ),
      m_targetVtkTransform( 0 ),
      m_resultTransform( 0 ),
      m_itkSourceImage( 0 ),
      m_itkTargetImage( 0 ),
      m_useMask( false ),
      m_orientationNumberOfPixels( 16000 ),
      m_orientationPercentile( 0.8 ),
      m_orientationSelectivity( 2 ),
      m_targetSpatialObjectMask( 0 ),
      m_lambdaMetricBalance( 0.5 ),
      m_orientationSamplingStrategy( OrientationSamplingStrategy::RANDOM ),
      m_registrationMetricToUse( RegistrationMetricToUseType::INTENSITY )
{
}

GPU_WeightRigidRegistration::~GPU_WeightRigidRegistration() {}

void GPU_WeightRigidRegistration::runRegistration()
{
    // Make sure all params have been specified
    if( !m_itkTargetImage )
    {
        std::cerr << "Fixed image is invalid " << m_itkTargetImage << std::endl;
        std::cerr << "Use SetItkTargetImage( )" << std::endl;
        return;
    }
    IbisItkFloat3ImageType::Pointer itkTargetImage = m_itkTargetImage;

    if( !m_itkSourceImage )
    {
        std::cerr << "Moving image is invalid " << m_itkSourceImage << std::endl;
        std::cerr << "Use SetItkSourceImage( )" << std::endl;
        return;
    }
    IbisItkFloat3ImageType::Pointer itkSourceImage = m_itkSourceImage;

    if( m_sourceVtkTransform == 0 )
    {
        m_sourceVtkTransform = vtkTransform::New();
    }

    if( m_targetVtkTransform == 0 )
    {
        m_targetVtkTransform = vtkTransform::New();
    }

    if( m_resultTransform == 0 )
    {
        m_resultTransform = m_sourceVtkTransform;
    }

    vtkTransform * sourceVtkTransform = m_sourceVtkTransform;
    vtkTransform * targetVtkTransform = m_targetVtkTransform;

    itk::TimeProbesCollectorBase timer;

    if( m_debug )
    {
        std::cout << "Processing..(patience is a virtue)";
        std::cout << "Setting up registration..." << std::endl;
        timer.Start( "Pre-processing" );
    }

    // Registration
    ItkRigidTransformType::Pointer itkTransform = ItkRigidTransformType::New();

    OptimizerType::Pointer optimizer = OptimizerType::New();

    double initialSigma         = m_initialSigma;
    unsigned int populationSize = m_populationSize;

    // Initialize Transform
    vtkSmartPointer<vtkMatrix4x4> finalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    sourceVtkTransform->GetInverse( finalMatrix );

    ItkRigidTransformType::OffsetType offset;

    vnl_matrix<double> M( 3, 3 );

    for( unsigned int i = 0; i < 3; i++ )
    {
        for( unsigned int j = 0; j < 3; j++ )
        {
            M[i][j] = finalMatrix->GetElement( i, j );
        }
        offset[i] = finalMatrix->GetElement( i, 3 );
    }

    double angleX, angleY, angleZ;
    angleX   = vcl_asin( M[2][1] );
    double A = vcl_cos( angleX );
    if( vcl_fabs( A ) > 0.00005 )
    {
        double x = M[2][2] / A;
        double y = -M[2][0] / A;
        angleY   = vcl_atan2( y, x );

        x      = M[1][1] / A;
        y      = -M[0][1] / A;
        angleZ = vcl_atan2( y, x );
    }
    else
    {
        angleZ   = 0;
        double x = M[0][0];
        double y = M[1][0];
        angleY   = vcl_atan2( y, x );
    }

    ItkRigidTransformType::ParametersType params = ItkRigidTransformType::ParametersType( 6 );
    params[0]                                    = angleX;
    params[1]                                    = angleY;
    params[2]                                    = angleZ;

    ItkRigidTransformType::CenterType center;
    center[0] = itkTargetImage->GetOrigin()[0] +
                itkTargetImage->GetSpacing()[0] * itkTargetImage->GetBufferedRegion().GetSize()[0] / 2.0;
    center[1] = itkTargetImage->GetOrigin()[1] +
                itkTargetImage->GetSpacing()[1] * itkTargetImage->GetBufferedRegion().GetSize()[1] / 2.0;
    center[2] = itkTargetImage->GetOrigin()[2] +
                itkTargetImage->GetSpacing()[2] * itkTargetImage->GetBufferedRegion().GetSize()[2] / 2.0;

    for( unsigned int i = 0; i < 3; i++ )
    {
        params[i + 3] = offset[i] - center[i];
        for( unsigned int j = 0; j < 3; j++ )
        {
            params[i + 3] += M[i][j] * center[j];
        }
    }

    itkTransform->SetCenter( center );
    itkTransform->SetParameters( params );

    GPUCostFunctionPointer costFunction = GPUCostFunctionType::New();
    costFunction->SetFixedImage( itkTargetImage );
    costFunction->SetMovingImage( itkSourceImage );
    costFunction->SetMetricTransform( itkTransform );

    costFunction->SetLambda( m_lambdaMetricBalance );

    // set gradient orientation parameters
    costFunction->SetOrientationSamplingStrategy( m_orientationSamplingStrategy );
    costFunction->SetOrientationPercentile( m_orientationPercentile );
    costFunction->SetOrientationUseMask( m_useMask );
    costFunction->SetOrientationSelectivity( m_orientationSelectivity );
    costFunction->SetOrientationNumberOfPixels( m_orientationNumberOfPixels );
    if( m_targetSpatialObjectMask )
    {
        costFunction->SetFixedSpatialObjectImageMask( m_targetSpatialObjectMask );
        costFunction->SetOrientationUseMask( true );
    }

    costFunction->SetDebug( m_debug );
    costFunction->SetRegistrationMetricToUse( m_registrationMetricToUse );

    costFunction->UpdateMetrics();

    if( m_debug )
    {
        timer.Stop( "Pre-processing" );
        timer.Start( "Registration" );
    }

    optimizer->SetCostFunction( costFunction );
    optimizer->SetInitialPosition( itkTransform->GetParameters() );
    OptimizerType::ScalesType scales = OptimizerType::ScalesType( itkTransform->GetNumberOfParameters() );
    scales[0]                        = 3500;
    scales[1]                        = 3500;
    scales[2]                        = 3500;
    scales[3]                        = 0.1;
    scales[4]                        = 0.1;
    scales[5]                        = 0.1;
    optimizer->SetScales( scales );
    optimizer->SetUseCovarianceMatrixAdaptation( true );
    optimizer->SetUpdateBDPeriod( 0 );
    optimizer->SetValueTolerance( 0.0001 );
    optimizer->SetUseScales( true );
    optimizer->SetPopulationSize( populationSize );
    optimizer->SetNumberOfParents( populationSize / 2 );
    optimizer->SetMaximumNumberOfIterations( 300 );
    optimizer->SetInitialSigma( initialSigma );

    CommandIterationUpdateWeightOpenCL::Pointer observer = CommandIterationUpdateWeightOpenCL::New();
    observer->SetVtkTransform( m_resultTransform );
    observer->SetTargetImageVtkTransform( targetVtkTransform );
    observer->SetParentTransform( m_parentVtkTransform );
    observer->SetDebug( m_debug );
    optimizer->AddObserver( itk::IterationEvent(), observer );

    if( m_debug ) std::cout << "Starting registration..." << std::endl;

    m_OptimizationRunning = true;
    try
    {
        optimizer->StartOptimization();
        if( m_debug )
            std::cout << "Optimizer stop condition: " << optimizer->GetStopConditionDescription() << std::endl;
    }
    catch( itk::ExceptionObject & err )
    {
        std::cerr << "ExceptionObject caught !" << std::endl;
        std::cerr << err << std::endl;
    }

    if( m_debug ) timer.Stop( "Registration" );

    if( m_debug )
    {
        std::cout << "Done." << std::endl;
        timer.Report( std::cout );
    }

    m_OptimizationRunning = false;
}
