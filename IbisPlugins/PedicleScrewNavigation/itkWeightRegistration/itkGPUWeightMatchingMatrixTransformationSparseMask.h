/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// by Houssem Gueziri, from itkGPUOrientationMatchingMatrixTransformationSparseMask.h

#ifndef __itkGPUWeightMatchingMatrixTransformationSparseMask_h__
#define __itkGPUWeightMatchingMatrixTransformationSparseMask_h__

#include <itkCovariantVector.h>
#include <itkHistogram.h>
#include <itkImage.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkListSample.h>
#include <itkMatrixOffsetTransformBase.h>
#include <itkNormalizeImageFilter.h>
#include <itkObject.h>
#include <itkOpenCLUtil.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkSampleToHistogramFilter.h>

namespace itk
{
template <class TFixedImage, class TMovingImage>
class ITK_EXPORT GPUWeightMatchingMatrixTransformationSparseMask : public Object
{
public:
    /** Standard class typedefs. */
    typedef GPUWeightMatchingMatrixTransformationSparseMask Self;
    typedef Object Superclass;
    typedef SmartPointer<Self> Pointer;
    typedef SmartPointer<const Self> ConstPointer;

    typedef float InternalRealType;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );

    /** Run-time type information (and related methods). */
    itkTypeMacro( GPUWeightMatchingMatrixTransformationSparseMask, Object );

    /** FixedImage image type. */
    typedef TFixedImage FixedImageType;
    typedef typename FixedImageType::PixelType FixedImagePixelType;
    typedef typename FixedImageType::Pointer FixedImagePointer;
    typedef typename FixedImageType::ConstPointer FixedImageConstPointer;
    typedef typename FixedImageType::DirectionType FixedImageDirectionType;
    typedef typename FixedImageType::PointType FixedImagePointType;

    itkGetObjectMacro( FixedImage, FixedImageType );
    itkSetObjectMacro( FixedImage, FixedImageType );

    typedef itk::NormalizeImageFilter<FixedImageType, FixedImageType> NormalizeFixedImageFilterType;
    typedef itk::RescaleIntensityImageFilter<FixedImageType, FixedImageType> RescaleIntensityFixedImageFilterType;

    /** MovingImage image type. */
    typedef TMovingImage MovingImageType;
    typedef typename MovingImageType::PixelType MovingImagePixelType;
    typedef typename MovingImageType::Pointer MovingImagePointer;
    typedef typename MovingImageType::ConstPointer MovingImageConstPointer;
    typedef typename MovingImageType::DirectionType MovingImageDirectionType;
    typedef typename MovingImageType::PointType MovingImagePointType;

    itkGetObjectMacro( MovingImage, MovingImageType );
    itkSetObjectMacro( MovingImage, MovingImageType );

    typedef itk::NormalizeImageFilter<MovingImageType, MovingImageType> NormalizeMovingImageFilterType;
    typedef itk::RescaleIntensityImageFilter<MovingImageType, MovingImageType> RescaleIntensityMovingImageFilterType;

    /** Extract dimension from input image. */
    itkStaticConstMacro( FixedImageDimension, unsigned int, TFixedImage::ImageDimension );
    itkStaticConstMacro( MovingImageDimension, unsigned int, TMovingImage::ImageDimension );

    typedef Image<InternalRealType, FixedImageDimension> RealImageType;
    typedef typename RealImageType::Pointer RealImagePointer;

    typedef CovariantVector<InternalRealType, itkGetStaticConstMacro( FixedImageDimension )> FixedGradientType;

    typedef Image<FixedGradientType, itkGetStaticConstMacro( FixedImageDimension )> FixedImageGradientType;
    typedef typename FixedImageGradientType::Pointer FixedImageGradientPointer;
    typedef typename FixedImageGradientType::ConstPointer FixedImageGradientConstPointer;

    typedef CovariantVector<InternalRealType, itkGetStaticConstMacro( MovingImageDimension )> MovingGradientType;

    typedef Image<MovingGradientType, itkGetStaticConstMacro( MovingImageDimension )> MovingImageGradientType;
    typedef typename MovingImageGradientType::Pointer MovingImageGradientPointer;
    typedef typename MovingImageGradientType::ConstPointer MovingImageGradientConstPointer;

    typedef double TransformRealType;

    typedef MatrixOffsetTransformBase<TransformRealType, FixedImageDimension, MovingImageDimension> MatrixTransformType;
    typedef typename MatrixTransformType::Pointer MatrixTransformPointer;
    typedef typename MatrixTransformType::ConstPointer MatrixTransformConstPointer;

    typedef typename MatrixTransformType::MatrixType TransformMatrixType;
    typedef typename MatrixTransformType::InverseMatrixType TransformInverseMatrixType;
    typedef typename MatrixTransformType::OutputVectorType TransformOffsetType;

    itkGetConstObjectMacro( Transform, MatrixTransformType );
    itkSetObjectMacro( Transform, MatrixTransformType );

    itkGetConstMacro( MetricValue, InternalRealType );

    itkSetMacro( Debug, bool );

    typedef itk::Vector<InternalRealType, 1> MeasurementVectorType;
    typedef itk::Statistics::ListSample<MeasurementVectorType> FixedGradientMagnitudeSampleType;

    typedef itk::Statistics::ListSample<itk::Vector<unsigned int, 1> > IdxSampleType;

    typedef itk::Statistics::Histogram<float, itk::Statistics::DenseFrequencyContainer2> HistogramType;

    typedef itk::Statistics::SampleToHistogramFilter<FixedGradientMagnitudeSampleType, HistogramType>
        SampleToHistogramFilterType;

    using MovingImageIteratorType = itk::ImageRegionConstIteratorWithIndex<MovingImageType>;
    using FixedImageIteratorType  = itk::ImageRegionConstIteratorWithIndex<FixedImageType>;

    void Update( void );

    unsigned int NextPow2( unsigned int x );

protected:
    GPUWeightMatchingMatrixTransformationSparseMask();
    ~GPUWeightMatchingMatrixTransformationSparseMask();

    void InitializeGPUContext( void );

    void PrintSelf( std::ostream & os, Indent indent ) const override;

    void PrepareFixedImage( void );
    void PrepareMovingImage( void );
    void CreateGPUVariablesForCostFunction( void );
    void UpdateGPUTransformVariables( void );

    cl_kernel CreateKernelFromFile( const char * filename, const char * cPreamble, const char * kernelname,
                                    const char * cOptions );
    cl_kernel CreateKernelFromString( const char * cOriginalSourceString, const char * cPreamble,
                                      const char * kernelname, const char * cOptions );

    void PrepareGPUImages( void );

    unsigned int m_NumberOfPixels;

    bool m_Debug;

    unsigned int m_Blocks;
    unsigned int m_Threads;
    MovingImagePixelType m_MovingIntensityThreshold;

    FixedImagePointer m_FixedImage;
    cl_mem m_FixedImageGPUBuffer;

    MovingImagePointer m_MovingImage;
    cl_mem m_MovingImageGPUBuffer;

    cl_mem m_FixedImageGPUImage;

    InternalRealType m_MetricValue;

    MatrixTransformConstPointer m_Transform;

    TransformMatrixType m_TransformMatrix;
    TransformOffsetType m_TransformOffset;

    InternalRealType * m_cpuFixedTransformPhysicalToIndex;
    cl_mem m_gpuFixedTransformPhysicalToIndex;
    InternalRealType * m_cpuTransform;
    cl_mem m_gpuTransform;

    InternalRealType * m_cpuMovingSamples;
    cl_mem m_gpuMovingSamples;

    InternalRealType * m_cpuMovingLocationSamples;
    cl_mem m_gpuMovingLocationSamples;
    InternalRealType * m_cpuLocationsDebug;
    cl_mem m_gpuLocationsDebug;

    InternalRealType * m_cpuMetricAccum;
    cl_mem m_gpuMetricAccum;

    cl_kernel m_IntensityMatchingKernel;

    cl_platform_id m_Platform;
    cl_context m_Context;
    cl_device_id * m_Devices;
    cl_command_queue * m_CommandQueue;
    cl_program m_Program;

    cl_uint m_NumberOfDevices, m_NumberOfPlatforms;

    InternalRealType * m_cpuFixedImageBuffer;

private:
    GPUWeightMatchingMatrixTransformationSparseMask( const Self & );  // purposely not implemented
    void operator=( const Self & );                                   // purposely not implemented
};
}  // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGPUWeightMatchingMatrixTransformationSparseMask.hxx"
#endif

#endif
