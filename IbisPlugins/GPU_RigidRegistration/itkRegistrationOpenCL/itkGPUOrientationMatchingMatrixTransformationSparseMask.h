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

#ifndef __itkGPUOrientationMatchingMatrixTransformationSparseMask_h
#define __itkGPUOrientationMatchingMatrixTransformationSparseMask_h

#include <itkCovariantVector.h>
#include <itkGaussianDerivativeOperator.h>
#include <itkHistogram.h>
#include <itkImage.h>
#include <itkImageMaskSpatialObject.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkListSample.h>
#include <itkMatrixOffsetTransformBase.h>
#include <itkObject.h>
#include <itkOpenCLUtil.h>
#include <itkSampleToHistogramFilter.h>

#include <itkImageFullSampler.h>
#include <itkImageGridSampler.h>
#include <itkImageRandomSampler.h>
#include <itkImageRandomSamplerBase.h>
#include <itkImageSample.h>
#include <itkImageSamplerBase.h>

namespace itk
{
template <class TFixedImage, class TMovingImage>
class ITK_EXPORT GPUOrientationMatchingMatrixTransformationSparseMask : public Object
{
public:
    /** Standard class typedefs. */
    typedef GPUOrientationMatchingMatrixTransformationSparseMask Self;
    typedef Object Superclass;
    typedef SmartPointer<Self> Pointer;
    typedef SmartPointer<const Self> ConstPointer;

    typedef float InternalRealType;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );

    /** Run-time type information (and related methods). */
    itkTypeMacro( GPUOrientationMatchingMatrixTransformationSparseMask, Object );

    /** FixedImage image type. */
    typedef TFixedImage FixedImageType;
    typedef typename FixedImageType::PixelType FixedImagePixelType;
    typedef typename FixedImageType::Pointer FixedImagePointer;
    typedef typename FixedImageType::ConstPointer FixedImageConstPointer;

    itkGetObjectMacro( FixedImage, FixedImageType );
    itkSetObjectMacro( FixedImage, FixedImageType );

    /** MovingImage image type. */
    typedef TMovingImage MovingImageType;
    typedef typename MovingImageType::PixelType MovingImagePixelType;
    typedef typename MovingImageType::Pointer MovingImagePointer;
    typedef typename MovingImageType::ConstPointer MovingImageConstPointer;
    typedef typename MovingImageType::DirectionType MovingImageDirectionType;
    typedef typename MovingImageType::PointType MovingImagePointType;

    itkGetObjectMacro( MovingImage, MovingImageType );
    itkSetObjectMacro( MovingImage, MovingImageType );

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

    itkSetMacro( NumberOfPixels, unsigned int );
    itkSetMacro( N, unsigned int );
    itkSetMacro( Percentile, double );

    itkSetMacro( ComputeMask, bool );
    itkSetMacro( MaskThreshold, double );

    itkSetMacro( GradientScale, double );

    itkSetMacro( Debug, bool );

    typedef GaussianDerivativeOperator<InternalRealType, FixedImageDimension> FixedDerivativeOperatorType;
    typedef GaussianDerivativeOperator<InternalRealType, MovingImageDimension> MovingDerivativeOperatorType;

    typedef itk::Vector<InternalRealType, 1> MeasurementVectorType;
    typedef itk::Statistics::ListSample<MeasurementVectorType> FixedGradientMagnitudeSampleType;

    typedef itk::Statistics::ListSample<itk::Vector<unsigned int, 1> > IdxSampleType;

    typedef itk::Statistics::Histogram<float, itk::Statistics::DenseFrequencyContainer2> HistogramType;

    typedef itk::Statistics::SampleToHistogramFilter<FixedGradientMagnitudeSampleType, HistogramType>
        SampleToHistogramFilterType;

    /** Image Sampler typedefs */
    using ImageSamplerType       = itk::ImageSamplerBase<FixedImageType>;
    using RandomImageSamplerType = itk::ImageRandomSampler<FixedImageType>;
    using GridImageSamplerType   = itk::ImageGridSampler<FixedImageType>;
    using FullImageSamplerType   = itk::ImageFullSampler<FixedImageType>;

    using SampleContainerType = typename ImageSamplerType::ImageSampleContainerType;
    using SampleType          = typename ImageSamplerType::ImageSampleType;
    using GridSpacingType     = typename GridImageSamplerType::SampleGridSpacingType;

    enum SamplingStrategyType
    {
        RANDOM = 0,
        GRID   = 1,
        FULL   = 2
    };

    itkSetMacro( SamplingStrategy, SamplingStrategyType );
    void SetSamplingStrategyToRandom() { this->m_SamplingStrategy = RANDOM; }
    void SetSamplingStrategyToGrid() { this->m_SamplingStrategy = GRID; }
    void SetSamplingStrategyToFull() { this->m_SamplingStrategy = FULL; }

    using FixedImageMaskSpatialObjectType    = itk::ImageMaskSpatialObject<FixedImageDimension>;
    using FixedImageMaskSpatialObjectPointer = typename FixedImageMaskSpatialObjectType::Pointer;
    using FixedImageMaskType                 = typename FixedImageMaskSpatialObjectType::ImageType;
    using FixedImageMaskPointer              = typename FixedImageMaskType::Pointer;
    using FixedImageMaskPixelType            = typename FixedImageMaskType::PixelType;

    using MovingImageMaskSpatialObjectType    = itk::ImageMaskSpatialObject<MovingImageDimension>;
    using MovingImageMaskSpatialObjectPointer = typename MovingImageMaskSpatialObjectType::Pointer;
    using MovingImageMaskType                 = typename MovingImageMaskSpatialObjectType::ImageType;
    using MovingImageMaskPointer              = typename MovingImageMaskType::Pointer;
    using MovingImageMaskPixelType            = typename MovingImageMaskType::PixelType;

    itkSetMacro( FixedImageMaskSpatialObject, FixedImageMaskSpatialObjectPointer );
    itkSetMacro( MovingImageMaskSpatialObject, MovingImageMaskSpatialObjectPointer );
    itkSetMacro( UseFixedImageMask, bool );
    itkSetMacro( UseMovingImageMask, bool );

    using FixedImageMaskIteratorType = itk::ImageRegionConstIteratorWithIndex<FixedImageMaskType>;
    using FixedImageIteratorType     = itk::ImageRegionConstIteratorWithIndex<FixedImageType>;

    using MovingImageMaskIteratorType = itk::ImageRegionConstIteratorWithIndex<MovingImageMaskType>;
    using MovingImageIteratorType     = itk::ImageRegionConstIteratorWithIndex<MovingImageType>;

    void Update( void );

    unsigned int NextPow2( unsigned int x );

protected:
    GPUOrientationMatchingMatrixTransformationSparseMask();
    ~GPUOrientationMatchingMatrixTransformationSparseMask();

    void InitializeGPUContext( void );

    void PrintSelf( std::ostream & os, Indent indent ) const override;

    void ComputeFixedImageGradient( void );
    void ComputeMovingImageGradient( void );
    void CreateGPUVariablesForCostFunction( void );
    void UpdateGPUTransformVariables( void );

    cl_kernel CreateKernelFromFile( const char * filename, const char * cPreamble, const char * kernelname,
                                    const char * cOptions );
    cl_kernel CreateKernelFromString( const char * cOriginalSourceString, const char * cPreamble,
                                      const char * kernelname, const char * cOptions );

    void PrepareGPUImages( void );

    unsigned int m_NumberOfPixels;
    double m_Percentile;
    unsigned int m_N;
    double m_GradientScale;

    // m_ComputeMask: when true only select strong gradient magnitudes
    bool m_ComputeMask;
    double m_MaskThreshold;

    bool m_Debug;

    unsigned int m_Blocks;
    unsigned int m_Threads;

    SamplingStrategyType m_SamplingStrategy;

    FixedImagePointer m_FixedImage;
    cl_mem m_FixedImageGPUBuffer;
    MovingImagePointer m_MovingImage;
    cl_mem m_MovingImageGPUBuffer;

    std::vector<InternalRealType *> m_CPUDerivOperatorBuffers;
    std::vector<cl_mem> m_GPUDerivOperatorBuffers;

    cl_mem m_FixedImageGradientGPUBuffer;
    cl_mem m_MovingImageGradientGPUBuffer;
    cl_mem m_MovingImageGradientGPUImage;

    cl_mem m_MovingImageMaskGPUBuffer;
    cl_mem m_FixedImageMaskGPUBuffer;

    InternalRealType m_MetricValue;

    MatrixTransformConstPointer m_Transform;

    TransformMatrixType m_TransformMatrix;
    TransformOffsetType m_TransformOffset;

    InternalRealType * m_cpuDummy;
    cl_mem m_gpuDummy;

    unsigned int m_NbrPixelsInMask;

    InternalRealType * m_cpuFixedGradientSamples;
    cl_mem m_gpuFixedGradientSamples;

    InternalRealType * m_cpuFixedLocationSamples;
    cl_mem m_gpuFixedLocationSamples;

    InternalRealType * m_cpuMovingGradientImageBuffer;

    InternalRealType * m_cpuMetricAccum;
    cl_mem m_gpuMetricAccum;

    cl_kernel m_OrientationMatchingKernel;
    cl_kernel m_GradientKernel;

    vnl_matrix_fixed<TransformRealType, MovingImageDimension, MovingImageDimension> m_locToIdx;
    vnl_matrix_fixed<TransformRealType, MovingImageDimension, MovingImageDimension> m_matrixDummy;
    vnl_matrix_fixed<TransformRealType, MovingImageDimension, MovingImageDimension> m_matrixTranspose;
    vnl_vector_fixed<TransformRealType, MovingImageDimension> m_mOrigin;
    vnl_vector_fixed<TransformRealType, MovingImageDimension> m_OffsetOrigin;
    vnl_vector_fixed<TransformRealType, MovingImageDimension> m_vectorDummy;

    cl_platform_id m_Platform;
    cl_context m_Context;
    cl_device_id * m_Devices;
    cl_command_queue * m_CommandQueue;
    cl_program m_Program;

    cl_uint m_NumberOfDevices, m_NumberOfPlatforms;

    FixedImageMaskSpatialObjectPointer m_FixedImageMaskSpatialObject;
    MovingImageMaskSpatialObjectPointer m_MovingImageMaskSpatialObject;

    // m_UseFixedImageMask: when true samples gradients from masked region in FixedImage
    bool m_UseFixedImageMask;
    // m_UseMovingImageMask: when true samples gradients from masked region in MovingImage
    bool m_UseMovingImageMask;

    InternalRealType * m_cpuMovingImageBuffer;
    InternalRealType * m_cpuFixedImageBuffer;

private:
    GPUOrientationMatchingMatrixTransformationSparseMask( const Self & );  // purposely not implemented
    void operator=( const Self & );                                        // purposely not implemented
};
}  // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGPUOrientationMatchingMatrixTransformationSparseMask.hxx"
#endif

#endif
