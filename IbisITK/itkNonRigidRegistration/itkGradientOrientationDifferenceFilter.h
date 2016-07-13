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

#ifndef __itkGradientOrientationDifferenceFilter_h
#define __itkGradientOrientationDifferenceFilter_h
 
#include "itkImageToImageFilter.h"
#include "itkConceptChecking.h"

namespace itk
{
template< class TImage >
class GradientOrientationDifferenceFilter : public ImageToImageFilter< TImage, TImage >
{
public:
  /** Standard class typedefs. */
  typedef GradientOrientationDifferenceFilter             Self;
  typedef ImageToImageFilter< TImage, TImage > Superclass;
  typedef SmartPointer< Self >        Pointer;
 
  /** Method for creation through the object factory. */
  itkNewMacro(Self);
 
  /** Run-time type information (and related methods). */
  itkTypeMacro(GradientOrientationDifferenceFilter, ImageToImageFilter);
 
  itkStaticConstMacro(ImageDimension, unsigned int, TImage::ImageDimension);

  typedef typename TImage::PixelType     PixelType;
  typedef typename TImage::IndexType     IndexType;   

  TImage* GetOutput1();
  TImage* GetOutput2();
 
#ifdef ITK_USE_CONCEPT_CHECKING

  itkConceptMacro( InputHasNumericTraitsCheck,
                   ( Concept::HasNumericTraits< typename TImage::PixelType::ValueType > ) );

  itkConceptMacro( SameDimensionCheck,
                   ( Concept::SameDimension< TImage::PixelType::Dimension, TImage::ImageDimension > ) );  

#endif

protected:
  GradientOrientationDifferenceFilter();
  ~GradientOrientationDifferenceFilter(){}
 
  /** Does the real work. */
  virtual void GenerateData();
 
  /**  Create the Output */
  DataObject::Pointer MakeOutput(unsigned int idx);
 
private:
  GradientOrientationDifferenceFilter(const Self &); //purposely not implemented
  void operator=(const Self &);  //purposely not implemented
 
};
} //namespace ITK
 
 
#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGradientOrientationDifferenceFilter.hxx"
#endif
 
 
#endif // __itkGradientOrientationDifferenceFilter_h
