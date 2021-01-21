#ifndef __itkCompoundImageFilter_hxx
#define __itkCompoundImageFilter_hxx
 
#include "itkCompoundImageFilter.h"
 
namespace itk
{

template <typename TInputImage, typename TOutputImage>
CompoundImageFilter< TInputImage >
::CompoundImageFilter()
{

    m_InputImages.clear();
    m_Method = AVERAGE;

}
 
template <typename TInputImage, typename TOutputImage>
void CompoundImageFilter< TInputImage, TOutputImage >
::GenerateData()
{

    typename ImageType::Pointer input = ImageType::New();
    input->Graft( const_cast< ImageType * >( this->GetInput() ));
    OutputImagePointer     outputPtr = this->GetOutput();

    if( m_InputImages.empty() )
    {
        typedef itk::ImageDuplicator<ImageType> DuplicatorType;
        DuplicatorType::Pointer duplicator = DuplicatorType::New();
        duplicator->SetInputImage(input);
        //duplicator->Update();

    }
    //this->CopyInputImageInformations(input, m_PointVolume);

    this->GraftOutput( m_ProcessedImage );


}

//template< typename TImage >
//void
//CompoundImageFilter< TImage >
//::CopyInputImageInformations( ImageType * input , itk::SmartPointer<Internal3DMaskType> & destination)
//{
//    typedef itk::ChangeInformationImageFilter< Internal3DMaskType > InfoFilterType;
//    typename InfoFilterType::Pointer infoFilter = InfoFilterType::New();
//    infoFilter->SetInput( destination );
//    infoFilter->SetOutputDirection(input->GetDirection());
//    infoFilter->ChangeDirectionOn();
//    infoFilter->SetOutputSpacing(input->GetSpacing());
//    infoFilter->ChangeSpacingOn();
//    infoFilter->SetOutputOrigin(input->GetOrigin());
//    infoFilter->ChangeOriginOn();
//    infoFilter->Update();
//
//    destination = infoFilter->GetOutput();
//}
 
template <typename TInputImage, typename TOutputImage>
void
CompoundImageFilter< TInputImage, TOutputImage >
::PrintSelf( std::ostream & os, Indent indent ) const
{
    Superclass::PrintSelf(os, indent);

    os << indent << "Threshold:" << this->m_Radius
    << std::endl;
}

template <typename TInputImage, typename TOutputImage>
void
CompoundImageFilter<TInputImage, TOutputImage>::AddInputImage(InputImagePointer input)
{
    if( input )
    {
        this->Modified();
        m_InputImages.push_back(input);
    }
}

template <typename TInputImage, typename TOutputImage>
void
CompoundImageFilter<TInputImage, TOutputImage>::GenerateOutputInformation()
{
    // Call the superclass' implementation of this method
    Superclass::GenerateOutputInformation();

    // Get pointers to the input and output
    const InputImageType * inputPtr = this->GetInput();
    OutputImageType * outputPtr = this->GetOutput();

    itkAssertInDebugAndIgnoreInReleaseMacro(inputPtr);
    itkAssertInDebugAndIgnoreInReleaseMacro(outputPtr != nullptr);

    // Compute the output spacing, the output image size, and the
    // output image start index
    //unsigned int                              i;
    const typename TInputImage::SpacingType & inputSpacing = inputPtr->GetSpacing();
    const typename TInputImage::SizeType & inputSize = inputPtr->GetLargestPossibleRegion().GetSize();
    const typename TInputImage::IndexType & inputStartIndex = inputPtr->GetLargestPossibleRegion().GetIndex();

    typename TOutputImage::SpacingType outputSpacing;
    typename TOutputImage::SizeType    outputSize;
    typename TOutputImage::IndexType   outputStartIndex;
    typename TOutputImage::PointType   outputOrigin;

    double outputExtent[6];
    this->GetImagePhysicalExtent(inputPtr, outputExtent);
    for( InputImagePointer additionalImage : m_InputImages )
    {
        double extent[6];
        this->GetImagePhysicalExtent(additionalImage, extent);
        for( unsigned int ii = 0; ii < TInputImage::ImageDimension; ii++ )
        {
            outputExtent[ii * 2] = (extent[ii] < outputExtent[ii * 2]) ? extent[ii] : outputExtent[ii * 2];
            outputExtent[ii * 2 + 1] = (extent[ii] > outputExtent[ii * 2 + 1]) ? extent[ii] : outputExtent[ii * 2 + 1];
        }
    }

    for( unsigned int i = 0; i < TOutputImage::ImageDimension; i++ )
    {
        outputSpacing[i] = inputSpacing[i];
        outputSize[i] = static_cast<IndexValueType>(std::round((outputExtent[i * 2 + 1] - outputExtent[i * 2]) / outputSpacing[i]));
        outputOrigin[i] = static_cast<ValueType>(outputExtent[i * 2]);
        outputStartIndex[i] = static_cast<IndexValueType>(inputStartIndex[i]);
    }


    outputPtr->SetSpacing(outputSpacing);
    outputPtr->SetOrigin(outputOrigin);

    typename TOutputImage::RegionType outputLargestPossibleRegion;
    outputLargestPossibleRegion.SetSize(outputSize);
    outputLargestPossibleRegion.SetIndex(outputStartIndex);

    outputPtr->SetLargestPossibleRegion(outputLargestPossibleRegion);

}

template <typename TInputImage, typename TOutputImage>
void
CompoundImageFilter<TInputImage, TOutputImage>::GetImagePhysicalExtent(InputImagePointer image, double (&extent)[6])
{
    typename TInputImage::IndexType upperCornerIndex;
    typename TInputImage::IndexType lowerCornerIndex;
    typename TInputImage::IndexType index;
    typename TInputImage::PointType point;

    lowerCornerIndex = image->GetLargestPossibleRegion().GetIndex();
    upperCornerIndex = image->GetLargestPossibleRegion().GetUpperIndex();
    
    std::vector<typename TInputImage::IndexType> corners;
    corners.push_back(lowerCornerIndex);
    corners.push_back(upperCornerIndex);

    // set extent to maximum values (TODO: set reasonable values)
    for( int ii = 0; ii < TOutputImage::ImageDimension; ++ii )
    {
        extent[ii * 2] = itk::NumericTraits<TInputImage::PointType>::max());
        extent[ii * 2 + 1] = itk::NumericTraits<TInputImage::PointType>::NonpositiveMin();
    }

    for( unsigned int i = 0; i < std::pow(2, TOutputImage::ImageDimension); ++i )
    {
        std::bitset<TOutputImage::ImageDimension> elem(i);
        for( unsigned int j = 0; j < TOutputImage::ImageDimension; ++j )
        {
            index[j] = corners[elem[j]][j];
            image->TransformIndexToPhysicalPoint(index, point);
            for( int ii = 0; ii < TOutputImage::ImageDimension; ++ii )
            {
                extent[ii * 2] = (point[ii] < extent[ii * 2]) ? point[ii] : extent[ii * 2];
                extent[ii * 2 + 1] = (point[ii] > extent[ii * 2 + 1]) ? point[ii] : extent[ii * 2 + 1];
            }
        }
            
    }

}

}// end namespace
 
 
#endif
