/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

#ifndef __ImageRandomSamplerSparseMaskRandomOffset_h
#define __ImageRandomSamplerSparseMaskRandomOffset_h

#include "itkImageRandomSamplerBase.h"
#include "itkMersenneTwisterRandomVariateGenerator.h"
#include "itkImageFullSampler.h"

namespace itk
{

  /** \class ImageRandomSamplerSparseMaskRandomOffset
   *
   * \brief Samples randomly some voxels of an image.
   *
   * This version takes into account that the mask may be very small.
   * Also, it may be more efficient when very many different sample sets
   * of the same input image are required, because it does some precomputation.
	 * \ingroup ImageSamplers
   */

  template < class TInputImage >
  class ImageRandomSamplerSparseMaskRandomOffset :
    public ImageRandomSamplerBase< TInputImage >
  {
  public:

    /** Standard ITK-stuff. */
    typedef ImageRandomSamplerSparseMaskRandomOffset                Self;
    typedef ImageRandomSamplerBase< TInputImage >       Superclass;
    typedef SmartPointer<Self>                Pointer;
    typedef SmartPointer<const Self>          ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );

    /** Run-time type information (and related methods). */
    itkTypeMacro( ImageRandomSamplerSparseMaskRandomOffset, ImageRandomSamplerBase );

    /** Typedefs inherited from the superclass. */
    typedef typename Superclass::DataObjectPointer            DataObjectPointer;
    typedef typename Superclass::OutputVectorContainerType    OutputVectorContainerType;
    typedef typename Superclass::OutputVectorContainerPointer OutputVectorContainerPointer;
    typedef typename Superclass::InputImageType               InputImageType;
    typedef typename Superclass::InputImagePointer            InputImagePointer;
    typedef typename Superclass::InputImageConstPointer       InputImageConstPointer;
    typedef typename Superclass::InputImageRegionType         InputImageRegionType;
    typedef typename Superclass::InputImagePixelType          InputImagePixelType;
    typedef typename Superclass::ImageSampleType              ImageSampleType;
    typedef typename Superclass::ImageSampleContainerType     ImageSampleContainerType;
    typedef typename Superclass::MaskType                     MaskType;

    /** The input image dimension. */
    itkStaticConstMacro( InputImageDimension, unsigned int,
      Superclass::InputImageDimension );

    /** Other typdefs. */
    typedef typename InputImageType::IndexType    InputImageIndexType;
    typedef typename InputImageType::PointType    InputImagePointType;

    /** The random number generator used to generate random indices. */
    typedef itk::Statistics::MersenneTwisterRandomVariateGenerator RandomGeneratorType;

  protected:

    typedef itk::ImageFullSampler<InputImageType>           InternalFullSamplerType;

    /** The constructor. */
    ImageRandomSamplerSparseMaskRandomOffset();
    /** The destructor. */
    virtual ~ImageRandomSamplerSparseMaskRandomOffset() {};

    /** PrintSelf. */
    void PrintSelf( std::ostream& os, Indent indent ) const;

    /** Function that does the work. */
    virtual void GenerateData( void );

    typename RandomGeneratorType::Pointer     m_RandomGenerator;
    typename InternalFullSamplerType::Pointer m_InternalFullSampler;

  private:

    /** The private constructor. */
    ImageRandomSamplerSparseMaskRandomOffset( const Self& );          // purposely not implemented
    /** The private copy constructor. */
    void operator=( const Self& );            // purposely not implemented


  }; // end class ImageRandomSamplerSparseMaskRandomOffset


} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkImageRandomSamplerSparseMaskRandomOffset.txx"
#endif

#endif // end #ifndef __ImageRandomSamplerSparseMaskRandomOffset_h

