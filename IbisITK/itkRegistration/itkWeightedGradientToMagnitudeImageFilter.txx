/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkWeightedGradientToMagnitudeImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-17 16:30:53 $
  Version:   $Revision: 1.12 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkWeightedGradientToMagnitudeImageFilter_txx
#define __itkWeightedGradientToMagnitudeImageFilter_txx

#include "itkWeightedGradientToMagnitudeImageFilter.h"


namespace itk
{

/**
 *
 */
template <class TInputImage, class TOutputImage>
WeightedGradientToMagnitudeImageFilter<TInputImage, TOutputImage>
::WeightedGradientToMagnitudeImageFilter()
{
  m_Weights.Fill(1.0);
}


/**
 *
 */
template <class TInputImage, class TOutputImage>
void 
WeightedGradientToMagnitudeImageFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Weights: "
     << m_Weights
     << std::endl;

}

/**
 *
 */
template <class TInputImage, class TOutputImage>
void 
WeightedGradientToMagnitudeImageFilter<TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{  
  // set up the functor values
  this->GetFunctor().SetWeights( m_Weights );  
}


} // end namespace itk

#endif
