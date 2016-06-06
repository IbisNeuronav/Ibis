/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

#ifndef _itkAdvancedOrientationMatchingBetaImageToImageMetric_txx
#define _itkAdvancedOrientationMatchingBetaImageToImageMetric_txx

#include "itkAdvancedOrientationMatchingBetaImageToImageMetric.h"
#include "itkImageFileWriter.h"
#include "vnl/algo/vnl_matrix_update.h"
#include "vnl/vnl_math.h"
#include "vnl/vnl_vector.h"
#include "vnl/vnl_cross.h"


namespace itk
{

/**
 * ******************* Constructor *******************
 */

template <class TFixedImage, class TMovingImage>
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::AdvancedOrientationMatchingBetaImageToImageMetric()
 {
	this->SetUseImageSampler( true );
	this->SetUseFixedImageLimiter( false );
	this->SetUseMovingImageLimiter( false );

	this->ComputeGradientOff();
	this->SetDebug(0);

	this->m_UseNormalization = false;
	this->m_NormalizationFactor = 1.0;

	this->m_Percentile = 0.9;

	this->m_FixedGradientImage = 0;
	this->m_MovingGradientImage = 0;

 } // end constructor

 /**
  * ********************* Initialize ****************************
  */

template <class TFixedImage, class TMovingImage>
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::Initialize(void) throw ( ExceptionObject )
 {
	/** Initialize transform, interpolator, etc. */
	Superclass::Initialize();

	if( !this->m_FixedGradientImage || !this->m_MovingGradientImage )
	{
	  itkExceptionMacro(<<"Gradient Images have not been defined")
	}
	
	if( !this->m_FixedMaskImage )
	{
	  itkExceptionMacro(<<"Fixed Mask Image has not been defined")
	}	

	m_MaskIsDefined = false;

	this->m_LocalSimilarityMeasureParameter = this->m_N;

        std::cout << "Setting up OM Function" << std::endl;
	this->m_OrientationMatchingFunction = OrientationMatchingFunctionType::New();
	this->m_OrientationMatchingFunction->SetInputImage(this->m_MovingGradientImage);
	this->m_OrientationMatchingFunction->SetLocalSimilarityMeasureParameter(this->m_LocalSimilarityMeasureParameter);
	this->m_OrientationMatchingFunction->SetTransform(this->m_AdvancedTransform);
	this->m_OrientationMatchingFunction->SetDebug(false);
	this->m_OrientationMatchingFunction->Initialize();
         std::cout << "Setting up OM Function" << std::endl;
  
        this->m_VectorInterpolator = VectorInterpolatorType::New();
        this->m_VectorInterpolator->SetInputImage(this->m_FixedGradientImage);
  
  
	/** Check if this transform is a B-spline transform. */
  /*
	BSplineTransformPointer localBSplineTransform = 0;
	bool transformIsBSpline = this->CheckForBSplineTransform( localBSplineTransform );
	if ( transformIsBSpline ) this->SetBSplineTransform( localBSplineTransform );
  */
  /** Computing Orientation Derivatives **/

  this->ComputeFixedMask();
 } // end Initialize

/**
 * ******************* PrintSelf *******************
 */

template < class TFixedImage, class TMovingImage>
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::PrintSelf(std::ostream& os, Indent indent) const
 {
	Superclass::PrintSelf( os, indent );

 } // end PrintSelf


/*
 * Compute the fixed mask.
 */
template <class TFixedImage, class TMovingImage>
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::ComputeFixedMask(void)
 {
  std::cout << "Computing Fixed Mask" << std::endl;
  ThresholdFilterPointer thFilter = ThresholdFilterType::New();

  thFilter->SetInput(this->m_FixedMaskImage);
  thFilter->SetLowerThreshold(0.1);
  thFilter->SetInsideValue(1);
  thFilter->SetOutsideValue(0);
  thFilter->UpdateLargestPossibleRegion();

  typedef ImageRegionIteratorWithIndex< FixedMaskImageType > MaskIteratorType;
  MaskIteratorType mit( thFilter->GetOutput(), thFilter->GetOutput()->GetBufferedRegion() );

  typename FixedImageMaskType::ConstPointer mask = this->GetFixedImageMask();
  unsigned long myCnt = 0;
  typename FixedMaskImageType::PointType itPoint;

  typename NNInterpolatorType::Pointer  nnInterpolator = NNInterpolatorType::New();
  if(!mask.IsNull())
  {
    //std::cout << "ComputeFixedMask:mask.." << std::endl << mask << std::endl;
    ImageMaskSpatialObjectConstPointer imso = dynamic_cast<const ImageMaskSpatialObjectType*>(mask.GetPointer());
    if(imso->GetSource())
    {
      imso->GetSource()->Update();
    }
    nnInterpolator->SetInputImage(imso->GetImage());

  }

  mit.GoToBegin();
  while(!mit.IsAtEnd())
  {
    if(mit.Get()>0)
    {

      if(!mask.IsNull())
      {
        thFilter->GetOutput()->TransformIndexToPhysicalPoint(mit.GetIndex(), itPoint);
        if (nnInterpolator->Evaluate( itPoint ))
        {
          myCnt++;
          mit.Set(1);
        }
        else
        {
          mit.Set(0);
        }
      }
      else
      {
        myCnt++;    
        mit.Set(1);      
      }
    }
    ++mit;
  }
  std::cout << "Resulting Nbr of Samples  : " << myCnt << std::endl;

  if(FixedImageDimension==2)
  {
    std::cout << "Writing mask image to Mask.tiff ..." << std::endl;
    typedef ImageFileWriter<FixedMaskImageType> WriterType;
    typename WriterType::Pointer  writer = WriterType::New();
    writer->SetInput(thFilter->GetOutput());
    writer->SetFileName("Mask.tiff");
    writer->Update();
    writer->Write();
    std::cout << "done" << std::endl;
  }
  else if(FixedImageDimension==3)
  {
    std::cout << "Writing mask image to Mask.mhd ..." << std::endl;
    typedef ImageFileWriter<FixedMaskImageType> WriterType;
    typename WriterType::Pointer  writer = WriterType::New();
    writer->SetInput(thFilter->GetOutput());
    writer->SetFileName("Mask.mhd");
    writer->Update();
    writer->Write();
    std::cout << "done" << std::endl;
  }

  this->m_MaskIsDefined = true;
 }


template <class TFixedImage, class TMovingImage>
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::GetRotationMatrix(
    const FixedImagePointType fixedPoint,
    SpatialJacobianType & rotation) const
{
  SpatialJacobianType spatialJacobian;
  this->m_AdvancedTransform->GetSpatialJacobian(fixedPoint, spatialJacobian);
  rotation = spatialJacobian.GetTranspose();
}

/**
 * ******************* GetValue *******************
 */

template <class TFixedImage, class TMovingImage>
typename AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>::MeasureType
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::GetValue( const TransformParametersType & parameters ) const
 {
	itkDebugMacro( "GetValue( " << parameters << " ) " );

	/** Initialize some variables */
	this->m_NumberOfPixelsCounted = 0;
	MeasureType measure = NumericTraits< MeasureType >::Zero;
	double normalizationFactor = 0;

	GradientJacobianType imageJacobian;
	TransformJacobianType jacobian;

	/** Make sure the transform parameters are up to date. */
	this->SetTransformParameters( parameters );

  this->m_OrientationMatchingFunction->SetInputImage(this->m_MovingGradientImage);
	this->m_OrientationMatchingFunction->SetTransform(this->m_AdvancedTransform);


	/** Update the imageSampler and get a handle to the sample container. */
	this->GetImageSampler()->Update();
	ImageSampleContainerPointer sampleContainer = this->GetImageSampler()->GetOutput();

	/** Create iterator over the sample container. */
	this->GetImageSampler()->Update();
	typename ImageSampleContainerType::ConstIterator fiter;
	typename ImageSampleContainerType::ConstIterator fbegin = sampleContainer->Begin();
	typename ImageSampleContainerType::ConstIterator fend = sampleContainer->End();

	SpatialJacobianType rotation;
	typename MovingImageType::SpacingType spacing = this->m_MovingImage->GetSpacing();

	/** Loop over the fixed image samples to calculate the local mutual information. */
	for ( fiter = fbegin; fiter != fend; ++fiter )
	{
		/** Read fixed coordinates and initialize some variables. */
		const FixedImagePointType & fixedPoint = (*fiter).Value().m_ImageCoordinates;
		MovingImagePointType mappedPoint;
		FixedImageIndexType  fixedIndex;

		/** Transform point and check if it is inside the bspline support region. */
		bool sampleOk = this->TransformPoint( fixedPoint, mappedPoint );
    sampleOk &= this->m_FixedImage->TransformPhysicalPointToIndex(
        fixedPoint, fixedIndex);

		/** Check if point is inside mask. */
		if ( sampleOk )
		{
			sampleOk = (this->IsInsideMovingMask( mappedPoint ) & 
			          this->m_OrientationMatchingFunction->IsInsideBuffer( mappedPoint ) &
                 this->m_VectorInterpolator->IsInsideBuffer(fixedPoint) );
		}

		/** Compute the moving image value and check if the point is
		 * inside the moving image buffer. */

		if ( sampleOk )
		{
			this->m_NumberOfPixelsCounted++;

			/** Get the fixed image value. */
			//const FixedGradientPixelType fixedGradient = this->m_FixedGradientImage->GetPixel(fixedIndex);
			const FixedGradientPixelType fixedGradient = this->m_VectorInterpolator->Evaluate(fixedPoint);

			if(!this->m_OrientationMatchingFunction)
			{
			  itkExceptionMacro(<<"this->m_OrientationMatchingFunction not defined")
			}
			double cost = -this->m_OrientationMatchingFunction->EvaluateValueAtPoint(fixedPoint, fixedGradient);
			measure += cost;
			//normalizationFactor += 1.0;
		} // end if sampleOk

		normalizationFactor += 1.0;
	} // end for loop over the image sample container

	/** Check if enough samples were valid. */
	this->CheckNumberOfSamples(
			sampleContainer->Size(), this->m_NumberOfPixelsCounted );

	/** Update measure value. */
  if( normalizationFactor > 0 )
  {
	  measure *= (1/normalizationFactor);
  }

	/** Return the mean squares measure value. */
	return measure;

 } // end GetValue


/**
 * ******************* GetDerivative *******************
 */

template < class TFixedImage, class TMovingImage>
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::GetDerivative( const TransformParametersType & parameters,
		DerivativeType & derivative ) const
{
  /** When the derivative is calculated, all information for calculating
  * the metric value is available. It does not cost anything to calculate
  * the metric value now. Therefore, we have chosen to only implement the
  * GetValueAndDerivative(), supplying it with a dummy value variable. */
  MeasureType dummyvalue = NumericTraits< MeasureType >::Zero;
  this->GetValueAndDerivative( parameters, dummyvalue, derivative );

} // end GetDerivative


/**
 * ******************* GetValueAndDerivative *******************
 */

template <class TFixedImage, class TMovingImage>
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::GetValueAndDerivative( const TransformParametersType & parameters,
		MeasureType & value, DerivativeType & derivative ) const
		{
	itkDebugMacro("GetValueAndDerivative( " << parameters << " ) ");

	typedef typename DerivativeType::ValueType        DerivativeValueType;
	typedef typename TransformJacobianType::ValueType TransformJacobianValueType;

	/** Initialize some variables. */
	this->m_NumberOfPixelsCounted = 0;
	MeasureType measure = NumericTraits< MeasureType >::Zero;
	derivative = DerivativeType( this->GetNumberOfParameters() );
	derivative.Fill( NumericTraits< DerivativeValueType >::Zero );

	/** Array that stores dM(x)/dmu, and the sparse jacobian+indices. */
	NonZeroJacobianIndicesType nzji( this->m_AdvancedTransform->GetNumberOfNonZeroJacobianIndices() );
	GradientJacobianType imageJacobian;
	TransformJacobianType jacobian;

	/** Make sure the transform parameters are up to date. */
	this->SetTransformParameters( parameters );

	/** Update the imageSampler and get a handle to the sample container. */
	this->GetImageSampler()->Update();
	ImageSampleContainerPointer sampleContainer = this->GetImageSampler()->GetOutput();

	/** Create iterator over the sample container. */
	typename ImageSampleContainerType::ConstIterator fiter;
	typename ImageSampleContainerType::ConstIterator fbegin = sampleContainer->Begin();
	typename ImageSampleContainerType::ConstIterator fend = sampleContainer->End();

	double normalizationFactor = 0;
	SpatialJacobianType rotation;

	typename MovingImageType::SpacingType spacing = this->m_MovingImage->GetSpacing();

	for ( fiter = fbegin; fiter != fend; ++fiter )
	{
		/** Read fixed coordinates and initialize some variables. */
		const FixedImagePointType & fixedPoint = (*fiter).Value().m_ImageCoordinates;
		MovingImagePointType      mappedPoint;
		MovingImageDerivativeType movingImageDerivative;
		FixedImageIndexType       fixedIndex;
		MovingImageDerivativeType preRotGradient, movingGradient;
		GradientGradientType      movingGradientGradient;

    /** Transform point and check if it is inside the bspline support region. */
    bool sampleOk = this->TransformPoint( fixedPoint, mappedPoint );
    sampleOk &= this->m_FixedImage->TransformPhysicalPointToIndex(
        fixedPoint, fixedIndex);

    /** Check if point is inside mask. */
    if ( sampleOk )
    {
      sampleOk = (this->IsInsideMovingMask( mappedPoint ) 
                & this->m_OrientationMatchingFunction->IsInsideBuffer( mappedPoint )
                & this->m_VectorInterpolator->IsInsideBuffer(fixedPoint) );
    }


    if ( sampleOk )
		{
			this->m_NumberOfPixelsCounted++;

			/** Get the fixed image value. */
			//const FixedGradientPixelType fixedGradient = this->m_FixedGradientImage->GetPixel(fixedIndex);
      const FixedGradientPixelType fixedGradient = this->m_VectorInterpolator->Evaluate(fixedPoint);
			double value;
			DerivativeType valueDerivative = DerivativeType( this->GetNumberOfParameters() );

      /*
      const MovingGradientPixelType orientationDerivative1 = this->m_ODInterpolator1->Evaluate( mappedPoint );
      MovingGradientPixelType orientationDerivative2;
      if(MovingImageDimension==3)
      {
        orientationDerivative2 = this->m_ODInterpolator2->Evaluate( mappedPoint );
      }
      */
      /*
			this->m_OrientationMatchingFunction->EvaluateValueAndDerivativeAtPoint(fixedPoint, fixedGradient, orientationDerivative1, orientationDerivative2, value, valueDerivative);
      measure += -value;
      derivative += -valueDerivative;
      */

      //std::cout << "valueDerivative:\t" << valueDerivative << std::endl;
      //std::cout << "derivative:\t" << derivative << std::endl;

			normalizationFactor += 1.0;

		} // end if sampleOk

	} // end for loop over the image sample container

	/** Check if enough samples were valid. */
	this->CheckNumberOfSamples(
			sampleContainer->Size(), this->m_NumberOfPixelsCounted );

	/** Compute the measure value and derivative. */
	measure *= (1/normalizationFactor);
	derivative *= (1/normalizationFactor);

	/** The return value. */
	value = measure;

} // end GetValueAndDerivative()


/**
 * *************** UpdateValueAndDerivativeTerms ***************************
 */

/*
template < class TFixedImage, class TMovingImage >
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::UpdateValueAndDerivativeTerms(
		const GradientPixelType fixedGradient,
		const GradientPixelType movingGradient,
		const GradientJacobianType & imageJacobian,
		const NonZeroJacobianIndicesType & nzji,
		MeasureType & measure,
		DerivativeType & deriv ) const
{
  typedef typename DerivativeType::ValueType        DerivativeValueType;

  if(fixedGradient.GetNorm() > 0 && movingGradient.GetNorm() > 0)
  {
  measure += -EvaluateLocalSimilarityMeasureValue(fixedGradient, movingGradient);

  double theta = angle(fixedGradient.GetVnlVector(), movingGradient.GetVnlVector() );
  double thetaDeriv1 = (-1.0 / sqrt(1-pow(cos(theta), 2.0)) );

  if(!isinf(thetaDeriv1))
  {
    double metricDerivative = EvaluateLocalSimilarityMeasureDerivative(fixedGradient, movingGradient);

    GradientPixelType dderiv1 = ( fixedGradient/ (fixedGradient.GetNorm()*movingGradient.GetNorm()) );
    GradientPixelType dderiv2 = (1.0 / (fixedGradient.GetNorm() * pow(movingGradient.GetNorm(), 3.0))) \
        * dot_product(fixedGradient.GetVnlVector(), movingGradient.GetVnlVector()) * movingGradient;

    if ( nzji.size() == this->GetNumberOfParameters() )
    {
      DerivativeType temp;
      temp.Fill(0);
      temp += - (metricDerivative) * thetaDeriv1* ( ( dderiv1.GetVnlVector() - dderiv2.GetVnlVector()) * imageJacobian);
      deriv += -(metricDerivative) * thetaDeriv1* ( ( dderiv1.GetVnlVector() - dderiv2.GetVnlVector()) * imageJacobian);
      if(isinf(deriv.magnitude()))
      {
        std::cout << "Derivative is Infinite! - 1" << std::endl;
        std::cout << "thetaDeriv1:\t" << thetaDeriv1 << std::endl;
        std::cout << "( dderiv1.GetVnlVector() - dderiv2.GetVnlVector()):\t" <<  ( dderiv1.GetVnlVector() - dderiv2.GetVnlVector()) << std::endl;
        exit(1);
      }
    }
    else
    {
      vnl_vector<double> diff = -(metricDerivative) * thetaDeriv1 * ( dderiv1.GetVnlVector() - dderiv2.GetVnlVector() );
      for ( unsigned int i = 0; i < imageJacobian.columns(); ++i )
      {
        const unsigned int index = nzji[ i ];
        deriv[ index ] += dot_product(diff, imageJacobian.get_column(i));
        if(isinf(dot_product(diff, imageJacobian.get_column(i))))
        {
          std::cout << "Derivative is Infinite! - 2" << std::endl;
          std::cout << "diff:\t" << diff << std::endl;
          std::cout << "thetaDeriv1:\t" << thetaDeriv1 << std::endl;
        }
      }
    }

  }
  }

} // end UpdateValueAndDerivativeTerms
*/

/**
 * ******************* GetSelfHessian *******************
 */

template <class TFixedImage, class TMovingImage>
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::GetSelfHessian( const TransformParametersType & parameters, HessianType & H ) const
 {
	itkDebugMacro("GetSelfHessian()");

	typedef typename DerivativeType::ValueType        DerivativeValueType;
	typedef typename TransformJacobianType::ValueType TransformJacobianValueType;

	/** Initialize some variables. */
	this->m_NumberOfPixelsCounted = 0;

	/** Array that stores dM(x)/dmu, and the sparse jacobian+indices. */
	NonZeroJacobianIndicesType nzji( this->m_AdvancedTransform->GetNumberOfNonZeroJacobianIndices() );
	//DerivativeType imageJacobian( nzji.size() );
	GradientJacobianType imageJacobian;
	TransformJacobianType jacobian;

	/** Make sure the transform parameters are up to date. */
	this->SetTransformParameters( parameters );


 } // end GetSelfHessian


/**
 * *************** UpdateSelfHessianTerms ***************************
 */

template < class TFixedImage, class TMovingImage >
void
AdvancedOrientationMatchingBetaImageToImageMetric<TFixedImage,TMovingImage>
::UpdateSelfHessianTerms(
		const GradientJacobianType & imageJacobian,
		const NonZeroJacobianIndicesType & nzji,
		double hessianWeight,
		HessianType & H ) const
		{
	/** Do rank-1 update of H */
	if ( nzji.size() == this->GetNumberOfParameters() )
	{
		GradientJacobianType wImageJacobian = hessianWeight * imageJacobian;
		GradientJacobianType temp = wImageJacobian.transpose() * imageJacobian;
		H += temp;
		/** Loop over all jacobians. */
		//vnl_matrix_update( H, wImageJacobian, imageJacobian );
	}
	else
	{
		/** Only pick the nonzero jacobians.
		 * Todo: we could use the symmetry here. Anyway, it won't give much probably. */
		itkExceptionMacro("NOT GOOD");
		/*
      unsigned int imjacsize = imageJacobian.GetSize();
      for ( unsigned int i = 0; i < imjacsize; ++i )
      {
        const unsigned int row = nzji[ i ];
        const double imjacrow = hessianWeight * imageJacobian[ i ];
        for ( unsigned int j = 0; j < imjacsize; ++j )
        {          
          const unsigned int col = nzji[ j ];
          H(row,col) += imjacrow * imageJacobian[ j ];       
        }
      }
		 */
	} // end else

		} // end UpdateSelfHessianTerms



} // end namespace itk


#endif // end #ifndef _itkAdvancedOrientationMatchingBetaImageToImageMetric_txx

