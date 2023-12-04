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

#ifndef __itkGPUOrientationMatchingMatrixTransformationSparseMask_hxx
#define __itkGPUOrientationMatchingMatrixTransformationSparseMask_hxx

// uncomment this line to enable output gradient images for debug
//#define __OUTPUT_GRADIENTS__

#include <itkGaussianDerivativeOperator.h>
#include <itkMacro.h>
#include <itkMatrix.h>
#include <itkOpenCLUtil.h>
#include <itkTimeProbe.h>
#include <vnl/vnl_matrix.h>

#include "GPUDiscreteGaussianGradientImageFilter.h"
#include "GPUOrientationMatchingMatrixTransformationSparseMaskKernel.h"
#include "itkGPUOrientationMatchingMatrixTransformationSparseMask.h"

#ifdef __OUTPUT_GRADIENTS__
#include <itkImageFileWriter.h>
#endif

namespace itk
{
/**
 * Default constructor
 */
template <class TFixedImage, class TMovingImage>
GPUOrientationMatchingMatrixTransformationSparseMask<
    TFixedImage, TMovingImage>::GPUOrientationMatchingMatrixTransformationSparseMask()
{
    m_Debug = false;

    if( !itk::IsGPUAvailable() )
    {
        itkExceptionMacro( << "OpenCL-enabled GPU is not present." );
    }

    /* Initialize GPU Context */
    this->InitializeGPUContext();

    m_OrientationMatchingKernel = 0;
    m_Percentile                = 0.9;
    m_N                         = 2;

    m_GradientScale = 1.0;

    m_ComputeMask   = true;
    m_MaskThreshold = 0.0;

    m_FixedImage  = nullptr;
    m_MovingImage = nullptr;

    m_MovingImageGradientGPUImage = nullptr;

    m_Transform   = nullptr;
    m_MetricValue = 0;

    m_UseFixedImageMask            = false;
    m_UseMovingImageMask           = false;
    m_FixedImageMaskSpatialObject  = nullptr;
    m_MovingImageMaskSpatialObject = nullptr;
    SetSamplingStrategyToRandom();

    m_FixedImageGradientGPUBuffer  = NULL;
    m_FixedImageGPUBuffer          = NULL;
    m_FixedImageMaskGPUBuffer      = NULL;
    m_MovingImageGradientGPUBuffer = NULL;
    m_MovingImageGPUBuffer         = NULL;
    m_MovingImageMaskGPUBuffer     = NULL;
    m_gpuFixedGradientSamples      = NULL;
    m_gpuFixedLocationSamples      = NULL;
    m_MovingImageGradientGPUImage  = NULL;
    m_gpuMetricAccum               = NULL;
    m_gpuDummy                     = NULL;
}

template <class TFixedImage, class TMovingImage>
GPUOrientationMatchingMatrixTransformationSparseMask<
    TFixedImage, TMovingImage>::~GPUOrientationMatchingMatrixTransformationSparseMask()
{
    // this->ReleaseGPUInputBuffer();
    if( m_OrientationMatchingKernel )
    {
        clReleaseKernel( m_OrientationMatchingKernel );
    }

    if( m_gpuFixedGradientSamples ) clReleaseMemObject( m_gpuFixedGradientSamples );
    if( m_gpuFixedLocationSamples ) clReleaseMemObject( m_gpuFixedLocationSamples );
    if( m_MovingImageGradientGPUImage ) clReleaseMemObject( m_MovingImageGradientGPUImage );
    if( m_gpuMetricAccum ) clReleaseMemObject( m_gpuMetricAccum );
    if( m_gpuDummy ) clReleaseMemObject( m_gpuDummy );
    if( m_FixedImageGradientGPUBuffer ) clReleaseMemObject( m_FixedImageGradientGPUBuffer );
    if( m_FixedImageGPUBuffer ) clReleaseMemObject( m_FixedImageGPUBuffer );
    if( m_FixedImageMaskGPUBuffer ) clReleaseMemObject( m_FixedImageMaskGPUBuffer );
    if( m_MovingImageGradientGPUBuffer ) clReleaseMemObject( m_MovingImageGradientGPUBuffer );
    if( m_MovingImageGPUBuffer ) clReleaseMemObject( m_MovingImageGPUBuffer );
    if( m_MovingImageMaskGPUBuffer ) clReleaseMemObject( m_MovingImageMaskGPUBuffer );
}

template <class TFixedImage, class TMovingImage>
void GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::InitializeGPUContext( void )
{
    cl_int errid;

    // Get the platforms
    errid = clGetPlatformIDs( 0, nullptr, &m_NumberOfPlatforms );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    // Get NVIDIA platform by default
    m_Platform = OpenCLSelectPlatform( "NVIDIA" );
    assert( m_Platform != nullptr );

    cl_device_type devType = CL_DEVICE_TYPE_GPU;
    m_Devices              = OpenCLGetAvailableDevices( m_Platform, devType, &m_NumberOfDevices );

    // create context
    m_Context = clCreateContext( 0, m_NumberOfDevices, m_Devices, nullptr, nullptr, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    // create command queues
    m_CommandQueue = (cl_command_queue *)malloc( m_NumberOfDevices * sizeof( cl_command_queue ) );
    for( unsigned int i = 0; i < m_NumberOfDevices; i++ )
    {
#ifdef __APPLE__
        m_CommandQueue[i] = clCreateCommandQueue( m_Context, m_Devices[i], 0, &errid );
#elif defined( WIN32 ) || defined( _WIN32 )
        m_CommandQueue[i] = clCreateCommandQueue( m_Context, m_Devices[i], 0, &errid );
#else
        m_CommandQueue[i] = clCreateCommandQueueWithProperties( m_Context, m_Devices[i], 0, &errid );
#endif
        OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
    }
}

/**
 * Standard "PrintSelf" method.
 */
template <class TFixedImage, class TMovingImage>
void GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::PrintSelf( std::ostream & os,
                                                                                                 Indent indent ) const
{
    Superclass::PrintSelf( os, indent );

    // GetTypenameInString( typeid(TElement), os);
}

template <class TFixedImage, class TMovingImage>
unsigned int GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::NextPow2( unsigned int x )
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

/**
 * Create OpenCL Kernel from File and Preamble
 */
template <class TFixedImage, class TMovingImage>
cl_kernel GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::CreateKernelFromFile(
    const char * filename, const char * cPreamble, const char * kernelname, const char * cOptions )
{
    FILE * pFileStream = nullptr;
    pFileStream        = fopen( filename, "rb" );
    if( pFileStream == 0 )
    {
        itkExceptionMacro( << "Cannot open OpenCL source file " << filename );
    }
    fseek( pFileStream, 0, SEEK_END );
    size_t szSourceLength       = ftell( pFileStream );
    char * OriginalSourceString = new char[szSourceLength];
    rewind( pFileStream );
    fread( OriginalSourceString, sizeof( char ), szSourceLength, pFileStream );

    cl_kernel kernel = CreateKernelFromString( OriginalSourceString, cPreamble, kernelname, cOptions );
}

template <class TFixedImage, class TMovingImage>
cl_kernel GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::CreateKernelFromString(
    const char * cOriginalSourceString, const char * cPreamble, const char * kernelname, const char * cOptions )
{
    cl_int errid;

    size_t szSourceLength, szFinalLength;

    size_t szPreambleLength = strlen( cPreamble );
    szSourceLength          = strlen( cOriginalSourceString );

    char * cSourceString = (char *)malloc( szSourceLength + szPreambleLength + 1000 );
    if( szPreambleLength > 0 ) memcpy( cSourceString, cPreamble, szPreambleLength );
    if( szSourceLength > 0 ) memcpy( cSourceString + szPreambleLength, cOriginalSourceString, szSourceLength );

    szFinalLength                = szSourceLength + szPreambleLength;
    cSourceString[szFinalLength] = '\0';

    m_Program = clCreateProgramWithSource( m_Context, 1, (const char **)&cSourceString, &szFinalLength, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
    free( cSourceString );

    errid = clBuildProgram( m_Program, 0, nullptr, cOptions, nullptr, nullptr );
    if( errid != CL_SUCCESS )
    {
        size_t paramValueSize = 0;
        clGetProgramBuildInfo( m_Program, 0, CL_PROGRAM_BUILD_LOG, 0, nullptr, &paramValueSize );
        char * paramValue;
        paramValue = new char[paramValueSize + 1];
        clGetProgramBuildInfo( m_Program, 0, CL_PROGRAM_BUILD_LOG, paramValueSize, paramValue, nullptr );
        paramValue[paramValueSize] = '\0';
        if( m_Debug ) std::cerr << paramValue << std::endl;
        free( paramValue );
        OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
        itkExceptionMacro( << "Cannot Build Program" );
    }

    cl_kernel kernel = clCreateKernel( m_Program, kernelname, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    return kernel;
}

/**
 * Compute Gradients of Fixed and Moving Image
 */
template <class TFixedImage, class TMovingImage>
void GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::ComputeFixedImageGradient( void )
{
    if( !m_FixedImage )
    {
        itkExceptionMacro( << "Fixed Image is not set" );
    }
    m_FixedImage->Update();

    itk::TimeProbe clockGPUKernel;
    clockGPUKernel.Start();
    /*Create Fixed Image Buffer */
    unsigned int nbrOfPixelsInFixedImage = m_FixedImage->GetBufferedRegion().GetNumberOfPixels();

    cl_int errid;
    m_FixedImageGPUBuffer = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                            sizeof( FixedImagePixelType ) * nbrOfPixelsInFixedImage,
                                            (void *)m_FixedImage->GetBufferPointer(), &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    int imgSize[3];
    imgSize[0] = m_FixedImage->GetLargestPossibleRegion().GetSize()[0];
    imgSize[1] = m_FixedImage->GetLargestPossibleRegion().GetSize()[1];
    imgSize[2] = m_FixedImage->GetLargestPossibleRegion().GetSize()[2];

    InternalRealType * cpuFixedGradientBuffer =
        (InternalRealType *)malloc( 4 * nbrOfPixelsInFixedImage * sizeof( InternalRealType ) );
    memset( cpuFixedGradientBuffer, (InternalRealType)0, 4 * nbrOfPixelsInFixedImage * sizeof( InternalRealType ) );

    m_FixedImageGradientGPUBuffer =
        clCreateBuffer( m_Context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                        4 * sizeof( FixedImagePixelType ) * nbrOfPixelsInFixedImage, cpuFixedGradientBuffer, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    FixedImageMaskPixelType * fixedMaskBuffer;
    if( ( m_UseFixedImageMask ) && ( m_FixedImageMaskSpatialObject ) )
    {
        fixedMaskBuffer = (FixedImageMaskPixelType *)m_FixedImageMaskSpatialObject->GetImage()->GetBufferPointer();
    }
    else
    {
        if( m_UseFixedImageMask )
        {
            itkWarningMacro( << "FixedImageMaskSpatialObject was not found, UseFixedImageMask is set to OFF" );
            m_UseFixedImageMask = false;
        }
        fixedMaskBuffer =
            (FixedImageMaskPixelType *)malloc( nbrOfPixelsInFixedImage * sizeof( FixedImageMaskPixelType ) );
        memset( fixedMaskBuffer, (FixedImageMaskPixelType)1,
                nbrOfPixelsInFixedImage * sizeof( FixedImageMaskPixelType ) );
    }
    m_FixedImageMaskGPUBuffer = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                sizeof( FixedImageMaskPixelType ) * nbrOfPixelsInFixedImage,
                                                (unsigned char *)fixedMaskBuffer, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    /* Create Gauss Derivative Operator and Populate GPU Buffer */
    std::vector<FixedDerivativeOperatorType> opers;
    opers.resize( FixedImageDimension );
    m_GPUDerivOperatorBuffers.resize( FixedImageDimension );

    std::vector<InternalRealType> kernelNorms;
    kernelNorms.resize( FixedImageDimension );
    for( int dim = 0; dim < FixedImageDimension; dim++ )
    {
        // Set up the operator for this dimension
        opers[dim].SetDirection( dim );
        opers[dim].SetOrder( 1 );
        // convert the variance from physical units to pixels
        double s = m_FixedImage->GetSpacing()[dim];
        s        = s * s;
        opers[dim].SetVariance( m_GradientScale / s );

        opers[dim].CreateDirectional();

        unsigned int numberOfElements = 1;
        for( int j = 0; j < FixedImageDimension; j++ )
        {
            numberOfElements *= opers[dim].GetSize()[j];
        }

        kernelNorms[dim] = 0;
        for( unsigned int k = 0; k < opers[dim].GetSize( 0 ); k++ )
        {
            kernelNorms[dim] += pow( opers[dim].GetElement( k ), 2 );
        }
        kernelNorms[dim] = sqrt( kernelNorms[dim] );

        m_GPUDerivOperatorBuffers[dim] = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                         sizeof( InternalRealType ) * numberOfElements,
                                                         (InternalRealType *)opers[dim].Begin(), &errid );
        OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
    }

    int radius[3];
    radius[0] = radius[1] = radius[2] = 0;

    for( int i = 0; i < FixedImageDimension; i++ )
    {
        radius[i] = opers[i].GetRadius( i );
    }

    size_t localSize[3], globalSize[3];
    localSize[0] = localSize[1] = localSize[2] = OpenCLGetLocalBlockSize( FixedImageDimension );

    if( localSize[0] == 0 )
    {
        itkExceptionMacro( << "localSize == 0" );
    }

    std::ostringstream kernelDefines;
    kernelDefines << "#define THRESHOLD " << m_MaskThreshold << std::endl;
    kernelDefines << "#define DIM_3 " << std::endl;

    m_GradientKernel = CreateKernelFromString( GPUDiscreteGaussianGradientImageFilter, kernelDefines.str().c_str(),
                                               "SeparableNeighborOperatorFilterWithMask", "" );

    for( unsigned int i = 0; i < FixedImageDimension; i++ )
    {
        globalSize[i] = localSize[i] * (unsigned int)ceil( (float)imgSize[i] / (float)localSize[i] );
    }

    int argidx = 0;
    clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_FixedImageGPUBuffer );
    clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_FixedImageGradientGPUBuffer );

    clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_FixedImageMaskGPUBuffer );

    for( unsigned int i = 0; i < FixedImageDimension; i++ )
    {
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_GPUDerivOperatorBuffers[i] );
    }

    for( unsigned int i = 0; i < FixedImageDimension; i++ )
    {
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( int ), &( radius[i] ) );
    }

    for( unsigned int i = 0; i < FixedImageDimension; i++ )
    {
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( int ), &( imgSize[i] ) );
    }

    InternalRealType spacing[3];
    for( unsigned int i = 0; i < FixedImageDimension; i++ )
    {
        spacing[i] = m_FixedImage->GetSpacing()[i];
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( int ), &( spacing[i] ) );
    }

    clEnqueueNDRangeKernel( m_CommandQueue[0], m_GradientKernel, FixedImageDimension, nullptr, globalSize, localSize, 0,
                            nullptr, nullptr );

    errid = clFinish( m_CommandQueue[0] );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    errid = clEnqueueReadBuffer( m_CommandQueue[0], m_FixedImageGradientGPUBuffer, CL_TRUE, 0,
                                 nbrOfPixelsInFixedImage * 4 * sizeof( InternalRealType ), cpuFixedGradientBuffer, 0,
                                 nullptr, nullptr );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    errid = clFinish( m_CommandQueue[0] );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

#ifdef __OUTPUT_GRADIENTS__
    {
        using GradientPixelType                 = itk::Vector<float, 3>;
        using VectorImageType                   = itk::Image<GradientPixelType, 3>;
        VectorImageType::Pointer outputGradient = VectorImageType::New();
        outputGradient->SetOrigin( m_FixedImage->GetOrigin() );
        outputGradient->SetDirection( m_FixedImage->GetDirection() );
        outputGradient->SetSpacing( m_FixedImage->GetSpacing() );
        typename FixedImageType::SizeType fsize = m_FixedImage->GetLargestPossibleRegion().GetSize();
        VectorImageType::SizeType gsize;
        gsize[0] = fsize[0];
        gsize[1] = fsize[1];
        gsize[2] = fsize[2];
        VectorImageType::RegionType region;
        VectorImageType::IndexType start;
        start.Fill( 0 );
        region.SetSize( gsize );
        region.SetIndex( start );
        outputGradient->SetRegions( region );
        outputGradient->Allocate();
        VectorImageType::PixelType pixelValue;
        pixelValue.Fill( 0 );
        outputGradient->FillBuffer( pixelValue );
        outputGradient->Update();
        using GradientImageIterator = itk::ImageRegionIteratorWithIndex<VectorImageType>;
        GradientImageIterator imageIterator( outputGradient, outputGradient->GetRequestedRegion() );
        for( imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator )
        {
            unsigned int idx = static_cast<unsigned int>( outputGradient->ComputeOffset( imageIterator.GetIndex() ) );
            pixelValue[0]    = cpuFixedGradientBuffer[idx * 4 + 0];
            pixelValue[1]    = cpuFixedGradientBuffer[idx * 4 + 1];
            pixelValue[2]    = cpuFixedGradientBuffer[idx * 4 + 2];
            imageIterator.Set( pixelValue );
        }

        using WriterType           = itk::ImageFileWriter<VectorImageType>;
        WriterType::Pointer writer = WriterType::New();
        writer->SetFileName( "FixedGradientImage.nrrd" );
        writer->SetInput( outputGradient );

        try
        {
            writer->Update();
        }
        catch( itk::ExceptionObject & err )
        {
            std::cerr << "ExceptionObject caught !" << std::endl;
            std::cerr << err << std::endl;
        }
    }
#endif

    clockGPUKernel.Stop();
    if( m_Debug ) std::cerr << "Fixed Image Gradient GPU Kernel took:\t" << clockGPUKernel.GetMean() << std::endl;

    FixedGradientMagnitudeSampleType::Pointer sample = FixedGradientMagnitudeSampleType::New();
    IdxSampleType::Pointer maskIdxSample             = IdxSampleType::New();

    itk::TimeProbe clock;
    clock.Start();

    // process full sampling separately
    if( m_SamplingStrategy == FULL )
    {
        typename FixedImageType::RegionType region = m_FixedImage->GetRequestedRegion();
        typename FixedImageType::SizeType size     = region.GetSize();
        typename FixedImageType::IndexType start   = region.GetIndex();
        region.SetIndex( start );
        region.SetSize( size );

        FixedImageIteratorType imageIterator( m_FixedImage, region );
        for( imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator )
        {
            unsigned int idx = static_cast<unsigned int>( m_FixedImage->ComputeOffset( imageIterator.GetIndex() ) );

            if( ( !m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)-1.0 ) ) ||
                ( m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)0.0 ) ) )
            {
                InternalRealType magnitudeValue = 0;
                MeasurementVectorType tempSample;

                for( unsigned int d = 0; d < FixedImageDimension; ++d )
                {
                    magnitudeValue += pow( cpuFixedGradientBuffer[idx * 4 + d] / kernelNorms[d], 2.0 );
                }
                magnitudeValue = sqrt( magnitudeValue );
                tempSample[0]  = magnitudeValue;

                if( imageIterator.Get() > 0 )
                {
                    sample->PushBack( tempSample );
                    maskIdxSample->PushBack( idx );
                }
            }
        }
    }
    else
    {
        typename ImageSamplerType::Pointer imageSampler            = nullptr;
        typename SampleContainerType::Pointer imageSampleContainer = SampleContainerType::New();
        SampleType imageSample;
        typename FixedImageType::RegionType bufferedRegion = m_FixedImage->GetBufferedRegion();
        typename FixedImageType::IndexType imageIndex;
        unsigned int nbrOfPixelsForHistogram = 100000;

        if( ( m_SamplingStrategy == RANDOM ) )
        {
            typename RandomImageSamplerType::Pointer temporaryImageSampler = RandomImageSamplerType::New();
            temporaryImageSampler->SetNumberOfSamples( nbrOfPixelsForHistogram );
            imageSampler = temporaryImageSampler;
        }
        else if( m_SamplingStrategy == GRID )
        {
            typename GridImageSamplerType::Pointer temporaryImageSampler = GridImageSamplerType::New();
            temporaryImageSampler->SetNumberOfSamples( nbrOfPixelsForHistogram );
            imageSampler = temporaryImageSampler;
        }

        imageSampler->SetInput( m_FixedImage );
        imageSampler->SetInputImageRegion( bufferedRegion );

        try
        {
            imageSampler->Update();
        }
        catch( itk::ExceptionObject & err )
        {
            std::cerr << err << std::endl;
            std::cerr << "Cannot grid sample the image" << std::endl;
        }

        imageSampleContainer = imageSampler->GetOutput();

        for( unsigned int i = 0; i < imageSampleContainer->Size(); ++i )
        {
            imageSample = imageSampleContainer->ElementAt( i );
            m_FixedImage->TransformPhysicalPointToIndex( imageSample.m_ImageCoordinates, imageIndex );
            if( bufferedRegion.IsInside( imageIndex ) )
            {
                InternalRealType magnitudeValue = 0;
                MeasurementVectorType tempSample;
                unsigned int idx = static_cast<unsigned int>( m_FixedImage->ComputeOffset( imageIndex ) );
                for( unsigned int d = 0; d < FixedImageDimension; ++d )
                {
                    magnitudeValue += pow( cpuFixedGradientBuffer[idx * 4 + d] / kernelNorms[d], 2.0 );
                }
                magnitudeValue = sqrt( magnitudeValue );
                tempSample[0]  = magnitudeValue;
                if( ( !m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)-1.0 ) ) ||
                    ( m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)0.0 ) ) )
                {
                    sample->PushBack( tempSample );
                    maskIdxSample->PushBack( idx );
                }
            }
        }
    }

    if( m_Debug )
    {
        std::cerr << "Sample Size:\t" << sample->Size() << std::endl;
    }

    itk::TimeProbe clock2;
    clock2.Start();
    SampleToHistogramFilterType::Pointer sampleToHistogramFilter = SampleToHistogramFilterType::New();
    sampleToHistogramFilter->SetInput( sample );

    SampleToHistogramFilterType::HistogramSizeType histogramSize( 1 );
    histogramSize.Fill( 100 );
    sampleToHistogramFilter->SetHistogramSize( histogramSize );

    sampleToHistogramFilter->Update();
    HistogramType::ConstPointer histogram = sampleToHistogramFilter->GetOutput();

    InternalRealType magnitudeThreshold = histogram->Quantile( 0, m_Percentile );

    clock2.Stop();
    if( m_Debug )
    {
        std::cerr << "Computing histogram took:\t" << clock2.GetMean() << std::endl;
        std::cerr << "Magnitude Threshold:\t" << magnitudeThreshold << std::endl;
    }

    unsigned int maxThreads = 256;
    m_Threads = ( m_NumberOfPixels < maxThreads * 2 ) ? this->NextPow2( ( m_NumberOfPixels + 1 ) / 2 ) : maxThreads;
    m_Blocks  = ( m_NumberOfPixels + m_Threads - 1 ) / ( m_Threads );

    m_cpuFixedGradientSamples = (InternalRealType *)malloc( m_Blocks * m_Threads * 4 * sizeof( InternalRealType ) );
    memset( m_cpuFixedGradientSamples, (InternalRealType)0, 4 * m_Blocks * m_Threads * sizeof( InternalRealType ) );

    m_cpuFixedLocationSamples = (InternalRealType *)malloc( m_Blocks * m_Threads * 4 * sizeof( InternalRealType ) );
    memset( m_cpuFixedLocationSamples, (InternalRealType)0, 4 * m_Blocks * m_Threads * sizeof( InternalRealType ) );

    unsigned int numberOfSamples = m_Blocks * m_Threads;

    // process full sampling separately
    if( m_SamplingStrategy == FULL )
    {
        typename FixedImageType::RegionType region = m_FixedImage->GetRequestedRegion();
        typename FixedImageType::SizeType size     = region.GetSize();
        typename FixedImageType::IndexType start   = region.GetIndex();
        region.SetIndex( start );
        region.SetSize( size );

        FixedImageIteratorType imageIterator( m_FixedImage, region );
        unsigned int pixelCntr = 0;

        for( imageIterator.GoToBegin(); !imageIterator.IsAtEnd() & ( pixelCntr < numberOfSamples ); ++imageIterator )
        {
            unsigned int idx = static_cast<unsigned int>( m_FixedImage->ComputeOffset( imageIterator.GetIndex() ) );

            if( ( !m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)-1.0 ) ) ||
                ( m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)0.0 ) ) )
            {
                InternalRealType magnitudeValue = 0;

                for( unsigned int d = 0; d < FixedImageDimension; ++d )
                {
                    magnitudeValue += pow( cpuFixedGradientBuffer[idx * 4 + d] / kernelNorms[d], 2.0 );
                }
                magnitudeValue = sqrt( magnitudeValue );

                if( magnitudeValue > magnitudeThreshold )
                {
                    typename FixedImageType::PointType fixedLocation;
                    this->m_FixedImage->TransformIndexToPhysicalPoint( imageIterator.GetIndex(), fixedLocation );
                    for( unsigned int d = 0; d < FixedImageDimension; ++d )
                    {
                        m_cpuFixedGradientSamples[4 * pixelCntr + d] = cpuFixedGradientBuffer[idx * 4 + d];
                        m_cpuFixedLocationSamples[4 * pixelCntr + d] = (InternalRealType)fixedLocation[d];
                    }
                    m_cpuFixedLocationSamples[4 * pixelCntr + 3] = (InternalRealType)1.0;
                    pixelCntr++;
                }
            }
        }
    }
    else
    {
        typename ImageSamplerType::Pointer imageSampler            = nullptr;
        typename SampleContainerType::Pointer imageSampleContainer = SampleContainerType::New();
        SampleType imageSample;
        typename FixedImageType::RegionType bufferedRegion = m_FixedImage->GetBufferedRegion();
        typename FixedImageType::IndexType imageIndex;

        if( m_SamplingStrategy == RANDOM )
        {
            typename RandomImageSamplerType::Pointer temporaryImageSampler = RandomImageSamplerType::New();
            temporaryImageSampler->SetNumberOfSamples( m_NumberOfPixels );
            imageSampler = temporaryImageSampler;
        }
        else if( m_SamplingStrategy == GRID )
        {
            typename GridImageSamplerType::Pointer temporaryImageSampler = GridImageSamplerType::New();
            temporaryImageSampler->SetNumberOfSamples( m_NumberOfPixels );
            imageSampler = temporaryImageSampler;
        }

        imageSampler->SetInput( m_FixedImage );
        imageSampler->SetInputImageRegion( bufferedRegion );

        try
        {
            imageSampler->Update();
        }
        catch( itk::ExceptionObject & err )
        {
            std::cerr << err << std::endl;
            std::cerr << "Cannot grid sample the image" << std::endl;
        }

        imageSampleContainer = imageSampler->GetOutput();

        unsigned int pixelCntr = 0;
        for( unsigned int i = 0; i < imageSampleContainer->Size(); ++i )
        {
            if( imageSampleContainer->GetElementIfIndexExists( i, &imageSample ) )
            {
                m_FixedImage->TransformPhysicalPointToIndex( imageSample.m_ImageCoordinates, imageIndex );
                if( bufferedRegion.IsInside( imageIndex ) )
                {
                    unsigned int idx = static_cast<unsigned int>( m_FixedImage->ComputeOffset( imageIndex ) );
                    if( ( !m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)-1.0 ) ) ||
                        ( m_ComputeMask && ( cpuFixedGradientBuffer[idx * 4 + 3] > (InternalRealType)0.0 ) ) )
                    {
                        InternalRealType magnitudeValue = 0.0;
                        for( unsigned int d = 0; d < FixedImageDimension; ++d )
                        {
                            magnitudeValue += pow( cpuFixedGradientBuffer[idx * 4 + d] / kernelNorms[d], 2.0 );
                        }
                        magnitudeValue = sqrt( magnitudeValue );
                        if( magnitudeValue > magnitudeThreshold )
                        {
                            typename FixedImageType::PointType fixedLocation;
                            this->m_FixedImage->TransformIndexToPhysicalPoint( m_FixedImage->ComputeIndex( idx ),
                                                                               fixedLocation );
                            for( unsigned int d = 0; d < FixedImageDimension; ++d )
                            {
                                m_cpuFixedGradientSamples[4 * pixelCntr + d] = cpuFixedGradientBuffer[idx * 4 + d];
                                m_cpuFixedLocationSamples[4 * pixelCntr + d] = (InternalRealType)fixedLocation[d];
                            }
                            m_cpuFixedLocationSamples[4 * pixelCntr + 3] = (InternalRealType)1.0;
                            pixelCntr++;
                        }
                    }
                }
            }
        }
    }

    clock.Stop();
    if( m_Debug ) std::cerr << "Post-Processing Fixed Image Gradient took:\t" << clock.GetMean() << std::endl;

    clReleaseKernel( m_GradientKernel );
    clReleaseMemObject( m_FixedImageGradientGPUBuffer );
    clReleaseMemObject( m_FixedImageGPUBuffer );
    clReleaseMemObject( m_FixedImageMaskGPUBuffer );
    m_FixedImageGradientGPUBuffer = NULL;
    m_FixedImageGPUBuffer         = NULL;
    m_FixedImageMaskGPUBuffer     = NULL;
    for( int d = 0; d < FixedImageDimension; ++d )
    {
        clReleaseMemObject( m_GPUDerivOperatorBuffers[d] );
    }
    delete[] cpuFixedGradientBuffer;
}

/**
 * Compute Gradients of Fixed and Moving Image
 */
template <class TFixedImage, class TMovingImage>
void GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::ComputeMovingImageGradient( void )
{
    if( !m_MovingImage )
    {
        itkExceptionMacro( << "Moving Image is not set" );
    }
    if( m_Debug ) std::cout << "Computing Moving Image Gradient" << std::endl;
    /*Create Moving Image Buffer */

    unsigned int nbrOfPixelsInMovingImage = m_MovingImage->GetBufferedRegion().GetNumberOfPixels();

    cl_int errid;

    m_MovingImageGPUBuffer = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                             sizeof( MovingImagePixelType ) * nbrOfPixelsInMovingImage,
                                             m_MovingImage->GetBufferPointer(), &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    int imgSize[3];
    imgSize[0] = m_MovingImage->GetLargestPossibleRegion().GetSize()[0];
    imgSize[1] = m_MovingImage->GetLargestPossibleRegion().GetSize()[1];
    imgSize[2] = m_MovingImage->GetLargestPossibleRegion().GetSize()[2];

#ifdef __OUTPUT_GRADIENTS__
    InternalRealType * cpuMovingGradientBuffer =
        (InternalRealType *)malloc( 4 * nbrOfPixelsInMovingImage * sizeof( InternalRealType ) );
    memset( cpuMovingGradientBuffer, (InternalRealType)0, 4 * nbrOfPixelsInMovingImage * sizeof( InternalRealType ) );

    m_MovingImageGradientGPUBuffer = clCreateBuffer( m_Context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                                     4 * sizeof( MovingImagePixelType ) * nbrOfPixelsInMovingImage,
                                                     cpuMovingGradientBuffer, &errid );
#else
    m_MovingImageGradientGPUBuffer = clCreateBuffer(
        m_Context, CL_MEM_WRITE_ONLY, 4 * sizeof( MovingImagePixelType ) * nbrOfPixelsInMovingImage, nullptr, &errid );
#endif

    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    MovingImageMaskPixelType * movingMaskBuffer;
    if( ( m_UseMovingImageMask ) && ( m_MovingImageMaskSpatialObject ) )
    {
        movingMaskBuffer = (MovingImageMaskPixelType *)m_MovingImageMaskSpatialObject->GetImage()->GetBufferPointer();
    }
    else
    {
        if( m_UseMovingImageMask )
        {
            itkWarningMacro( << "MovingImageMaskSpatialObject was not found, UseMovingImageMask is set to OFF" );
            m_UseMovingImageMask = false;
        }

        movingMaskBuffer =
            (MovingImageMaskPixelType *)malloc( nbrOfPixelsInMovingImage * sizeof( MovingImageMaskPixelType ) );
        memset( movingMaskBuffer, (MovingImageMaskPixelType)1,
                nbrOfPixelsInMovingImage * sizeof( MovingImageMaskPixelType ) );
    }

    m_MovingImageMaskGPUBuffer = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 sizeof( MovingImageMaskPixelType ) * nbrOfPixelsInMovingImage,
                                                 (unsigned char *)movingMaskBuffer, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    /* Create Gauss Derivative Operator and Populate GPU Buffer */
    std::vector<MovingDerivativeOperatorType> opers;
    opers.resize( MovingImageDimension );
    m_GPUDerivOperatorBuffers.resize( MovingImageDimension );

    std::vector<InternalRealType> kernelNorms;
    kernelNorms.resize( MovingImageDimension );
    for( int dim = 0; dim < MovingImageDimension; dim++ )
    {
        // Set up the operator for this dimension
        opers[dim].SetDirection( dim );
        opers[dim].SetOrder( 1 );
        // convert the variance from physical units to pixels
        double s = m_MovingImage->GetSpacing()[dim];
        s        = s * s;
        opers[dim].SetVariance( m_GradientScale / s );

        opers[dim].CreateDirectional();

        unsigned int numberOfElements = 1;
        for( int j = 0; j < MovingImageDimension; j++ )
        {
            numberOfElements *= opers[dim].GetSize()[j];
        }

        kernelNorms[dim] = 0;
        for( unsigned int k = 0; k < opers[dim].GetSize( 0 ); k++ )
        {
            kernelNorms[dim] += pow( opers[dim].GetElement( k ), 2 );
        }
        kernelNorms[dim] = sqrt( kernelNorms[dim] );

        m_GPUDerivOperatorBuffers[dim] = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                         sizeof( InternalRealType ) * numberOfElements,
                                                         (InternalRealType *)opers[dim].Begin(), &errid );
        OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
    }

    std::ostringstream kernelDefines;
    kernelDefines << "#define THRESHOLD " << m_MaskThreshold << std::endl;
    kernelDefines << "#define DIM_3 " << std::endl;

    m_GradientKernel = CreateKernelFromString( GPUDiscreteGaussianGradientImageFilter, kernelDefines.str().c_str(),
                                               "SeparableNeighborOperatorFilterWithMask", "" );

    int radius[3];
    radius[0] = radius[1] = radius[2] = 0;
    for( int i = 0; i < MovingImageDimension; i++ )
    {
        radius[i] = opers[i].GetRadius( i );
    }

    size_t localSize[3], globalSize[3];
    localSize[0] = localSize[1] = localSize[2] = OpenCLGetLocalBlockSize( MovingImageDimension );
    for( int i = 0; i < MovingImageDimension; i++ )
    {
        globalSize[i] = localSize[i] * (unsigned int)ceil( (float)imgSize[i] / (float)localSize[i] );
    }

    int argidx = 0;
    clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_MovingImageGPUBuffer );

    clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_MovingImageGradientGPUBuffer );

    clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_MovingImageMaskGPUBuffer );

    for( int i = 0; i < MovingImageDimension; i++ )
    {
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( cl_mem ), (void *)&m_GPUDerivOperatorBuffers[i] );
    }

    for( int i = 0; i < MovingImageDimension; i++ )
    {
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( int ), &( radius[i] ) );
    }

    for( int i = 0; i < MovingImageDimension; i++ )
    {
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( int ), &( imgSize[i] ) );
    }

    InternalRealType spacing[3];
    for( int i = 0; i < MovingImageDimension; i++ )
    {
        spacing[i] = m_MovingImage->GetSpacing()[i];
        clSetKernelArg( m_GradientKernel, argidx++, sizeof( int ), &( spacing[i] ) );
    }

    // create N-D range object with work-item dimensions and execute kernel
    clEnqueueNDRangeKernel( m_CommandQueue[0], m_GradientKernel, MovingImageDimension, nullptr, globalSize, localSize,
                            0, nullptr, nullptr );
    errid = clFinish( m_CommandQueue[0] );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    cl_image_format gpu_gradient_image_format;
    gpu_gradient_image_format.image_channel_order     = CL_RGBA;
    gpu_gradient_image_format.image_channel_data_type = CL_FLOAT;

    cl_image_desc desc;
    desc.image_type        = CL_MEM_OBJECT_IMAGE3D;
    desc.image_width       = imgSize[0];
    desc.image_height      = imgSize[1];
    desc.image_depth       = imgSize[2];
    desc.image_array_size  = 0;
    desc.image_row_pitch   = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels    = 0;
    desc.num_samples       = 0;
    desc.buffer            = nullptr;

    m_cpuMovingGradientImageBuffer = nullptr;
    m_MovingImageGradientGPUImage  = clCreateImage( m_Context, CL_MEM_READ_ONLY, &( gpu_gradient_image_format ), &desc,
                                                   m_cpuMovingGradientImageBuffer, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    size_t origin[3] = { 0, 0, 0 };
    size_t region[3] = { ( size_t )( imgSize[0] ), ( size_t )( imgSize[1] ), ( size_t )( imgSize[2] ) };
    errid            = clEnqueueCopyBufferToImage( m_CommandQueue[0], m_MovingImageGradientGPUBuffer,
                                        m_MovingImageGradientGPUImage, 0, origin, region, 0, nullptr, nullptr );
    errid            = clFinish( m_CommandQueue[0] );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

#ifdef __OUTPUT_GRADIENTS__
    {
        errid = clEnqueueReadBuffer( m_CommandQueue[0], m_MovingImageGradientGPUBuffer, CL_TRUE, 0,
                                     nbrOfPixelsInMovingImage * 4 * sizeof( InternalRealType ), cpuMovingGradientBuffer,
                                     0, nullptr, nullptr );
        OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

        errid = clFinish( m_CommandQueue[0] );
        OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

        using GradientPixelType                 = itk::Vector<float, 3>;
        using VectorImageType                   = itk::Image<GradientPixelType, 3>;
        VectorImageType::Pointer outputGradient = VectorImageType::New();
        outputGradient->SetOrigin( m_MovingImage->GetOrigin() );
        outputGradient->SetDirection( m_MovingImage->GetDirection() );
        outputGradient->SetSpacing( m_MovingImage->GetSpacing() );
        typename MovingImageType::SizeType fsize = m_MovingImage->GetLargestPossibleRegion().GetSize();
        VectorImageType::SizeType gsize;
        gsize[0] = fsize[0];
        gsize[1] = fsize[1];
        gsize[2] = fsize[2];
        VectorImageType::RegionType region;
        VectorImageType::IndexType start;
        start.Fill( 0 );
        region.SetSize( gsize );
        region.SetIndex( start );
        outputGradient->SetRegions( region );
        outputGradient->Allocate();
        VectorImageType::PixelType pixelValue;
        pixelValue.Fill( 0 );
        outputGradient->FillBuffer( pixelValue );
        outputGradient->Update();

        using GradientImageIterator = itk::ImageRegionIteratorWithIndex<VectorImageType>;
        GradientImageIterator imageIterator( outputGradient, outputGradient->GetRequestedRegion() );
        for( imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator )
        {
            unsigned int idx = static_cast<unsigned int>( outputGradient->ComputeOffset( imageIterator.GetIndex() ) );
            pixelValue[0]    = cpuMovingGradientBuffer[idx * 4 + 0];
            pixelValue[1]    = cpuMovingGradientBuffer[idx * 4 + 1];
            pixelValue[2]    = cpuMovingGradientBuffer[idx * 4 + 2];
            imageIterator.Set( pixelValue );
        }

        using WriterType           = itk::ImageFileWriter<VectorImageType>;
        WriterType::Pointer writer = WriterType::New();
        writer->SetFileName( "MovingGradientImage.nrrd" );
        writer->SetInput( outputGradient );

        try
        {
            writer->Update();
        }
        catch( itk::ExceptionObject & err )
        {
            std::cerr << "ExceptionObject caught !" << std::endl;
            std::cerr << err << std::endl;
        }
    }
#endif

    clReleaseKernel( m_GradientKernel );
    clReleaseMemObject( m_MovingImageGradientGPUBuffer );
    clReleaseMemObject( m_MovingImageGPUBuffer );
    clReleaseMemObject( m_MovingImageMaskGPUBuffer );
    m_MovingImageGradientGPUBuffer = NULL;
    m_MovingImageGPUBuffer         = NULL;
    m_MovingImageMaskGPUBuffer     = NULL;
    for( int d = 0; d < MovingImageDimension; ++d )
    {
        clReleaseMemObject( m_GPUDerivOperatorBuffers[d] );
    }
}

/**
 * Update Metric Value
 */
template <class TFixedImage, class TMovingImage>
void GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::CreateGPUVariablesForCostFunction(
    void )
{
    MovingImageDirectionType movingIndexToLocation = m_MovingImage->GetDirection();

    MovingImageDirectionType scale;

    for( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
        scale[i][i] = m_MovingImage->GetSpacing()[i];
    }
    movingIndexToLocation = movingIndexToLocation * scale;

    MovingImageDirectionType movingLocationToIndex = MovingImageDirectionType( movingIndexToLocation.GetInverse() );
    MovingImagePointType movingOrigin              = m_MovingImage->GetOrigin();

    for( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
        m_mOrigin[i] = movingOrigin[i];
        for( unsigned int j = 0; j < MovingImageDimension; j++ )
        {
            m_locToIdx[i][j] = movingLocationToIndex[i][j];
        }
    }

    cl_int errid;
    unsigned int dummySize = 24;
    m_cpuDummy             = (InternalRealType *)malloc( dummySize * sizeof( InternalRealType ) );
    memset( m_cpuDummy, 0, dummySize * sizeof( InternalRealType ) );
    m_gpuDummy = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 dummySize * sizeof( InternalRealType ), m_cpuDummy, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    m_gpuMetricAccum = clCreateBuffer( m_Context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                       m_Blocks * sizeof( InternalRealType ), nullptr, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    m_gpuFixedGradientSamples =
        clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        m_Blocks * m_Threads * 4 * sizeof( InternalRealType ), m_cpuFixedGradientSamples, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    m_gpuFixedLocationSamples =
        clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        m_Blocks * m_Threads * 4 * sizeof( InternalRealType ), m_cpuFixedLocationSamples, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
}

/**
 * Update Metric Value
 */
template <class TFixedImage, class TMovingImage>
void GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::UpdateGPUTransformVariables(
    void )
{
    m_TransformMatrix = m_Transform->GetMatrix();
    m_TransformOffset = m_Transform->GetOffset();

    m_matrixDummy     = m_locToIdx * m_TransformMatrix.GetVnlMatrix();
    m_matrixTranspose = m_TransformMatrix.GetVnlMatrix().transpose();
    m_OffsetOrigin    = m_TransformOffset.GetVnlVector() - m_mOrigin;
    m_vectorDummy     = m_locToIdx * m_OffsetOrigin;
    for( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
        for( unsigned int j = 0; j < MovingImageDimension; j++ )
        {
            m_cpuDummy[4 * i + j]      = m_matrixDummy[i][j];
            m_cpuDummy[4 * i + j + 12] = m_matrixTranspose[i][j];
        }
        m_cpuDummy[4 * i + 3] = m_vectorDummy[i];
    }

    cl_int errid;
    errid = clEnqueueWriteBuffer( m_CommandQueue[0], m_gpuDummy, CL_TRUE, 0, 24 * sizeof( InternalRealType ),
                                  m_cpuDummy, 0, nullptr, nullptr );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
}

/**
 * Update Metric Value
 */
template <class TFixedImage, class TMovingImage>
void GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::Update( void )
{
    if( !m_Transform )
    {
        itkExceptionMacro( << "Transform is not set." );
    }

    if( m_UseFixedImageMask )
    {
        if( !m_FixedImageMaskSpatialObject )
        {
            itkWarningMacro( << "FixedImageMaskSpatialObject was not found, UseFixedImageMask is set to OFF" );
            m_UseFixedImageMask = false;
        }
    }

    if( m_UseMovingImageMask )
    {
        if( !m_MovingImageMaskSpatialObject )
        {
            itkWarningMacro( << "MovingImageMaskSpatialObject was not found, UseMovingImageMask is set to OFF" );
            m_UseMovingImageMask = false;
        }
    }

    if( !m_MovingImageGradientGPUImage )
    {
        if( m_Debug ) std::cout << "Preparing to compute image gradients.." << std::endl;
        this->ComputeFixedImageGradient();
        this->ComputeMovingImageGradient();

        this->CreateGPUVariablesForCostFunction();

        /* Build Orientation Matching Kernel */
        std::ostringstream defines2;
        defines2 << "#define SEL " << m_N << std::endl;
        defines2 << "#define N " << m_Blocks * m_Threads << std::endl;
        defines2 << "#define LOCALSIZE " << m_Threads << std::endl;
        defines2 << "#define USEMASK " << m_ComputeMask << std::endl;

        m_OrientationMatchingKernel =
            CreateKernelFromString( GPUOrientationMatchingMatrixTransformationSparseMaskKernel, defines2.str().c_str(),
                                    "OrientationMatchingMetricSparseMask", "" );
    }

    if( m_TransformMatrix == m_Transform->GetMatrix() && m_TransformOffset == m_TransformOffset )
    {
        return;
    }

    this->UpdateGPUTransformVariables();

    size_t globalSize[1];
    size_t localSize[1];

    globalSize[0] = m_Blocks * m_Threads;
    localSize[0]  = m_Threads;

    int argidx = 0;

    clSetKernelArg( m_OrientationMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuDummy );
    clSetKernelArg( m_OrientationMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuFixedGradientSamples );
    clSetKernelArg( m_OrientationMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuFixedLocationSamples );
    clSetKernelArg( m_OrientationMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_MovingImageGradientGPUImage );
    clSetKernelArg( m_OrientationMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuMetricAccum );
    clSetKernelArg( m_OrientationMatchingKernel, argidx++, sizeof( InternalRealType ) * m_Threads, nullptr );

    clEnqueueNDRangeKernel( m_CommandQueue[0], m_OrientationMatchingKernel, 1, nullptr, globalSize, localSize, 0,
                            nullptr, nullptr );

    cl_int errid;
    errid = clFinish( m_CommandQueue[0] );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    m_cpuMetricAccum =
        (InternalRealType *)clEnqueueMapBuffer( m_CommandQueue[0], m_gpuMetricAccum, CL_TRUE, CL_MAP_READ, 0,
                                                m_Blocks * sizeof( InternalRealType ), 0, nullptr, nullptr, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    InternalRealType metricSum = 0;
    for( unsigned int i = 0; i < m_Blocks; i++ )
    {
        metricSum += m_cpuMetricAccum[i];
    }

    m_MetricValue = 0;
    if( metricSum > 0 ) m_MetricValue = ( InternalRealType )( metricSum / ( (InternalRealType)globalSize[0] ) );
}

}  // end namespace itk

#endif
