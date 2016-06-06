/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkDiscreteGaussianGradientImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2010-06-10 13:02:23 $
  Version:   $Revision: 1.3 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkDiscreteGaussianGradientImageFilter_txx
#define __itkDiscreteGaussianGradientImageFilter_txx

#include "itkNeighborhoodOperatorImageFilter.h"
#include "itkGaussianDerivativeOperator.h"
#include "itkImageRegionIterator.h"
#include "itkProgressAccumulator.h"
#include "itkStreamingImageFilter.h"

namespace itk
{

template <class TInputImage, class TOutputImage>
void
DiscreteGaussianGradientImageFilter<TInputImage,TOutputImage>
::GenerateInputRequestedRegion() throw(InvalidRequestedRegionError)
{
  // call the superclass' implementation of this method. this should
  // copy the output requested region to the input requested region
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the input and output
  typename Superclass::InputImagePointer  inputPtr =
    const_cast< TInputImage *>( this->GetInput() );

  if ( !inputPtr )
    {
    return;
    }

  // Build an operator so that we can determine the kernel size
  GaussianDerivativeOperator<InternalRealType, ImageDimension> oper;
  typename TInputImage::SizeType radius;

  for (unsigned int i = 0; i < TInputImage::ImageDimension; i++)
   {
    oper.SetDirection(i);
    if (m_UseImageSpacing == true)
      {
      if (this->GetInput()->GetSpacing()[i] == 0.0)
        {
        itkExceptionMacro(<< "Pixel spacing cannot be zero");
        }
      else
        {
        oper.SetSpacing(this->GetInput()->GetSpacing()[i]);
        }
      }

    // GaussianDerivativeOperator modifies the variance when setting image spacing
    oper.SetVariance(m_Variance[i]);
    oper.SetMaximumError(m_MaximumError);
    oper.SetMaximumKernelWidth(m_MaximumKernelWidth);
    oper.CreateDirectional();

    radius = oper.GetRadius();

    m_KernelNorms[i] = 0;
    for(unsigned int k = 0; k<oper.GetSize(0); k++)
    {
      m_KernelNorms[i] += pow(oper.GetElement(k),2);
    }
    m_KernelNorms[i] = sqrt(m_KernelNorms[i]);

    m_DerivativeFilters[i] = NeighborhoodOperatorFilterType::New();
  }

  // get a copy of the input requested region (should equal the output
  // requested region)
  typename TInputImage::RegionType inputRequestedRegion;
  inputRequestedRegion = inputPtr->GetRequestedRegion();

  // pad the input requested region by the operator radius
  inputRequestedRegion.PadByRadius( radius );

  // crop the input requested region at the input's largest possible region
  if ( inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()) )
    {
    inputPtr->SetRequestedRegion( inputRequestedRegion );
    return;
    }
  else
    {
    // Couldn't crop the region (requested region is outside the largest
    // possible region).  Throw an exception.

    // store what we tried to request (prior to trying to crop)
    inputPtr->SetRequestedRegion( inputRequestedRegion );

    // build an exception
    InvalidRequestedRegionError e(__FILE__, __LINE__);
    e.SetLocation(ITK_LOCATION);
    e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
    e.SetDataObject(inputPtr);
    throw e;
    }
}


template< class TInputImage, class TOutputImage >
void
DiscreteGaussianGradientImageFilter<TInputImage, TOutputImage>
::GenerateData()
{

	// Create a process accumulator for tracking the progress of this
	// minipipeline
	ProgressAccumulator::Pointer progress = ProgressAccumulator::New();
	progress->SetMiniPipelineFilter(this);

	// Compute the contribution of each filter to the total progress.
	const double weight = 1.0 / ( ImageDimension * ImageDimension );


	int imageDimensionMinus1 = static_cast<int>(ImageDimension)-1;
	for( int i = 0; i<imageDimensionMinus1; i++ )
	{
		progress->RegisterInternalFilter( m_DerivativeFilters[i], weight );
	}

	progress->ResetProgress();

	const typename TInputImage::ConstPointer   inputImage( this->GetInput() );

	m_ImageAdaptor->SetImage( this->GetOutput() );

	m_ImageAdaptor->SetLargestPossibleRegion(
			inputImage->GetLargestPossibleRegion() );

	m_ImageAdaptor->SetBufferedRegion(
			inputImage->GetBufferedRegion() );

	m_ImageAdaptor->SetRequestedRegion(
			inputImage->GetRequestedRegion() );

	m_ImageAdaptor->Allocate();

	GaussianDerivativeOperator<InputPixelType, ImageDimension> oper;

	for( int dim=0; dim < ImageDimension; dim++ )
	{
		// Set up the operator for this dimension
		oper.SetDirection(dim);
		oper.SetOrder(1);
		if (m_UseImageSpacing == true)
		{
			if (inputImage->GetSpacing()[dim] == 0.0)
			{
				itkExceptionMacro(<< "Pixel spacing cannot be zero");
			}
			else
			{
				// convert the variance from physical units to pixels
				double s = inputImage->GetSpacing()[dim];
				s = s*s;
				oper.SetVariance(m_Variance[dim] / s);
			}
		}
		else
		{
			oper.SetVariance(m_Variance[dim]);
		}

		oper.SetMaximumKernelWidth(m_MaximumKernelWidth);
		oper.SetMaximumError(m_MaximumError);
		oper.CreateDirectional();

    m_KernelNorms[dim] = 0;
    for(unsigned int k = 0; k < oper.GetSize(0); k++)
    {
      m_KernelNorms[dim] += pow(oper.GetElement(k),2);
    }
    m_KernelNorms[dim] = sqrt(m_KernelNorms[dim]);

		m_DerivativeFilters[dim]->SetInput( inputImage );

		m_DerivativeFilters[dim]->SetOperator( oper );

		progress->RegisterInternalFilter(m_DerivativeFilters[dim],1.0f/ImageDimension);

		m_DerivativeFilters[dim]->Update();

	  progress->ResetFilterProgressAndKeepAccumulatedProgress();

	  m_ImageAdaptor->SelectNthElement( dim );

	  typename RealImageType::Pointer derivativeImage;
	  derivativeImage = m_DerivativeFilters[dim]->GetOutput();

	  ImageRegionIteratorWithIndex< RealImageType > it(
	    derivativeImage,
	    derivativeImage->GetRequestedRegion() );

	  ImageRegionIteratorWithIndex< OutputImageAdaptorType > ot(
	    m_ImageAdaptor,
	    m_ImageAdaptor->GetRequestedRegion() );

	  const RealType spacing = inputImage->GetSpacing()[ dim ];

	  it.GoToBegin();
	  ot.GoToBegin();
	  while( !it.IsAtEnd() )
	    {
	    ot.Set( it.Get() / spacing );
	    ++it;
	    ++ot;
	    }
	}
}


template< class TInputImage, class TOutputImage >
void
DiscreteGaussianGradientImageFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Variance: " << m_Variance << std::endl;
  os << indent << "MaximumError: " << m_MaximumError << std::endl;
  os << indent << "MaximumKernelWidth: " << m_MaximumKernelWidth << std::endl;
  os << indent << "UseImageSpacing: " << m_UseImageSpacing << std::endl;
  os << indent << "InternalNumberOfStreamDivisions: " << m_InternalNumberOfStreamDivisions << std::endl;
}

} // end namespace itk

#endif
