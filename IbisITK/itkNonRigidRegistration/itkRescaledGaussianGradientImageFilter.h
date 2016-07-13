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

#ifndef __itkRescaledGaussianGradientImageFilter_h
#define __itkRescaledGaussianGradientImageFilter_h
 
#include "itkImageToImageFilter.h"

#include "itkResampleImageFilter.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkGradientRecursiveGaussianImageFilter.h"
 
namespace itk
{
template< class TInputImage, class TOutputImage>
class RescaledGaussianGradientImageFilter:public ImageToImageFilter< TInputImage, TOutputImage >
{
public:
  /** Standard class typedefs. */
  typedef RescaledGaussianGradientImageFilter             Self;
  typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;
  typedef SmartPointer< Self >                            Pointer;
 
  /** Method for creation through the object factory. */
  itkNewMacro(Self);
 
  /** Run-time type information (and related methods). */
  itkTypeMacro(RescaledGaussianGradientImageFilter, ImageToImageFilter);

  typedef TInputImage                                InputImageType;
  typedef TOutputImage                               OutputImageType;

  /** Dimension of the domain space. */
  itkStaticConstMacro( SpaceDimension, unsigned int, InputImageType::ImageDimension );

  typedef ResampleImageFilter< InputImageType, InputImageType >
                                                     ResamplerType;
  typedef typename ResamplerType::Pointer            ResamplerPointer;

  typedef SmoothingRecursiveGaussianImageFilter< InputImageType, InputImageType >
                                                     SmootherType;
  typedef typename SmootherType::Pointer             SmootherPointer;

  typedef GradientRecursiveGaussianImageFilter< InputImageType, OutputImageType >
                                                     GradientFilterType;
  typedef typename GradientFilterType::Pointer       GradientFilterPointer;
 

  itkGetMacro( ImageSpacing, double );
  itkSetMacro( ImageSpacing, double );

  itkGetMacro( GradientScale, double );
  itkSetMacro( GradientScale, double );

protected:
  RescaledGaussianGradientImageFilter();
  ~RescaledGaussianGradientImageFilter();
 
  /** Does the real work. */
  virtual void GenerateData();
 
private:
  RescaledGaussianGradientImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);  //purposely not implemented
 

  ResamplerPointer        m_Resampler;
  SmootherPointer         m_Smoother;
  GradientFilterPointer   m_GradientFilter;
  double                  m_GradientScale;
  double                  m_ImageSpacing;
};
} //namespace ITK
 
 
#ifndef ITK_MANUAL_INSTANTIATION
#include "itkRescaledGaussianGradientImageFilter.hxx"
#endif
 
 
#endif // __itkImageFilter_h
