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

#ifndef __itkRescaledImageFilter_hxx
#define __itkRescaledImageFilter_hxx

#include "itkRescaledImageFilter.h"

namespace itk
{

// Constructor with default arguments
template< class TInputImage, class TOutputImage>
RescaledImageFilter<TInputImage, TOutputImage>
::RescaledImageFilter()
{

  m_ImageSpacing = 2.0;

  m_Smoother  = SmootherType::New();

  m_Resampler = ResamplerType::New();
  m_Resampler->SetInput( m_Smoother->GetOutput() );


}

// Destructor
template< class TInputImage, class TOutputImage>
RescaledImageFilter<TInputImage, TOutputImage>
::~RescaledImageFilter()
{

}

template< class TInputImage, class TOutputImage>
void
RescaledImageFilter<TInputImage, TOutputImage>
::GenerateData()
{
  typename InputImageType::ConstPointer input = this->GetInput();

  if( m_ImageSpacing > 0 )
    {
    m_Smoother->SetInput( input );
    m_Smoother->SetSigma( m_ImageSpacing ); 
    m_Smoother->UpdateLargestPossibleRegion();


    typename InputImageType::SizeType inputSize = input->GetLargestPossibleRegion().GetSize();
    typename InputImageType::SizeType outputSize;
    typename InputImageType::SpacingType outputSpacing;
    outputSpacing.Fill( m_ImageSpacing );

    for( unsigned int d = 0; d < SpaceDimension; ++d )
      {
        outputSize[d] = ceil( (input->GetSpacing()[d] * static_cast<double>(inputSize[d])) / (static_cast<double>(outputSpacing[d])) );
      }

    m_Resampler->SetInput( m_Smoother->GetOutput() );
    m_Resampler->SetSize(outputSize);
    m_Resampler->SetOutputSpacing(outputSpacing);
    m_Resampler->SetOutputOrigin( input->GetOrigin() );
    m_Resampler->SetOutputDirection( input->GetDirection() );
    m_Resampler->UpdateLargestPossibleRegion();
   
    }
  else
    {

    typename InputImageType::SizeType inputSize = input->GetLargestPossibleRegion().GetSize();
    
    m_Resampler->SetInput( input );
    m_Resampler->SetSize( inputSize );
    m_Resampler->SetOutputSpacing( input->GetSpacing() );
    m_Resampler->SetOutputOrigin( input->GetOrigin() );
    m_Resampler->SetOutputDirection( input->GetDirection() );
    m_Resampler->UpdateLargestPossibleRegion();    

    }

  this->GraftOutput( m_Resampler->GetOutput() );
}

} // namespace
#endif