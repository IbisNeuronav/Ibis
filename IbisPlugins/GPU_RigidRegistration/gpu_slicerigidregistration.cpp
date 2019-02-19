/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Dante De Nigris for writing this class
#include "gpu_slicerigidregistration.h"
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include "vnl/algo/vnl_real_eigensystem.h"
#include "vtkSmartPointer.h"
#include "itkTimeProbesCollectorBase.h"
#include "itkCastImageFilter.h"

class SliceCommandIterationUpdateOpenCL : public itk::Command
{
public:
  typedef  SliceCommandIterationUpdateOpenCL   Self;
  typedef  itk::Command             Superclass;
  typedef itk::SmartPointer<Self>  Pointer;
  itkNewMacro( Self );

protected:
  SliceCommandIterationUpdateOpenCL() {};

public:

  typedef const GPU_SliceRigidRegistration::OptimizerType                         *OptimizerPointer;
  typedef const GPU_SliceRigidRegistration::GPUCostFunctionType                   *GPUConstCostFunctionPointer;
  typedef GPU_SliceRigidRegistration::ItkRigidTransformType::Pointer              ItkRigidTransformPointer;
  vtkTransform                                        * m_targetImageVtkTransform;
  vtkTransform                                        * m_vtktransform;
  vtkTransform                                        * m_parentTransform;
  bool                                                m_Debug;

  void SetDebug(bool debug)
  {
    m_Debug = debug;
  }

  void SetVtkTransform( vtkTransform * vtkTransform )
  {
      m_vtktransform = vtkTransform;
  }

  void SetTargetImageVtkTransform( vtkTransform * targetImageVtkTransform )
  {
      m_targetImageVtkTransform = targetImageVtkTransform;
  }

  void SetParentTransform( vtkTransform * transform )
  {
      m_parentTransform = transform;
  }

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }

  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    OptimizerPointer optimizer = dynamic_cast< OptimizerPointer >( object );
    GPUConstCostFunctionPointer   metric = dynamic_cast< GPUConstCostFunctionPointer >( optimizer->GetCostFunction() );

    if( ! itk::IterationEvent().CheckEvent( &event ) )
    {
        return;
    }

    if(m_Debug)
      std::cout << "Optimizer Value:\t" << optimizer->GetCurrentValue() << std::endl;

    ItkRigidTransformPointer itkTransform = GPU_SliceRigidRegistration::ItkRigidTransformType::New();

    GPU_SliceRigidRegistration::ItkRigidTransformType::ParametersType params = optimizer->GetCurrentPosition();
    GPU_SliceRigidRegistration::ItkRigidTransformType::CenterType center = metric->GetCenter();
    itkTransform->SetCenter(center);
    itkTransform->SetParameters(params);


    vtkTransform * vtktransform = m_vtktransform;

    vtkTransform * targetImageVtktransform = m_targetImageVtkTransform;


    GPU_SliceRigidRegistration::ItkRigidTransformType::MatrixType matrix =
    itkTransform->GetMatrix();

    GPU_SliceRigidRegistration::ItkRigidTransformType::OffsetType offset =
    itkTransform->GetOffset();

    vtkSmartPointer<vtkMatrix4x4> localMatrix_inv = vtkSmartPointer<vtkMatrix4x4>::New();

    for(unsigned int i=0; i<3; i++ )
    {
        for(unsigned int j=0; j<3; j++ )
        {
            localMatrix_inv->SetElement(i,j,
            matrix.GetVnlMatrix().get(i,j));
        }

        localMatrix_inv->SetElement( i, 3, offset[i]);
    }
    localMatrix_inv->Invert();

    vtkSmartPointer<vtkMatrix4x4> targetLocalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    targetImageVtktransform->GetMatrix( targetLocalMatrix );

    vtkSmartPointer<vtkMatrix4x4> finalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();

    if( m_parentTransform != 0 )
    {
        vtkSmartPointer<vtkMatrix4x4> parentWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
        m_parentTransform->GetInverse( parentWorldMatrix );
        finalMatrix->Multiply4x4( parentWorldMatrix, localMatrix_inv, localMatrix_inv );
    }

    finalMatrix->Multiply4x4( localMatrix_inv, targetLocalMatrix, finalMatrix );
    vtktransform->SetMatrix( finalMatrix );
    vtktransform->Modified();
  }
};

GPU_SliceRigidRegistration::GPU_SliceRigidRegistration( ) :
    m_OptimizationRunning(false),
    m_debug(false),
    m_useMask(false),
    m_percentile(0.8),
    m_initialSigma(1.0),
    m_gradientScale(1.0),
    m_numberOfPixels(16000),
    m_orientationSelectivity(2),
    m_populationSize(0),
    m_parentVtkTransform(0),
    m_sourceVtkTransform(0),
    m_targetVtkTransform(0),
    m_resultTransform(0),
    m_targetSpatialObjectMask(0),
    m_itkSourceImage(0),
    m_itkTargetImage(0),
    m_targetUSAcquisition(0)
{
    m_samplingStrategy = SamplingStrategy::RANDOM;
}

GPU_SliceRigidRegistration::~GPU_SliceRigidRegistration()
{
}


void GPU_SliceRigidRegistration::runRegistration()
{
    // Make sure all params have been specified
    if ( !m_targetUSAcquisition )
    {
        std::cerr << "Ultrasound Acquisition is invalid " << std::endl;
        std::cerr << "Use SetTargetUSAcquisition( )" << std::endl;
        return;
    }
//    IbisItkFloat3ImageType::Pointer itkTargetImage = m_itkTargetImage;

    if ( !m_itkSourceImage )
    {
        std::cerr << "Moving image is invalid " << m_itkSourceImage << std::endl;
        std::cerr << "Use SetItkSourceImage( )" << std::endl;
        return;
    }
    IbisItkFloat3ImageType::Pointer itkSourceImage = m_itkSourceImage;

    if ( m_sourceVtkTransform == 0 )
    {
        m_sourceVtkTransform = this->GetVtkTransformFromItkImage(itkSourceImage);
    }

    if ( m_targetVtkTransform == 0 )
    {
        m_targetVtkTransform = m_targetUSAcquisition->GetLocalTransform();
    }

    if ( m_resultTransform == 0 )
    {
        m_resultTransform = m_sourceVtkTransform;
    }

    vtkTransform * sourceVtkTransform = m_sourceVtkTransform;
    vtkTransform * targetVtkTransform = m_targetVtkTransform;

    itk::TimeProbesCollectorBase timer;


    if(m_debug)
    {
        std::cout << "Processing..(patience is a virtue)";
        std::cout << "Setting up registration..." << std::endl;
        timer.Start("Pre-processing");
    }

    // Registration
    ItkRigidTransformType::Pointer    itkTransform  = ItkRigidTransformType::New();
    
    OptimizerType::Pointer      optimizer     = OptimizerType::New();   

    double percentile =  m_percentile;
    double initialSigma = m_initialSigma;
    double gradientScale = m_gradientScale;
    unsigned int numberOfPixels = m_numberOfPixels;
    unsigned int orientationSelectivity = m_orientationSelectivity;
    unsigned int populationSize = m_populationSize;

    GPUMetricPointer metric = GPUMetricType::New();

    using ImageCastFilterUC2F = itk::CastImageFilter< IbisItkUnsignedChar3ImageType, IbisItkFloat3ImageType >;
    ImageCastFilterUC2F::Pointer caster = ImageCastFilterUC2F::New();
    IbisItkFloat3ImageType::PointType centerPoint;
    centerPoint.Fill(0);
    IbisItkFloat3ImageType::IndexType centerIndex;
    centerIndex[0] = m_targetUSAcquisition->GetSliceHeight() / 2;
    centerIndex[1] = m_targetUSAcquisition->GetSliceWidth() / 2;
    centerIndex[2] = 0;

    IbisItkFloat3ImageType::SizeType maskSize;
    IbisItkFloat3ImageType::SpacingType maskSpacing;

    metric->SetNumberOfSlices( m_targetUSAcquisition->GetNumberOfSlices() );
    for (int i = 0; i < m_targetUSAcquisition->GetNumberOfSlices(); ++i) {
       IbisItkFloat3ImageType::Pointer itkImage = IbisItkFloat3ImageType::New();
       IbisItkUnsignedChar3ImageType::Pointer itkUCImage = IbisItkUnsignedChar3ImageType::New();

       m_targetUSAcquisition->GetItkImage(itkUCImage, i, true, true); // maybe use calibrated transform?
       caster->SetInput(itkUCImage);
       caster->Update();
       itkImage = caster->GetOutput();
       metric->SetFixedSlice(i, itkImage);

       IbisItkFloat3ImageType::PointType currentPoint;
       itkImage->TransformIndexToPhysicalPoint(centerIndex, currentPoint);
       centerPoint[0] += currentPoint[0];
       centerPoint[1] += currentPoint[1];
       centerPoint[2] += currentPoint[2];

       if( i == 0){
           maskSpacing = itkImage->GetSpacing();
           maskSize = itkImage->GetLargestPossibleRegion().GetSize();
       }
    }

    centerPoint[0] = 0; //centerPoint[0] / m_targetUSAcquisition->GetNumberOfSlices();
    centerPoint[1] = 0; //centerPoint[1] / m_targetUSAcquisition->GetNumberOfSlices();
    centerPoint[2] = 0; //centerPoint[2] / m_targetUSAcquisition->GetNumberOfSlices();

//    vtkImageData * vtkMask = m_targetUSAcquisition->GetMask();
//    IbisItkFloat3ImageType::Pointer itkFMask = IbisItkFloat3ImageType::New();
//    IbisItkVtkConverter * itktovtkConverter = IbisItkVtkConverter::New();
//    vtkMatrix4x4 *tmpMat = vtkMatrix4x4::New();
//    itktovtkConverter->ConvertVtkImageToItkImage(itkFMask, vtkMask, tmpMat);
//    metric->SetFixedSliceMask(itkFMask);

    IbisItkFloat3ImageType::Pointer itkFMask = IbisItkFloat3ImageType::New();
    IbisItkFloat3ImageType::RegionType maskRegion;
    IbisItkFloat3ImageType::IndexType maskIndex;
    maskIndex.Fill(0);
    maskRegion.SetIndex(maskIndex);
    maskRegion.SetSize(maskSize);
    itkFMask->SetSpacing(maskSpacing);
    itkFMask->SetBufferedRegion(maskRegion);
    itkFMask->Allocate();
    itkFMask->FillBuffer(0);

    for(unsigned int r = 100; r < 350; r++)
      {
          for(unsigned int c = 240; c < 400; c++)
          {
              maskIndex[0] = c; maskIndex[1] = r;
              itkFMask->SetPixel(maskIndex, 1);
          }
      }
    metric->SetFixedSliceMask(itkFMask);

    metric->SetMovingImage(itkSourceImage);

//    if ( m_targetSpatialObjectMask )
//    {
//        std::cout << "Using mask" << std::endl;
//        metric->SetFixedImageMaskSpatialObject(m_targetSpatialObjectMask);
//        metric->SetUseImageMask(true);
//    }

    // Initialize Transform
    vtkSmartPointer<vtkMatrix4x4> localMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    sourceVtkTransform->GetInverse(localMatrix);

    vtkSmartPointer<vtkMatrix4x4> finalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    finalMatrix->Multiply4x4( targetVtkTransform->GetMatrix(), localMatrix, finalMatrix);

    ItkRigidTransformType::OffsetType offset;
 
    vnl_matrix<double> M(3,3); 

    for(unsigned int i=0; i<3; i++ )
     {
     for(unsigned int j=0; j<3; j++ )
       {
        M[i][j] = finalMatrix->GetElement(i,j);
       }     
      offset[i] = finalMatrix->GetElement(i,3);
      }

    double angleX, angleY, angleZ;
    angleX = vcl_asin(M[2][1]);
    double A = vcl_cos(angleX);
    if( vcl_fabs(A) > 0.00005 )
      {
      double x = M[2][2] / A;
      double y = -M[2][0] / A;
      angleY = vcl_atan2(y, x);

      x = M[1][1] / A;
      y = -M[0][1] / A;
      angleZ = vcl_atan2(y, x);
      }
    else
      {
      angleZ = 0;
      double x = M[0][0];
      double y = M[1][0];
      angleY = vcl_atan2(y, x);
      }

    ItkRigidTransformType::ParametersType params = ItkRigidTransformType::ParametersType(6);
    params[0] = angleX; params[1] = angleY; params[2] = angleZ;

    ItkRigidTransformType::CenterType center;
//    center[0] = itkTargetImage->GetOrigin()[0] + itkTargetImage->GetSpacing()[0] * itkTargetImage->GetBufferedRegion().GetSize()[0] / 2.0;
//    center[1] = itkTargetImage->GetOrigin()[1] + itkTargetImage->GetSpacing()[1] * itkTargetImage->GetBufferedRegion().GetSize()[1] / 2.0;
//    center[2] = itkTargetImage->GetOrigin()[2] + itkTargetImage->GetSpacing()[2] * itkTargetImage->GetBufferedRegion().GetSize()[2] / 2.0;
    center[0] = centerPoint[0];
    center[1] = centerPoint[1];
    center[2] = centerPoint[2];



    for( unsigned int i = 0; i < 3; i++ )
      {
      params[i+3] = offset[i] - center[i];
      for( unsigned int j = 0; j < 3; j++ )
        {
        params[i+3] += M[i][j] * center[j];
        }
      }

    itkTransform->SetCenter(center);
    itkTransform->SetParameters(params);

    metric->SetSamplingStrategy(m_samplingStrategy);
    metric->SetTransform( itkTransform );
    metric->SetNumberOfPixels( numberOfPixels );
    metric->SetPercentile( percentile );
    metric->SetN( orientationSelectivity );
//    metric->SetComputeMask( m_useMask );
    metric->SetMaskThreshold( 0.5 );
    metric->SetGradientScale( gradientScale );
    GPUCostFunctionPointer costFunction = GPUCostFunctionType::New();
    costFunction->SetGPUMetric( metric );
    costFunction->SetDebug(m_debug);

    metric->Update();

    if (m_debug)
    {
        timer.Stop("Pre-processing");
        timer.Start("Registration");
    }

    optimizer->SetCostFunction( costFunction ); 
    optimizer->SetInitialPosition( itkTransform->GetParameters() );
    OptimizerType::ScalesType   scales =  OptimizerType::ScalesType(itkTransform->GetNumberOfParameters());
//    scales[0] = 3500;  scales[1] = 3500;  scales[2] = 3500;
//    scales[3] = 0.1;  scales[4] = 0.1;  scales[5] = 0.1;
    scales[0] = 3500;  scales[1] = 3500;  scales[2] = 3500;
    scales[3] = 0.001;  scales[4] = 0.001;  scales[5] = 0.001;
    scales = scales * 100;
    optimizer->SetScales( scales );
    optimizer->SetUseCovarianceMatrixAdaptation( false );
    optimizer->SetUpdateBDPeriod( 0 );
    optimizer->SetValueTolerance(0.01);
    optimizer->SetMaximumDeviation( 1 );
//    optimizer->SetMinimumDeviation( 1 );
    optimizer->SetUseScales( true );
    optimizer->SetPopulationSize( populationSize );
    optimizer->SetNumberOfParents( 0 );
    optimizer->SetMaximumNumberOfIterations( 300 );
    optimizer->SetInitialSigma( initialSigma );

    SliceCommandIterationUpdateOpenCL::Pointer observer = SliceCommandIterationUpdateOpenCL::New();
    observer->SetVtkTransform( m_resultTransform );
    observer->SetTargetImageVtkTransform( targetVtkTransform );
    observer->SetParentTransform( m_parentVtkTransform );
    observer->SetDebug(m_debug);
    optimizer->AddObserver( itk::IterationEvent(), observer );

    if(m_debug)
      std::cout << "Starting registration..." << std::endl;

    m_OptimizationRunning = true;
    try
    {
      optimizer->StartOptimization();
      if (m_debug)
        std::cout << "Optimizer stop condition: " << optimizer->GetStopConditionDescription() << std::endl;
    }
    catch( itk::ExceptionObject & err )
    {
      std::cerr << "ExceptionObject caught !" << std::endl;
      std::cerr << err << std::endl;
    }

    if (m_debug)
        timer.Stop("Registration");

    if(m_debug)
    {
      std::cout << "Done." << std::endl;
      timer.Report(std::cout);
    }

    m_OptimizationRunning = false;


}


vtkTransform * GPU_SliceRigidRegistration::GetVtkTransformFromItkImage(IbisItkFloat3ImageType::Pointer itkImage, bool load_translation)
{
    vnl_matrix<double> vnlMatrix = itkImage->GetDirection().GetVnlMatrix();
    vtkSmartPointer<vtkMatrix4x4> vtkMatrix = vtkMatrix4x4::New();

    for (int i = 0; i < vnlMatrix.rows(); ++i) {
        if (load_translation)
        {
            vtkMatrix->SetElement(i,3, itkImage->GetOrigin()[i]);
        }
        for (int j = 0; j < vnlMatrix.cols() ; ++j) {
            vtkMatrix->SetElement(i,j, vnlMatrix[i][j]);
        }
    }
    vtkTransform * vtktransform = vtkTransform::New();
    vtktransform->SetMatrix(vtkMatrix);
    return vtktransform;
}