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

#ifndef __itkGOAGradientAscent_hxx
#define __itkGOAGradientAscent_hxx

#include "itkGOAGradientAscent.h"
#include <algorithm>
#include "itkTimeProbe.h"

namespace itk
{

// Constructor with default arguments
template < class TScalarType, unsigned int NInputDimensions>
GOAGradientAscent< TScalarType, NInputDimensions >
::GOAGradientAscent()
{
  m_CostFunction = 0;
  m_Verbose = false;

  m_SamplingMode = FullSampling;
  m_NumberOfSamples = 0;
}

// Destructor
template < class TScalarType, unsigned int NInputDimensions>
GOAGradientAscent< TScalarType, NInputDimensions >
::~GOAGradientAscent()
{

}

//Random Selection of Samples for Stochastic Gradient Ascent
template < class TScalarType, unsigned int NInputDimensions>
typename GOAGradientAscent< TScalarType, NInputDimensions >::IndicesContainer
GOAGradientAscent< TScalarType, NInputDimensions >
::RandomSelection( IndicesContainer fullSet, unsigned int nbrOfSamples )
{
  

  unsigned int subsetSize = fullSet.size() <= nbrOfSamples ? fullSet.size() : nbrOfSamples;
  
  if( fullSet.size() <= nbrOfSamples  )
    {
    return fullSet;
    }
  else
    {
    // itk::TimeProbe shuffleTimer;
    // shuffleTimer.Start();      
    IndicesContainer subSet( subsetSize );
    std::random_shuffle ( fullSet.begin(), fullSet.end() );  
    for( unsigned int i = 0; i < nbrOfSamples; i++ )
      {
        subSet[i] = fullSet[i];
      }
    // shuffleTimer.Stop();      
    // std::cout << "Shuffling took : " << shuffleTimer.GetTotal() << std::endl;
    return subSet;
    }
  
}


//Start Optimization
template < class TScalarType, unsigned int NInputDimensions>
void
GOAGradientAscent< TScalarType, NInputDimensions >
::StartOptimization()
{

  if( !m_CostFunction )
    {
      itkExceptionMacro(<<"Cost Function has not been set");
    }


  IndicesContainer fullSetFixedIndices, subSetFixedIndices;
  IndicesContainer fullSetMovingIndices, subSetMovingIndices;
  
  if( m_SamplingMode != FullSampling  )
    {
     fullSetFixedIndices = m_CostFunction->GetFixedIndices(); 
     fullSetMovingIndices = m_CostFunction->GetMovingIndices(); 
     
     subSetFixedIndices = RandomSelection( fullSetFixedIndices, m_NumberOfSamples );
     subSetMovingIndices = RandomSelection( fullSetMovingIndices, m_NumberOfSamples );
     
     m_CostFunction->SetFixedIndices( subSetFixedIndices );
     m_CostFunction->SetMovingIndices( subSetMovingIndices );
    }
  

  unsigned int iter = 0;
  bool continueOptimization = true;
  double finalMetricValue = 0;
  double initialMetricValue = m_CostFunction->GetValue( m_InitialParameters );
  double expMeanDiff = 1;
  double learningRate = 1000;

  DerivativeType metricGradient = DerivativeType(  m_CostFunction->GetNumberOfParameters() );  
  ParametersType parametersGradient = ParametersType(  m_CostFunction->GetNumberOfParameters() );
  ParametersType lineDirection = ParametersType(  m_CostFunction->GetNumberOfParameters() );  
  m_OptimizedParameters = ParametersType( m_InitialParameters );
  while( continueOptimization  )
    {
      itk::TimeProbe iterationTimer;
      iterationTimer.Start();
      itk::TimeProbe nodeOptimizationTimer1, nodeOptimizationTimer2;
      nodeOptimizationTimer1.Start();      
      if( m_Verbose )
        {
          std::cout << "Iteration: " << iter << std::endl;
        }
      std::vector< ParametersType> vectorLogParameters;
      bool lineDirectionZero = true;

      m_CostFunction->GetDerivative( m_OptimizedParameters, metricGradient );

      nodeOptimizationTimer1.Stop();
      
      itk::TimeProbe lineSearchTimer;
      lineSearchTimer.Start();        
      for( unsigned int i = 0; i < metricGradient.Size(); ++i )
        {          
        parametersGradient[i] = metricGradient[i];  
        }          
      
      m_OptimizedParameters = m_CostFunction->GetTransform()->UpdateParameters( parametersGradient, learningRate );
      // m_OptimizedParameters = m_CostFunction->GetTransform()->GetParameters();
      // for( unsigned int i = 0; i < metricGradient.Size(); ++i )
      //   {          
      //   m_OptimizedParameters[i] += learningRate * metricGradient[i];  
      //   }       
      finalMetricValue = m_CostFunction->GetValue( m_OptimizedParameters );      
      lineSearchTimer.Stop(); 
      iterationTimer.Stop();
      if( m_Verbose )
        {
        std::cout << "Iteration Time: " << iterationTimer.GetTotal() << std::endl;
        }
      if( ( iter % 20 ) == 0 )
        {
        std::cout << "Iter: " << iter << std::setw(3) << "\t Initial : " << std::setw(3) << initialMetricValue;
        std::cout << "\t Final: " << std::setw(3) << finalMetricValue << "\t time: ";
        std::cout << std::setw(3) << iterationTimer.GetTotal() << ",";
        std::cout << std::setw(3) << nodeOptimizationTimer1.GetTotal() << ",";        
        std::cout << std::setw(3)  << lineSearchTimer.GetTotal() ;
        std::cout << "\tLearning Rate " << learningRate << std::endl;
        // std::cout << "Initial" << std::endl << m_InitialParameters << std::endl;        
        // std::cout << "Optimized" << std::endl << m_OptimizedParameters << std::endl;
        }

      expMeanDiff = 0.2*(finalMetricValue-initialMetricValue) + (1-0.2)*expMeanDiff;

      ++iter;
      if( iter >= m_NumberOfIterations || fabs(expMeanDiff) < m_ValueTolerance )
        {
        continueOptimization = false;
        std::cout << "Iter: " << iter << std::setw(3) << "\t Initial : " << std::setw(3) << initialMetricValue;
        std::cout << "\t Final: " << std::setw(3) << finalMetricValue << "\t time: ";
        std::cout << std::setw(3) << iterationTimer.GetTotal() << ",";
        std::cout << std::setw(3) << nodeOptimizationTimer1.GetTotal() << ",";        
        std::cout << std::setw(3)  << lineSearchTimer.GetTotal() ;
        std::cout << "\tLearning Rate : " << learningRate << std::endl;            
        std::cout << "\tExp Mean Diff : " << expMeanDiff << std::endl;
        if( m_SamplingMode != FullSampling )
          {
           m_CostFunction->SetFixedIndices( fullSetFixedIndices );
           m_CostFunction->SetMovingIndices( fullSetMovingIndices );
          }        
        }
      else
        {
        if( m_SamplingMode == StochasticSubsetSampling )
          {
           subSetFixedIndices = RandomSelection( fullSetFixedIndices, m_NumberOfSamples );
           subSetMovingIndices = RandomSelection( fullSetMovingIndices, m_NumberOfSamples );
           
           m_CostFunction->SetFixedIndices( subSetFixedIndices );
           m_CostFunction->SetMovingIndices( subSetMovingIndices );
          }
        }

      if( ( finalMetricValue - initialMetricValue ) > 1e-6 )
      {
        learningRate *= 1.05;
      }  
      else if( ( initialMetricValue - finalMetricValue ) > 1e-12 )
      {
        learningRate *= 0.5;
      }
      initialMetricValue = finalMetricValue;
      if( m_SamplingMode == StochasticSubsetSampling )
        {
        initialMetricValue = m_CostFunction->GetValue( m_OptimizedParameters );
        }
    }

}


// Print self
template < class TScalarType, unsigned int NInputDimensions>
void
GOAGradientAscent< TScalarType, NInputDimensions >
::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);

}

} // namespace


#endif
