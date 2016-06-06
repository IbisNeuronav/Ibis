/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

/** Parts of the code were taken from an ITK file.
 * Original ITK copyright message, just for reference: */
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date: 2008-04-15 19:54:41 +0200 (Tue, 15 Apr 2008) $
  Version:   $Revision: 1573 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkMultiResolutionDiscreteGaussianDerivativePyramidImageFilter_txx
#define _itkMultiResolutionDiscreteGaussianDerivativePyramidImageFilter_txx

#include "itkMultiResolutionDiscreteGaussianDerivativePyramidImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkDiscreteGaussianGradientImageFilter.h"
#include "itkWeightedGradientToMagnitudeImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkExceptionObject.h"

#include "vnl/vnl_math.h"

namespace itk
{


/*
 * Constructor
 */
template <class TInputImage, class TOutputImage,class TGradientOutputImage>
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::MultiResolutionDiscreteGaussianDerivativePyramidImageFilter()
{
	this->m_Percentile = 0.9;
	this->m_Threshold = 0.0;	
	this->m_ComputeMask = true;
	this->m_FirstRun = true;
	this->m_OriginalMaskImageSet = false;
}


template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::SetNumberOfLevels( unsigned int num )
{
  if( this->m_NumberOfLevels == num )
    {
    return;
    }

  this->Modified();

  // clamp value to be at least one
  this->m_NumberOfLevels = num;
  if( this->m_NumberOfLevels < 1 ) this->m_NumberOfLevels = 1;

  // resize the schedules
  ScheduleType temp( this->m_NumberOfLevels, ImageDimension );
  temp.Fill( 0 );
  this->m_Schedule = temp;
 

  // determine initial shrink factor
  unsigned int startfactor = 1;
  startfactor = startfactor << ( this->m_NumberOfLevels - 1 );

  // set the starting shrink factors
  this->SetStartingShrinkFactors( startfactor );
  
  VarianceScheduleType temp2( this->m_NumberOfLevels, ImageDimension );
  for(unsigned int i=0;i<this->m_NumberOfLevels; i++)
  {
    for(unsigned int j=0;j<ImageDimension; j++)
    {
      temp2[i][j] = this->m_Schedule[i][j];
    }  
  }
  this->m_VarianceSchedule = temp2;
  //std::cout << "m_VarianceSchedule\t" << this->m_VarianceSchedule << std::endl;
  // set the required number of outputs
  this->SetNumberOfRequiredOutputs( this->m_NumberOfLevels );

  unsigned int numOutputs = static_cast<unsigned int>( this->GetNumberOfOutputs() );
  unsigned int idx;
  if( numOutputs < this->m_NumberOfLevels )
    {
    // add extra outputs
    for( idx = numOutputs; idx < this->m_NumberOfLevels; idx++ )
      {
      typename DataObject::Pointer output =
        this->MakeOutput( idx );
      this->SetNthOutput( idx, output.GetPointer() );
      }

    }
  else if( numOutputs > this->m_NumberOfLevels )
    {
    // remove extra outputs
    for( idx = this->m_NumberOfLevels; idx < numOutputs; idx++ )
      {
#if ITK_VERSION_MAJOR > 3
      this->RemoveOutput( idx );        
#else      
      typename DataObject::Pointer output =
        this->GetOutputs()[idx];
      this->RemoveOutput( output );
#endif      
      }
    }

}



/*
 * Set the multi-resolution schedule
 */
template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::SetSchedule( const ScheduleType& schedule )
{
  if( schedule.rows() != this->m_NumberOfLevels ||
      schedule.columns() != ImageDimension )
  {
    itkDebugMacro(<< "Schedule has wrong dimensions" );
    return;
  }

  if( this->m_Schedule.rows() != this->m_NumberOfLevels ||
      this->m_Schedule.columns() != ImageDimension )
  {
    itkDebugMacro(<< "m_Schedule has wrong dimensions" );
    return;
  }

  if( schedule == this->m_Schedule )
  {
    return;
  }
  this->Modified();
  unsigned int level, dim;
  for( level = 0; level < this->m_NumberOfLevels; level++ )
  {
    for( dim = 0; dim < ImageDimension; dim++ )
    {

      this->m_Schedule[level][dim] = schedule[level][dim];

      /** Minimum schedule of 0. For the rest no restrictions
       * as imposed in the superclass */
      if( this->m_Schedule[level][dim] < 0 )
      {
        this->m_Schedule[level][dim] = 0;
      }
    }
  }
}

/*
 * GenerateData for non downward divisible schedules
 */
template <class TInputImage, class TOutputImage, class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::GenerateData()
{

  if( this->m_VarianceSchedule.rows() != this->m_NumberOfLevels ||
      this->m_VarianceSchedule.columns() != ImageDimension )
  {
    itkExceptionMacro(<< "VarianceSchedule has wrong dimensions" );
  }

  if(!this->m_FirstRun)
  {
    return;
  }
  this->m_FirstRun = false;

  this->m_KernelNormsMatrix.SetSize(this->m_NumberOfLevels, ImageDimension);

  m_GradientImageArray.resize(this->m_NumberOfLevels);
  m_MaskImageArray.resize(this->m_NumberOfLevels);  
  // Get the input and output pointers
  InputImageConstPointer  inputPtr = this->GetInput();

  // Create caster and smoother  filters
  typedef DiscreteGaussianGradientImageFilter<InputImageType, GradientOutputImageType>
                                                                          SmootherType;
  typedef typename SmootherType::Pointer                                  SmootherPointer;
  typedef typename ImageSource<GradientOutputImageType>::Pointer          BaseFilterPointer;
  typedef FixedArray<SmootherPointer, ImageDimension>                     SmootherArrayType;
  typedef FixedArray<BaseFilterPointer, ImageDimension>                   SmootherPointerArrayType;
  typedef typename InputImageType::SpacingType                            SpacingType;
  
  typedef WeightedGradientToMagnitudeImageFilter<GradientOutputImageType,OutputImageType>
                                                                          MagnitudeFilterType;
  typedef typename MagnitudeFilterType::Pointer                           MagnitudeFilterPointer;
  
  typedef itk::BinaryThresholdImageFilter<
      OutputImageType, OutputImageType>                   ThresholdFilterType;
  typedef typename ThresholdFilterType::Pointer           ThresholdFilterPointer;  

  //SmootherPointer smoother = SmootherType::New();
  std::vector<SmootherPointer>   smoothers;
  smoothers.resize(this->m_NumberOfLevels);
  //MagnitudeFilterPointer magnitudeFilter = MagnitudeFilterType::New();
  std::vector<MagnitudeFilterPointer>   magnituders;
  magnituders.resize(this->m_NumberOfLevels);  
  

  //ThresholdFilterPointer thFilter = ThresholdFilterType::New();
  std::vector<ThresholdFilterPointer>   thresholders;
  thresholders.resize(this->m_NumberOfLevels);
 

  /** Set the standard deviation and do the smoothing */
  unsigned int ilevel, idim;
//  unsigned int factors[ImageDimension];
//  double       stdev[ImageDimension];
  double       variance[ImageDimension];
  SpacingType  spacing = inputPtr->GetSpacing();

  for( ilevel = this->m_NumberOfLevels-1; ilevel+1 > 0; ilevel-- )
  {
    
     this->UpdateProgress( static_cast<float>( ilevel - (this->m_NumberOfLevels-1) ) /
                          static_cast<float>( this->m_NumberOfLevels ) );   

    OutputImagePointer outputPtr = this->GetOutput( ilevel );
    outputPtr->SetBufferedRegion( outputPtr->GetRequestedRegion() );
    outputPtr->Allocate();

    for( idim = 0; idim < ImageDimension; idim++ )
    {
      //factors[idim] = this->m_Schedule[ilevel][idim];
      /** Compute the standard deviation: 0.5 * factor * spacing
       * This is exactly like in the superclass
       * In the superclass, the DiscreteGaussianImageFilter is used, which
       * requires the variance, and has the option to ignore the image spacing.
       * That's why the formula looks maybe different at first sight.   */
      //stdev[idim] = 0.5 * static_cast<float>( factors[idim] )*spacing[idim];
      //variance[idim] = pow(stdev[idim],2);
      variance[idim] = this->m_VarianceSchedule[ilevel][idim];
    }
    smoothers[ilevel] = SmootherType::New();
    smoothers[ilevel]->SetVariance( variance );
    smoothers[ilevel]->SetInput( inputPtr );
    
    smoothers[ilevel]->Update();
    
    //m_GradientImageArray[ilevel] = GradientOutputImageType::New();
    //m_GradientImageArray[ilevel]->SetBufferedRegion(smoother->GetOutput()->GetBufferedRegion() );
    //m_GradientImageArray[ilevel]->Allocate();
    //smoother->GraftOutput( m_GradientImageArray[ilevel] );
    m_GradientImageArray[ilevel] = smoothers[ilevel]->GetOutput();

    const ArrayType kernelNorms = smoothers[ilevel]->GetKernelNorms( );
    GradientPixelType weights;
    for(unsigned int dim=0; dim<ImageDimension; dim++)
      {
      this->m_KernelNormsMatrix[ilevel][dim] = kernelNorms[dim];
      weights[dim] = kernelNorms[dim];
      }
    magnituders[ilevel] = MagnitudeFilterType::New();  
    magnituders[ilevel]->SetInput( smoothers[ilevel]->GetOutput() );
    magnituders[ilevel]->SetWeights( weights );
    
    magnituders[ilevel]->GraftOutput( outputPtr );
    magnituders[ilevel]->Update();
    
    this->GraftNthOutput( ilevel, magnituders[ilevel]->GetOutput() );

    
    if(this->m_ComputeMask)
    {
      if(ilevel == (this->m_NumberOfLevels-1)) // Need to compute threshold
      {
        std::cout << "Computing Threshold at index " << ilevel << " of " << this->m_NumberOfLevels << std::endl;
        this->m_Threshold = this->EvaluateThresholdBasedOnPercentile(ilevel);
      }
      else
      {
        std::cout << "NOT Computing Threshold at index " << ilevel << " of " << this->m_NumberOfLevels << std::endl;
      }
      thresholders[ilevel] = ThresholdFilterType::New();
      thresholders[ilevel]->SetInput( outputPtr );
      thresholders[ilevel]->SetLowerThreshold( this->m_Threshold );
      thresholders[ilevel]->SetInsideValue(1);
      thresholders[ilevel]->SetOutsideValue(0);
      thresholders[ilevel]->UpdateLargestPossibleRegion();
      m_MaskImageArray[ilevel] = thresholders[ilevel]->GetOutput();
      
      /*
      thFilter->UpdateLargestPossibleRegion();
      m_MaskImageArray[ilevel] = OutputImageType::New();
      m_MaskImageArray[ilevel]->SetBufferedRegion(thFilter->GetOutput()->GetBufferedRegion() );
      m_MaskImageArray[ilevel]->Allocate();      
      thFilter->GraftOutput( m_MaskImageArray[ilevel] );
      */
    }
    
  
  } // for ilevel...

}

template <class TInputImage, class TOutputImage,class TGradientOutputImage>
double
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::EvaluateThresholdBasedOnPercentile(unsigned int ilevel)
{

  typename ListSampleType::Pointer listSample = ListSampleType::New();
  listSample->Clear();

  InputImagePointer magnitudeImage = this->GetOutput( ilevel );

  unsigned int myCnt = 0;

  typedef ImageRegionConstIterator< InputImageType > IteratorType;
  typedef ImageRegionConstIterator< MaskImageType >  MaskIteratorType;
  
  if(this->m_OriginalMaskImageSet)
  {
    std::cout << "Mask has been set and is used for evaluating threshold" << std::cout;
    IteratorType it( magnitudeImage, magnitudeImage->GetLargestPossibleRegion() );
    MaskIteratorType mit( this->m_OriginalMaskImage, this->m_OriginalMaskImage->GetLargestPossibleRegion() );
    it.GoToBegin();
    mit.GoToBegin();
    while (!it.IsAtEnd() && !mit.IsAtEnd())
    {
      MeasurementVectorType m;
      if( mit.Get() > 0 )
        {
        m[0] = it.Get();
        listSample->PushBack(m);
        myCnt++;
        }         
      ++it;
      ++mit;      
    }
  }
  else
  {
    std::cout << "Mask has NOT been set and is NOT used for evaluating threshold" << std::cout;
    IteratorType it( magnitudeImage, magnitudeImage->GetLargestPossibleRegion() );
    it.GoToBegin();
    while (!it.IsAtEnd())
    {
      MeasurementVectorType m;
      m[0] = it.Get();
      listSample->PushBack(m);
      ++it;
      myCnt++;
    }  
  }


  std::cout << "Nbr of Samples: " << listSample->Size() << std::endl;

  SampleToHistogramFilterPointer sampleToHist = SampleToHistogramFilterType::New();
  typename HistogramType::SizeType  histSize(1);
  histSize.Fill(100);
  sampleToHist->SetHistogramSize(histSize);
  sampleToHist->SetInput(listSample);
  sampleToHist->Update();
  const HistogramType * histogram = sampleToHist->GetOutput();

  double thresh = histogram->Quantile(0, this->m_Percentile);
  std::cout << "a Gradient Magnitude Threshold corresponding to Percentile " << this->m_Percentile << "  :  " << thresh << std::endl;

  return thresh;

 }




template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::GetKernelNorms( unsigned long currentLevel, ArrayType & kernelNorms)
{

  if(currentLevel>this->m_NumberOfLevels-1)
    {
    itkExceptionMacro(<<"Current Image Level is higher than total number of levels.");
    }

  if(!this->m_KernelNormsMatrix.size())
    {
    itkExceptionMacro(<<"m_KernelNormsMatrix is not defined.");
    }

  for(unsigned int dim=0; dim<ImageDimension; dim++)
    {
       kernelNorms[dim] = this->m_KernelNormsMatrix[currentLevel][dim];
    }

}

/*
 * PrintSelf method
 */
template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}


/*
 * GenerateOutputInformation
 */
template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::GenerateOutputInformation()
{
  // call the supersuperclass's implementation of this method
  typedef typename Superclass::Superclass SuperSuperclass;
  SuperSuperclass::GenerateOutputInformation();

  // get pointers to the input and output
  InputImageConstPointer inputPtr = this->GetInput();

  if ( !inputPtr  )
    {
    std::cout << this->GetInput() << std::endl;
    itkExceptionMacro( << "Input has not been set" );
    }

  OutputImagePointer outputPtr;

  unsigned int ilevel;
  for( ilevel = 0; ilevel < this->m_NumberOfLevels; ilevel++ )
  {
    /** The same as the input image for each resolution
     * \todo: is this not already done in the supersuperclass?  */
    OutputImagePointer outputPtr = this->GetOutput( ilevel );
    if( !outputPtr ) { continue; }

    outputPtr->SetLargestPossibleRegion( inputPtr->GetLargestPossibleRegion() );
    outputPtr->SetSpacing( inputPtr->GetSpacing() );
  }

}


/*
 * GenerateOutputRequestedRegion
 */
template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::GenerateOutputRequestedRegion(DataObject * refOutput )
{
  // call the supersuperclass's implementation of this method
  typedef typename Superclass::Superclass SuperSuperclass;
  SuperSuperclass::GenerateOutputRequestedRegion( refOutput );

  // find the index for this output
  unsigned int refLevel = refOutput->GetSourceOutputIndex();

  // compute baseIndex and baseSize
  typedef typename OutputImageType::SizeType    SizeType;
  typedef typename SizeType::SizeValueType      SizeValueType;
  typedef typename OutputImageType::IndexType   IndexType;
  typedef typename IndexType::IndexValueType    IndexValueType;
  typedef typename OutputImageType::RegionType  RegionType;

  /** \todo: shouldn't this be a dynamic_cast? */
  TOutputImage * ptr = static_cast<TOutputImage*>( refOutput );
  if( !ptr )
  {
    itkExceptionMacro( << "Could not cast refOutput to TOutputImage*." );
  }

  unsigned int ilevel;

  if ( ptr->GetRequestedRegion() == ptr->GetLargestPossibleRegion() )
  {

    // set the requested regions for the other outputs to their
    // requested region

    for( ilevel = 0; ilevel < this->m_NumberOfLevels; ilevel++ )
      {
      if( ilevel == refLevel ) { continue; }
      if( !this->GetOutput(ilevel) ) { continue; }

      this->GetOutput(ilevel)->SetRequestedRegionToLargestPossibleRegion();
      }

  }
  else
  {
    // compute requested regions for the other outputs based on
    // the requested region of the reference output

    /** Set them all to the same region */
    RegionType outputRegion = ptr->GetRequestedRegion();

    for( ilevel = 0; ilevel < this->m_NumberOfLevels; ilevel++ )
    {
      if( ilevel == refLevel ) { continue; }
      if( !this->GetOutput(ilevel) ) { continue; }

      // make sure the region is within the largest possible region
      outputRegion.Crop( this->GetOutput( ilevel )->
                         GetLargestPossibleRegion() );
      // set the requested region
      this->GetOutput( ilevel )->SetRequestedRegion( outputRegion );
    }
  }

}


/*
 * GenerateInputRequestedRegion
 */
template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::GenerateInputRequestedRegion()
{
  // call the supersuperclass's implementation of this method. This should
  // copy the output requested region to the input requested region
  typedef typename Superclass::Superclass SuperSuperclass;
  SuperSuperclass::GenerateInputRequestedRegion();

  // This filter needs all of the input, because it uses the
  // the GausianRecursiveFilter.
  InputImagePointer image = const_cast<InputImageType *>( this->GetInput() );

  if ( !image )
  {
    itkExceptionMacro( << "Input has not been set." );
  }

  if( image )
  {
    image->SetRequestedRegion( this->GetInput()->GetLargestPossibleRegion() );
  }

}


/*
 * EnlargeOutputRequestedRegion
 */

template <class TInputImage, class TOutputImage,class TGradientOutputImage>
void
MultiResolutionDiscreteGaussianDerivativePyramidImageFilter<TInputImage, TOutputImage, TGradientOutputImage>
::EnlargeOutputRequestedRegion(DataObject *output)
{
  TOutputImage *out = dynamic_cast<TOutputImage*>(output);

  if (out)
  {
    out->SetRequestedRegion( out->GetLargestPossibleRegion() );
  }
}


} // namespace itk

#endif
