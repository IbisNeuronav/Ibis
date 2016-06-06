/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkWeightedGradientToMagnitudeImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2009-04-01 14:36:19 $
  Version:   $Revision: 1.23 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkWeightedGradientToMagnitudeImageFilter_h
#define __itkWeightedGradientToMagnitudeImageFilter_h

#include "itkUnaryFunctorImageFilter.h"

namespace itk
{
  
/** \class WeightedGradientToMagnitudeImageFilter
 *
 * \brief Take an image of vectors as input and produce an image with the
 *  magnitude of those vectors.
 *
 * The filter expects the input image pixel type to be a vector and 
 * the output image pixel type to be a scalar.
 *
 * This filter assumes that the PixelType of the input image is a VectorType
 * that provides a GetNorm() method.
 *
 * \ingroup IntensityImageFilters  Multithreaded
 */

namespace Functor {  
  
template< class TInput, class TOutput>
class WeightedGradientMagnitude
{
public:
  WeightedGradientMagnitude() 
    {
      m_Weights.Fill(1.0);
    }
  ~WeightedGradientMagnitude() {}

  void SetWeights( TInput & W ) { m_Weights = W; }
  
  bool operator!=( const WeightedGradientMagnitude & ) const
    {
    return false;
    }
  bool operator==( const WeightedGradientMagnitude & other ) const
    {
    return !(*this != other);
    }
  inline TOutput operator()( const TInput & A ) const
    {
      vnl_vector<double> B = element_quotient(A.GetVnlVector(), this->m_Weights.GetVnlVector());
      return static_cast<TOutput>( B.magnitude() );
    }
    
private:
   TInput   m_Weights;   
}; 
}
 
template <class TInputImage, class TOutputImage>
class ITK_EXPORT WeightedGradientToMagnitudeImageFilter :
    public
UnaryFunctorImageFilter<TInputImage,TOutputImage, 
                        Functor::WeightedGradientMagnitude< typename TInputImage::PixelType, 
                                                    typename TOutputImage::PixelType>   >
{
public:
  /** Standard class typedefs. */
  typedef WeightedGradientToMagnitudeImageFilter Self;
  typedef UnaryFunctorImageFilter<
    TInputImage,TOutputImage, 
    Functor::WeightedGradientMagnitude< typename TInputImage::PixelType,
                                typename TOutputImage::PixelType> >
                                         Superclass;
  typedef SmartPointer<Self>             Pointer;
  typedef SmartPointer<const Self>       ConstPointer;
  
  typedef typename Superclass::InputImageType           InputImageType;
  typedef typename Superclass::InputImagePixelType      InputImagePixelType;
  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(WeightedGradientToMagnitudeImageFilter, 
               UnaryFunctorImageFilter);

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
    (Concept::HasNumericTraits<typename TInputImage::PixelType::ValueType>));
  /** End concept checking */
#endif

  itkSetMacro( Weights,  InputImagePixelType);
  itkGetConstMacro( Weights,  InputImagePixelType);

  /** Process to execute before entering the multithreaded section */
  void BeforeThreadedGenerateData(void);

  /** Print internal ivars */
  void PrintSelf(std::ostream& os, Indent indent) const;

protected:
  WeightedGradientToMagnitudeImageFilter();
  virtual ~WeightedGradientToMagnitudeImageFilter() {}
    
private:
  WeightedGradientToMagnitudeImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented
  
  InputImagePixelType    m_Weights;
};
 
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkWeightedGradientToMagnitudeImageFilter.txx"
#endif

#endif
