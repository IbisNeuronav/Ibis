#ifndef __itkBoneExtractionFilter_h
#define __itkBoneExtractionFilter_h
 
#include "itkImageToImageFilter.h"
#include "itkObjectFactory.h"
#include "itkImageRegionIterator.h"
//#include "itkImageRegionConstIterator.h"
#include "itkImageFileReader.h"
#include "itkChangeInformationImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkBresenhamLine.h"



namespace itk
{
template< class TImage >
class BoneExtractionFilter : public ImageToImageFilter< TImage, TImage >
{
public:
    /** Standard class typedefs. */
    typedef BoneExtractionFilter Self;
    typedef ImageToImageFilter< TImage, TImage > Superclass;
    typedef SmartPointer< Self > Pointer;

    typedef TImage ImageType;
    typedef typename ImageType::PixelType PixelType;

    /** Define image type for mask:
        We need a 2D? Image and a 3D image for processing */
    typedef itk::Image <PixelType, 2> Internal2DImageType;
    typedef itk::Image <unsigned char, 3> Internal3DImageMaskType;
    typedef itk::Image <unsigned char, 2> Internal2DImageMaskType;
    typedef itk::Index<3> IndexType3D;
    typedef itk::Index<2> IndexType2D;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );
    itkGetMacro( Threshold, PixelType );
    itkSetMacro( Threshold, PixelType );
//    itkGetMacro( PointsImage, itk::SmartPointer<Internal2DImageMaskType>);
//    itkGetMacro( MaskFileName, std::string );

    /** Run-time type information (and related methods). */
    itkTypeMacro(BoneExtractionFilter, ImageToImageFilter);
//    itkTypeMacro(InternalMaskType, Internal3DImageMaskType);

protected:
    BoneExtractionFilter();
    ~BoneExtractionFilter(){}
    void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

    /** Does the real work. */
    virtual void GenerateData();


    /// deprecated
//    void ReadMaskFile();

    void CreateItkMask();
    void CreateBottomPointsOfMask();
    void RayCast(ImageType *);
    void GetMaxLinePixels(ImageType *, IndexType2D, IndexType2D, IndexType2D &);
    void ThresholdByDistanceMap(ImageType *, PixelType );

private:

  BoneExtractionFilter(const Self &); //purposely not implemented
  void operator=(const Self &);  //purposely not implemented
 
protected:

//  std::string m_MaskFileName;
  PixelType m_Threshold;
  itk::SmartPointer<Internal3DImageMaskType> m_MaskImage;
  itk::SmartPointer<Internal2DImageMaskType> m_PointsImage; /**/
  itk::SmartPointer<ImageType> m_ProcessedImage;
  std::vector< IndexType2D > m_BottomIndexList;

};
} //namespace ITK
 
 
#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBoneExtractionFilter.hxx"
#endif
 
 
#endif // __itkImageFilter_h
