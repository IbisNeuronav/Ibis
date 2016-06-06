/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

/** This class is a slight modification of the original ITK class:
 * MultiResolutionImageRegistrationMethod.
 * The original copyright message is pasted here, which includes also
 * the version information: */
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date: 2008-11-06 16:31:54 +0100 (Thu, 06 Nov 2008) $
  Version:   $Revision: 2637 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkMultiResolutionImageGradientRegistrationMethod_txx
#define _itkMultiResolutionImageGradientRegistrationMethod_txx

#include "itkMultiResolutionImageGradientRegistrationMethod.h"
#include "itkMultiResolutionDiscreteGaussianDerivativePyramidImageFilter.h"
#include "itkContinuousIndex.h"
#include "vnl/vnl_math.h"

namespace itk
{

/*
 * Constructor
 */
template < typename TFixedImage, typename TMovingImage >
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::MultiResolutionImageGradientRegistrationMethod()
{
  this->SetNumberOfRequiredOutputs( 1 );  // for the Transform

  this->m_FixedImage   = 0; // has to be provided by the user.
  this->m_MovingImage  = 0; // has to be provided by the user.
  this->m_Transform    = 0; // has to be provided by the user.
  this->m_Interpolator = 0; // has to be provided by the user.
  this->m_Metric       = 0; // has to be provided by the user.
  this->m_Optimizer    = 0; // has to be provided by the user.

  // Use MultiResolutionPyramidImageFilter as the default
  // image pyramids.
  this->m_FixedImagePyramid  = FixedImageGaussianDerivativePyramidType::New();
  this->m_MovingImagePyramid = MovingImageGaussianDerivativePyramidType::New();

  this->m_NumberOfLevels = 1;
  this->m_CurrentLevel = 0;

  this->m_Stop = false;

  this->m_InitialTransformParameters = ParametersType(1);
  this->m_InitialTransformParametersOfNextLevel = ParametersType(1);
  this->m_LastTransformParameters = ParametersType(1);

  this->m_InitialTransformParameters.Fill( 0.0f );
  this->m_InitialTransformParametersOfNextLevel.Fill( 0.0f );
  this->m_LastTransformParameters.Fill( 0.0f );


  TransformOutputPointer transformDecorator =
                 static_cast< TransformOutputType * >(
                                  this->MakeOutput(0).GetPointer() );

  this->ProcessObject::SetNthOutput( 0, transformDecorator.GetPointer() );

} // end Constructor


/*
 * Initialize by setting the interconnects between components.
 */
template < typename TFixedImage, typename TMovingImage >
void
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::Initialize( void ) throw (ExceptionObject)
{

  // Sanity checks
  if ( !this->m_Metric )
    {
    itkExceptionMacro(<<"Metric is not present" );
    }
  if( !CheckForOrientationMatchingMetric(m_OrientationMatchingMetric) )
    {
    itkExceptionMacro(<<"Metric must be Orientation Matching Metric");
    }

  if ( !this->m_Optimizer )
    {
    itkExceptionMacro(<<"Optimizer is not present" );
    }

  if( !this->m_Transform )
    {
    itkExceptionMacro(<<"Transform is not present");
    }

  if( !this->m_Interpolator )
    {
    itkExceptionMacro(<<"Interpolator is not present");
    }

  //ArrayType kernelNorms;
  std::cout << "SetMovingImage" << std::endl;
  m_OrientationMatchingMetric->SetMovingImage( this->m_MovingImagePyramid->GetOutput(this->m_CurrentLevel) );
  std::cout << "SetFixedImage" << std::endl;
  m_OrientationMatchingMetric->SetFixedImage( this->m_FixedImagePyramid->GetOutput(this->m_CurrentLevel) );
  std::cout << "SetFixedMaskImage" << std::endl;
  m_OrientationMatchingMetric->SetFixedMaskImage( this->m_FixedImagePyramid->GetMaskImage(this->m_CurrentLevel) );
  std::cout << "SetMovingGradientImage" << std::endl;
  m_OrientationMatchingMetric->SetMovingGradientImage( this->m_MovingImagePyramid->GetGradientImage(this->m_CurrentLevel) );
  std::cout << "SetFixedGradientImage" << std::endl;
  m_OrientationMatchingMetric->SetFixedGradientImage( this->m_FixedImagePyramid->GetGradientImage(this->m_CurrentLevel) );

  this->m_Metric->SetTransform( this->m_Transform );
  this->m_Metric->SetInterpolator( this->m_Interpolator );
  std::cout << "SetFixedImageRegion" << std::endl;
  this->m_Metric->SetFixedImageRegion( this->m_FixedImageRegionPyramid[ this->m_CurrentLevel ] );
  std::cout << "Initialize" << std::endl;
  this->m_Metric->Initialize();
    
  // Setup the optimizer
  std::cout << "Optimizer" << std::endl;
  this->m_Optimizer->SetCostFunction( this->m_Metric );
  this->m_Optimizer->SetInitialPosition( this->m_InitialTransformParametersOfNextLevel );

  //
  // Connect the transform to the Decorator.
  //
  TransformOutputType * transformOutput =
     static_cast< TransformOutputType * >( this->ProcessObject::GetOutput(0) );

  transformOutput->Set( this->m_Transform.GetPointer() );

} // end Initialize()

template < typename TFixedImage, typename TMovingImage >
bool
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::CheckForGaussianDerivativePyramids(
		  FixedImageGaussianDerivativePyramidPointer & fixedPyramid,
		  MovingImageGaussianDerivativePyramidPointer & movingPyramid
		  ) const
{
  /** Check if fixed image pyramid is a gaussian derivative pyramid */
	FixedImageGaussianDerivativePyramidType * testPtr1
		= dynamic_cast<FixedImageGaussianDerivativePyramidType *>( this->m_FixedImagePyramid.GetPointer() );
  bool fixedPyramidIsGaussianDerivative = false;
  if ( testPtr1 )
  {
    fixedPyramidIsGaussianDerivative = true;
    fixedPyramid = testPtr1;
  }

  /** Check if moving image pyramid is a gaussian derivative pyramid */
	MovingImageGaussianDerivativePyramidType * testPtr2
    = dynamic_cast<MovingImageGaussianDerivativePyramidType *>( this->m_MovingImagePyramid.GetPointer() );
  bool movingPyramidIsGaussianDerivative = false;
  if ( testPtr2 )
  {
    movingPyramidIsGaussianDerivative = true;
    movingPyramid = testPtr2;
  }

  return (fixedPyramidIsGaussianDerivative & movingPyramidIsGaussianDerivative);

} // end CheckForBSplineTransform()

template < typename TFixedImage, typename TMovingImage >
bool
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::CheckForOrientationMatchingMetric(
		  OrientationMatchingMetricPointer & metric
		  ) const
{
	OrientationMatchingMetricType * testPtr1
		= dynamic_cast<OrientationMatchingMetricType *>( this->m_Metric.GetPointer() );
  bool metricIsOrientationMatching = false;
  if ( testPtr1 )
  {
	  metricIsOrientationMatching = true;
	  metric = testPtr1;
  }

  return (metricIsOrientationMatching);

} // end CheckForBSplineTransform()


/*
 * Stop the Registration Process
 */
template < typename TFixedImage, typename TMovingImage >
void
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::StopRegistration( void )
{
  this->m_Stop = true;
}


/*
 * Stop the Registration Process
 */
template < typename TFixedImage, typename TMovingImage >
void
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::PreparePyramids( void )
{

  if( !this->m_Transform )
    {
    itkExceptionMacro(<<"Transform is not present");
    }

  this->m_InitialTransformParametersOfNextLevel = this->m_InitialTransformParameters;

  if ( this->m_InitialTransformParametersOfNextLevel.Size() !=
       this->m_Transform->GetNumberOfParameters() )
    {
    itkExceptionMacro(<<"Size mismatch between initial parameter and transform");
    }

  // Sanity checks
  if( !this->m_FixedImage )
    {
    itkExceptionMacro(<<"FixedImage is not present");
    }

  if( !this->m_MovingImage )
    {
    itkExceptionMacro(<<"MovingImage is not present");
    }

  if( !this->m_FixedImagePyramid )
    {
    itkExceptionMacro(<<"Fixed image pyramid is not present");
    }

  if( !this->m_MovingImagePyramid )
    {
    itkExceptionMacro(<<"Moving image pyramid is not present");
    }

  // Setup the fixed image pyramid
  this->m_FixedImagePyramid->SetNumberOfLevels( this->m_NumberOfLevels );
  this->m_FixedImagePyramid->SetInput( this->m_FixedImage );
  this->m_FixedImagePyramid->UpdateLargestPossibleRegion();

  // Setup the moving image pyramid
  this->m_MovingImagePyramid->SetNumberOfLevels( this->m_NumberOfLevels );
  this->m_MovingImagePyramid->SetInput( this->m_MovingImage );
  this->m_MovingImagePyramid->SetComputeMask( false );
  this->m_MovingImagePyramid->UpdateLargestPossibleRegion();

  typedef typename FixedImageRegionType::SizeType         SizeType;
  typedef typename FixedImageRegionType::IndexType        IndexType;
  typedef typename FixedImagePyramidType::ScheduleType    ScheduleType;

  ScheduleType schedule = this->m_FixedImagePyramid->GetSchedule();

  SizeType  inputSize  = this->m_FixedImageRegion.GetSize();
  IndexType inputStart = this->m_FixedImageRegion.GetIndex();
  IndexType inputEnd = inputStart;
  for ( unsigned int dim = 0; dim < TFixedImage::ImageDimension; dim++)
  {
    inputEnd[dim] += ( inputSize[dim]-1 );
  }

  this->m_FixedImageRegionPyramid.reserve( this->m_NumberOfLevels );
  this->m_FixedImageRegionPyramid.resize( this->m_NumberOfLevels );

  typedef typename FixedImageType::PointType PointType;
  typedef typename PointType::CoordRepType CoordRepType;
  typedef typename IndexType::IndexValueType IndexValueType;
  typedef typename SizeType::SizeValueType SizeValueType;
  typedef ContinuousIndex<CoordRepType, TFixedImage::ImageDimension> CIndexType;

  PointType inputStartPoint;
  PointType inputEndPoint;
  this->m_FixedImage->TransformIndexToPhysicalPoint(inputStart, inputStartPoint);
  this->m_FixedImage->TransformIndexToPhysicalPoint(inputEnd, inputEndPoint);

  for ( unsigned int level=0; level < this->m_NumberOfLevels; level++ )
  {
    FixedImageType * fixedImageAtLevel = this->m_FixedImagePyramid->GetOutput(level);
    this->m_FixedImageRegionPyramid[ level ] = fixedImageAtLevel->GetBufferedRegion();
  }

} // end PreparePyramids()


/*
 * Starts the Registration Process
 */
template < typename TFixedImage, typename TMovingImage >
void
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::StartRegistration( void )
{

  // StartRegistration is an old API from before
  // this egistrationMethod was a subclass of ProcessObject.
  // Historically, one could call StartRegistration() instead of
  // calling Update().  However, when called directly by the user, the
  // inputs to the RegistrationMethod may not be up to date.  This
  // may cause an unexpected behavior.
  //
  // Since we cannot eliminate StartRegistration for backward
  // compability reasons, we check whether StartRegistration was
  // called directly or whether Update() (which in turn called
  // StartRegistration()).
  if (!this->m_Updating)
    {
    this->Update();
    }
  else
    {
    this->m_Stop = false;

    this->PreparePyramids();

    for ( this->m_CurrentLevel = 0; this->m_CurrentLevel < this->m_NumberOfLevels;
          this->m_CurrentLevel++ )
      {

      // Invoke an iteration event.
      // This allows a UI to reset any of the components between
      // resolution level.
      this->InvokeEvent( IterationEvent() );

      // Check if there has been a stop request
      if ( this->m_Stop )
        {
        break;
        }

      try
        {
        // initialize the interconnects between components
        this->Initialize();
        }
      catch( ExceptionObject& err )
        {
        this->m_LastTransformParameters = ParametersType(1);
        this->m_LastTransformParameters.Fill( 0.0f );

        // pass exception to caller
        throw err;
        }

      try
        {
        // do the optimization
        this->m_Optimizer->StartOptimization();
        }
      catch( ExceptionObject& err )
        {
        // An error has occurred in the optimization.
        // Update the parameters
        this->m_LastTransformParameters = this->m_Optimizer->GetCurrentPosition();

        // Pass exception to caller
        throw err;
        }

      // get the results
      this->m_LastTransformParameters = this->m_Optimizer->GetCurrentPosition();
      this->m_Transform->SetParameters( this->m_LastTransformParameters );

      // setup the initial parameters for next level
      if ( this->m_CurrentLevel < this->m_NumberOfLevels - 1 )
        {
        this->m_InitialTransformParametersOfNextLevel =
          this->m_LastTransformParameters;
        }

      // Remove pyramid output of current level to release memory
      this->m_FixedImagePyramid->GetOutput( this->m_CurrentLevel )
        ->ReleaseData();
      this->m_MovingImagePyramid->GetOutput( this->m_CurrentLevel )
        ->ReleaseData();
      }
    }

} // end StartRegistration()


/*
 * PrintSelf
 */
template < typename TFixedImage, typename TMovingImage >
void
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );
  os << indent << "Metric: " << this->m_Metric.GetPointer() << std::endl;
  os << indent << "Optimizer: " << this->m_Optimizer.GetPointer() << std::endl;
  os << indent << "Transform: " << this->m_Transform.GetPointer() << std::endl;
  os << indent << "Interpolator: " << this->m_Interpolator.GetPointer() << std::endl;
  os << indent << "FixedImage: " << this->m_FixedImage.GetPointer() << std::endl;
  os << indent << "MovingImage: " << this->m_MovingImage.GetPointer() << std::endl;
  os << indent << "FixedImagePyramid: "
    << this->m_FixedImagePyramid.GetPointer() << std::endl;
  os << indent << "MovingImagePyramid: "
    << this->m_MovingImagePyramid.GetPointer() << std::endl;

  os << indent << "NumberOfLevels: " << this->m_NumberOfLevels << std::endl;
  os << indent << "CurrentLevel: " << this->m_CurrentLevel << std::endl;

  os << indent << "InitialTransformParameters: "
    << this->m_InitialTransformParameters << std::endl;
  os << indent << "InitialTransformParametersOfNextLevel: "
    << this->m_InitialTransformParametersOfNextLevel << std::endl;
  os << indent << "LastTransformParameters: "
    << this->m_LastTransformParameters << std::endl;
  os << indent << "FixedImageRegion: "
    << this->m_FixedImageRegion << std::endl;

  for( unsigned int level = 0; level < this->m_FixedImageRegionPyramid.size(); level++ )
  {
    os << indent << "FixedImageRegion at level " << level << ": "
      << this->m_FixedImageRegionPyramid[ level ] << std::endl;
  }

} // end PrintSelf()


/*
 * Generate Data
 */
template < typename TFixedImage, typename TMovingImage >
void
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::GenerateData( void )
{
  this->StartRegistration();
}



template < typename TFixedImage, typename TMovingImage >
unsigned long
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::GetMTime( void ) const
{
  unsigned long mtime = Superclass::GetMTime();
  unsigned long m;


  // Some of the following should be removed once ivars are put in the
  // input and output lists

  if (this->m_Transform)
    {
    m = this->m_Transform->GetMTime();
    mtime = (m > mtime ? m : mtime);
    }

  if (this->m_Interpolator)
    {
    m = this->m_Interpolator->GetMTime();
    mtime = (m > mtime ? m : mtime);
    }

  if (this->m_Metric)
    {
    m = this->m_Metric->GetMTime();
    mtime = (m > mtime ? m : mtime);
    }

  if (this->m_Optimizer)
    {
    m = this->m_Optimizer->GetMTime();
    mtime = (m > mtime ? m : mtime);
    }

  if (this->m_FixedImage)
    {
    m = this->m_FixedImage->GetMTime();
    mtime = (m > mtime ? m : mtime);
    }

  if (this->m_MovingImage)
    {
    m = this->m_MovingImage->GetMTime();
    mtime = (m > mtime ? m : mtime);
    }

  return mtime;

} // end GetMTime()


/*
 *  Get Output
 */
template < typename TFixedImage, typename TMovingImage >
const typename MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>::TransformOutputType *
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::GetOutput() const
{
  return static_cast< const TransformOutputType * >( this->ProcessObject::GetOutput(0) );
}


template < typename TFixedImage, typename TMovingImage >
DataObject::Pointer
MultiResolutionImageGradientRegistrationMethod<TFixedImage,TMovingImage>
::MakeOutput(unsigned int output)
{
  switch (output)
    {
    case 0:
      return static_cast<DataObject*>(TransformOutputType::New().GetPointer());
      break;
    default:
      itkExceptionMacro("MakeOutput request for an output number larger than the expected number of outputs");
      return 0;
    }
}

} // end namespace itk


#endif
