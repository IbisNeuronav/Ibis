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

#ifndef __itkVelocityFieldTransformCustom_hxx
#define __itkVelocityFieldTransformCustom_hxx

#include "itkVelocityFieldTransformCustom.h"

#include "itkContinuousIndex.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"

namespace itk
{

// Constructor with default arguments
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::VelocityFieldTransformCustom() : Superclass( 0 )
{

  this->m_InternalParametersBuffer = ParametersType( 0 );
  // Make sure the parameters pointer is not NULL after construction.
  this->m_InputParametersPointer = &( this->m_InternalParametersBuffer );

  // Instantiate a weights function
  this->m_VectorInterpolationFunction = VectorInterpolationFunctionType::New();

  this->m_VelocityImage = VelocityImageType::New();
  this->m_GradientVelocityImage = VelocityImageType::New();

  this->m_NumberOfSquareCompositions = 2;
  m_Pow2N = pow(2, this->m_NumberOfSquareCompositions);

  this->m_FixedParameters.SetSize( NDimensions * ( NDimensions + 3 )  + 2 );

  m_SmoothingFilter = SmoothingFilterType::New();
  m_SmoothingSigma = 0.0;

  m_RigidTransform = 0;

  m_IntegrationEndTime = 1.0;
}

// Destructor
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::~VelocityFieldTransformCustom()
{

}

// Get the number of parameters
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>::NumberOfParametersType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::GetNumberOfParameters() const
{
  // The number of parameters equal SpaceDimension * number of
  // of pixels in the grid region.
  return SpaceDimension * this->GetNumberOfParametersPerDimension();
}

// Get the number of parameters per dimension
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>::NumberOfParametersType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::GetNumberOfParametersPerDimension() const
{
  // The number of parameters per dimension equal number of
  // of pixels in the grid region.
  NumberOfParametersType numberOfParametersPerDimension = 1;

  for( unsigned int i = 0; i < SpaceDimension; i++ )
    {
      numberOfParametersPerDimension *= static_cast<NumberOfParametersType>( this->m_FixedParameters[i] );
    }
  return numberOfParametersPerDimension;
}

// Set the Fixed Parameters
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::SetFixedParameters( const ParametersType & passedParameters )
{
  // check if the number of passedParameters match the
  // expected number of this->m_FixedParameters
  if( passedParameters.Size() == this->m_FixedParameters.Size() )
    {
      for( unsigned int i = 0; i < NDimensions * ( 3 + NDimensions ) + 2; ++i )
        {
          this->m_FixedParameters[i] = passedParameters[i];
        }
      this->Modified();
    }
  else
    {
      itkExceptionMacro( << "Mismatched between parameters size "
                         << passedParameters.size()
                         << " and the required number of fixed parameters "
                         << this->m_FixedParameters.Size() );
    }

  SizeType gridSize;
  for( unsigned int i = 0; i < NDimensions; i++ )
    {
      gridSize[i] = this->m_FixedParameters[i];
    }
  this->m_VelocityImage->SetRegions( gridSize );

  // Set the origin parameters
  InputPointType origin;
  for( unsigned int i = 0; i < NDimensions; i++ )
    {
      origin[i] = this->m_FixedParameters[NDimensions + i];
    }
  this->m_VelocityImage->SetOrigin( origin );

  // Set the spacing parameters
  SpacingType spacing;
  for( unsigned int i = 0; i < NDimensions; i++ )
    {
      spacing[i] = this->m_FixedParameters[2 * NDimensions + i];
    }
  this->m_VelocityImage->SetSpacing( spacing );

  // Set the direction parameters
  DirectionType direction;
  for( unsigned int di = 0; di < NDimensions; di++ )
    {
      for( unsigned int dj = 0; dj < NDimensions; dj++ )
        {
          direction[di][dj] =
            this->m_FixedParameters[3 * NDimensions + ( di * NDimensions + dj )];
        }
    }
  this->m_VelocityImage->SetDirection( direction );
  this->m_VelocityImage->Allocate();
  VelocityType nullVelocity;
  nullVelocity.Fill( 0 );
  this->m_VelocityImage->FillBuffer( nullVelocity );

  unsigned int numberOfSquareCompositions = (unsigned int)( this->m_FixedParameters[ NDimensions * ( NDimensions + 3 ) + 0 ] );
  this->SetNumberOfSquareCompositions( numberOfSquareCompositions );

  double smoothingSigma = ( this->m_FixedParameters[ NDimensions * ( NDimensions + 3 ) + 1 ] );
  this->SetSmoothingSigma( smoothingSigma );
  
  this->m_GradientVelocityImage->SetRegions( this->m_VelocityImage->GetLargestPossibleRegion() );
  this->m_GradientVelocityImage->SetOrigin( this->m_VelocityImage->GetOrigin() );
  this->m_GradientVelocityImage->SetDirection( this->m_VelocityImage->GetDirection() );
  this->m_GradientVelocityImage->SetSpacing( this->m_VelocityImage->GetSpacing() );
  this->m_GradientVelocityImage->Allocate( );
  this->m_GradientVelocityImage->FillBuffer( nullVelocity );
}

// Set the parameters
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::SetParameters( const ParametersType & parameters )
{
  // check if the number of parameters match the
  // expected number of parameters
  if( parameters.Size() != this->GetNumberOfParameters() )
    {
      itkExceptionMacro( << "Mismatch between parameters size "
                         << parameters.Size() << " and expected number of parameters "
                         << this->GetNumberOfParameters()
                         << ( this->m_VelocityImage->GetLargestPossibleRegion().GetNumberOfPixels() == 0 ?
                              ". \nSince the size of the grid region is 0, perhaps you forgot to "
                              "SetGridRegion or SetFixedParameters before setting the Parameters."
                              : "" ) );
    }

  if( &parameters != &( this->m_InternalParametersBuffer ) )
    {
      // Clean up this->m_InternalParametersBuffer because we will
      // use an externally supplied set of parameters as the buffer
      this->m_InternalParametersBuffer = ParametersType( 0 );
    }

  // Keep a reference to the input parameters
  // directly from the calling environment.
  // this requires that the parameters persist
  // in the calling evironment while being used
  // here.
  this->m_InputParametersPointer = &parameters;

  m_InternalParametersBuffer = ParametersType( parameters );

  // Wrap flat array as images of coefficients
  this->WrapAsImages();

  // Modified is always called since we just have a pointer to the
  // parameters and cannot know if the parameters have changed.
  this->Modified();

  m_VectorInterpolationFunction->SetInputImage( m_VelocityImage ); 
}

// Update the parameters
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>::ParametersType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::UpdateParameters( ParametersType & parametersGradient, double weight )
{

  if( m_SmoothingSigma > 0 )
    {
    this->WrapAsVelocityImage( parametersGradient, m_GradientVelocityImage );  
    typename SmoothingFilterType::SigmaArrayType sigmas;
    for( unsigned int d = 0; d < SpaceDimension; ++d )
     {     
       sigmas[d] = m_SmoothingSigma;
     }
     m_SmoothingFilter->SetSigmaArray( sigmas );
     m_SmoothingFilter->SetInput( m_GradientVelocityImage );
     m_SmoothingFilter->InPlaceOff();
     m_SmoothingFilter->Update();

    m_GradientVelocityImage = m_SmoothingFilter->GetOutput();
    m_GradientVelocityImage->DisconnectPipeline();
    this->UnwrapAsParameters( m_GradientVelocityImage, parametersGradient );
    }

  ParametersType updatedParameters = m_InternalParametersBuffer;
  updatedParameters += weight * parametersGradient;

  return updatedParameters;

}

//// Construct initial displacement image
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>::ParametersType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::InitializeVelocityField( Pointer referenceTransform )
{

  ParametersType parameters = ParametersType( this->GetNumberOfParameters() );

  InputPointType inputPoint;
  VelocityType   velocity;
  for( unsigned int p = 0; p < this->m_VelocityImage->GetLargestPossibleRegion().GetNumberOfPixels(); ++p )
  {
    this->m_VelocityImage->TransformIndexToPhysicalPoint( this->m_VelocityImage->ComputeIndex(p), inputPoint );
    referenceTransform->ComputeVelocity( inputPoint, velocity );
    this->m_VelocityImage->SetPixel( this->m_VelocityImage->ComputeIndex(p), velocity );
    for( unsigned int d=0; d < SpaceDimension; ++d )
    {
      parameters[p*SpaceDimension+d] = velocity[d];
    }
  }

  return parameters;
}


// Wrap flat parameters as images
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::WrapAsImages()
{
  /**
   * Wrap flat parameters array into SpaceDimension number of ITK images
   * NOTE: For efficiency, parameters are not copied locally. The parameters
   * are assumed to be maintained by the caller.
   */
//  ScalarType *dataPointer = const_cast<ScalarType *>(
//        this->m_InputParametersPointer->data_block() );
  const NumberOfParametersType numberOfPixels = this->GetNumberOfParametersPerDimension();


  for( unsigned int p = 0; p < numberOfPixels; ++p )
    {
    VelocityType velocity;  
    for( unsigned int d = 0; d < SpaceDimension; ++d )
      {
      velocity[d] = (ScalarType)(this->m_InputParametersPointer->data_block()[p*SpaceDimension + d]);
      }    
    this->m_VelocityImage->SetPixel( m_VelocityImage->ComputeIndex(p), velocity );
    }

}

// Wrap flat parameters as images
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::WrapAsVelocityImage( const ParametersType & parameters, VelocityImagePointer velocityImage )
{

  const NumberOfParametersType numberOfPixels = this->GetNumberOfParametersPerDimension();

  for( unsigned int p = 0; p < numberOfPixels; ++p )
    {
    VelocityType velocity;  
    for( unsigned int d = 0; d < SpaceDimension; ++d )
      {
      velocity[d] = parameters[p*SpaceDimension + d];
      }    
    velocityImage->SetPixel( velocityImage->ComputeIndex(p), velocity );
    }

}

// Wrap flat parameters as images
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::UnwrapAsParameters( VelocityImagePointer velocityImage,  ParametersType & parameters )
{

  const NumberOfParametersType numberOfPixels = this->GetNumberOfParametersPerDimension();

  for( unsigned int p = 0; p < numberOfPixels; ++p )
    {
    VelocityType velocity = velocityImage->GetPixel( velocityImage->ComputeIndex(p) );
    for( unsigned int d = 0; d < SpaceDimension; ++d )
      {
      parameters[p*SpaceDimension + d] = velocity[d];
      }    
    }

}



// Transform a point
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
bool
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::IsInside(const InputPointType & point) const
{
  IndexType imageIndex;
  this->m_VelocityImage->TransformPhysicalPointToIndex( point, imageIndex );
  return this->m_VelocityImage->GetLargestPossibleRegion().IsInside( imageIndex );
}


// Transform a point
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::TransformPoint(const InputPointType & point) const
{

  OutputPointType         outputPoint;
  InputPointType          inputPoint = point;
  if( m_RigidTransform )
    {
    inputPoint = m_RigidTransform->TransformPoint( inputPoint );    
    }
  outputPoint = ComputeRecursiveTransform( inputPoint );

  return outputPoint;
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeJacobianWithRespectToParameters( const InputPointType & point, 
                                          std::map<unsigned int,ScalarType> & jacobianMap ) const
{

  // nonZeroNodeIndices.clear();
  OutputPointType         outputPoint;
  InputPointType          inputPoint = point;
  if( m_RigidTransform )
    {
    inputPoint = m_RigidTransform->TransformPoint( inputPoint );
    }
  jacobianMap.clear();
  ComputeRecursiveJacobian( inputPoint, jacobianMap );
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeJacobianWithRespectToParameters( const InputPointType & point, 
                                          std::vector<std::pair<unsigned int,ScalarType> > & jacobian ) const
{

  // nonZeroNodeIndices.clear();
  OutputPointType         outputPoint;
  InputPointType          inputPoint = point;
  if( m_RigidTransform )
    {
    inputPoint = m_RigidTransform->TransformPoint( inputPoint );
    }
  jacobian.clear();
  ComputeRecursiveJacobian( inputPoint, jacobian );
}


template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::InverseTransformPoint(const InputPointType & point) const
{

  OutputPointType         outputPoint;

  InputPointType          inputPoint = point;
  if( m_RigidTransform )
    {  
    inputPoint = m_RigidTransform->GetInverseTransform()->TransformPoint( inputPoint );
    }

  outputPoint = ComputeInverseRecursiveTransform( inputPoint );

  return outputPoint;
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeInverseJacobianWithRespectToParameters( const InputPointType & point, 
                                                  std::map<unsigned int,ScalarType> & jacobianMap) const
{

  // nonZeroNodeIndices.clear();
  OutputPointType         outputPoint;
  InputPointType          inputPoint = point;
  if( m_RigidTransform )
    {
    inputPoint = m_RigidTransform->GetInverseTransform()->TransformPoint( inputPoint );
    }
  jacobianMap.clear();
  ComputeInverseRecursiveJacobian( inputPoint, jacobianMap );
}


template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeInverseJacobianWithRespectToParameters( const InputPointType & point, 
                                                  std::vector< std::pair<unsigned int,ScalarType> > & jacobian ) const
{

  // nonZeroNodeIndices.clear();
  OutputPointType         outputPoint;
  InputPointType          inputPoint = point;
  if( m_RigidTransform )
    {
    inputPoint = m_RigidTransform->GetInverseTransform()->TransformPoint( inputPoint );
    }
  jacobian.clear();
  ComputeInverseRecursiveJacobian( inputPoint, jacobian);
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>::OutputCovariantVectorType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::TransformOrientationVector(const InputPointType & point, const InputCovariantVectorType & inputVector) const
{

  InputCovariantVectorType inputVectorNormalized = inputVector;
  inputVectorNormalized.Normalize();
  OutputCovariantVectorType outputVector;
  OutputPointType  outputN, outputP;
  InputPointType   inputN, inputP;

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      inputN[d] = point[d] - inputVectorNormalized[d] / 2.0;
      inputP[d] = point[d] + inputVectorNormalized[d] / 2.0;
    }

  outputN = TransformPoint(inputN);
  outputP = TransformPoint(inputP);

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      outputVector[d] = outputP[d] - outputN[d];
    }

  if(outputVector.GetNorm() > 1e-9)
    outputVector.Normalize();

  return outputVector;

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::TransformPointAndOrientationVector(const InputPointType & point, const InputCovariantVectorType & inputVector, OutputPointType & outputPoint, OutputCovariantVectorType & outputVector) const
{

  InputCovariantVectorType inputVectorNormalized = inputVector;
  inputVectorNormalized.Normalize();
  OutputPointType  outputN, outputP;
  InputPointType   inputN, inputP;

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      inputN[d] = point[d] - inputVectorNormalized[d] / 2.0;
      inputP[d] = point[d] + inputVectorNormalized[d] / 2.0;
    }

  outputN = TransformPoint(inputN);
  outputP = TransformPoint(inputP);

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      outputVector[d] = outputP[d] - outputN[d];
      outputPoint[d] = 0.5 *( outputP[d] + outputN[d] );
    }

  if(outputVector.GetNorm() > 1e-9)
    outputVector.Normalize();

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>::OutputCovariantVectorType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::InverseTransformOrientationVector(const InputPointType & point, const InputCovariantVectorType & inputVector) const
{

  InputCovariantVectorType inputVectorNormalized = inputVector;
  inputVectorNormalized.Normalize();
  OutputCovariantVectorType outputVector;
  OutputPointType  outputN, outputP;
  InputPointType   inputN, inputP;

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      inputN[d] = point[d] - inputVectorNormalized[d] / 2.0;
      inputP[d] = point[d] + inputVectorNormalized[d] / 2.0;
    }

  outputN = InverseTransformPoint(inputN);
  outputP = InverseTransformPoint(inputP);

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      outputVector[d] = outputP[d] - outputN[d];
    }

  if(outputVector.GetNorm() > 1e-9)
    outputVector.Normalize();

  return outputVector;

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::InverseTransformPointAndOrientationVector(const InputPointType & point, const InputCovariantVectorType & inputVector,OutputPointType & outputPoint, OutputCovariantVectorType & outputVector) const
{

  InputCovariantVectorType inputVectorNormalized = inputVector;
  inputVectorNormalized.Normalize();
  OutputPointType  outputN, outputP;
  InputPointType   inputN, inputP;

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      inputN[d] = point[d] - inputVectorNormalized[d] / 2.0;
      inputP[d] = point[d] + inputVectorNormalized[d] / 2.0;
    }

  outputN = InverseTransformPoint(inputN);
  outputP = InverseTransformPoint(inputP);

  for(unsigned int d = 0; d < SpaceDimension; d++)
    {
      outputVector[d] = outputP[d] - outputN[d];
      outputPoint[d] = 0.5*( outputP[d] + outputN[d] );
    }

  if(outputVector.GetNorm() > 1e-9)
    outputVector.Normalize();

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::UpdateNodeParameters( unsigned int nodeId, const ParametersType & parameters )
{

  IndexType         imageIndex = this->m_VelocityImage->ComputeIndex( nodeId );
  VelocityType      velocity;
  for( unsigned int i = 0; i < SpaceDimension; ++i )
    {
      velocity[i] = parameters[i];
    }

  this->m_VelocityImage->SetPixel( imageIndex, velocity );  
}




template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeRecursiveTransform(const InputPointType & point ) const
{

  OutputPointType         outputPoint = point;
  OutputPointType         inputPoint;

  //Direct Integration of ODE
  for( unsigned int i=0; i < m_Pow2N; i++ )
    {
      inputPoint = outputPoint;
      outputPoint = TransformPointDelta( inputPoint );
    }

  return outputPoint;
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeInverseRecursiveTransform(const InputPointType & point) const
{

  OutputPointType         outputPoint = point;
  OutputPointType         inputPoint;

  //Direct Integration of ODE
  for( unsigned int i=0; i < m_Pow2N; i++ )
    {
      inputPoint = outputPoint;
      outputPoint = InverseTransformPointDelta( inputPoint );
    }

  return outputPoint;
}


// Transform a point
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::TransformPointDelta(const OutputPointType & point) const
{

  OutputPointType         outputPoint;
  VelocityType            velocity;

  ComputeVelocity( point, velocity );

  for( unsigned int d=0; d < SpaceDimension; d++)
    {
      outputPoint[d] = point[d] + ( velocity[d] * ( m_IntegrationEndTime / m_Pow2N ) );
    }

  return outputPoint;
}


// Transform a point
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::InverseTransformPointDelta(const OutputPointType & point) const
{

  OutputPointType         outputPoint;
  VelocityType            velocity;
  
  ComputeVelocity( point, velocity );

  for( unsigned int d=0; d < SpaceDimension; d++)
    {
      outputPoint[d] = point[d] - ( velocity[d] * ( m_IntegrationEndTime / m_Pow2N ) );
    }

  return outputPoint;
}


template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeVelocity( const InputPointType & point, VelocityType & velocity ) const
{
  velocity.Fill( NumericTraits<ScalarType>::Zero );

  if( !m_VectorInterpolationFunction->IsInsideBuffer( point ) )
    return;

  velocity = m_VectorInterpolationFunction->Evaluate(point);

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeVelocity( const InputPointType & point, VelocityType & velocity,
                  WeightsContainer & weights, IndicesContainer & indices ) const
{
  velocity.Fill( NumericTraits<ScalarType>::Zero );
  // weights.clear();
  // indices.clear();

  if( !m_VectorInterpolationFunction->IsInsideBuffer( point ) )
    return;

  velocity = m_VectorInterpolationFunction->Evaluate(point, weights, indices);

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeRecursiveJacobian(const InputPointType & point, std::map<unsigned int,ScalarType> & jacobianMap ) const
{

  OutputPointType         outputPoint = point;
  OutputPointType         inputPoint;

  //Direct Integration of ODE
  for( unsigned int i=0; i < m_Pow2N; i++ )
    {
      inputPoint = outputPoint;
      outputPoint = AccumulateJacobian( inputPoint, jacobianMap );
    }
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeRecursiveJacobian(const InputPointType & point, std::vector<std::pair<unsigned int,ScalarType> > & jacobian ) const
{

  std::pair<unsigned int, ScalarType> nullValue = std::pair<unsigned int, ScalarType>( 0, 0 );
  const unsigned int maxNbrOfJacobianElements = m_Pow2N * pow( 2, NDimensions );
  jacobian.resize( maxNbrOfJacobianElements, nullValue );

  OutputPointType         outputPoint = point;
  OutputPointType         inputPoint;

  unsigned int jacId = 0;
  //Direct Integration of ODE
  for( unsigned int i=0; i < m_Pow2N; i++ )
    {
      inputPoint = outputPoint;
      // outputPoint = AccumulateJacobian( inputPoint, jacobianMap );
      VelocityType            velocity;
      typename VectorInterpolationFunctionType::WeightsContainer weights;
      typename VectorInterpolationFunctionType::IndicesContainer indices;      
      ComputeVelocity( inputPoint, velocity, weights, indices);

      for( unsigned int d=0; d < SpaceDimension; d++)
        {
        outputPoint[d] = inputPoint[d] + ( velocity[d] / m_Pow2N );
        }

      for( unsigned int n=0; n < weights.size(); ++n )
        {
        ScalarType weight = weights[n];
        if( weight > 0 )
          {
          IndexType index = indices[n]; 
          unsigned int linearIndex = this->m_VelocityImage->ComputeOffset( index );
          jacobian[jacId] = std::pair<unsigned int,ScalarType>( linearIndex, weight / m_Pow2N );
          ++jacId;  
          }        
        }
    }
}



// Transform a point
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::AccumulateJacobian(const OutputPointType & point, std::map<unsigned int,ScalarType> & jacobianMap ) const
{

  OutputPointType         outputPoint;
  VelocityType            velocity;
  typename VectorInterpolationFunctionType::WeightsContainer weights;
  typename VectorInterpolationFunctionType::IndicesContainer indices;

  ComputeVelocity( point, velocity, weights, indices);

  for( unsigned int d=0; d < SpaceDimension; d++)
    {
    outputPoint[d] = point[d] + ( velocity[d] / m_Pow2N );
    }

  for( unsigned int n=0; n < weights.size(); ++n )
    {
    ScalarType weight = weights[n];
    if( weight > 0 )
      {
      IndexType index = indices[n]; 
      unsigned int linearIndex = this->m_VelocityImage->ComputeOffset( index );

      if( jacobianMap.count( linearIndex ) ) //Index already in map
        {
        ScalarType originalWeight = jacobianMap[linearIndex];
        jacobianMap[linearIndex] = originalWeight + weight / m_Pow2N;
        }
      else
        {
        jacobianMap[linearIndex] = weight / m_Pow2N;
        }      
      }      
    // nonZeroNodeIndices.insert( linearIndex );
    // for( unsigned int d=0; d < SpaceDimension; d++)
    //   {        
    //     jacobian[d][linearIndex*SpaceDimension + d] += weight / m_Pow2N;
    //   }    
    }


  return outputPoint;
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeInverseRecursiveJacobian(const InputPointType & point, std::map<unsigned int,ScalarType> & jacobianMap  ) const
{

  OutputPointType         outputPoint = point;
  OutputPointType         inputPoint;

  //Direct Integration of ODE
  for( unsigned int i=0; i < m_Pow2N; i++ )
    {
      inputPoint = outputPoint;
      outputPoint = AccumulateInverseJacobian( inputPoint, jacobianMap );
    }

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::ComputeInverseRecursiveJacobian(const InputPointType & point, std::vector<std::pair<unsigned int,ScalarType> > & jacobian ) const
{

  std::pair<unsigned int, ScalarType> nullValue = std::pair<unsigned int, ScalarType>( 0, 0 );
  const unsigned int maxNbrOfJacobianElements = m_Pow2N * pow( 2, NDimensions);
  jacobian.resize( maxNbrOfJacobianElements, nullValue );

  OutputPointType         outputPoint = point;
  OutputPointType         inputPoint;
  
  unsigned int jacId = 0;
  //Direct Integration of ODE
  for( unsigned int i=0; i < m_Pow2N; i++ )
    {
      inputPoint = outputPoint;
      // outputPoint = AccumulateInverseJacobian( inputPoint, jacobianMap );
      VelocityType            velocity;
      typename VectorInterpolationFunctionType::WeightsContainer weights;
      typename VectorInterpolationFunctionType::IndicesContainer indices;

      ComputeVelocity( inputPoint, velocity, weights, indices);

      for( unsigned int d=0; d < SpaceDimension; d++)
        {
          outputPoint[d] = inputPoint[d] - ( velocity[d] / m_Pow2N );
        }

      for( unsigned int n=0; n < weights.size(); ++n )
        {
        ScalarType weight = weights[n];
        if ( weight > 0 )
          {
          IndexType index = indices[n]; 
          unsigned int linearIndex = this->m_VelocityImage->ComputeOffset( index );
          jacobian[jacId] = std::pair<unsigned int,ScalarType>( linearIndex, -weight / m_Pow2N );
          ++jacId;  
          }        
        }      
    }

}

// Transform a point
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
typename VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::OutputPointType
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::AccumulateInverseJacobian(const OutputPointType & point, std::map<unsigned int,ScalarType> & jacobianMap  ) const
{

  OutputPointType         outputPoint;
  VelocityType            velocity;
  typename VectorInterpolationFunctionType::WeightsContainer weights;
  typename VectorInterpolationFunctionType::IndicesContainer indices;

  ComputeVelocity( point, velocity, weights, indices);

  for( unsigned int d=0; d < SpaceDimension; d++)
    {
      outputPoint[d] = point[d] - ( velocity[d] / m_Pow2N );
    }

  for( unsigned int n=0; n < weights.size(); ++n )
    {
    ScalarType weight = weights[n];
    if ( weight > 0 )
      {
      IndexType index = indices[n]; 
      unsigned int linearIndex = this->m_VelocityImage->ComputeOffset( index );
      if( jacobianMap.count( linearIndex ) ) //Index already in map
        {
        ScalarType originalWeight = jacobianMap[linearIndex];
        jacobianMap[linearIndex] = originalWeight - weight / m_Pow2N;
        }
      else
        {
        jacobianMap[linearIndex] = - weight / m_Pow2N;
        }      
      }      

    // nonZeroNodeIndices.insert( linearIndex );
    // for( unsigned int d=0; d < SpaceDimension; d++)
    //   {        
    //     jacobian[d][linearIndex*SpaceDimension + d] -= weight / m_Pow2N;
    //   }    
    }


  return outputPoint;
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::EvaluateDisplacementField( ImageBasePointer referenceImage, DisplacementFieldPointer displacementImage )
{

  DispFieldIteratorType dispIterator = DispFieldIteratorType( displacementImage, displacementImage->GetLargestPossibleRegion() );

  InputPointType inputPoint;
  OutputPointType outputPoint;
  OutputVectorType displacement;
  while( !dispIterator.IsAtEnd() )
    {
      displacementImage->TransformIndexToPhysicalPoint( dispIterator.GetIndex(), inputPoint );
      outputPoint = this->TransformPoint( inputPoint );
      for( unsigned int d = 0; d < SpaceDimension; ++d )
      {
        displacement[d] = outputPoint[d] - inputPoint[d];
      }        
      dispIterator.Set( displacement );
      ++dispIterator;
    }    

}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::WriteDisplacementFieldTransform(  ImageBasePointer referenceImage, const std::string filename )
{

  DisplacementFieldPointer displacementImage = DisplacementFieldType::New();
  displacementImage->SetOrigin( referenceImage->GetOrigin()  );
  displacementImage->SetSpacing( referenceImage->GetSpacing()  );
  displacementImage->SetRegions( referenceImage->GetLargestPossibleRegion()  );
  displacementImage->SetDirection( referenceImage->GetDirection()  );
  displacementImage->Allocate();
  this->EvaluateDisplacementField( referenceImage, displacementImage );


  DisplacementFieldTransformPointer dispFieldTransform = DisplacementFieldTransformType::New();    
  dispFieldTransform->SetDisplacementField( displacementImage );

  TransformWriterType::Pointer transformWriter = TransformWriterType::New();
  transformWriter->SetInput( dispFieldTransform );
  transformWriter->SetFileName( filename );

  try
    {
    transformWriter->Update(); 
    }
  catch ( itk :: ExceptionObject &err )
    {
      std :: cout << " ExceptionObject caught !" << std :: endl ;
      std :: cout << err << std :: endl ;
    }    
     
}

template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::WriteDisplacementFieldImage(  ImageBasePointer referenceImage, const std::string filename )
{

  DisplacementFieldPointer displacementImage = DisplacementFieldType::New();
  displacementImage->SetOrigin( referenceImage->GetOrigin()  );
  displacementImage->SetSpacing( referenceImage->GetSpacing()  );
  displacementImage->SetRegions( referenceImage->GetLargestPossibleRegion()  );
  displacementImage->SetDirection( referenceImage->GetDirection()  );
  displacementImage->Allocate();
  this->EvaluateDisplacementField( referenceImage, displacementImage );


  typename DisplacementFieldImageWriterType::Pointer dispFieldWriter = DisplacementFieldImageWriterType::New();    
  dispFieldWriter->SetInput( displacementImage );
  dispFieldWriter->SetFileName( filename );

  try
    {
    dispFieldWriter->Update(); 
    }
  catch ( itk :: ExceptionObject &err )
    {
      std :: cout << " ExceptionObject caught !" << std :: endl ;
      std :: cout << err << std :: endl ;
    }    
     
}  

// Print self
template <class TScalarType, unsigned int NDimensions, unsigned int VSplineOrder>
void
VelocityFieldTransformCustom<TScalarType, NDimensions, VSplineOrder>
::PrintSelf( std::ostream & os, Indent indent ) const
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "GridSize: "
     << this->m_VelocityImage->GetLargestPossibleRegion().GetSize();
  os << std::endl;
  os << indent << "GridOrigin: "
     << this->m_VelocityImage->GetOrigin();
  os << std::endl;
  os << indent << "GridSpacing: "
     << this->m_VelocityImage->GetSpacing();
  os << std::endl;
  os << indent << "GridDirection: "
     << this->m_VelocityImage->GetDirection();
  os << std::endl;
}

} // namespace


#endif
