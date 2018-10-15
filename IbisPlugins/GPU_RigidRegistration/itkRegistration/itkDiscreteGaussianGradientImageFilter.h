/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkDiscreteGaussianGradientImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2010-06-10 13:02:23 $
  Version:   $Revision: 1.8 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkDiscreteGaussianGradientImageFilter_h
#define __itkDiscreteGaussianGradientImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkFixedArray.h"
#include "itkImage.h"
#include "itkNthElementImageAdaptor.h"
#include "itkImage.h"
#include "itkCovariantVector.h"
#include "itkPixelTraits.h"
#include <vector>

namespace itk
{
/**
 * \class DiscreteGaussianGradientImageFilter
 * \brief Calculates image derivatives using discrete derivative gaussian kernels.
 * This filter calculates Gaussian derivative by separable convolution of an image
 * and a discrete Gaussian derivative operator (kernel).
 *
 * The Gaussian operators used here were described by Tony Lindeberg (Discrete
 * Scale-Space Theory and the Scale-Space Primal Sketch.  Dissertation. Royal
 * Institute of Technology, Stockholm, Sweden. May 1991.)
 *
 * The variance or standard deviation (sigma) will be evaluated as pixel units
 * if SetUseImageSpacing is off (false) or as physical units if
 * SetUseImageSpacing is on (true, default). The variance can be set
 * independently in each dimension.
 *
 * When the Gaussian kernel is small, this filter tends to run faster than
 * itk::RecursiveGaussianImageFilter.
 *
 * \author Ivan Macia, VICOMTech, Spain, http://www.vicomtech.es
 *
 * This implementation was taken from the Insight Journal paper:
 * http://hdl.handle.net/1926/1290
 *
 * \sa GaussianDerivativeOperator
 * \sa Image
 * \sa Neighborhood
 * \sa NeighborhoodOperator
 *
 * \ingroup ImageEnhancement
 * \ingroup ImageFeatureExtraction
 */

template <class TInputImage,
		 class TOutputImage>
class ITK_EXPORT DiscreteGaussianGradientImageFilter :
    public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef DiscreteGaussianGradientImageFilter           Self;
  typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(DiscreteGaussianGradientImageFilter, ImageToImageFilter);

  /** Image type information. */
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  typedef typename TOutputImage::PixelType         OutputPixelType;
  typedef typename TOutputImage::InternalPixelType OutputInternalPixelType;
  typedef typename TInputImage::PixelType          InputPixelType;
  typedef typename TInputImage::InternalPixelType  InputInternalPixelType;
  typedef typename TInputImage::PixelType                PixelType;
  typedef typename NumericTraits<PixelType>::RealType    RealType;

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  typedef float                                            InternalRealType;
  typedef Image<InternalRealType,
                itkGetStaticConstMacro(ImageDimension) >   RealImageType;


  /** Typedef of double containers */
  typedef FixedArray<double, itkGetStaticConstMacro(ImageDimension)> ArrayType;

  /** Array for storing desired order of derivatives */
  typedef FixedArray<unsigned int, itkGetStaticConstMacro(ImageDimension)> OrderArrayType;

  typedef NeighborhoodOperatorImageFilter<InputImageType,
	  RealImageType, InputPixelType> 						  NeighborhoodOperatorFilterType;
  typedef typename NeighborhoodOperatorFilterType::Pointer        NeighborhoodOperatorFilterPointer;


  /**  Output Image Nth Element Adaptor
   *  This adaptor allows to use conventional scalar
   *  smoothing filters to compute each one of the
   *  components of the gradient image pixels. */
  typedef NthElementImageAdaptor< TOutputImage,
                                  InternalRealType >  OutputImageAdaptorType;
  typedef typename OutputImageAdaptorType::Pointer    OutputImageAdaptorPointer;

  /** The variance for the discrete Gaussian kernel.  Sets the variance
   * independently for each dimension, but
   * see also SetVariance(const double v). The default is 0.0 in each
   * dimension. If UseImageSpacing is true, the units are the physical units
   * of your image.  If UseImageSpacing is false then the units are
   * pixels. */
  itkSetMacro(Variance, ArrayType);
  itkGetConstMacro(Variance, const ArrayType);

  /** The algorithm will size the discrete kernel so that the error
   * resulting from truncation of the kernel is no greater than
   * MaximumError. The default is 0.01 in each dimension. */
  itkSetMacro(MaximumError, double);
  itkGetConstMacro(MaximumError, const double);

  /** Set the kernel to be no wider than MaximumKernelWidth pixels,
   *  even if MaximumError demands it. The default is 32 pixels. */
  itkGetConstMacro(MaximumKernelWidth, int);
  itkSetMacro(MaximumKernelWidth, int);
  
  /** \brief Set/Get number of pieces to divide the input for the
   * internal composite pipeline. The upstream pipeline will not be
   * effected.  
   *
   * The default value is $ImageDimension^2$.
   *
   * This parameter was introduced to reduce the memory used by images
   * internally, at the cost of performance. 
   */
  itkSetMacro(InternalNumberOfStreamDivisions,unsigned int);
  itkGetConstMacro(InternalNumberOfStreamDivisions,unsigned int);
  
  itkGetConstMacro(KernelNorms, const ArrayType);

  /** Set/Get whether or not the filter will use the spacing of the input
      image in its calculations. Default is ImageSpacingOn. */
  itkSetMacro(UseImageSpacing, bool);
  itkGetConstMacro(UseImageSpacing, bool);
  itkBooleanMacro(UseImageSpacing);

#ifdef ITK_USE_CONCEPT_CHECKING
     itkConceptMacro( InputHasNumericTraitsCheck,
                      ( Concept::HasNumericTraits< InputPixelType > ) );
     itkConceptMacro( OutputHasPixelTraitsCheck,
                      ( Concept::HasPixelTraits< OutputPixelType > ) );
#endif

protected:

  DiscreteGaussianGradientImageFilter()
    {
    m_Variance.Fill(1.0);
    m_MaximumError = 0.01;
    m_MaximumKernelWidth = 32;
    m_UseImageSpacing = true;
    m_InternalNumberOfStreamDivisions = ImageDimension*ImageDimension;
    m_ImageAdaptor = OutputImageAdaptorType::New();
    m_DerivativeFilters.resize(ImageDimension);
    m_KernelNorms.Fill(1.0);
    }
  virtual ~DiscreteGaussianGradientImageFilter() {}
  void PrintSelf(std::ostream& os, Indent indent) const;

  /** DiscreteGaussianGradientImageFilter needs a larger input requested region
   * than the output requested region (larger by the size of the
   * Gaussian kernel).  As such, DiscreteGaussianGradientImageFilter needs to
   * provide an implementation for GenerateInputRequestedRegion() in
   * order to inform the pipeline execution model.
   * \sa ImageToImageFilter::GenerateInputRequestedRegion() */
  virtual void GenerateInputRequestedRegion() throw(InvalidRequestedRegionError);

  /** Standard pipeline method. While this class does not implement a
   * ThreadedGenerateData(), its GenerateData() delegates all
   * calculations to an NeighborhoodOperatorImageFilter.  Since the
   * NeighborhoodOperatorImageFilter is multithreaded, this filter is
   * multithreaded by default. */
  void GenerateData();

private:

  DiscreteGaussianGradientImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  /** The variance of the gaussian blurring kernel in each dimensional direction. */
  ArrayType m_Variance;

  /** The maximum error of the gaussian blurring kernel in each dimensional
   * direction. For definition of maximum error, see GaussianOperator.
   * \sa GaussianOperator */
  double m_MaximumError;

  /** Maximum allowed kernel width for any dimension of the discrete Gaussian
      approximation */
  int m_MaximumKernelWidth;

  /** Flag to indicate whether to use image spacing */
  bool m_UseImageSpacing;
  
  /** Number of pieces to divide the input on the internal composite
  pipeline. The upstream pipeline will not be effected. */
  unsigned int m_InternalNumberOfStreamDivisions;

  ArrayType  m_KernelNorms;

  std::vector<NeighborhoodOperatorFilterPointer>   m_DerivativeFilters;

  OutputImageAdaptorPointer            m_ImageAdaptor;
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkDiscreteGaussianGradientImageFilter.txx"
#endif

#endif
