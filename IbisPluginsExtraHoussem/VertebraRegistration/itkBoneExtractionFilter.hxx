#ifndef __itkBoneExtractionFilter_hxx
#define __itkBoneExtractionFilter_hxx
 
#include "itkBoneExtractionFilter.h"
 
namespace itk
{

template< typename TImage >
BoneExtractionFilter< TImage >
::BoneExtractionFilter()
{
    /// deprecated: used for ReadMaskFile
//    m_MaskFileName = "../IbisPlugins/VertebraRegistration/extra_data/Mask-withoutOrientation.mnc";

    m_Threshold = 15;
    m_MaskImage = Internal3DImageMaskType::New();
    m_PointsImage = Internal2DImageMaskType::New(); /**/
    m_ProcessedImage = ImageType::New();

//    this->ReadMaskFile();
    this->CreateItkMask();
    this->CreateBottomPointsOfMask();

}
 
template< class TImage >
void BoneExtractionFilter< TImage >
::GenerateData()
{

    typename ImageType::Pointer input = ImageType::New();
    input->Graft( const_cast< ImageType * >( this->GetInput() ));

    this->RayCast(input);

    this->ThresholdByDistanceMap(input, m_Threshold);

    this->CopyInputImageInformations(input);

    this->GraftOutput( m_ProcessedImage );


}


//template< class TImage >
//void BoneExtractionFilter< TImage >
//::ReadMaskFile()
//{

//    typedef itk::ImageFileReader<Internal3DImageMaskType> FilerReaderType;
//    FilerReaderType::Pointer reader = FilerReaderType::New();
//    reader->SetFileName(m_MaskFileName.c_str());

//    m_MaskImage = reader->GetOutput();
//    m_MaskImage->Update();
//}

template< class TImage >
void BoneExtractionFilter< TImage >
::CreateItkMask()
{
    Internal3DImageMaskType::RegionType region;
    Internal3DImageMaskType::SizeType size;

    Internal3DImageMaskType::IndexType pixelIndex;

    unsigned int width = 640;
    unsigned int height = 480;
    double crop0 = 50.0;
    double crop1 = 540.0;
    double origin0 = 318.0;
    double origin1 = 448.0;
    double bottom = 400.0;
    double top = 24.0;
    double left_angle = -M_PI_4;
    double right_angle = -M_PI_4;


    size[0] = width; size[1] = height; size[2] = 1;
    pixelIndex.Fill(0);

    region.SetSize(size);
    region.SetIndex(pixelIndex);

    m_MaskImage->SetRegions(region);
    m_MaskImage->Allocate();

    typedef itk::ImageRegionIterator<Internal3DImageMaskType> IteratorType;
    IteratorType imageIterator( m_MaskImage, region );


    for(imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator)
    {
         pixelIndex = imageIterator.GetIndex();

         // Test 1 : x, y bounds
         if( pixelIndex[0] < crop0 || pixelIndex[0] > crop1 || pixelIndex[1] >= origin1 )
         {
             imageIterator.Set(0);
         }
         else
         {
             // Test 2 : Distance from origin
             double diff[2];
             diff[0] = (double)pixelIndex[0] - origin0;
             diff[1] = (double)pixelIndex[1] - origin1;
             double dist = sqrt( diff[0] * diff[0] + diff[1] * diff[1] );
             if( dist > bottom || dist < top )
                 imageIterator.Set(0);
             else
             {
                 // Test 3 : between angles
                 double angle =  atan( std::abs( diff[0] ) / diff[1] );
                 if( ( diff[0] < 0.0 && angle < left_angle ) || ( diff[0] > 0.0 && angle < right_angle ) )
                     imageIterator.Set(0);
                 else
                     imageIterator.Set(1);
             }
         }

    }

}

template< class TImage >
void BoneExtractionFilter< TImage >
::CreateBottomPointsOfMask()
{

    IndexType3D index3d;
    IndexType2D index2d;
    typename ImageType::SizeType imageSize = m_MaskImage->GetBufferedRegion().GetSize();
    typename ImageType::PixelType imagePixel;

    Internal2DImageMaskType::RegionType maskRegion;
    Internal2DImageMaskType::SizeType maskSize;
    maskSize[0] = imageSize[0];
    maskSize[1] = imageSize[1];

    maskRegion.SetSize(maskSize);
    index2d[0] = 0;
    index2d[1] = 0;
    maskRegion.SetIndex(index2d);

    IndexType2D constIndex2d;
    constIndex2d[0] = 320; constIndex2d[1] = 430;


    bool stopCondition = false;

    for (int x = 0; x < imageSize[0]; ++x) {
        stopCondition = false;
        for (int y = 0; (y < imageSize[1]) & (!stopCondition); ++y) {
            index3d[0] = x; index3d[1] = y; index3d[2] = 0;
            index2d[0] = x; index2d[1] = y;
            imagePixel = m_MaskImage->GetPixel(index3d);
            if (imagePixel > 0)
            {
                stopCondition = true;
                m_BottomIndexList.push_back(index2d);
            }
        }
    }
}


template< class TImage >
void BoneExtractionFilter< TImage >
::RayCast(ImageType * inputImage)
{
    IndexType3D index3d;
    IndexType2D index2d;
    typename ImageType::SizeType imageSize = inputImage->GetBufferedRegion().GetSize();
    typename ImageType::PixelType imagePixel;

    Internal2DImageMaskType::RegionType maskRegion;
    Internal2DImageMaskType::SizeType maskSize;
    maskSize[0] = imageSize[0];
    maskSize[1] = imageSize[1];

    maskRegion.SetSize(maskSize);
    index2d[0] = 0;
    index2d[1] = 0;
    maskRegion.SetIndex(index2d);
    m_PointsImage->SetRegions(maskRegion);
    m_PointsImage->Allocate();
    m_PointsImage->FillBuffer(0);

    IndexType2D constIndex2d;
    IndexType2D maxIndex;
    constIndex2d[0] = 320; constIndex2d[1] = 430;

    for (int i = 0; i < m_BottomIndexList.size(); ++i) {
        index2d = m_BottomIndexList[i];
        this->GetMaxLinePixels(inputImage, index2d, constIndex2d, maxIndex);
        m_PointsImage->SetPixel(maxIndex, 1);
    }

}


template< class TImage >
void
BoneExtractionFilter< TImage >
::GetMaxLinePixels(ImageType * inputImage,
                   BoneExtractionFilter::IndexType2D startIndex,
                   BoneExtractionFilter::IndexType2D endIndex,
                   BoneExtractionFilter::IndexType2D & resultIndex)
{

    IndexType3D currIndex;
    resultIndex[0] = startIndex[0];
    resultIndex[1] = startIndex[1];

    unsigned int maxVectorIndex = 0;

    typename ImageType::PixelType currPixel;
    typename ImageType::PixelType maxPixel = 0.0;

    bool stopCondition = false;

    itk::BresenhamLine<2> line;

    std::vector< itk::Index<2> > pixels = line.BuildLine(startIndex, endIndex);

    for(unsigned int i = 0; i < pixels.size() & !stopCondition; i++)
    {
        currIndex[0] = pixels[i][0];
        currIndex[1] = pixels[i][1];
        currIndex[2] = 0;
        currPixel = inputImage->GetPixel(currIndex);
        if (currPixel > maxPixel)
        {
            maxPixel = currPixel;
            maxVectorIndex = i;

            resultIndex[0] = currIndex[0];
            resultIndex[1] = currIndex[1];
        }

        if ((currIndex[0] >= endIndex[0]) & (currIndex[1] >= endIndex[1]))
            stopCondition = true;

    }

}

template< class TImage >
void BoneExtractionFilter< TImage >
::ThresholdByDistanceMap(ImageType * itkInputImage,
                         PixelType threshold )
{
    typedef itk::SignedMaurerDistanceMapImageFilter< Internal2DImageMaskType, Internal2DImageType > SignedMaurerDistanceMapImageFilterType;
    typename SignedMaurerDistanceMapImageFilterType::Pointer distanceMapImageFilter = SignedMaurerDistanceMapImageFilterType::New();
    distanceMapImageFilter->SetInput(m_PointsImage);
    distanceMapImageFilter->SetBackgroundValue(0);

    typedef itk::BinaryThresholdImageFilter <Internal2DImageType, Internal2DImageMaskType> BinaryThresholdImageFilterType;
    typename BinaryThresholdImageFilterType::Pointer thresholdFilter = BinaryThresholdImageFilterType::New();
    thresholdFilter->SetInput(distanceMapImageFilter->GetOutput());
    thresholdFilter->SetLowerThreshold(-threshold);
    thresholdFilter->SetUpperThreshold(threshold);
    thresholdFilter->SetInsideValue(1);
    thresholdFilter->SetOutsideValue(0);

    typedef itk::CastImageFilter< Internal2DImageMaskType, Internal3DImageMaskType > CastFilter2Dto3DType;
    typename CastFilter2Dto3DType::Pointer castFilter2to3 = CastFilter2Dto3DType::New();
    castFilter2to3->SetInput(thresholdFilter->GetOutput());

    typedef itk::ChangeInformationImageFilter< ImageType > InformationFilterType;
    typename InformationFilterType::Pointer informationFilter = InformationFilterType::New();

    informationFilter->SetInput( itkInputImage );
    informationFilter->SetOutputDirection(castFilter2to3->GetOutput()->GetDirection());
    informationFilter->ChangeDirectionOn();
    informationFilter->SetOutputSpacing(castFilter2to3->GetOutput()->GetSpacing());
    informationFilter->ChangeSpacingOn();
    informationFilter->SetOutputOrigin(castFilter2to3->GetOutput()->GetOrigin());
    informationFilter->ChangeOriginOn();

    typedef itk::MaskImageFilter< ImageType, Internal3DImageMaskType > MaskFilterType;
    typename MaskFilterType::Pointer maskFilter = MaskFilterType::New();
    maskFilter->SetInput(informationFilter->GetOutput());
    maskFilter->SetMaskImage(castFilter2to3->GetOutput());
    maskFilter->Update();

    m_ProcessedImage = maskFilter->GetOutput();
    m_ProcessedImage->DisconnectPipeline();

}

template< typename TImage >
void
BoneExtractionFilter< TImage >
::CopyInputImageInformations( ImageType * input )
{
    typedef itk::ChangeInformationImageFilter< TImage > InfoFilterType;
    typename InfoFilterType::Pointer infoFilter = InfoFilterType::New();
    infoFilter->SetInput( m_ProcessedImage );
    infoFilter->SetOutputDirection(input->GetDirection());
    infoFilter->ChangeDirectionOn();
    infoFilter->SetOutputSpacing(input->GetSpacing());
    infoFilter->ChangeSpacingOn();
    infoFilter->SetOutputOrigin(input->GetOrigin());
    infoFilter->ChangeOriginOn();
    infoFilter->Update();

    m_ProcessedImage = infoFilter->GetOutput();
}
 
template< typename TImage >
void
BoneExtractionFilter< TImage >
::PrintSelf( std::ostream & os, Indent indent ) const
{
    Superclass::PrintSelf(os, indent);

    os << indent << "Threshold:" << this->m_Threshold
    << std::endl;
}

}// end namespace
 
 
#endif
