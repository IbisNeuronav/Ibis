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

#ifndef __itkVectorLinearInterpolateImageFunction2_hxx
#define __itkVectorLinearInterpolateImageFunction2_hxx

#include "itkVectorLinearInterpolateImageFunction2.h"

#include "vnl/vnl_math.h"

namespace itk
{
/**
 * Define the number of neighbors
 */
template< typename TInputImage, typename TCoordRep >
const unsigned long
VectorLinearInterpolateImageFunction2< TInputImage, TCoordRep >
::m_Neighbors = 1 << TInputImage::ImageDimension;

/**
 * Constructor
 */
template< typename TInputImage, typename TCoordRep >
VectorLinearInterpolateImageFunction2< TInputImage, TCoordRep >
::VectorLinearInterpolateImageFunction2()
{}

/**
 * PrintSelf
 */
template< typename TInputImage, typename TCoordRep >
void
VectorLinearInterpolateImageFunction2< TInputImage, TCoordRep >
::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);
}

/**
 * Evaluate at image index position
 */
template< typename TInputImage, typename TCoordRep >
typename VectorLinearInterpolateImageFunction2< TInputImage, TCoordRep >
::OutputType
VectorLinearInterpolateImageFunction2< TInputImage, TCoordRep >
::EvaluateAtContinuousIndex(
  const ContinuousIndexType & index) const
{
  /**
   * Compute base index = closet index below point
   * Compute distance from point to base index
   */
  IndexType baseIndex;
  InternalComputationType    distance[ImageDimension];
  const TInputImage * const inputImgPtr=this->GetInputImage();
  for (unsigned int dim = 0; dim < ImageDimension; ++dim )
    {
    baseIndex[dim] = Math::Floor< IndexValueType >(index[dim]);
    distance[dim] = index[dim] - static_cast< InternalComputationType >( baseIndex[dim] );
    }

  /**
   * Interpolated value is the weighted sum of each of the surrounding
   * neighbors. The weight for each neighbor is the fraction overlap
   * of the neighbor pixel with respect to a pixel centered on point.
   */
  OutputType output;
  output.Fill(0.0);

  typedef typename NumericTraits< PixelType >::ScalarRealType ScalarRealType;
  ScalarRealType totalOverlap = NumericTraits< ScalarRealType >::Zero;

  for ( unsigned int counter = 0; counter < m_Neighbors; ++counter )
    {
    InternalComputationType overlap = 1.0;    // fraction overlap
    unsigned int upper = counter;  // each bit indicates upper/lower neighbour

    IndexType    neighIndex;
    // get neighbor index and overlap fraction
    for ( unsigned int dim = 0; dim < ImageDimension; ++dim )
      {
      if ( upper & 1 )
        {
        neighIndex[dim] = baseIndex[dim] + 1;
        // Take care of the case where the pixel is just
        // in the outer upper boundary of the image grid.
        if ( neighIndex[dim] > this->m_EndIndex[dim] )
          {
          neighIndex[dim] = this->m_EndIndex[dim];
          }
        overlap *= distance[dim];
        }
      else
        {
        neighIndex[dim] = baseIndex[dim];
        // Take care of the case where the pixel is just
        // in the outer lower boundary of the image grid.
        if ( neighIndex[dim] < this->m_StartIndex[dim] )
          {
          neighIndex[dim] = this->m_StartIndex[dim];
          }
        overlap *= 1.0 - distance[dim];
        }
      upper >>= 1;
      }

    // get neighbor value only if overlap is not zero
    if ( overlap )
      {
      const PixelType & input = inputImgPtr->GetPixel(neighIndex);
      for ( unsigned int k = 0; k < Dimension; ++k )
        {
        output[k] += overlap * static_cast< InternalComputationType >( input[k] );
        }
      totalOverlap += overlap;
      }

    if ( totalOverlap == 1.0 )
      {
      // finished
      break;
      }
    }

  return ( output );
}

/**
 * Evaluate at image index position
 */
template< typename TInputImage, typename TCoordRep >
typename VectorLinearInterpolateImageFunction2< TInputImage, TCoordRep >
::OutputType
VectorLinearInterpolateImageFunction2< TInputImage, TCoordRep >
::Evaluate(
    const PointType & point,
    std::vector<RealType> & interpolationWeights, 
    std::vector<IndexType> & interpolationIndices ) const
{

  ContinuousIndexType index;
  this->GetInputImage()->TransformPhysicalPointToContinuousIndex( point, index );

  /**
   * Compute base index = closet index below point
   * Compute distance from point to base index
   */
  IndexType baseIndex;
  InternalComputationType    distance[ImageDimension];
  const TInputImage * const inputImgPtr=this->GetInputImage();
  for (unsigned int dim = 0; dim < ImageDimension; ++dim )
    {
    baseIndex[dim] = Math::Floor< IndexValueType >(index[dim]);
    distance[dim] = index[dim] - static_cast< InternalComputationType >( baseIndex[dim] );
    }

  /**
   * Interpolated value is the weighted sum of each of the surrounding
   * neighbors. The weight for each neighbor is the fraction overlap
   * of the neighbor pixel with respect to a pixel centered on point.
   */
  OutputType output;
  output.Fill(0.0);


  if( !this->GetInputImage()->GetBufferedRegion().IsInside(baseIndex) )
  {
    return output;
  }

  // interpolationWeights.clear();
  // interpolationIndices.clear();
  interpolationWeights.resize( m_Neighbors, 0 );
  interpolationIndices.resize( m_Neighbors );  

  typedef typename NumericTraits< PixelType >::ScalarRealType ScalarRealType;
  ScalarRealType totalOverlap = NumericTraits< ScalarRealType >::Zero;

  for ( unsigned int counter = 0; counter < m_Neighbors; ++counter )
    {
    InternalComputationType overlap = 1.0;    // fraction overlap
    unsigned int upper = counter;  // each bit indicates upper/lower neighbour

    IndexType    neighIndex;
    // get neighbor index and overlap fraction
    for ( unsigned int dim = 0; dim < ImageDimension; ++dim )
      {
      if ( upper & 1 )
        {
        neighIndex[dim] = baseIndex[dim] + 1;
        // Take care of the case where the pixel is just
        // in the outer upper boundary of the image grid.
        if ( neighIndex[dim] > this->m_EndIndex[dim] )
          {
          neighIndex[dim] = this->m_EndIndex[dim];
          }
        overlap *= distance[dim];
        }
      else
        {
        neighIndex[dim] = baseIndex[dim];
        // Take care of the case where the pixel is just
        // in the outer lower boundary of the image grid.
        if ( neighIndex[dim] < this->m_StartIndex[dim] )
          {
          neighIndex[dim] = this->m_StartIndex[dim];
          }
        overlap *= 1.0 - distance[dim];
        }
      upper >>= 1;
      }

    // get neighbor value only if overlap is not zero
    if ( overlap )
      {
      const PixelType & input = inputImgPtr->GetPixel(neighIndex);
      // interpolationIndices.push_back( neighIndex );
      // interpolationWeights.push_back( overlap );
      interpolationIndices[counter] = neighIndex ;
      interpolationWeights[counter] = overlap ;

      for ( unsigned int k = 0; k < Dimension; ++k )
        {
        output[k] += overlap * static_cast< InternalComputationType >( input[k] );
        }
      totalOverlap += overlap;
      }

    if ( totalOverlap == 1.0 )
      {
      // finished
      break;
      }
    }

  return ( output );
}


} // end namespace itk

#endif
