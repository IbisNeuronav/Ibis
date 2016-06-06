/*=========================================================================


=========================================================================*/
#ifndef __itkOrientationMatchingBetaImageFunction_h
#define __itkOrientationMatchingBetaImageFunction_h

// First make sure that the configuration is available.
// This line can be removed once the optimized versions
// gets integrated into the main directories.
#include "itkConfigure.h"


#include "itkAdvancedTransform.h"
#include "itkVectorLinearInterpolateImageFunction.h"
//#include "itkAdvancedBSplineDeformableTransform.h"
//#include "itkAdvancedCombinationTransform.h"
#include "itkAdvancedTransform.h"

namespace itk
{

/** \class Gradient LinearInterpolateImageFunction
 * \brief Linearly interpolate an image at specified positions.
 *
 * LinearInterpolateImageFunction linearly interpolates image intensity at
 * a non-integer pixel position. This class is templated
 * over the input image type and the coordinate representation type
 * (e.g. float or double).
 *
 * This function works for N-dimensional images.
 *
 * \warning This function work only for images with scalar pixel
 * types. For vector images use VectorLinearInterpolateImageFunction.
 *
 * \sa VectorLinearInterpolateImageFunction
 *
 * \ingroup ImageFunctions ImageInterpolators
 */
template <class TInputImage,
          class TCoordRep = double,
          class TInterpolatorPrecisionType = double>
class ITK_EXPORT OrientationMatchingBetaImageFunction :
  public VectorLinearInterpolateImageFunction< TInputImage, TCoordRep>
{
public:
  /** Standard class typedefs. */
  typedef OrientationMatchingBetaImageFunction                Self;
  typedef VectorLinearInterpolateImageFunction<TInputImage,TCoordRep>
                                                          Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(OrientationMatchingBetaImageFunction, VectorLinearInterpolateImageFunction);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** OutputType typedef support. */
  typedef typename Superclass::OutputType                 OutputType;

  /** InputImageType typedef support. */
  typedef typename Superclass::InputImageType             InputImageType;

  /** InputPixelType typedef support. */
  typedef typename Superclass::InputPixelType             InputPixelType;

  /** RealType typedef support. */
  //typedef typename Superclass::RealType                   RealType;
  /** RealType typedef support. */
  typedef typename NumericTraits<
               typename TInputImage::PixelType>::RealType   RealType;


  /** Dimension underlying input image. */
  itkStaticConstMacro(ImageDimension, unsigned int,Superclass::ImageDimension);

  /** Index typedef support. */
  typedef typename Superclass::IndexType                  IndexType;
  typedef typename Superclass::IndexValueType             IndexValueType;

  /** ContinuousIndex typedef support. */
  typedef typename Superclass::ContinuousIndexType        ContinuousIndexType;

  typedef typename Superclass::PointType                  PointType;

  typedef double                                          ScalarType;

  /** Transform typedef. */
  typedef AdvancedTransform<TInterpolatorPrecisionType,
    itkGetStaticConstMacro(ImageDimension),
    itkGetStaticConstMacro(ImageDimension)>               TransformType;
  typedef typename TransformType::Pointer                 TransformPointerType;

  typedef typename TransformType::ParametersType          ParametersType;
  typedef typename TransformType::ParametersValueType     ParametersValueType;
  typedef typename TransformType::JacobianType            JacobianType;
  typedef typename TransformType::SpatialJacobianType     SpatialJacobianType;
  typedef typename TransformType::JacobianOfSpatialJacobianType
                                                          JacobianOfSpatialJacobianType;
  typedef typename TransformType::NonZeroJacobianIndicesType
                                                          NonZeroJacobianIndicesType;

  typedef Array< ParametersValueType >                    DerivativeType;
  /** Set the coordinate transformation.
   * Set the coordinate transform to use for resampling.  Note that this must
   * be in physical coordinates and it is the output-to-input transform, NOT
   * the input-to-output transform that you might naively expect.  By default
   * the filter uses an Identity transform. You must provide a different
   * transform here, before attempting to run the filter, if you do not want to
   * use the default Identity transform. */
  itkSetObjectMacro( Transform, TransformType );
  itkGetObjectMacro( Transform, TransformType );

  itkSetMacro( LocalSimilarityMeasureType,  unsigned int);
  itkGetConstMacro( LocalSimilarityMeasureType,  unsigned int);

  itkSetMacro( LocalSimilarityMeasureParameter,  double);
  itkGetConstMacro( LocalSimilarityMeasureParameter,  double);

  /** Typedef's for B-spline transform. */
  /*
  typedef AdvancedBSplineDeformableTransform< ScalarType,
    ImageDimension, 3 >                                 BSplineTransformType;
  typedef typename BSplineTransformType::Pointer        BSplineTransformPointer;
  */

  /*
  typedef AdvancedCombinationTransform<
    ScalarType, ImageDimension >                   CombinationTransformType;
  */
  //itkSetObjectMacro( BSplineTransform, BSplineTransformType );

  virtual double EvaluateLocalSimilarityMeasure(
      const InputPixelType fixedGradient,
      const InputPixelType movingGradient ) const;

  virtual void EvaluateLocalSimilarityDerivative(
      const InputPixelType fixedGradient,
      const InputPixelType movingGradient,
      double & derivativeOrientation1,
      double & derivativeOrientation2  ) const;

  virtual double EvaluateValueAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient ) const;

  virtual void EvaluateLocalValueAndDerivativeAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient,
    double & value,
    double & derivativeOrientation1,
    double & derivativeOrientation2 ) const;


  virtual double EvaluateValueAtTransformedPoint(
      const PointType & transformedPoint,
      const InputPixelType fixedGradient,
      const SpatialJacobianType rotation ) const;

  InputPixelType EvaluateGradientAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient ) const;

  void EvaluateValueAndGradientAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient,
    double & value,
    InputPixelType & measureGradient  ) const;

  void EvaluateValueAndDerivativeAtPoint(
      const PointType & point,
      const InputPixelType fixedGradient,
      const InputPixelType orientation1Gradient,
      const InputPixelType orientation2Gradient,
      double & value,
      DerivativeType & measureGradient ) const;

  virtual void Initialize(void) throw ( ExceptionObject );

protected:
  OrientationMatchingBetaImageFunction();
  ~OrientationMatchingBetaImageFunction(){};
  void PrintSelf(std::ostream& os, Indent indent) const;

  /** A function to check if the transform is B-spline, for speedup. */
  //bool CheckForBSplineTransform( BSplineTransformPointer & bspline );

  /** A function to check if the transform is B-spline, for speedup. */
  virtual void GetRotationMatrix(
      const PointType & fixedPoint,
      SpatialJacobianType & rotation ) const;

private:
  OrientationMatchingBetaImageFunction( const Self& ); //purposely not implemented
  void operator=( const Self& ); //purposely not implemented

  /** Number of neighbors used in the interpolation */
  static const unsigned long  m_Neighbors;
  TransformPointerType        m_Transform;         // Coordinate transform to use

  // flag to take or not the image direction into account
  // when computing the derivatives.
  bool          m_UseImageDirection;
  unsigned int  m_LocalSimilarityMeasureType;
  double        m_LocalSimilarityMeasureParameter;
  //BSplineTransformPointer
  //              m_BSplineTransform;
};

} // end namespace itk

// Define instantiation macro for this template.
#define ITK_TEMPLATE_OrientationMatchingBetaImageFunction(_, EXPORT, x, y) namespace itk { \
  _(2(class EXPORT OrientationMatchingBetaImageFunction< ITK_TEMPLATE_2 x >)) \
  namespace Templates { typedef OrientationMatchingBetaImageFunction< ITK_TEMPLATE_2 x > \
                                                  OrientationMatchingBetaImageFunction##y; } \
  }

#if ITK_TEMPLATE_EXPLICIT
# include "Templates/itkOrientationMatchingBetaImageFunction+-.h"
#endif

#if ITK_TEMPLATE_TXX
# include "itkOrientationMatchingBetaImageFunction.txx"
#endif

#endif
