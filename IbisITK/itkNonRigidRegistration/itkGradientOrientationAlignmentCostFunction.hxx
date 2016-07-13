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

#ifndef __itkGradientOrientationAlignmentCostFunction_hxx
#define __itkGradientOrientationAlignmentCostFunction_hxx

#include "itkGradientOrientationAlignmentCostFunction.h"
#include "vnl/vnl_math.h" 

namespace itk
{

// Constructor with default arguments
template < class TScalarType, unsigned int NInputDimensions>
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::GradientOrientationAlignmentCostFunction()
{
  m_FixedVectorInterpolator = VectorInterpolatorType::New();
  m_MovingVectorInterpolator = VectorInterpolatorType::New();
  m_N = 2;

  m_FixedDistanceInterpolator = DistanceInterpolatorType::New();
  m_MovingDistanceInterpolator = DistanceInterpolatorType::New();  

  m_FixedDistanceGradient = GradientImageFilterType::New();
  m_MovingDistanceGradient = GradientImageFilterType::New();

  m_FixedDistanceGradientInterpolator = DistanceGradientInterpolatorType::New();
  m_MovingDistanceGradientInterpolator = DistanceGradientInterpolatorType::New();    

  //Orientation Difference
  m_FixedOrientationGradient = OrientationGradientFilterType::New();
  m_MovingOrientationGradient = OrientationGradientFilterType::New();

  m_FixedThetaGradientInterpolator = VectorInterpolatorType::New();
  m_FixedPhiGradientInterpolator = VectorInterpolatorType::New();

  m_MovingThetaGradientInterpolator = VectorInterpolatorType::New();
  m_MovingPhiGradientInterpolator = VectorInterpolatorType::New();

  m_DistanceVariance = 4.0;

  m_Threader = MultiThreaderType::New();
  this->m_ThreaderParameter.metric = this;  
  this->m_NumberOfThreads = this->m_Threader->GetNumberOfThreads();
 
  m_FiniteDifference = true;
}


// Destructor
template < class TScalarType, unsigned int NInputDimensions>
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::~GradientOrientationAlignmentCostFunction()
{

}

/**
 * Set the number of threads. This will be clamped by the
 * multithreader, so we must check to see if it is accepted.
 */
template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::SetNumberOfThreads(ThreadIdType numberOfThreads)
{
  m_Threader->SetNumberOfThreads(numberOfThreads);
  m_NumberOfThreads = m_Threader->GetNumberOfThreads();
}


template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::Initialize(void)
throw ( ExceptionObject )
{
  this->MultiThreadingInitialize();

  m_PerThread = new AlignedPerThreadType[this->m_NumberOfThreads];

  for( ThreadIdType threadID = 0; threadID < this->m_NumberOfThreads; threadID++ )
    {
    m_PerThread[threadID].fixedGradient = ParametersType(this->GetNumberOfParameters());
    m_PerThread[threadID].movingGradient = ParametersType(this->GetNumberOfParameters());
    }


}

/**
 * MultiThreading Initialize
 */
template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::MultiThreadingInitialize(void)
throw ( ExceptionObject )
{
  this->SetNumberOfThreads(m_NumberOfThreads);

  // Allocate the array of transform clones to be used in every thread
  m_ThreaderTransform.clear();
  m_ThreaderTransform.resize( m_NumberOfThreads );  
  for ( ThreadIdType ithread = 0; ithread < m_NumberOfThreads - 1; ++ithread )
    {
    this->m_ThreaderTransform[ithread] = TransformType::New();
    this->m_ThreaderTransform[ithread]->Duplicate( m_Transform );    
    }

}

/**
 * Get the match Measure
 */
template < class TScalarType, unsigned int NInputDimensions>
ITK_THREAD_RETURN_TYPE
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::GetValueMultiThreaded(void *arg)
{
  ThreadIdType                threadID;
  MultiThreaderParameterType *mtParam;

  threadID = ( (MultiThreaderType::ThreadInfoStruct *)( arg ) )->ThreadID;

  mtParam = (MultiThreaderParameterType *)
            ( ( (MultiThreaderType::ThreadInfoStruct *)( arg ) )->UserData );

  mtParam->metric->ComputeIndicesValueThread( threadID );

  return NULL;
}


/**
 * Get the match Measure
 */
template < class TScalarType, unsigned int NInputDimensions>
ITK_THREAD_RETURN_TYPE
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::GetDerivativeMultiThreaded(void *arg)
{
  ThreadIdType                threadID;
  MultiThreaderParameterType *mtParam;

  threadID = ( (MultiThreaderType::ThreadInfoStruct *)( arg ) )->ThreadID;

  mtParam = (MultiThreaderParameterType *)
            ( ( (MultiThreaderType::ThreadInfoStruct *)( arg ) )->UserData );

  mtParam->metric->ComputeIndicesDerivativeThread( threadID );

  return NULL;
}


//GetValue
template < class TScalarType, unsigned int NInputDimensions>
typename GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >::MeasureType
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::GetValue(const ParametersType & parameters) const
{
  return GetValueFullParameters( parameters );
}

template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::GetDerivative( const ParametersType & parameters, DerivativeType & derivative) const
{

  // std::cout << "GetDerivative" << std::endl;
  if(!m_Transform)
    {
      itkExceptionMacro(<< "Transform has not been set");
    }
  m_Transform->SetParameters(parameters);

  for ( ThreadIdType ithread = 0; ithread < m_NumberOfThreads - 1; ++ithread )
    {
    this->m_ThreaderTransform[ithread]->SetParameters( parameters );    
    }  

  ParametersType parameterGradient = ComputeDerivative();

  derivative.SetSize(m_Transform->GetNumberOfParameters()); 
  for( unsigned int n=0; n< m_Transform->GetNumberOfParameters(); ++n )
  {
    derivative[n] = parameterGradient[n];
  }

}

template < class TScalarType, unsigned int NInputDimensions>
typename GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >::MeasureType
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::GetValueFullParameters(const ParametersType & parameters) const
{
  if(!m_Transform)
    {
      itkExceptionMacro(<< "Transform has not been set");
    }
  m_Transform->SetParameters(parameters);
  for ( ThreadIdType ithread = 0; ithread < m_NumberOfThreads - 1; ++ithread )
    {
    this->m_ThreaderTransform[ithread]->SetParameters( parameters );    
    }  

  return ComputeValue();


}

template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::ComputeIndicesValueThread(unsigned int threadId) const
{

  MeasureType metricValue;
  
  MeasureType accumFixedValue = 0;  
  unsigned int fixedValidCount = 0;
  
  MeasureType accumMovingValue = 0;  
  unsigned int movingValidCount = 0;  

  unsigned int numberOfIndicesPerThread, startId, numberOfIndices;

  TransformPointer transform;
  if( threadId > 0 )
  {
    transform = m_ThreaderTransform[threadId - 1];
  }
  else
  {
    transform = m_Transform;
  }


  if ( m_FixedIndices.size() > 0 )
    {
    numberOfIndicesPerThread = ceil( m_FixedIndices.size() / m_NumberOfThreads );
    startId = threadId * numberOfIndicesPerThread;
    numberOfIndices = ( numberOfIndicesPerThread < m_FixedIndices.size() - startId ) ? numberOfIndicesPerThread : m_FixedIndices.size() - startId;


    typename RequestedIndicesContainer::const_iterator indexIterator = m_FixedIndices.begin() + startId;
    for( unsigned int i = 0; i < numberOfIndices; i++ )
      {
      if( EvaluateMetricValue(*indexIterator, metricValue, transform) )
        {
        accumFixedValue+= metricValue;
        fixedValidCount++;
        }
      ++indexIterator;
      } 
    }

  if ( m_MovingIndices.size() > 0 )
    {
    numberOfIndicesPerThread = ceil( m_MovingIndices.size() / m_NumberOfThreads );
    startId = threadId * numberOfIndicesPerThread;
    numberOfIndices = ( numberOfIndicesPerThread < m_MovingIndices.size() - startId ) ? numberOfIndicesPerThread : m_MovingIndices.size() - startId;

    typename RequestedIndicesContainer::const_iterator indexIterator2 = m_MovingIndices.begin() + startId;
    for( unsigned int i = 0; i < numberOfIndices; i++ )
      {
      if( EvaluateMovingIndexMetricValue(*indexIterator2, metricValue, transform) )
        {
        accumMovingValue+= metricValue;
        movingValidCount++;
        }
      ++indexIterator2;
      } 
    }

  m_PerThread[threadId].fixedValue  = accumFixedValue;
  m_PerThread[threadId].fixedValidCount = fixedValidCount;  
  m_PerThread[threadId].movingValue  = accumMovingValue;
  m_PerThread[threadId].movingValidCount = movingValidCount;  
}

//GetValue
template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::ComputeIndicesDerivativeThread(unsigned int threadId) const
{
  // ParametersType metricGradient = m_PerThread[threadId].gradient;
  // metricGradient.Fill( 0 );
  unsigned int fixedValidCount = 0;
  unsigned int movingValidCount = 0;  

  unsigned int numberOfIndicesPerThread, startId, numberOfIndices;

  TransformPointer transform;
  if( threadId > 0 )
  {
    transform = m_ThreaderTransform[threadId - 1];
  }
  else
  {
    transform = m_Transform;
  }


  if ( m_FixedIndices.size() > 0 )
    {
    numberOfIndicesPerThread = ceil( m_FixedIndices.size() / m_NumberOfThreads );
    startId = threadId * numberOfIndicesPerThread;
    numberOfIndices = ( numberOfIndicesPerThread < m_FixedIndices.size() - startId ) ? numberOfIndicesPerThread : m_FixedIndices.size() - startId;
    typename RequestedIndicesContainer::const_iterator indexIterator = m_FixedIndices.begin() + startId;
    for( unsigned int i = 0; i <  numberOfIndices; i++ )
      {
      if( EvaluateMetricGradient(*indexIterator, m_PerThread[threadId].fixedGradient, transform))
        {
        fixedValidCount++;
        }
      ++indexIterator;
      } 

    }

  if ( m_MovingIndices.size() > 0 )
    {
    numberOfIndicesPerThread = ceil( m_MovingIndices.size() / m_NumberOfThreads );
    startId = threadId * numberOfIndicesPerThread;
    numberOfIndices = ( numberOfIndicesPerThread < m_MovingIndices.size() - startId ) ? numberOfIndicesPerThread : m_MovingIndices.size() - startId;

    typename RequestedIndicesContainer::const_iterator indexIterator2 = m_MovingIndices.begin() + startId;
    for( unsigned int i = 0; i < numberOfIndices; i++ )
      {
      if( EvaluateMovingIndexMetricGradient(*indexIterator2, m_PerThread[threadId].movingGradient, transform ) )
        {
        movingValidCount++;
        }
      ++indexIterator2;
      } 
    }

  m_PerThread[threadId].fixedValidCount = fixedValidCount;    
  m_PerThread[threadId].movingValidCount = movingValidCount;  
}


template < class TScalarType, unsigned int NInputDimensions>
typename GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >::MeasureType
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::ComputeValue() const
{

  for( unsigned int threadId = 0; threadId <  m_NumberOfThreads; ++threadId )
  {
    m_PerThread[threadId].fixedValue = 0;         
    m_PerThread[threadId].fixedValidCount = 0;
    m_PerThread[threadId].movingValue = 0;         
    m_PerThread[threadId].movingValidCount = 0;     
  }  


  m_Threader->SetSingleMethod( GetValueMultiThreaded,
                               const_cast< void * >( static_cast< const void * >( &m_ThreaderParameter ) ) );
  m_Threader->SingleMethodExecute();

  MeasureType meanFixedValue = 0;
  MeasureType meanMovingValue = 0;  
  unsigned int fixedValidCount = 0;
  unsigned int movingValidCount = 0;

  for( unsigned int threadId = 0; threadId <  m_NumberOfThreads; ++threadId )
  {
    meanFixedValue += m_PerThread[threadId].fixedValue;
    fixedValidCount += m_PerThread[threadId].fixedValidCount;       
    meanMovingValue += m_PerThread[threadId].movingValue;
    movingValidCount += m_PerThread[threadId].movingValidCount;            
  }     

  MeasureType meanMetricValue = 0;
  if( fixedValidCount > 0 && movingValidCount > 0 )
    {
    meanFixedValue /= fixedValidCount;
    meanMovingValue /= movingValidCount;  
    meanMetricValue = 0.5 * meanFixedValue + 0.5 * meanMovingValue;
    }
  else if( fixedValidCount > 0 )
    {
    meanFixedValue /= fixedValidCount;
    meanMetricValue = meanFixedValue;
    }
  else if( movingValidCount > 0 )
    {
    meanMovingValue /= movingValidCount;  
    meanMetricValue = meanMovingValue;      
    }

  if ( meanMetricValue != meanMetricValue )
  {
    std::cerr << "Big UPS!" << std::endl;
  }

  return meanMetricValue;

}


template < class TScalarType, unsigned int NInputDimensions>
bool 
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::EvaluateMetricValue(IndexType imageIndex, MeasureType & metricValue, TransformPointer transform) const
{

  bool isValid = false;
  VectorType fixedVector = m_FixedVectorImage->GetPixel(imageIndex);
  PointType fixedPoint, movingPoint;
  VectorType fixedVector2;

  if(fixedVector.GetNorm() > 1e-9)
    {
      fixedVector.Normalize();

      m_FixedVectorImage->TransformIndexToPhysicalPoint(imageIndex, fixedPoint);
      transform->TransformPointAndOrientationVector(fixedPoint, fixedVector, movingPoint, fixedVector2);

      if(fixedVector2.GetNorm() > 1e-9)
        {

          if(m_MovingVectorInterpolator->IsInsideBuffer(movingPoint))
            {              
              VectorType movingVector = m_MovingVectorInterpolator->Evaluate(movingPoint);
              if(movingVector.GetNorm() > 1e-9)
                {
                  isValid = true;
                  movingVector.Normalize();
                  metricValue = pow(fixedVector2 * movingVector, m_N);
                  //metricValue = metricValue < 0 ? 0 :  metricValue;
                  if(metricValue != metricValue || metricValue > (1.0 + 1e-9))
                    {
                      std::cerr << "SHIT!" << std::endl;
                    }
                if( m_MovingDistanceImage )    
                  {
                  if( m_MovingDistanceInterpolator->IsInsideBuffer(movingPoint) )
                    {
                    ScalarType distance = m_MovingDistanceInterpolator->Evaluate( movingPoint );
                    ScalarType weight =  exp( - (distance*distance) / ( 2 * m_DistanceVariance ) );
                    metricValue *= weight;
                    }                         
                  }
             
                }
            }
        }
    }

  return isValid;
}

template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::EvaluateFixedVectorMetricValue(VectorType fixedVector, PointType movingPoint, MeasureType & metricValue ) const
{

  metricValue = 0;

  if(m_MovingVectorInterpolator->IsInsideBuffer(movingPoint))
    {              
    VectorType movingVector = m_MovingVectorInterpolator->Evaluate(movingPoint);
    if(movingVector.GetNorm() > 1e-9)
      {
        movingVector.Normalize();
        metricValue = pow(fixedVector * movingVector, m_N);
        //metricValue = metricValue < 0 ? 0 :  metricValue;
        if(metricValue != metricValue || metricValue > (1.0 + 1e-9))
          {
            std::cerr << "SHIT!" << std::endl;
          }
      if( m_MovingDistanceImage )    
        {
        if( m_MovingDistanceInterpolator->IsInsideBuffer(movingPoint) )
          {
          ScalarType distance = m_MovingDistanceInterpolator->Evaluate( movingPoint );
          ScalarType weight =  exp( - (distance*distance) / ( 2 * m_DistanceVariance ) );
          metricValue *= weight;
          }                         
        }
   
      }
    }

}


template < class TScalarType, unsigned int NInputDimensions>
bool 
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::EvaluateMovingIndexMetricValue(IndexType imageIndex, MeasureType & metricValue, TransformPointer transform) const
{

  bool isValid = false;
  VectorType movingVector = m_MovingVectorImage->GetPixel(imageIndex);
  PointType movingPoint, fixedPoint;
  VectorType movingVector2;

  if(movingVector.GetNorm() > 1e-9)
    {
      movingVector.Normalize();
      m_MovingVectorImage->TransformIndexToPhysicalPoint(imageIndex, movingPoint);
      transform->InverseTransformPointAndOrientationVector(movingPoint, movingVector, fixedPoint, movingVector2);

      if(movingVector2.GetNorm() > 1e-9)
        {
          if(m_FixedVectorInterpolator->IsInsideBuffer(fixedPoint))
            {              
              VectorType fixedVector = m_FixedVectorInterpolator->Evaluate(fixedPoint);
              if(fixedVector.GetNorm() > 1e-9)
                {
                  isValid = true;
                  fixedVector.Normalize();
                  metricValue = pow(movingVector2 * fixedVector, m_N);
                  //metricValue = metricValue < 0 ? 0 :  metricValue;
                  if(metricValue != metricValue || metricValue > (1.0 + 1e-9))
                    {
                      std::cerr << "SHIT!" << std::endl;
                    }
                if( m_FixedDistanceImage )
                  {
                  if( m_FixedDistanceInterpolator->IsInsideBuffer(fixedPoint) )
                    {
                    ScalarType distance = m_FixedDistanceInterpolator->Evaluate( fixedPoint );
                    ScalarType weight =  exp( - (distance*distance) / ( 2 * m_DistanceVariance ) );
                    metricValue *= weight;
                    }                     
                  }                                           
                }
            }
        }
    }

  return isValid;
}

template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::EvaluateMovingVectorMetricValue(VectorType movingVector, PointType fixedPoint, MeasureType & metricValue) const
{
  metricValue = 0;
  if(m_FixedVectorInterpolator->IsInsideBuffer(fixedPoint))
    {              
      VectorType fixedVector = m_FixedVectorInterpolator->Evaluate(fixedPoint);
      if(fixedVector.GetNorm() > 1e-9)
        {
          fixedVector.Normalize();
          metricValue = pow(movingVector * fixedVector, m_N);
          //metricValue = metricValue < 0 ? 0 :  metricValue;
          if(metricValue != metricValue || metricValue > (1.0 + 1e-9))
            {
              std::cerr << "SHIT!" << std::endl;
            }
        if( m_FixedDistanceImage )
          {
          if( m_FixedDistanceInterpolator->IsInsideBuffer(fixedPoint) )
            {
            ScalarType distance = m_FixedDistanceInterpolator->Evaluate( fixedPoint );
            ScalarType weight =  exp( - (distance*distance) / ( 2 * m_DistanceVariance ) );
            metricValue *= weight;
            }                     
          }                                           
        }
    }

}



template < class TScalarType, unsigned int NInputDimensions>
typename GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >::ParametersType
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::ComputeDerivative() const
{



  for( unsigned int threadId = 0; threadId <  m_NumberOfThreads; ++threadId )
  {
    m_PerThread[threadId].fixedGradient = ParametersType( this->GetNumberOfParameters() );
    m_PerThread[threadId].fixedGradient.Fill( 0 );    
    m_PerThread[threadId].fixedValidCount = 0;         
    m_PerThread[threadId].movingGradient = ParametersType( this->GetNumberOfParameters() );
    m_PerThread[threadId].movingGradient.Fill( 0 );    
    m_PerThread[threadId].movingValidCount = 0;      
  }  


  m_Threader->SetSingleMethod( GetDerivativeMultiThreaded,
                               const_cast< void * >( static_cast< const void * >( &m_ThreaderParameter ) ) );
  m_Threader->SingleMethodExecute();


  ParametersType fixedGradient = ParametersType( this->GetNumberOfParameters() );
  fixedGradient.Fill( 0 );
  unsigned int fixedValidCount = 0;
  ParametersType movingGradient = ParametersType( this->GetNumberOfParameters() );
  movingGradient.Fill( 0 );
  unsigned int movingValidCount = 0;  
  for( unsigned int threadId = 0; threadId <  m_NumberOfThreads; ++threadId )
    {
    fixedGradient += m_PerThread[threadId].fixedGradient;
    fixedValidCount += m_PerThread[threadId].fixedValidCount;       
    movingGradient += m_PerThread[threadId].movingGradient;
    movingValidCount += m_PerThread[threadId].movingValidCount;             
    }     


  ParametersType meanGradient = ParametersType( this->GetNumberOfParameters() );
  meanGradient.Fill( 0 );
  if( fixedValidCount > 0 && movingValidCount > 0 )
    {
    fixedGradient /= fixedValidCount;
    movingGradient /= movingValidCount;  
    meanGradient = 0.5 * fixedGradient + 0.5 * movingGradient;
    }
  else if( fixedValidCount > 0 )
    {
    fixedGradient /= fixedValidCount;
    meanGradient = fixedGradient;
    }
  else if( movingValidCount > 0 )
    {
    movingGradient /= movingValidCount;  
    meanGradient = movingGradient;      
    }

  
  return meanGradient;

}


template < class TScalarType, unsigned int NInputDimensions>
bool 
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::EvaluateMetricGradient(IndexType imageIndex, ParametersType & metricGradient, TransformPointer transform) const
{

  bool isValid = false;
  VectorType fixedVector = m_FixedVectorImage->GetPixel(imageIndex);
  PointType fixedPoint, movingPoint;
  VectorType fixedVector2;
  typename std::map< unsigned int, ScalarType > jacobianMap;
  typename std::vector< std::pair< unsigned int, ScalarType> > jacobian;


  if(fixedVector.GetNorm() > 1e-9)
    {
      fixedVector.Normalize();

      m_FixedVectorImage->TransformIndexToPhysicalPoint(imageIndex, fixedPoint);
      transform->TransformPointAndOrientationVector(fixedPoint, fixedVector, movingPoint, fixedVector2);

      if(fixedVector2.GetNorm() > 1e-9)
        {
        isValid = true;
        // transform->ComputeJacobianWithRespectToParameters( fixedPoint, jacobianMap );
        transform->ComputeJacobianWithRespectToParameters( fixedPoint, jacobian );

        if( m_FiniteDifference )
          {
          double step = 0.5;
          PointType movingPoint2;
          MeasureType measureP, measureN;
          VectorType localGradient;
          for( unsigned int d=0; d < NInputDimensions; ++d )
            {
            movingPoint2 = movingPoint;
            movingPoint2[d] = movingPoint[d] + step;
            this->EvaluateFixedVectorMetricValue( fixedVector2, movingPoint2, measureP );
            movingPoint2 = movingPoint;
            movingPoint2[d] = movingPoint[d] - step;
            this->EvaluateFixedVectorMetricValue( fixedVector2, movingPoint2, measureN );
            localGradient[d] =  (measureP - measureN) / ( 2 * step );                          
            }

          // for( typename std::map<unsigned int, ScalarType>::iterator it=jacobianMap.begin(); it!=jacobianMap.end(); ++it )
          //   {
          //   unsigned int n = it->first;
          //   for( unsigned int d=0; d < NInputDimensions; ++d )
          //     {
          //     metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
          //     }                        
          //   }  
          for( typename std::vector< std::pair<unsigned int, ScalarType> >::iterator it=jacobian.begin(); it!=jacobian.end(); ++it )
            {
            if( std::fabs(it->second) > 0 )
              {
              unsigned int n = it->first;
              for( unsigned int d=0; d < NInputDimensions; ++d )
                {
                metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
                }                        
              }                      
            } 
                      
          }
        else // Magic Happens
          {


          double localMeasureA, localMeasureAp;
          double localMeasureB, localMeasureBp;
          double theta, phi;
          double cos_theta, sin_theta, cos_phi, sin_phi;

          if(m_MovingVectorInterpolator->IsInsideBuffer(movingPoint))
            {              
            VectorType movingVector = m_MovingVectorInterpolator->Evaluate(movingPoint);
            if(movingVector.GetNorm() > 1e-9)
              {
              movingVector.Normalize();
              localMeasureB = pow(fixedVector2 * movingVector, m_N);
              localMeasureBp = (m_N)*pow(fixedVector2 * movingVector, m_N-1);
              // //theta = vcl_acos( movingVector[2] );
              cos_theta = movingVector[2];
              sin_theta = vcl_sin( vcl_acos(movingVector[2]) );
              phi = vcl_atan2( movingVector[1], movingVector[0] );
              cos_phi = vcl_cos(phi);
              sin_phi = vcl_sin(phi);
              if( m_MovingDistanceInterpolator->IsInsideBuffer(movingPoint) )
                {
                ScalarType distance = m_MovingDistanceInterpolator->Evaluate( movingPoint );
                localMeasureA =  exp( - (distance*distance) / ( 2 * m_DistanceVariance ) );
                localMeasureAp = - (distance / m_DistanceVariance) * localMeasureA;

                //First-Component (based on Distance Gradient)
                VectorType localGradientA = localMeasureAp * localMeasureB * (m_MovingDistanceGradientInterpolator->Evaluate(movingPoint));          

                //Second-Component (based on Orientation Difference)
                VectorType localGradientB; // = localMeasureA * localMeasureBp * ..
                localGradientB.Fill(0);

                VectorType thetaGradient = m_MovingThetaGradientInterpolator->Evaluate( movingPoint );
                VectorType phiGradient = m_MovingPhiGradientInterpolator->Evaluate( movingPoint );

                localGradientB += fixedVector2[0] * ( cos_theta*cos_phi ) * thetaGradient;
                localGradientB -= fixedVector2[0] * ( sin_theta*sin_phi ) * phiGradient;
                localGradientB += fixedVector2[1] * ( cos_theta*sin_phi ) * thetaGradient;
                localGradientB += fixedVector2[1] * ( sin_theta*cos_phi ) * phiGradient;
                localGradientB -= fixedVector2[2] * ( sin_theta ) * thetaGradient;

                VectorType localGradient = localGradientA + localGradientB;

                // for( typename std::map<unsigned int, ScalarType>::iterator it=jacobianMap.begin(); it!=jacobianMap.end(); ++it )
                //   {
                //   unsigned int n = it->first;
                //   for( unsigned int d=0; d < NInputDimensions; ++d )
                //     {
                //     metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
                //     }                        
                //   }   
                for( typename std::vector< std::pair<unsigned int, ScalarType> >::iterator it=jacobian.begin(); it!=jacobian.end(); ++it )
                  {
                  if( std::fabs(it->second) > 0 )
                    {
                    unsigned int n = it->first;
                    for( unsigned int d=0; d < NInputDimensions; ++d )
                      {
                      metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
                      }                        
                    }                      
                  }  
                }                                    
              }
            }



          }


        }
    }

  return isValid;
}

template < class TScalarType, unsigned int NInputDimensions>
bool 
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::EvaluateMovingIndexMetricGradient(IndexType imageIndex, ParametersType & metricGradient, TransformPointer transform) const
{

  bool isValid = false;
  VectorType movingVector = m_MovingVectorImage->GetPixel(imageIndex);
  PointType movingPoint, fixedPoint;
  VectorType movingVector2;
  typename std::map< unsigned int, ScalarType > jacobianMap;
  typename std::vector< std::pair< unsigned int, ScalarType> > jacobian;

  if(movingVector.GetNorm() > 1e-9)
    {
      movingVector.Normalize();

      m_MovingVectorImage->TransformIndexToPhysicalPoint(imageIndex, movingPoint);
      transform->InverseTransformPointAndOrientationVector(movingPoint, movingVector, fixedPoint, movingVector2);

      if(movingVector2.GetNorm() > 1e-9)
        {
        isValid = true;
        // transform->ComputeInverseJacobianWithRespectToParameters( movingPoint, jacobianMap );
        transform->ComputeInverseJacobianWithRespectToParameters( movingPoint, jacobian );
        if( m_FiniteDifference )
          {
          double step = 0.5;
          PointType fixedPoint2;
          MeasureType measureP, measureN;
          VectorType localGradient;
          for( unsigned int d=0; d < NInputDimensions; ++d )
            {
            fixedPoint2 = fixedPoint;
            fixedPoint2[d] = fixedPoint[d] + step;
            this->EvaluateMovingVectorMetricValue( movingVector2, fixedPoint2, measureP );
            fixedPoint2 = fixedPoint;
            fixedPoint2[d] = fixedPoint[d] - step;
            this->EvaluateMovingVectorMetricValue( movingVector2, fixedPoint2, measureN );
            localGradient[d] =  (measureP - measureN) / ( 2 * step );                          
            }

          // for( typename std::map<unsigned int, ScalarType>::iterator it=jacobianMap.begin(); it!=jacobianMap.end(); ++it )
          //   {
          //   unsigned int n = it->first;
          //   for( unsigned int d=0; d < NInputDimensions; ++d )
          //     {
          //     metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
          //     }                        
          //   } 

          for( typename std::vector< std::pair<unsigned int, ScalarType> >::iterator it=jacobian.begin(); it!=jacobian.end(); ++it )
            {
            if( std::fabs(it->second) > 0 )
              {
              unsigned int n = it->first;
              for( unsigned int d=0; d < NInputDimensions; ++d )
                {
                metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
                }                        
              }                      
            }             
          }
        else //Magic Happens
          {

          double localMeasureA, localMeasureAp;
          double localMeasureB, localMeasureBp;
          double theta, phi;
          double cos_theta, sin_theta, cos_phi, sin_phi;

          if(m_FixedVectorInterpolator->IsInsideBuffer(fixedPoint))
            {              
            VectorType fixedVector = m_FixedVectorInterpolator->Evaluate(fixedPoint);
            if(fixedVector.GetNorm() > 1e-9)
              {
              fixedVector.Normalize();
              localMeasureB = pow(fixedVector * movingVector2, m_N);
              localMeasureBp = (m_N)*pow(fixedVector * movingVector2, m_N-1);
              // theta = vcl_acos( fixedVector[2] );
              phi = vcl_atan2( fixedVector[1], fixedVector[0] );
              cos_theta = fixedVector[2];
              sin_theta = vcl_sin( vcl_acos(fixedVector[2]) );
              phi = vcl_atan2( fixedVector[1], fixedVector[0] );
              cos_phi = vcl_cos(phi);
              sin_phi = vcl_sin(phi);

              if( m_FixedDistanceInterpolator->IsInsideBuffer(fixedPoint) )
                {
                ScalarType distance = m_FixedDistanceInterpolator->Evaluate( fixedPoint );
                localMeasureA =  exp( - (distance*distance) / ( 2 * m_DistanceVariance ) );
                localMeasureAp = - (distance / m_DistanceVariance) * localMeasureA;


                //First-Component (based on Distance Gradient)
                VectorType localGradientA = localMeasureAp * localMeasureB * (m_FixedDistanceGradientInterpolator->Evaluate(fixedPoint));          

                //Second-Component (based on Orientation Difference)
                VectorType localGradientB; // = localMeasureA * localMeasureBp * ..
                localGradientB.Fill(0);

                VectorType thetaGradient = m_FixedThetaGradientInterpolator->Evaluate( fixedPoint );
                VectorType phiGradient = m_FixedPhiGradientInterpolator->Evaluate( fixedPoint );

                localGradientB += movingVector2[0] * ( cos_theta*cos_phi ) * thetaGradient;
                localGradientB -= movingVector2[0] * ( sin_theta*sin_phi ) * phiGradient;
                localGradientB += movingVector2[1] * ( cos_theta*sin_phi ) * thetaGradient;
                localGradientB += movingVector2[1] * ( sin_theta*cos_phi ) * phiGradient;
                localGradientB -= movingVector2[2] * ( sin_theta ) * thetaGradient;

                VectorType localGradient = localGradientA + localGradientB;

                // for( typename std::map<unsigned int, ScalarType>::iterator it=jacobianMap.begin(); it!=jacobianMap.end(); ++it )
                //   {
                //   unsigned int n = it->first;
                //   for( unsigned int d=0; d < NInputDimensions; ++d )
                //     {
                //     metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
                //     }                        
                //   }   


                for( typename std::vector< std::pair<unsigned int, ScalarType> >::iterator it=jacobian.begin(); it!=jacobian.end(); ++it )
                  {
                  if( std::fabs(it->second) > 0 )
                    {
                    unsigned int n = it->first;
                    for( unsigned int d=0; d < NInputDimensions; ++d )
                      {
                      metricGradient[n*NInputDimensions+d] += localGradient[d] * (it->second);
                      }                        
                    }                      
                  } 
                }                                    
              }
            }





          }
                          
        }
    }

  return isValid;
}

// Print self
template < class TScalarType, unsigned int NInputDimensions>
void
GradientOrientationAlignmentCostFunction< TScalarType, NInputDimensions >
::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);

}

} // namespace


#endif
