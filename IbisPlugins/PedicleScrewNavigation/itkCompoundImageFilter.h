#ifndef __itkCompoundImageFilter_h
#define __itkCompoundImageFilter_h
 
#include <vector>
#include <cmath>
#include "itkImageToImageFilter.h"
#include "itkObjectFactory.h"
#include "itkImageRegionIterator.h"
#include "itkImageFileReader.h"
#include "itkChangeInformationImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkBresenhamLine.h"
#include "itkChangeInformationImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkPasteImageFilter.h"
#include "itkLabelStatisticsImageFilter.h"
#include "itkImageDuplicator.h"
#include <iterator>
#include "itkImageMaskSpatialObject.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkResampleImageFilter.h"
#include "itkBinaryThinningImageFilter.h"

#include "itkThresholdImageFilter.h"


namespace itk
{
template <typename TInputImage, typename TOutputImage>
class CompoundImageFilter : public ImageToImageFilter< TInputImage, TOutputImage >
{
public:
    /** Standard class typedefs. */
    using Self = CompoundImageFilter;
    using Superclass = ImageToImageFilter< TInputImage, TInputImage >;
    using Pointer = SmartPointer< Self >;
    
    using OutputImageType = TOutputImage;
    using InputImageType = TInputImage;
    using OutputImagePointer = typename OutputImageType::Pointer;
    using InputImagePointer = typename InputImageType::Pointer;
    
    using OutputPointType = typename OutputImageType::PointType;
    using InputPointType = typename InputImageType::PointType;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );
    
    enum MergingMethodType { AVERAGE = 0, MAXIMUM = 1 };
    itkGetMacro( Method, MergingMethodType);

    /** Run-time type information (and related methods). */
    itkTypeMacro(CompoundImageFilter, ImageToImageFilter);
//    itkTypeMacro(InternalMaskType, Internal3DMaskType);

    void SetMergingMethodToAverage() { this->m_Method = AVERAGE; }
    void SetMergingMethodToMaximum() { this->m_Method = MAXIMUM; }

    void AddInputImage(InputImagePointer);

public:
    CompoundImageFilter();
    ~CompoundImageFilter(){}
    void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

    /** Does the real work. */
    virtual void GenerateData();

    //void CopyInputImageInformations( ImageType *, itk::SmartPointer<Internal3DMaskType> & );

private:

    CompoundImageFilter(const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented

    void GetImagePhysicalExtent(InputImagePointer, double(&extent)[6]);

protected:

    MergingMethodType m_Method;
    std::vector< InputImagePointer > m_InputImages;
    
};
} //namespace ITK
 
 
#ifndef ITK_MANUAL_INSTANTIATION
#include "itkCompoundImageFilter.hxx"
#endif
 
 
#endif // __itkCompoundImageFilter_h
