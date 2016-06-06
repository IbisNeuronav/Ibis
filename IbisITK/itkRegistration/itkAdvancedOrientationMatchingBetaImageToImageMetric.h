/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/


#ifndef __itkAdvancedOrientationMatchingBetaImageToImageMetric_h
#define __itkAdvancedOrientationMatchingBetaImageToImageMetric_h


//#include "itkImageRandomCoordinateSampler.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkAdvancedImageToImageMetric.h"
#include "itkImageToListSampleFilter.h"
#include "itkSampleToHistogramFilter.h"
#include "itkImageToHistogramFilter.h"
#include "itkIdentityTransform.h"
#include "itkGradientToMagnitudeImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
//#include "itkImageMaskSpatialObject.h"
#include "itkImageMaskSpatialObject2.h"
#include "itkSpatialObjectToImageFilter.h"
//#include "itkAdvancedCombinationTransform.h"
#include "itkCovariantVector.h"
#include "itkImage.h"
#include "itkOrientationMatchingBetaImageFunction.h"
#include "itkVectorLinearInterpolateImageFunction.h"

namespace itk
{

/** \class AdvancedOrientationMatchingBetaImageToImageMetric
 * \brief Compute Local Mutual Information between two images, based on AdvancedImageToImageMetric...
 *
 * This Class is templated over the type of the fixed and moving
 * images to be compared.
 *
 * This metric computes the sum of local mutual information values between pixels in
 * the moving image and pixels in the fixed image. The spatial correspondance 
 * between both images is established through a Transform. Pixel values are
 * taken from the Moving image. Their positions are mapped to the Fixed image
 * and result in general in non-grid position on it. Values at these non-grid
 * position of the Fixed image are interpolated using a user-selected Interpolator.
 *
 * This implementation of Local Mutual Information is based on the 
 * AdvancedImageToImageMetric, which means that:
 * \li It uses the ImageSampler-framework
 * \li It makes use of the compact support of B-splines, in case of B-spline transforms.
 * \li Image derivatives are computed using either the B-spline interpolator's implementation
 * or by nearest neighbor interpolation of a precomputed central difference image.
 * \li A minimum number of samples that should map within the moving image (mask) can be specified.
 * 
 * \ingroup RegistrationMetrics
 * \ingroup Metrics
 */

template < class TFixedImage, class TMovingImage > 
class AdvancedOrientationMatchingBetaImageToImageMetric :
    public AdvancedImageToImageMetric< TFixedImage, TMovingImage>
{
public:

  /** Standard class typedefs. */
  typedef AdvancedOrientationMatchingBetaImageToImageMetric   Self;
  typedef AdvancedImageToImageMetric<
    TFixedImage, TMovingImage >                   Superclass;
  typedef SmartPointer<Self>                      Pointer;
  typedef SmartPointer<const Self>                ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro( Self );
 
  /** Run-time type information (and related methods). */
  itkTypeMacro( AdvancedOrientationMatchingBetaImageToImageMetric, AdvancedImageToImageMetric );

  /** Typedefs from the superclass. */
  typedef typename 
    Superclass::CoordinateRepresentationType              CoordinateRepresentationType;
  typedef typename Superclass::MovingImageType            MovingImageType;
  typedef typename Superclass::MovingImagePixelType       MovingImagePixelType;
  typedef typename Superclass::MovingImageConstPointer    MovingImageConstPointer;
  typedef typename Superclass::FixedImageType             FixedImageType;
  typedef typename Superclass::FixedImagePointer          FixedImagePointer;  
  typedef typename Superclass::FixedImageConstPointer     FixedImageConstPointer;
  typedef typename Superclass::FixedImageRegionType       FixedImageRegionType;
  typedef typename Superclass::TransformType              TransformType;
  typedef typename Superclass::TransformPointer           TransformPointer;
  typedef typename Superclass::AdvancedTransformType      AdvancedTransformType;
  typedef typename Superclass::InputPointType             InputPointType;
  typedef typename Superclass::OutputPointType            OutputPointType;
  typedef typename Superclass::TransformParametersType    TransformParametersType;
  typedef typename Superclass::TransformJacobianType      TransformJacobianType;
  typedef typename Superclass::InterpolatorType           InterpolatorType;
  typedef typename Superclass::InterpolatorPointer        InterpolatorPointer;
  typedef typename Superclass::RealType                   RealType;
  typedef typename Superclass::GradientPixelType          GradientPixelType;
  typedef typename Superclass::GradientImageType          GradientImageType;
  typedef typename Superclass::GradientImagePointer       GradientImagePointer;
  typedef typename Superclass::GradientImageFilterType    GradientImageFilterType;
  typedef typename Superclass::GradientImageFilterPointer GradientImageFilterPointer;
  typedef typename Superclass::GradientPixelType          SobelPixelType;
  typedef typename Superclass::GradientImageType          SobelImageType;
  typedef typename Superclass::GradientImagePointer       SobelImagePointer;
  typedef typename Superclass::FixedImageMaskType         FixedImageMaskType;
  typedef typename Superclass::FixedImageMaskPointer      FixedImageMaskPointer;
  typedef typename Superclass::MovingImageMaskType        MovingImageMaskType;
  typedef typename Superclass::MovingImageMaskPointer     MovingImageMaskPointer;
  typedef typename Superclass::MeasureType                MeasureType;
  typedef typename Superclass::DerivativeType             DerivativeType;
  typedef typename Superclass::ParametersType             ParametersType;
  typedef typename Superclass::FixedImagePixelType        FixedImagePixelType;
  typedef typename Superclass::MovingImageRegionType      MovingImageRegionType;
  typedef typename Superclass::ImageSamplerType           ImageSamplerType;
  typedef typename Superclass::ImageSamplerPointer        ImageSamplerPointer;
  typedef typename Superclass::ImageSampleContainerType   ImageSampleContainerType;
  typedef typename 
    Superclass::ImageSampleContainerPointer               ImageSampleContainerPointer;
  typedef typename Superclass::FixedImageLimiterType      FixedImageLimiterType;
  typedef typename Superclass::MovingImageLimiterType     MovingImageLimiterType;
  typedef typename
    Superclass::FixedImageLimiterOutputType               FixedImageLimiterOutputType;
  typedef typename
    Superclass::MovingImageLimiterOutputType              MovingImageLimiterOutputType;
  typedef typename
    Superclass::MovingImageDerivativeScalesType           MovingImageDerivativeScalesType;
  typedef typename
    Superclass::ScalarType						                    ScalarType;

  typedef typename
    Superclass::KernelNormsType						                KernelNormsType;


  /** Some typedefs for computing the SelfHessian */
  typedef typename DerivativeType::ValueType              HessianValueType;
  typedef Array2D<HessianValueType>                       HessianType;


  typedef itk::CovariantVector<ScalarType, FixedImageType::ImageDimension>
                                                          FixedGradientPixelType;

  typedef itk::Image<FixedGradientPixelType, FixedImageType::ImageDimension>
                                                          FixedGradientImageType;

  typedef typename FixedGradientImageType::ConstPointer   FixedGradientImageConstPointer;
  typedef typename FixedGradientImageType::Pointer        FixedGradientImagePointer;

  typedef itk::Image<ScalarType, FixedImageType::ImageDimension>
                                                          FixedScalarImageType;

  typedef itk::CovariantVector<ScalarType, FixedImageType::ImageDimension>
                                                          MovingGradientPixelType;

  typedef itk::Image<MovingGradientPixelType, FixedImageType::ImageDimension>
                                                          MovingGradientImageType;

  typedef typename MovingGradientImageType::ConstPointer  MovingGradientImageConstPointer;
  typedef typename MovingGradientImageType::Pointer       MovingGradientImagePointer;

  typedef itk::Image<unsigned char, FixedImageType::ImageDimension>
                                                          FixedMaskImageType;

  typedef NearestNeighborInterpolateImageFunction<FixedMaskImageType>
                                                          NNInterpolatorType;

  typedef typename AdvancedTransformType::JacobianType            JacobianType;
  typedef typename AdvancedTransformType::SpatialJacobianType     SpatialJacobianType;
  typedef typename AdvancedTransformType::JacobianOfSpatialJacobianType
                                                                 JacobianOfSpatialJacobianType;

  typedef itk::SpatialObjectToImageFilter<
    FixedImageMaskType, FixedMaskImageType>               MaskToImageFilterType;
  typedef typename MaskToImageFilterType::Pointer         MaskToImagePointer;

  typedef itk::ImageMaskSpatialObject2<FixedImageType::ImageDimension> ImageMaskSpatialObjectType;
  typedef typename ImageMaskSpatialObjectType::Pointer                 ImageMaskSpatialObjectPointer;
  typedef typename ImageMaskSpatialObjectType::ConstPointer            ImageMaskSpatialObjectConstPointer;


  typedef itk::Statistics::ImageToListSampleFilter
      < FixedScalarImageType, FixedMaskImageType  >             ImageToListSampleType;
  typedef typename ImageToListSampleType::Pointer               ImageToListSamplePointer;

  typedef typename ImageToListSampleType::MeasurementVectorType MeasurementVectorType;
  typedef typename ImageToListSampleType::ListSampleType        ListSampleType;

  typedef itk::Statistics::ImageToHistogramFilter<FixedImageType>
                                                                HistogramFilterType;
  typedef typename HistogramFilterType::Pointer
                                                                HistogramFilterPointer;
  typedef typename HistogramFilterType::HistogramMeasurementType
                                                                HistogramMeasurementType;
  typedef typename HistogramFilterType::HistogramType
                                                                HistogramType;
  typedef typename HistogramType::Pointer                       HistogramPointer;

  typedef itk::Statistics::SampleToHistogramFilter<
    ListSampleType, HistogramType >                         SampleToHistogramFilterType;
  typedef typename SampleToHistogramFilterType::Pointer     SampleToHistogramFilterPointer;

  typedef itk::GradientToMagnitudeImageFilter<
    FixedGradientImageType, FixedScalarImageType>         MagnitudeFilterType;
  typedef typename MagnitudeFilterType::Pointer           MagnitudeFilterPointer;

  typedef itk::BinaryThresholdImageFilter<
      FixedImageType, FixedMaskImageType>                 ThresholdFilterType;
  typedef typename ThresholdFilterType::Pointer           ThresholdFilterPointer;


  /** The fixed image dimension. */
  itkStaticConstMacro( FixedImageDimension, unsigned int,
    FixedImageType::ImageDimension );

  /** The moving image dimension. */
  itkStaticConstMacro( MovingImageDimension, unsigned int,
    MovingImageType::ImageDimension );

  /*
  typedef AdvancedCombinationTransform<
    ScalarType, FixedImageDimension >                   CombinationTransformType;
  */

  /** Get the value for single valued optimizers. */
  virtual MeasureType GetValue( const TransformParametersType & parameters ) const;

  /** Get the derivatives of the match measure. */
  virtual void GetDerivative( const TransformParametersType & parameters,
    DerivativeType & derivative ) const;

  /** Get value and derivatives for multiple valued optimizers. */
  virtual void GetValueAndDerivative( const TransformParametersType & parameters,
    MeasureType& Value, DerivativeType& Derivative ) const;

  /** Experimental feature: compute SelfHessian */
  virtual void GetSelfHessian( const TransformParametersType & parameters, HessianType & H ) const;

  /** Set the BSpline transform in this class.
   * This class expects a BSplineTransform! It is not suited for others.
   */
  //itkSetObjectMacro( BSplineTransform, BSplineTransformType );

  /** Default: 1.0 mm */
  itkSetMacro( SelfHessianSmoothingSigma, double );
  itkGetConstMacro( SelfHessianSmoothingSigma, double );

  /** Default: 100000 */
  itkSetMacro( NumberOfSamplesForSelfHessian, unsigned int );
  itkGetConstMacro( NumberOfSamplesForSelfHessian, unsigned int );

  /** Initialize the Metric by making sure that all the components
   *  are present and plugged together correctly.
   * \li Call the superclass' implementation
   * \li Estimate the normalization factor, if asked for.  */
  virtual void Initialize(void) throw ( ExceptionObject );

  /** Set/Get whether to normalize the mean squares measure.
   * This divides the MeanSquares by a factor (range/10)^2,
   * where range represents the maximum gray value range of the
   * images. Based on the ad hoc assumption that range/10 is the
   * maximum average difference that will be observed. 
   * Dividing by range^2 sounds less ad hoc, but will yield
   * very small values. */
  itkSetMacro( UseNormalization, bool );
  itkGetConstMacro( UseNormalization, bool );

  itkSetMacro( Percentile,  double);
  itkGetConstMacro( Percentile,  double);

  itkSetMacro( N,  double);
  itkGetConstMacro( N,  double);

  itkSetObjectMacro( FixedGradientImage,  FixedGradientImageType );
  itkGetObjectMacro( FixedGradientImage,  FixedGradientImageType );
   
  itkSetObjectMacro( MovingGradientImage,  MovingGradientImageType );
  itkGetObjectMacro( MovingGradientImage,  MovingGradientImageType );

  itkSetObjectMacro( FixedMaskImage,  FixedImageType );
  itkGetObjectMacro( FixedMaskImage,  FixedImageType );

  /** Compute Fixed Mask Image */
  void ComputeFixedMask(void);

protected:
  AdvancedOrientationMatchingBetaImageToImageMetric();
  virtual ~AdvancedOrientationMatchingBetaImageToImageMetric() {};
  void PrintSelf( std::ostream& os, Indent indent ) const;

  /** Protected Typedefs ******************/

  /** Typedefs inherited from superclass */
  typedef typename Superclass::FixedImageIndexType                FixedImageIndexType;
  typedef typename Superclass::FixedImageIndexValueType           FixedImageIndexValueType;
  typedef typename Superclass::MovingImageIndexType               MovingImageIndexType;
  typedef typename Superclass::FixedImagePointType                FixedImagePointType;
  typedef typename Superclass::MovingImagePointType               MovingImagePointType;
  typedef typename Superclass::MovingImageContinuousIndexType     MovingImageContinuousIndexType;
  typedef typename Superclass::BSplineInterpolatorType            BSplineInterpolatorType;
  typedef typename Superclass::CentralDifferenceGradientFilterType CentralDifferenceGradientFilterType;
  typedef typename Superclass::MovingImageDerivativeType          MovingImageDerivativeType; 
  typedef typename Superclass::NonZeroJacobianIndicesType         NonZeroJacobianIndicesType;
  typedef itk::Matrix
    <double, MovingImageDimension, MovingImageDimension>          GradientGradientType; 
  typedef typename Superclass::TransformJacobianType              GradientJacobianType;




  /** Protected typedefs for SelfHessian */
  typedef BSplineInterpolateImageFunction<
    FixedImageType, CoordinateRepresentationType>                 FixedImageInterpolatorType;
  typedef NearestNeighborInterpolateImageFunction<
    FixedImageType, CoordinateRepresentationType >                DummyFixedImageInterpolatorType;     
  //typedef ImageRandomCoordinateSampler<FixedImageType>            SelfHessianSamplerType;

  double m_NormalizationFactor;

  /** Proteced typedefs for Fixed Gradient Image */

  typedef typename itk::VectorLinearInterpolateImageFunction<
            FixedGradientImageType, double>                       VectorInterpolatorType;

  typedef typename itk::OrientationMatchingBetaImageFunction<
            MovingGradientImageType, double>                      OrientationMatchingFunctionType;

  /** Compute a pixel's contribution to the SelfHessian;
   * Called by GetSelfHessian(). */
  void UpdateSelfHessianTerms( 
    const GradientJacobianType & imageJacobian,    
    const NonZeroJacobianIndicesType & nzji,
    double hessianWeight,
    HessianType & H) const;
 
  /** A function to check if the transform is B-spline, for speedup. */
  virtual void GetRotationMatrix(
      const FixedImagePointType fixedPoint,
      SpatialJacobianType & rotation ) const;

private:
  AdvancedOrientationMatchingBetaImageToImageMetric(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  bool 			                    m_UseNormalization;
  double 		                    m_SelfHessianSmoothingSigma;
  unsigned int 	                m_NumberOfSamplesForSelfHessian;

  FixedGradientImagePointer    m_FixedGradientImage;
  MovingGradientImagePointer   m_MovingGradientImage;
  
  FixedImagePointer            m_FixedMaskImage;
  
  double		     m_N;
  double		     m_LocalSimilarityMeasureParameter;
  bool        	 m_MaskIsDefined;
  double 		     m_Percentile;

  //BSplineTransformPointer							m_BSplineTransform;

  //typename GradientGradientCalculatorType::Pointer    m_GradientGradientCalculator;
  typename VectorInterpolatorType::Pointer              m_VectorInterpolator;
  typename OrientationMatchingFunctionType::Pointer     m_OrientationMatchingFunction;
  
  ImageMaskSpatialObjectPointer                         m_OriginalFixedMask;


}; // end class AdvancedOrientationMatchingBetaImageToImageMetric

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkAdvancedOrientationMatchingBetaImageToImageMetric.hxx"
#endif

#endif // end #ifndef __itkAdvancedOrientationMatchingBetaImageToImageMetric_h

