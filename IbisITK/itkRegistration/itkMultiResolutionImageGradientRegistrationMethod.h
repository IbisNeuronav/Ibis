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
  Date:      $Date: 2008-06-27 17:50:36 +0200 (Fri, 27 Jun 2008) $
  Version:   $Revision: 1728 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkMultiResolutionImageGradientRegistrationMethod_h
#define __itkMultiResolutionImageGradientRegistrationMethod_h

#include "itkMultiResolutionImageRegistrationMethod2.h"
#include "itkAdvancedImageToImageMetric.h"
#include "itkAdvancedOrientationMatchingBetaImageToImageMetric.h"
#include "itkSingleValuedNonLinearOptimizer.h"
#include "itkMultiResolutionPyramidImageFilter2.h"
#include "itkMultiResolutionDiscreteGaussianDerivativePyramidImageFilter.h"
#include "itkNumericTraits.h"
#include "itkDataObjectDecorator.h"

namespace itk
{


template <typename TFixedImage, typename TMovingImage>
class ITK_EXPORT MultiResolutionImageGradientRegistrationMethod :
  public MultiResolutionImageRegistrationMethod2<TFixedImage, TMovingImage>
{
public:
  /** Standard class typedefs. */
  typedef MultiResolutionImageGradientRegistrationMethod      Self;
  typedef MultiResolutionImageRegistrationMethod2<
    TFixedImage, TMovingImage>                               Superclass;
  typedef SmartPointer<Self>                                 Pointer;
  typedef SmartPointer<const Self>                           ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro( Self );

  /** Run-time type information (and related methods). */
  itkTypeMacro( MultiResolutionImageGradientRegistrationMethod,
    MultiResolutionImageRegistrationMethod2 );

  /**  Type of the Fixed image. */
  typedef          TFixedImage                        FixedImageType;
  typedef typename FixedImageType::ConstPointer       FixedImageConstPointer;
  typedef typename FixedImageType::RegionType         FixedImageRegionType;
  typedef std::vector<FixedImageRegionType>           FixedImageRegionPyramidType;

  /**  Type of the Moving image. */
  typedef          TMovingImage                       MovingImageType;
  typedef typename MovingImageType::ConstPointer      MovingImageConstPointer;

  /** Type of Gradient Images */

  typedef          FixedArray<double, FixedImageType::ImageDimension>
                                                      ArrayType;

  /**  Type of the metric. */
  typedef AdvancedImageToImageMetric<
    FixedImageType, MovingImageType > MetricType;
  typedef typename MetricType::Pointer                MetricPointer;

  typedef AdvancedOrientationMatchingBetaImageToImageMetric<
    FixedImageType, MovingImageType >                                   OrientationMatchingMetricType;
  typedef typename OrientationMatchingMetricType::Pointer               OrientationMatchingMetricPointer;

  typedef typename OrientationMatchingMetricType::FixedGradientImageType   FixedGradientImageType;

  typedef typename OrientationMatchingMetricType::MovingGradientImageType  MovingGradientImageType;


  /**  Type of the Transform . */
  typedef typename MetricType::AdvancedTransformType  TransformType;
  typedef typename TransformType::Pointer             TransformPointer;

  /** Type for the output: Using Decorator pattern for enabling
   * the Transform to be passed in the data pipeline.
   */
  typedef  DataObjectDecorator< TransformType >       TransformOutputType;
  typedef typename TransformOutputType::Pointer       TransformOutputPointer;
  typedef typename TransformOutputType::ConstPointer  TransformOutputConstPointer;

  /**  Type of the Interpolator. */
  typedef typename MetricType::InterpolatorType       InterpolatorType;
  typedef typename InterpolatorType::Pointer          InterpolatorPointer;

  /**  Type of the optimizer. */
  typedef SingleValuedNonLinearOptimizer              OptimizerType;

  /** Type of the Fixed image multiresolution pyramid. */
  
  typedef MultiResolutionPyramidImageFilter2<
    FixedImageType,FixedImageType >                   FixedImagePyramidType;
  typedef typename FixedImagePyramidType::Pointer     FixedImagePyramidPointer;
  
  typedef MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<
    FixedImageType, FixedImageType, FixedGradientImageType >          FixedImageGaussianDerivativePyramidType;
  typedef typename FixedImageGaussianDerivativePyramidType::Pointer
                                                                      FixedImageGaussianDerivativePyramidPointer;

  /** Type of the moving image multiresolution pyramid. */
  
  typedef MultiResolutionPyramidImageFilter2<
    MovingImageType, MovingImageType >                                MovingImagePyramidType;
  typedef typename MovingImagePyramidType::Pointer                    MovingImagePyramidPointer;

  typedef MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<
    MovingImageType, MovingImageType, MovingGradientImageType >        MovingImageGaussianDerivativePyramidType;
  typedef typename MovingImageGaussianDerivativePyramidType::Pointer
                                                                       MovingImageGaussianDerivativePyramidPointer;

  /** Type of the Transformation parameters This is the same type used to
   *  represent the search space of the optimization algorithm.
   */
  typedef typename MetricType::TransformParametersType  ParametersType;

  /** Smart Pointer type to a DataObject. */
  typedef typename DataObject::Pointer DataObjectPointer;

  /** Method that initiates the registration. */
  virtual void StartRegistration( void );

  /** Method to stop the registration. */
  virtual void StopRegistration( void );

  /** Set/Get the Fixed image. */
  itkSetConstObjectMacro( FixedImage, FixedImageType );
  itkGetConstObjectMacro( FixedImage, FixedImageType );

  /** Set/Get the Moving image. */
  itkSetConstObjectMacro( MovingImage, MovingImageType );
  itkGetConstObjectMacro( MovingImage, MovingImageType );

  /** Set/Get the Optimizer. */
  itkSetObjectMacro( Optimizer, OptimizerType );
  itkGetObjectMacro( Optimizer, OptimizerType );

  /** Set/Get the Metric. */
  itkSetObjectMacro( Metric, MetricType );
  itkGetObjectMacro( Metric, MetricType );

  /** Set/Get the Metric. */
  itkSetMacro( FixedImageRegion, FixedImageRegionType );
  itkGetConstReferenceMacro( FixedImageRegion, FixedImageRegionType );

  /** Set/Get the Transform. */
  itkSetObjectMacro( Transform, TransformType );
  itkGetObjectMacro( Transform, TransformType );

  /** Set/Get the Interpolator. */
  itkSetObjectMacro( Interpolator, InterpolatorType );
  itkGetObjectMacro( Interpolator, InterpolatorType );

  /** Set/Get the Fixed image pyramid. */
  itkSetObjectMacro( FixedImagePyramid, FixedImageGaussianDerivativePyramidType );
  itkGetObjectMacro( FixedImagePyramid, FixedImageGaussianDerivativePyramidType );

  /** Set/Get the Moving image pyramid. */
  itkSetObjectMacro( MovingImagePyramid, MovingImageGaussianDerivativePyramidType );
  itkGetObjectMacro( MovingImagePyramid, MovingImageGaussianDerivativePyramidType );

  /** Set/Get the number of multi-resolution levels. */
  itkSetClampMacro( NumberOfLevels, unsigned long, 1,
    NumericTraits<unsigned long>::max() );
  itkGetMacro( NumberOfLevels, unsigned long );

  /** Get the current resolution level being processed. */
  itkGetMacro( CurrentLevel, unsigned long );

  /** Set/Get the initial transformation parameters. */
  itkSetMacro( InitialTransformParameters, ParametersType );
  itkGetConstReferenceMacro( InitialTransformParameters, ParametersType );

  /** Set/Get the initial transformation parameters of the next resolution
   * level to be processed. The default is the last set of parameters of
   * the last resolution level.
   */
  itkSetMacro( InitialTransformParametersOfNextLevel, ParametersType );
  itkGetConstReferenceMacro( InitialTransformParametersOfNextLevel, ParametersType );

  /** Get the last transformation parameters visited by
   * the optimizer.
   */
  itkGetConstReferenceMacro( LastTransformParameters, ParametersType );

  /** Returns the transform resulting from the registration process. */
  const TransformOutputType * GetOutput( void ) const;

  /** Make a DataObject of the correct type to be used as the specified
   * output.
   */
  virtual DataObjectPointer MakeOutput( unsigned int idx  );

  /** Method to return the latest modified time of this object or
   * any of its cached ivars.
   */
  unsigned long GetMTime( void ) const;

protected:

  /** Constructor. */
  MultiResolutionImageGradientRegistrationMethod();

  /** Destructor. */
  virtual ~MultiResolutionImageGradientRegistrationMethod() {};

  /** PrintSelf. */
  virtual void PrintSelf(std::ostream& os, Indent indent) const;

  /** Method invoked by the pipeline in order to trigger the computation of
   * the registration.
   */
  virtual void GenerateData( void );

  /** Initialize by setting the interconnects between the components.
      This method is executed at every level of the pyramid with the
      values corresponding to this resolution
   */
  virtual void Initialize() throw (ExceptionObject);

  /** Compute the size of the fixed region for each level of the pyramid. */
  virtual void PreparePyramids( void );

  bool CheckForGaussianDerivativePyramids(
		  FixedImageGaussianDerivativePyramidPointer & fixedPyramid,
		  MovingImageGaussianDerivativePyramidPointer & movingPyramid
		  ) const;

  bool CheckForOrientationMatchingMetric(
		  OrientationMatchingMetricPointer & metric
		  ) const;

  /** Set the current level to be processed. */
  itkSetMacro( CurrentLevel, unsigned long );

  /** The last transform parameters. Compared to the ITK class
   * itk::MultiResolutionImageRegistrationMethod these member variables
   * are made protected, so they can be accessed by children classes.
   */
  ParametersType                   m_LastTransformParameters;
  bool                             m_Stop;

private:
  MultiResolutionImageGradientRegistrationMethod(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  /** Member variables. */
  MetricPointer                    m_Metric;
  OrientationMatchingMetricPointer m_OrientationMatchingMetric;
  OptimizerType::Pointer           m_Optimizer;
  TransformPointer                 m_Transform;
  InterpolatorPointer              m_Interpolator;

  ParametersType                   m_InitialTransformParameters;
  ParametersType                   m_InitialTransformParametersOfNextLevel;

  MovingImageConstPointer          m_MovingImage;
  FixedImageConstPointer           m_FixedImage;
  
  

  MovingImageGaussianDerivativePyramidPointer
                                   m_MovingImagePyramid;
  FixedImageGaussianDerivativePyramidPointer
                                   m_FixedImagePyramid;


  FixedImageRegionType             m_FixedImageRegion;
  FixedImageRegionPyramidType      m_FixedImageRegionPyramid;

  unsigned long                    m_NumberOfLevels;
  unsigned long                    m_CurrentLevel;

  double                           m_Threshold;
 

}; // end class MultiResolutionImageGradientRegistrationMethod


} // end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkMultiResolutionImageGradientRegistrationMethod.txx"
#endif

#endif // end #ifndef __itkMultiResolutionImageGradientRegistrationMethod_h

