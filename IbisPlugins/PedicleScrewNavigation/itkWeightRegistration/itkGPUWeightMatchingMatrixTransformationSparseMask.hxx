/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// by Houssem Gueziri, from itkGPUOrientationMatchingMatrixTransformationSparseMask.hxx

#ifndef __itkGPUWeightMatchingMatrixTransformationSparseMask_hxx
#define __itkGPUWeightMatchingMatrixTransformationSparseMask_hxx

#include <itkGaussianDerivativeOperator.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkMacro.h>
#include <itkMatrix.h>
#include <itkOpenCLUtil.h>
#include <itkTimeProbe.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>

#include "GPUWeightMatchingMatrixTransformationSparseMaskKernel.h"
#include "itkGPUWeightMatchingMatrixTransformationSparseMask.h"

namespace itk
{
/**
 * Default constructor
 */
template <class TFixedImage, class TMovingImage>
GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage,
                                                TMovingImage>::GPUWeightMatchingMatrixTransformationSparseMask()
{
    m_Debug = false;

    if( !itk::IsGPUAvailable() )
    {
        itkExceptionMacro( << "OpenCL-enabled GPU is not present." );
    }

    m_IntensityMatchingKernel = 0;
    /* Initialize GPU Context */
    this->InitializeGPUContext();

    m_FixedImage          = NULL;
    m_MovingImage         = NULL;
    m_FixedImageGPUImage  = NULL;
    m_cpuFixedImageBuffer = NULL;

    m_Transform   = NULL;
    m_MetricValue = 0;

    m_MovingIntensityThreshold = 0;
}

template <class TFixedImage, class TMovingImage>
GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage,
                                                TMovingImage>::~GPUWeightMatchingMatrixTransformationSparseMask()
{
    // this->ReleaseGPUInputBuffer();
    if( m_IntensityMatchingKernel )
    {
        clReleaseKernel( m_IntensityMatchingKernel );
        clReleaseMemObject( m_gpuMovingSamples );
        clReleaseMemObject( m_gpuMovingLocationSamples );
        clReleaseMemObject( m_FixedImageGPUImage );
        clReleaseMemObject( m_gpuMetricAccum );
    }
}

template <class TFixedImage, class TMovingImage>
void GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::InitializeGPUContext( void )
{
    cl_int errid;

    // Get the platforms
    errid = clGetPlatformIDs( 0, NULL, &m_NumberOfPlatforms );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    // Get NVIDIA platform by default
    m_Platform = OpenCLSelectPlatform( "NVIDIA" );
    assert( m_Platform != NULL );

    cl_device_type devType = CL_DEVICE_TYPE_GPU;
    m_Devices              = OpenCLGetAvailableDevices( m_Platform, devType, &m_NumberOfDevices );

    // create context
    m_Context = clCreateContext( 0, m_NumberOfDevices, m_Devices, NULL, NULL, &errid );
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
void GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::PrintSelf( std::ostream & os,
                                                                                            Indent indent ) const
{
    Superclass::PrintSelf( os, indent );

    // GetTypenameInString( typeid(TElement), os);
}

template <class TFixedImage, class TMovingImage>
unsigned int GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::NextPow2( unsigned int x )
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
cl_kernel GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::CreateKernelFromFile(
    const char * filename, const char * cPreamble, const char * kernelname, const char * cOptions )
{
    FILE * pFileStream = NULL;
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
cl_kernel GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::CreateKernelFromString(
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

    errid = clBuildProgram( m_Program, 0, NULL, cOptions, NULL, NULL );

    if( errid != CL_SUCCESS )
    {
        size_t paramValueSize = 0;
        clGetProgramBuildInfo( m_Program, 0, CL_PROGRAM_BUILD_LOG, 0, NULL, &paramValueSize );
        char * paramValue;
        paramValue = new char[paramValueSize + 1];
        clGetProgramBuildInfo( m_Program, 0, CL_PROGRAM_BUILD_LOG, paramValueSize, paramValue, NULL );
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
 * Prepare images for GPU
 */
template <class TFixedImage, class TMovingImage>
void GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::PrepareFixedImage( void )
{
    if( !m_FixedImage )
    {
        itkExceptionMacro( << "Fixed Image is not set" );
    }
    m_FixedImage->Update();

    itk::TimeProbe clock;
    clock.Start();
    /*Create Fixed Image Buffer */
    unsigned int nbrOfPixelsInFixedImage = m_FixedImage->GetBufferedRegion().GetNumberOfPixels();

    typename NormalizeFixedImageFilterType::Pointer normalizeFilter = NormalizeFixedImageFilterType::New();
    normalizeFilter->SetInput( m_FixedImage );

    typename RescaleIntensityFixedImageFilterType::Pointer rescaleIntensityFilter =
        RescaleIntensityFixedImageFilterType::New();
    rescaleIntensityFilter->SetInput( normalizeFilter->GetOutput() );
    rescaleIntensityFilter->SetOutputMinimum( 0.0 );
    rescaleIntensityFilter->SetOutputMaximum( 1.0 );

    rescaleIntensityFilter->Update();
    //  m_FixedImage = normalizeFilter->GetOutput();

    cl_int errid;

    m_FixedImageGPUBuffer = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                            sizeof( FixedImagePixelType ) * nbrOfPixelsInFixedImage,
                                            rescaleIntensityFilter->GetOutput()->GetBufferPointer(), &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    int imgSize[3];
    imgSize[0] = m_FixedImage->GetLargestPossibleRegion().GetSize()[0];
    imgSize[1] = m_FixedImage->GetLargestPossibleRegion().GetSize()[1];
    imgSize[2] = m_FixedImage->GetLargestPossibleRegion().GetSize()[2];

    cl_image_format gpu_image_format;
    gpu_image_format.image_channel_order     = CL_R;
    gpu_image_format.image_channel_data_type = CL_FLOAT;

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
    desc.buffer            = NULL;

    m_FixedImageGPUImage =
        clCreateImage( m_Context, CL_MEM_READ_ONLY, &( gpu_image_format ), &desc, m_cpuFixedImageBuffer, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    size_t origin[3] = { 0, 0, 0 };
    size_t region[3] = { ( size_t )( imgSize[0] ), ( size_t )( imgSize[1] ), ( size_t )( imgSize[2] ) };
    errid = clEnqueueCopyBufferToImage( m_CommandQueue[0], m_FixedImageGPUBuffer, m_FixedImageGPUImage, 0, origin,
                                        region, 0, NULL, NULL );
    errid = clFinish( m_CommandQueue[0] );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    clReleaseMemObject( m_FixedImageGPUBuffer );
}

/**
 * Prepare Moving image
 */
template <class TFixedImage, class TMovingImage>
void GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::PrepareMovingImage( void )
{
    if( !m_MovingImage )
    {
        itkExceptionMacro( << "Moving Image is not set" );
    }
    m_MovingImage->Update();
    /*Create Moving Image Buffer */

    typename NormalizeMovingImageFilterType::Pointer normalizeFilter = NormalizeMovingImageFilterType::New();
    normalizeFilter->SetInput( m_MovingImage );

    typename RescaleIntensityMovingImageFilterType::Pointer rescaleIntensityFilter =
        RescaleIntensityMovingImageFilterType::New();
    rescaleIntensityFilter->SetInput( normalizeFilter->GetOutput() );
    rescaleIntensityFilter->SetOutputMinimum( 0.0 );
    rescaleIntensityFilter->SetOutputMaximum( 1.0 );

    rescaleIntensityFilter->Update();

    unsigned int nbrOfPixelsInMovingImage = m_MovingImage->GetBufferedRegion().GetNumberOfPixels();

    InternalRealType * cpuMovingBuffer =
        (InternalRealType *)malloc( nbrOfPixelsInMovingImage * sizeof( InternalRealType ) );
    memset( cpuMovingBuffer, (InternalRealType)0, nbrOfPixelsInMovingImage * sizeof( InternalRealType ) );

    InternalRealType * movingLocations =
        (InternalRealType *)malloc( 4 * nbrOfPixelsInMovingImage * sizeof( InternalRealType ) );
    memset( movingLocations, (InternalRealType)0, 4 * nbrOfPixelsInMovingImage * sizeof( InternalRealType ) );

    unsigned int nbrOfMaskPixelsInMovingImage = 0;
    MovingImageIteratorType imageIterator( m_MovingImage, m_MovingImage->GetRequestedRegion() );
    typename MovingImageType::IndexType index;
    typename MovingImageType::PointType physicalPoint;

    for( imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator )
    {
        index = imageIterator.GetIndex();
        if( imageIterator.Get() > m_MovingIntensityThreshold )
        {
            this->m_MovingImage->TransformIndexToPhysicalPoint( index, physicalPoint );
            cpuMovingBuffer[nbrOfMaskPixelsInMovingImage] =
                (InternalRealType)rescaleIntensityFilter->GetOutput()->GetPixel( index );
            for( int d = 0; d < MovingImageDimension; ++d )
            {
                movingLocations[4 * nbrOfMaskPixelsInMovingImage + d] = (InternalRealType)physicalPoint[d];
            }
            movingLocations[4 * nbrOfMaskPixelsInMovingImage + 3] = (InternalRealType)1.0;
            nbrOfMaskPixelsInMovingImage++;
        }
    }

    m_NumberOfPixels = nbrOfMaskPixelsInMovingImage;

    unsigned int maxThreads = 256;
    m_Threads = ( m_NumberOfPixels < maxThreads * 2 ) ? this->NextPow2( ( m_NumberOfPixels + 1 ) / 2 ) : maxThreads;
    m_Blocks  = ( m_NumberOfPixels + m_Threads - 1 ) / ( m_Threads );

    m_cpuMovingSamples = (InternalRealType *)malloc( m_Blocks * m_Threads * 4 * sizeof( InternalRealType ) );
    memset( m_cpuMovingSamples, (InternalRealType)0, m_Blocks * m_Threads * sizeof( InternalRealType ) );

    m_cpuMovingLocationSamples = (InternalRealType *)malloc( m_Blocks * m_Threads * 4 * sizeof( InternalRealType ) );
    memset( m_cpuMovingLocationSamples, (InternalRealType)0, 4 * m_Blocks * m_Threads * sizeof( InternalRealType ) );

    for( int i = 0; i < m_NumberOfPixels; ++i )
    {
        m_cpuMovingSamples[i] = (InternalRealType)cpuMovingBuffer[i];
        for( int d = 0; d < MovingImageDimension; ++d )
        {
            m_cpuMovingLocationSamples[4 * i + d] = (InternalRealType)movingLocations[4 * i + d];
        }
        m_cpuMovingLocationSamples[4 * i + 3] = (InternalRealType)1.0;
    }

    cl_int errid;
    m_gpuMovingSamples =
        clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        m_Blocks * m_Threads * sizeof( InternalRealType ), m_cpuMovingSamples, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    m_gpuMovingLocationSamples =
        clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        m_Blocks * m_Threads * 4 * sizeof( InternalRealType ), m_cpuMovingLocationSamples, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    delete[] cpuMovingBuffer;
    delete[] movingLocations;
}

/**
 * Update Metric Value
 */
template <class TFixedImage, class TMovingImage>
void GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::CreateGPUVariablesForCostFunction(
    void )
{
    cl_int errid;
    unsigned int dummySize             = 12;
    m_cpuFixedTransformPhysicalToIndex = (InternalRealType *)malloc( dummySize * sizeof( InternalRealType ) );
    memset( m_cpuFixedTransformPhysicalToIndex, 0, dummySize * sizeof( InternalRealType ) );

    m_cpuTransform = (InternalRealType *)malloc( dummySize * sizeof( InternalRealType ) );
    memset( m_cpuTransform, 0, dummySize * sizeof( InternalRealType ) );
    m_gpuTransform = clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                     dummySize * sizeof( InternalRealType ), m_cpuTransform, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    m_gpuMetricAccum = clCreateBuffer( m_Context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                       m_Blocks * sizeof( InternalRealType ), NULL, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    FixedImageDirectionType fixedIndexToLocation = m_FixedImage->GetDirection();
    FixedImageDirectionType scale;
    FixedImagePointType fixedOrigin = m_FixedImage->GetOrigin();

    for( unsigned int i = 0; i < FixedImageDimension; i++ )
    {
        scale[i][i] = m_FixedImage->GetSpacing()[i];
    }

    fixedIndexToLocation = fixedIndexToLocation * scale;
    vnl_matrix_fixed<TransformRealType, FixedImageDimension, FixedImageDimension> fixedLocToIdx;
    fixedLocToIdx = fixedIndexToLocation.GetInverse();

    for( unsigned int i = 0; i < FixedImageDimension; i++ )
    {
        for( unsigned int j = 0; j < FixedImageDimension; j++ )
        {
            m_cpuFixedTransformPhysicalToIndex[4 * i + j] = fixedLocToIdx[i][j];
        }
        m_cpuFixedTransformPhysicalToIndex[4 * i + 3] = fixedOrigin[i];
    }

    m_gpuFixedTransformPhysicalToIndex =
        clCreateBuffer( m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, dummySize * sizeof( InternalRealType ),
                        m_cpuFixedTransformPhysicalToIndex, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
}

/**
 * Update Metric Value
 */
template <class TFixedImage, class TMovingImage>
void GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::UpdateGPUTransformVariables( void )
{
    m_TransformMatrix = m_Transform->GetMatrix();
    m_TransformOffset = m_Transform->GetOffset();

    for( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
        for( unsigned int j = 0; j < MovingImageDimension; j++ )
        {
            m_cpuTransform[4 * i + j] = m_TransformMatrix[i][j];
        }
        m_cpuTransform[4 * i + 3] = m_TransformOffset[i];
    }

    cl_int errid;
    errid = clEnqueueWriteBuffer( m_CommandQueue[0], m_gpuTransform, CL_TRUE, 0, 12 * sizeof( InternalRealType ),
                                  m_cpuTransform, 0, NULL, NULL );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
}

/**
 * Update Metric Value
 */
template <class TFixedImage, class TMovingImage>
void GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>::Update( void )
{
    if( !m_Transform )
    {
        itkExceptionMacro( << "Transform is not set." );
    }

    if( !m_FixedImageGPUImage )
    {
        if( m_Debug ) std::cout << "Preparing images.." << std::endl;
        this->PrepareFixedImage();
        this->PrepareMovingImage();
        this->CreateGPUVariablesForCostFunction();

        /* Build Orientation Matching Kernel */
        std::ostringstream defines;
        defines << "#define N " << m_Blocks * m_Threads << std::endl;
        defines << "#define LOCALSIZE " << m_Threads << std::endl;
        defines << "#define DIM_3 " << std::endl;

        m_IntensityMatchingKernel =
            CreateKernelFromString( GPUWeightMatchingMatrixTransformationSparseMaskKernel, defines.str().c_str(),
                                    "IntensityMatchingMetricSparseMask", "" );
    }

    if( m_TransformMatrix == m_Transform->GetMatrix() && m_TransformOffset == m_Transform->GetOffset() )
    {
        return;
    }

    this->UpdateGPUTransformVariables();

    size_t globalSize[1];
    size_t localSize[1];

    globalSize[0] = m_Blocks * m_Threads;
    localSize[0]  = m_Threads;

    int argidx = 0;

    clSetKernelArg( m_IntensityMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuTransform );
    clSetKernelArg( m_IntensityMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuMovingSamples );
    clSetKernelArg( m_IntensityMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuMovingLocationSamples );
    clSetKernelArg( m_IntensityMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_FixedImageGPUImage );
    clSetKernelArg( m_IntensityMatchingKernel, argidx++, sizeof( cl_mem ),
                    (void *)&m_gpuFixedTransformPhysicalToIndex );
    clSetKernelArg( m_IntensityMatchingKernel, argidx++, sizeof( cl_mem ), (void *)&m_gpuMetricAccum );
    clSetKernelArg( m_IntensityMatchingKernel, argidx++, sizeof( InternalRealType ) * m_Threads, NULL );

    clEnqueueNDRangeKernel( m_CommandQueue[0], m_IntensityMatchingKernel, 1, NULL, globalSize, localSize, 0, NULL,
                            NULL );

    cl_int errid;
    errid = clFinish( m_CommandQueue[0] );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    m_cpuMetricAccum =
        (InternalRealType *)clEnqueueMapBuffer( m_CommandQueue[0], m_gpuMetricAccum, CL_TRUE, CL_MAP_READ, 0,
                                                m_Blocks * sizeof( InternalRealType ), 0, NULL, NULL, &errid );
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

    InternalRealType metricSum = 0;
    for( unsigned int i = 0; i < m_Blocks; i++ )
    {
        metricSum += m_cpuMetricAccum[i];
    }

    m_MetricValue = 0;
    if( metricSum > 0 ) m_MetricValue = (InternalRealType)metricSum;
    m_MetricValue = m_MetricValue / (InternalRealType)m_NumberOfPixels;
}

}  // end namespace itk

#endif
