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

#ifndef __itkSymbaRegistrationMethod_hxx
#define __itkSymbaRegistrationMethod_hxx

#include "itkSymbaRegistrationMethod.h"

namespace itk
{
/**
 * Constructor
 */
template< unsigned int NDimensions >
SymbaRegistrationMethod< NDimensions >
::SymbaRegistrationMethod()
{
  m_Verbose = false;
  m_NumberOfLevels = 1;
  m_NumberOfImages = 1;

  m_CombinationCost = CombinationCostFunctionType::New();


  m_Optimizer = GOAOptimizationType::New();

  this->SetNumberOfImages(1);

  this->SetNumberOfLevels(1);

  m_Selectivity = 3;

  m_SymmetryEnabled = true;

  m_RigidTransform = NULL;

  m_FixedMask = NULL;

}

template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::SetNumberOfLevels( unsigned int nbrOfLevels )
{

  m_NumberOfVoxelsList.resize(nbrOfLevels);
  m_NumberOfIterationsList.resize(nbrOfLevels);
  m_GradientScaleList.resize(nbrOfLevels);
  m_ImageSpacingList.resize(nbrOfLevels);
  m_SmoothingSigmaList.resize(nbrOfLevels);
  m_DistanceVarianceList.resize(nbrOfLevels);
  m_SamplingModeList.resize(nbrOfLevels);
  m_KnotSpacingList.resize(nbrOfLevels);
  m_TransformList.resize(nbrOfLevels);
  for( unsigned int l = 0; l < nbrOfLevels; ++l )
    {
    m_NumberOfVoxelsList[l] = 16000;
    m_NumberOfIterationsList[l] = 100;
    m_GradientScaleList[l] = 2.0;
    m_ImageSpacingList[l] = 2.0;
    m_SmoothingSigmaList[l] = 0;
    m_DistanceVarianceList[l] = 16;
    m_SamplingModeList[l] = GOAOptimizationType::SubsetSampling;
    m_KnotSpacingList[l] = 64;
    m_TransformList[l] = NULL;
    }

}

template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::SetNumberOfImages( unsigned int nbrOfImages )
{
  m_NumberOfImages = nbrOfImages;
  m_FixedImageList.resize(nbrOfImages);
  m_MovingImageList.resize(nbrOfImages);
  m_FixedGradientImageList.resize(nbrOfImages);
  m_MovingGradientImageList.resize(nbrOfImages);
  m_FixedCannyImageList.resize(nbrOfImages);
  m_MovingCannyImageList.resize(nbrOfImages);
  m_FixedDistanceImageList.resize(nbrOfImages);
  m_MovingDistanceImageList.resize(nbrOfImages);
  m_FixedIndicesList.resize(nbrOfImages);
  m_MovingIndicesList.resize(nbrOfImages);
  m_CostList.resize(nbrOfImages);  
  for( unsigned int i = 0; i < m_NumberOfImages; ++i )
    {
    m_FixedImageList[i] = NULL;
    m_MovingImageList[i] = NULL;
    m_FixedGradientImageList[i] = NULL;
    m_MovingGradientImageList[i] = NULL;
    m_FixedCannyImageList[i] = NULL;
    m_MovingCannyImageList[i] = NULL;
    m_FixedDistanceImageList[i] = NULL;
    m_MovingDistanceImageList[i] = NULL;
    m_CostList[i] = NULL;
    }  
}

/**
 * Initialize by setting the interconnects between components.
 */
template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::Initialize()
{

  for (int i = 0; i < m_NumberOfImages; ++i)
    {
    if ( !m_FixedImageList[i] )
      {
      itkExceptionMacro(<< "FixedImage is not present at index " << i );
      }
    if ( !m_MovingImageList[i] )
      {
      itkExceptionMacro(<< "MovingImage is not present at index " << i );
      }      
    }

}

template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::PreProcessImages(unsigned int level, unsigned int index)
{
  GradientFilterPointer fixedGradient = GradientFilterType::New();
  fixedGradient->SetInput( m_FixedImageList[index] );
  fixedGradient->SetGradientScale( m_GradientScaleList[level] );
  fixedGradient->SetImageSpacing(  m_ImageSpacingList[level] );
  fixedGradient->Update();
  m_FixedGradientImageList[index] = fixedGradient->GetOutput();

  if( m_Verbose )
    {
    std::cout << "Computing Fixed Canny Image" << std::endl;
    }

  RescalePointer fixedRescaler = RescaleFilterType::New();
  fixedRescaler->SetInput( m_FixedImageList[index] );
  fixedRescaler->SetImageSpacing( m_ImageSpacingList[level] );
  fixedRescaler->Update();

  CannyEdgePointer  fixedCannyDetector = CannyEdgeFilterType::New();
  fixedCannyDetector->SetInput( fixedRescaler->GetOutput() );  
  fixedCannyDetector->SetUsePercentiles( true );
  //TODO: Change !
  fixedCannyDetector->SetLowerPercentile( 0.5 );
  fixedCannyDetector->SetUpperPercentile( 0.8 );
  fixedCannyDetector->SetVariance( pow( m_GradientScaleList[level] * fixedRescaler->GetOutput()->GetSpacing()[0], 2.0 ) ) ;      

  typename MaskResampleFilterType::Pointer fixedMaskResampler = MaskResampleFilterType::New();
  if( m_FixedMask )
    {
    fixedMaskResampler->SetInput( m_FixedMask );        
    fixedMaskResampler->SetOutputParametersFromImage( fixedRescaler->GetOutput() );
    fixedMaskResampler->Update();
    fixedCannyDetector->SetMaskImage( fixedMaskResampler->GetOutput() );        
    }
  fixedCannyDetector->Update();
  m_FixedCannyImageList[index] = fixedCannyDetector->GetOutput();

  if( m_SymmetryEnabled )
    {
    if( m_Verbose )
      {
      std::cout << "Computing Fixed Distance Image" << std::endl;
      }

    DistanceFilterPointer fixedDistanceFilter = DistanceFilterType::New();
    fixedDistanceFilter->SetInput( m_FixedCannyImageList[index] );
    fixedDistanceFilter->Update();
    m_FixedDistanceImageList[index] = fixedDistanceFilter->GetOutput();
    }

  if( m_Verbose )
    {
    std::cout << "Computing Moving Gradient Image" << std::endl;
    }
  GradientFilterPointer movingGradient = GradientFilterType::New();
  movingGradient->SetInput( m_MovingImageList[index] );
  movingGradient->SetGradientScale( m_GradientScaleList[level] );
  movingGradient->SetImageSpacing( m_ImageSpacingList[level] );
  movingGradient->Update();
  m_MovingGradientImageList[index] = movingGradient->GetOutput();

  RescalePointer movingRescaler = RescaleFilterType::New();
  movingRescaler->SetInput( m_MovingImageList[index] );
  movingRescaler->SetImageSpacing( m_ImageSpacingList[level] );
  movingRescaler->Update();

  if( m_Verbose )
    {
    std::cout << "Computing Moving Canny Image " << std::endl;
    }

  CannyEdgePointer  movingCannyDetector = CannyEdgeFilterType::New();
  movingCannyDetector->SetInput( movingRescaler->GetOutput() );
  movingCannyDetector->SetUsePercentiles( true );
  movingCannyDetector->SetLowerPercentile( 0.5 );
  movingCannyDetector->SetUpperPercentile( 0.8 );      
  movingCannyDetector->SetVariance( pow( m_GradientScaleList[level] * movingRescaler->GetOutput()->GetSpacing()[0], 2.0 )  );
  // if( movingMaskImage )
  //   {
  //   movingMaskResampler->SetOutputParametersFromImage( movingRescaler->GetOutput() );
  //   movingMaskResampler->Update();  
  //   movingCannyDetector->SetMaskImage( movingMaskResampler->GetOutput() );
  //   }

  movingCannyDetector->Update();
  m_MovingCannyImageList[index] = movingCannyDetector->GetOutput();

  if( m_Verbose )
    {
    std::cout << "Computing Moving Distance Image" << std::endl;
    }
  DistanceFilterPointer movingDistanceFilter = DistanceFilterType::New();
  movingDistanceFilter->SetInput( m_MovingCannyImageList[index] );
  movingDistanceFilter->Update();
  m_MovingDistanceImageList[index] = movingDistanceFilter->GetOutput();

  if( m_Verbose )
    {
    std::cout << "Computing Fixed Indices" << std::endl;
    }

  m_FixedIndicesList[index].clear();
  for(unsigned int p = 0; p < m_FixedCannyImageList[index]->GetLargestPossibleRegion().GetNumberOfPixels(); p++)
    {
    typename ImageType::IndexType imageIndex = m_FixedCannyImageList[index]->ComputeIndex(p);
    if (m_FixedCannyImageList[index]->GetPixel(imageIndex) > 0.5)
      {
      m_FixedIndicesList[index].push_back(imageIndex); 
      }         
    }


  m_MovingIndicesList[index].clear();
  if( m_SymmetryEnabled )
    {
      if( m_Verbose )
        {
        std::cout << "Computing Moving Indices" << std::endl;
        }      
      for(unsigned int p = 0; p < m_MovingCannyImageList[index]->GetLargestPossibleRegion().GetNumberOfPixels(); p++)
        {
        typename ImageType::IndexType imageIndex = m_MovingCannyImageList[index]->ComputeIndex(p);
        if ( m_MovingCannyImageList[index]->GetPixel(imageIndex) > 0.5)
          {
          m_MovingIndicesList[index].push_back(imageIndex);
          }
        }
    }

}


template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::InitializeTransform(unsigned int level)
{

  //Find space region that overlaps both images
  typename ImageType::PointType startPoint(100000000);
  typename ImageType::PointType endPoint(-100000000);
  typename ImageType::PointType fixedOrigin = m_FixedGradientImageList[0]->GetOrigin();
  typename ImageType::PointType fixedEnd;
  m_FixedGradientImageList[0]->TransformIndexToPhysicalPoint( m_FixedGradientImageList[0]->ComputeIndex( m_FixedGradientImageList[0]->GetBufferedRegion().GetNumberOfPixels()-1 ), fixedEnd );
  typename ImageType::PointType movingOrigin = m_MovingGradientImageList[0]->GetOrigin();
  typename ImageType::PointType movingEnd;
  m_MovingGradientImageList[0]->TransformIndexToPhysicalPoint( m_MovingGradientImageList[0]->ComputeIndex( m_MovingGradientImageList[0]->GetBufferedRegion().GetNumberOfPixels()-1 ), movingEnd );
  for (unsigned int d = 0; d < ImageDimension; d++)
    {
    if( fixedOrigin[d] < startPoint[d] )
      {
      startPoint[d] = fixedOrigin[d];
      }
    if( fixedEnd[d] < startPoint[d] )
      {
      startPoint[d] = fixedEnd[d];
      }
    if( movingOrigin[d] < startPoint[d] )
      {
      startPoint[d] = movingOrigin[d];
      }
    if( movingEnd[d] < startPoint[d] )
      {
      startPoint[d] = movingEnd[d];
      }


    if( fixedOrigin[d] > endPoint[d] )
      {
      endPoint[d] = fixedOrigin[d];
      }
    if( fixedEnd[d] > endPoint[d] )
      {
      endPoint[d] = fixedEnd[d];
      }
    if( movingOrigin[d] > endPoint[d] )
      {
      endPoint[d] = movingOrigin[d];
      }
    if( movingEnd[d] > endPoint[d])
      {
      endPoint[d] = movingEnd[d];
      }

    }



  typename ImageType::SpacingType fixedSpacing = m_FixedGradientImageList[0]->GetSpacing();
  typename ImageType::SizeType fixedRegionSize = m_FixedGradientImageList[0]->GetLargestPossibleRegion().GetSize();

  m_TransformList[level] = NonRigidTransformType::New();
  ParametersType fixedParameters = ParametersType(ImageDimension * (3 + ImageDimension) + 2);
  typename ImageType::RegionType::SizeType coeffSize, radiusSize;
  for (unsigned int d = 0; d < ImageDimension; d++)
    {
    radiusSize[d] = 1 + ceil( ( 0.5 * ( endPoint[d] - startPoint[d] ) ) / m_KnotSpacingList[level]);
    coeffSize[d] = 2 * radiusSize[d] + 1;
    //Size
    fixedParameters[d] = coeffSize[d];
    //Origin = center -
    fixedParameters[d + ImageDimension] = (startPoint[d] + 0.5 * ( endPoint[d] - startPoint[d] ) ) - radiusSize[d] * m_KnotSpacingList[level];
    //Spacing
    fixedParameters[d + 2 * ImageDimension] = m_KnotSpacingList[level];
    //Direction
    for (unsigned int d2 = 0; d2 < ImageDimension; d2++)
      {
      if (d == d2)
        {
        fixedParameters[3 * ImageDimension + d*ImageDimension + d2] = 1.0;
        }
      else
        {
        fixedParameters[3 * ImageDimension + d*ImageDimension + d2] = 0.0;
        }
      }
    }
  fixedParameters[ImageDimension * (3 + ImageDimension) + 0] = 2;
  fixedParameters[ImageDimension * (3 + ImageDimension) + 1] = m_SmoothingSigmaList[level];
  m_TransformList[level]->SetFixedParameters( fixedParameters );
  ParametersType initialParameters = ParametersType(m_TransformList[level]->GetNumberOfParameters());
  initialParameters.Fill(0);
  m_TransformList[level]->SetParameters(initialParameters);

 if( m_RigidTransform )
   {
   m_TransformList[level]->SetRigidTransform( m_RigidTransform );
   }


  if( level > 0 )
    {
    initialParameters = m_TransformList[level]->InitializeVelocityField( m_TransformList[level-1] );
    } 

  if( m_Verbose )
    {
    std::cout << "Initializing Metrics"   << std::endl;
    }


}

template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::InitializeCostFunction(unsigned int level, unsigned int index)
{

  m_CostList[index] = CostFunctionType::New();
  m_CostList[index]->SetTransform(m_TransformList[level]);
  m_CostList[index]->SetFixedVectorImage( m_FixedGradientImageList[index] );
  m_CostList[index]->SetMovingVectorImage( m_MovingGradientImageList[index] );
  if( m_SymmetryEnabled )
    {
    m_CostList[index]->SetFixedDistanceImage( m_FixedDistanceImageList[index] );
    }
  m_CostList[index]->SetMovingDistanceImage( m_MovingDistanceImageList[index] );
  m_CostList[index]->SetDistanceVariance( m_DistanceVarianceList[level] );
  m_CostList[index]->SetFixedIndices( m_FixedIndicesList[index] );
  m_CostList[index]->SetMovingIndices( m_MovingIndicesList[index] );
  m_CostList[index]->SetN( m_Selectivity );
  m_CostList[index]->Initialize();

}

/**
 * Starts the Optimization process
 */
template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::StartOptimization(void)
{

  /** Where the magic happens **/

  for (unsigned int l = 0; l < m_NumberOfLevels; ++l)
    {
    if( m_Verbose )
      {
      std::cout << "Initializing level " << l+1 << " of " << m_NumberOfLevels << std::endl;
      }

    for (int i = 0; i < m_NumberOfImages; ++i)
      {
      this->PreProcessImages(l, i);
      }

    this->InitializeTransform(l);

    for (int i = 0; i < m_NumberOfImages; ++i)
      {
      this->InitializeCostFunction(l, i);
      m_CombinationCost->AddCostFunction( m_CostList[i], ( 1.0 / (double)m_NumberOfImages ) );
      }

    ParametersType initialParameters = ParametersType( m_TransformList[l]->GetNumberOfParameters() );
    initialParameters.Fill(0);
    m_Optimizer->SetCostFunction( m_CombinationCost  );
    m_Optimizer->SetVerbose( m_Verbose  );
    m_Optimizer->SetNumberOfIterations( m_NumberOfIterationsList[l] );
    m_Optimizer->SetInitialParameters( initialParameters );
    m_Optimizer->SetNumberOfSamples( m_NumberOfVoxelsList[l] );
    m_Optimizer->SetSamplingMode( m_SamplingModeList[l] );

    if( m_Verbose )
      {
      std::cout << "Starting optimization"   << std::endl;
      }

    itk::TimeProbe optimizationTimer;
    optimizationTimer.Start();
    try
      {
      // do the optimization
      m_Optimizer->StartOptimization();
      }
    catch ( ExceptionObject & err )
      {
      // Pass exception to caller
      throw err;
      }      
    optimizationTimer.Stop();

    ParametersType optimizedParameters = ParametersType( m_Optimizer->GetOptimizedParameters() );
    m_TransformList[l]->SetParameters( optimizedParameters );

    if( m_Verbose )
      {
      std::cout << "Optimization at level " << l << " took " << optimizationTimer.GetTotal() << std::endl;      
      }

    }




}

/**
 * PrintSelf
 */
template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

/*
 * Generate Data
 */
template< unsigned int NDimensions >
void
SymbaRegistrationMethod< NDimensions >
::GenerateData()
{
  ParametersType empty(1);
  empty.Fill(0.0);
  try
    {
    // initialize the interconnects between components
    this->Initialize();
    }
  catch ( ExceptionObject & err )
    {

    // pass exception to caller
    throw err;
    }

  this->StartOptimization();
}

/**
 *  Get Output
 */
template< unsigned int NDimensions >
const typename SymbaRegistrationMethod< NDimensions >::TransformOutputType *
SymbaRegistrationMethod< NDimensions >
::GetOutput() const
{
  return static_cast< const TransformOutputType * >( this->ProcessObject::GetOutput(0) );
}



} // end namespace itk
#endif
