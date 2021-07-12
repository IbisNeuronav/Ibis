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

#include <itkMacro.h>
#include <itkTimeProbe.h>
#include <itkMatrix.h>
#include <vnl/vnl_matrix.h>
#include <itkGaussianDerivativeOperator.h>
#include <itkOpenCLUtil.h>
#include "itkGPUOrientationMatchingMatrixTransformationSparseMask.h"

#include "GPUDiscreteGaussianGradientImageFilter.h"
#include "GPUOrientationMatchingMatrixTransformationSparseMaskKernel.h"

namespace itk
{
/**
 * Default constructor
 */
template< class TFixedImage, class TMovingImage >
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::GPUOrientationMatchingMatrixTransformationSparseMask()
{

  m_Debug = false;

  if(!itk::IsGPUAvailable())
  {
    itkExceptionMacro(<< "OpenCL-enabled GPU is not present.");
  }

  /* Initialize GPU Context */
  this->InitializeGPUContext();

  m_OrientationMatchingKernel = 0;
  m_Percentile = 0.9;
  m_N = 2;

  m_GradientScale = 1.0;

  m_ComputeMask = true;
  m_MaskThreshold = 0.0;

  m_FixedImage = nullptr;
  m_MovingImage = nullptr;

  m_MovingImageGradientGPUImage = nullptr;

  m_Transform = nullptr;
  m_MetricValue = 0;

  m_UseFixedImageMask = false;
  m_FixedImageMaskSpatialObject = nullptr;
  m_MovingImageMaskSpatialObject = nullptr;
  SetSamplingStrategyToGrid();

  m_MovingGPUImage = nullptr;
  m_FixedGPUImage = nullptr;
}


template< class TFixedImage, class TMovingImage >
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::~GPUOrientationMatchingMatrixTransformationSparseMask()
{
  //this->ReleaseGPUInputBuffer();
  if(m_OrientationMatchingKernel)
  {
    clReleaseKernel(m_OrientationMatchingKernel);
    clReleaseMemObject(m_gpuFixedGradientSamples);
    clReleaseMemObject(m_gpuFixedLocationSamples);
    clReleaseMemObject(m_MovingImageGradientGPUImage);
    clReleaseMemObject(m_gpuMetricAccum);
  }
    

}

template< class TFixedImage, class TMovingImage >
void
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::InitializeGPUContext(void)
{
  cl_int errid;

  // Get the platforms
  errid = clGetPlatformIDs(0, nullptr, &m_NumberOfPlatforms);
  OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

  // Get NVIDIA platform by default
  m_Platform = OpenCLSelectPlatform("NVIDIA");
  assert(m_Platform != nullptr);

  cl_device_type devType = CL_DEVICE_TYPE_GPU;
  m_Devices = OpenCLGetAvailableDevices(m_Platform, devType, &m_NumberOfDevices);

  // create context
  m_Context = clCreateContext(0, m_NumberOfDevices, m_Devices, nullptr, nullptr, &errid);
  OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

  // create command queues
  m_CommandQueue = (cl_command_queue *)malloc(m_NumberOfDevices * sizeof(cl_command_queue) );
  for(unsigned int i=0; i<m_NumberOfDevices; i++)
    {
#ifdef __APPLE__
    m_CommandQueue[i] = clCreateCommandQueue(m_Context, m_Devices[i], 0, &errid);
#elif defined(WIN32) || defined(_WIN32)
	  m_CommandQueue[i] = clCreateCommandQueue(m_Context, m_Devices[i], 0, &errid);
#else
    m_CommandQueue[i] = clCreateCommandQueueWithProperties(m_Context, m_Devices[i], 0, &errid);
#endif
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
    }  
}

/**
 * Standard "PrintSelf" method.
 */
template< class TFixedImage, class TMovingImage >
void
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  //GetTypenameInString( typeid(TElement), os);
}

template< class TFixedImage, class TMovingImage >
unsigned int
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::NextPow2( unsigned int x ) {
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
template< class TFixedImage, class TMovingImage >
cl_kernel
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::CreateKernelFromFile(const char * filename, const char * cPreamble, const char * kernelname, const char * cOptions)
{

  FILE*  pFileStream = nullptr;
  pFileStream = fopen(filename, "rb");
  if(pFileStream == 0)
    {
    itkExceptionMacro(<<"Cannot open OpenCL source file " << filename);    
    }  
  fseek(pFileStream, 0, SEEK_END);
  size_t szSourceLength = ftell(pFileStream);
  char *OriginalSourceString = new char[szSourceLength];
  rewind(pFileStream);
  fread(OriginalSourceString, sizeof(char), szSourceLength, pFileStream);

  cl_kernel kernel = CreateKernelFromString(OriginalSourceString, cPreamble, kernelname, cOptions);

}


template< class TFixedImage, class TMovingImage >
cl_kernel
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::CreateKernelFromString(const char * cOriginalSourceString, const char * cPreamble, const char * kernelname, const char * cOptions)
{

  cl_int errid;

  size_t szSourceLength, szFinalLength;

  size_t szPreambleLength = strlen(cPreamble);
  szSourceLength = strlen(cOriginalSourceString);

  char* cSourceString = (char *)malloc(szSourceLength + szPreambleLength + 1000);
  if(szPreambleLength > 0) memcpy(cSourceString, cPreamble, szPreambleLength);
  if(szSourceLength > 0) memcpy(cSourceString + szPreambleLength, cOriginalSourceString, szSourceLength);

  szFinalLength = szSourceLength + szPreambleLength;
  cSourceString[szFinalLength] = '\0';

  m_Program = clCreateProgramWithSource( m_Context, 1, (const char **)&cSourceString, &szFinalLength, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
  free(cSourceString);

  errid = clBuildProgram(m_Program, 0, nullptr, cOptions, nullptr, nullptr);
  if(errid != CL_SUCCESS)
    {
    size_t paramValueSize = 0;
    clGetProgramBuildInfo(m_Program, 0, CL_PROGRAM_BUILD_LOG, 0, nullptr, &paramValueSize);
    char *paramValue;
    paramValue = new char[paramValueSize+1];
    clGetProgramBuildInfo(m_Program, 0, CL_PROGRAM_BUILD_LOG, paramValueSize, paramValue, nullptr);
    paramValue[paramValueSize] = '\0';
    if(m_Debug)
        std::cerr << paramValue << std::endl;
    free( paramValue );
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
    itkExceptionMacro(<<"Cannot Build Program");
    }


  cl_kernel kernel = clCreateKernel(m_Program, kernelname, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

  return kernel;
}

/**
 * Compute Gradients of Fixed and Moving Image
 */
template< class TFixedImage, class TMovingImage >
void
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::ComputeFixedImageGradient(void)
{

  if( !m_FixedImage )
  {
    itkExceptionMacro(<< "Fixed Image is not set" );
  }
  m_FixedImage->Update();

  itk::TimeProbe clockGPUKernel;
  clockGPUKernel.Start();
  /*Create Fixed Image Buffer */
  unsigned int nbrOfPixelsInFixedImage = m_FixedImage->GetBufferedRegion().GetNumberOfPixels();
  
  cl_int errid;
  m_FixedImageGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(FixedImagePixelType)*nbrOfPixelsInFixedImage,
                          (void *)m_FixedImage->GetBufferPointer(), &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  int imgSize[3];
  imgSize[0] = m_FixedImage->GetLargestPossibleRegion().GetSize()[0];
  imgSize[1] = m_FixedImage->GetLargestPossibleRegion().GetSize()[1];
  imgSize[2] = m_FixedImage->GetLargestPossibleRegion().GetSize()[2];

  InternalRealType * cpuFixedGradientBuffer = (InternalRealType*)malloc(4 *
                                  nbrOfPixelsInFixedImage * sizeof(InternalRealType));
  memset(cpuFixedGradientBuffer, (InternalRealType)0, 4 * nbrOfPixelsInFixedImage * sizeof(InternalRealType) );

  m_FixedImageGradientGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                          4 * sizeof(FixedImagePixelType) * nbrOfPixelsInFixedImage,
                          cpuFixedGradientBuffer, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    


  /* Create Gauss Derivative Operator and Populate GPU Buffer */
  std::vector<FixedDerivativeOperatorType> opers;
  opers.resize(FixedImageDimension);
  m_GPUDerivOperatorBuffers.resize(FixedImageDimension);

  std::vector<InternalRealType> kernelNorms;
  kernelNorms.resize(FixedImageDimension);
  for( int dim=0; dim < FixedImageDimension; dim++ )
    {
    // Set up the operator for this dimension      
    opers[dim].SetDirection(dim);
    opers[dim].SetOrder(1);
    // convert the variance from physical units to pixels
    double s = m_FixedImage->GetSpacing()[dim];
    s = s*s;
    opers[dim].SetVariance(m_GradientScale / s);
    
    opers[dim].CreateDirectional();  

    unsigned int numberOfElements = 1;  
      for( int j=0; j < FixedImageDimension; j++ )
      {
        numberOfElements *= opers[dim].GetSize()[j];
      }

    kernelNorms[dim] = 0;
    for(unsigned int k = 0; k < opers[dim].GetSize(0); k++)
      {
      kernelNorms[dim] += pow(opers[dim].GetElement(k),2);
      }
    kernelNorms[dim] = sqrt(kernelNorms[dim]);      

    m_GPUDerivOperatorBuffers[dim] = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          sizeof(InternalRealType)*numberOfElements,
                          (InternalRealType *)opers[dim].Begin(), &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

  }

  int radius[3];
  radius[0] = radius[1] = radius[2] = 0;

  for(int i=0; i<FixedImageDimension; i++)
    {
    radius[i]  = opers[i].GetRadius(i);
    }

  size_t localSize[3], globalSize[3];
  localSize[0] = localSize[1] = localSize[2] = OpenCLGetLocalBlockSize(FixedImageDimension);

  if(localSize[0] == 0)
  {
    itkExceptionMacro(<<"localSize == 0");
  }

  std::string kernelname = "SeparableNeighborOperatorFilter";
  if((m_ComputeMask) & (!m_UseFixedImageMask))
  {
      kernelname = "SeparableNeighborOperatorFilterThresholder";
  }

  m_GradientKernel = CreateKernelFromString(GPUDiscreteGaussianGradientImageFilter,
                                                      "#define DIM_3\n", kernelname.c_str(), "");


  for(unsigned int i=0; i<FixedImageDimension; i++)
    {
    globalSize[i] = localSize[i]*(unsigned int)ceil( (float)imgSize[i]/(float)localSize[i]);
    }

  int argidx = 0;
  clSetKernelArg(m_GradientKernel, argidx++, sizeof(cl_mem), (void *)&m_FixedImageGPUBuffer);
  clSetKernelArg(m_GradientKernel, argidx++, sizeof(cl_mem), (void *)&m_FixedImageGradientGPUBuffer);

  for(unsigned int i=0; i<FixedImageDimension; i++)
    {
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(cl_mem), (void *)&m_GPUDerivOperatorBuffers[i]);
    }  

  for(unsigned int i=0; i<FixedImageDimension; i++)
    {
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(int),  &(radius[i]));
    }

  for(unsigned int i=0; i<FixedImageDimension; i++)
    {
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(int),  &(imgSize[i]) );
    }

  InternalRealType spacing[3];
  for(unsigned int i=0; i<FixedImageDimension; i++)
    {
    spacing[i] = m_FixedImage->GetSpacing()[ i ];
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(int),  &(spacing[i]) );
    } 

  clEnqueueNDRangeKernel(m_CommandQueue[0], m_GradientKernel, FixedImageDimension, nullptr, globalSize, localSize, 0, nullptr, nullptr);

  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  errid = clEnqueueReadBuffer(m_CommandQueue[0], m_FixedImageGradientGPUBuffer, CL_TRUE, 0, 
    nbrOfPixelsInFixedImage * 4 * sizeof(InternalRealType), cpuFixedGradientBuffer, 0, nullptr, nullptr);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  if( m_UseFixedImageMask )
     {
     typename FixedImageMaskType::ConstPointer constFixedImage = m_FixedImageMaskSpatialObject->GetImage();
     FixedImageMaskIteratorType imageIterator( constFixedImage, constFixedImage->GetRequestedRegion() );
     for (imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator)
        {
        unsigned int idx = static_cast<unsigned int>(constFixedImage->ComputeOffset(imageIterator.GetIndex()));

        if(imageIterator.Get() > 0)
           {
           cpuFixedGradientBuffer[idx*4 + 3] = (InternalRealType)1.0;
           }
        else
           {
           cpuFixedGradientBuffer[idx*4 + 3] = (InternalRealType)0.0;
           }
        }
     }

  clockGPUKernel.Stop();
  if(m_Debug)
    std::cerr << "Fixed Image Gradient GPU Kernel took:\t" << clockGPUKernel.GetMean() << std::endl;

  FixedGradientMagnitudeSampleType::Pointer sample = FixedGradientMagnitudeSampleType::New();
  IdxSampleType::Pointer maskIdxSample = IdxSampleType::New();

  itk::TimeProbe clock;
  clock.Start();

  // process full sampling separately
  if ( m_SamplingStrategy == FULL )
    {
    typename FixedImageType::RegionType region = m_FixedImage->GetRequestedRegion();
    typename FixedImageType::SizeType size = region.GetSize();
    typename FixedImageType::IndexType start = region.GetIndex();
    region.SetIndex(start);
    region.SetSize(size);

    FixedImageIteratorType imageIterator( m_FixedImage, region );
    for (imageIterator.GoToBegin(); !imageIterator.IsAtEnd(); ++imageIterator)
        {
        unsigned int idx = static_cast<unsigned int>(m_FixedImage->ComputeOffset(imageIterator.GetIndex()));

        if(!m_ComputeMask || (m_ComputeMask && (cpuFixedGradientBuffer[idx*4 + 3] > (InternalRealType)0.0)))
            {
            InternalRealType magnitudeValue = 0;
            MeasurementVectorType tempSample;

            for ( unsigned int d = 0; d < FixedImageDimension; ++d )
                {
                magnitudeValue += pow( cpuFixedGradientBuffer[idx*4 + d] /  kernelNorms[d], 2.0);
                }
            magnitudeValue = sqrt(magnitudeValue);
            tempSample[0] = magnitudeValue;

            if (imageIterator.Get() > 0)
               {
               sample->PushBack(tempSample);
               maskIdxSample->PushBack(idx);
               }
            }
        }
    }
  else
    {
    typename ImageSamplerType::Pointer imageSampler = 0;
    typename SampleContainerType::Pointer imageSampleContainer = SampleContainerType::New();
    SampleType imageSample;
    typename FixedImageType::RegionType bufferedRegion = m_FixedImage->GetBufferedRegion();
    typename FixedImageType::IndexType imageIndex;
    unsigned int nbrOfPixelsForHistogram =  100000;

    if ( (m_SamplingStrategy == RANDOM) )
        {
        typename RandomImageSamplerType::Pointer temporaryImageSampler = RandomImageSamplerType::New();
        temporaryImageSampler->SetNumberOfSamples( nbrOfPixelsForHistogram );
        imageSampler = temporaryImageSampler;
        }
    else if ( m_SamplingStrategy == GRID )
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
    catch (itk::ExceptionObject & err)
      {
      std::cerr << err << std::endl;
      std::cerr << "Cannot grid sample the image" << std::endl;
      }

    imageSampleContainer = imageSampler->GetOutput();

    for (unsigned int i = 0; i < imageSampleContainer->Size(); ++i)
      {
      imageSample = imageSampleContainer->ElementAt( i );
      m_FixedImage->TransformPhysicalPointToIndex( imageSample.m_ImageCoordinates, imageIndex);
      if (bufferedRegion.IsInside(imageIndex))
         {
         InternalRealType magnitudeValue = 0;
         MeasurementVectorType tempSample;
         unsigned int idx = static_cast<unsigned int>(m_FixedImage->ComputeOffset(imageIndex));
         for ( unsigned int d = 0; d < FixedImageDimension; ++d )
            {
            magnitudeValue += pow( cpuFixedGradientBuffer[idx*4 + d] /  kernelNorms[d], 2.0);
            }
         magnitudeValue = sqrt(magnitudeValue);
         tempSample[0] = magnitudeValue;
         if(!m_ComputeMask || (m_ComputeMask && (cpuFixedGradientBuffer[idx*4 + 3] > (InternalRealType)0.0)))
            {
            sample->PushBack(tempSample);
            maskIdxSample->PushBack(idx);
            }
         }
      }
   }

  if(m_Debug)
    {
      std::cerr << "Sample Size:\t" << sample->Size() << std::endl;
    }

  itk::TimeProbe clock2;
  clock2.Start();
  SampleToHistogramFilterType::Pointer sampleToHistogramFilter = SampleToHistogramFilterType::New();
  sampleToHistogramFilter->SetInput(sample);

  SampleToHistogramFilterType::HistogramSizeType histogramSize(1);
  histogramSize.Fill(100);
  sampleToHistogramFilter->SetHistogramSize(histogramSize);

  sampleToHistogramFilter->Update();
  HistogramType::ConstPointer histogram = sampleToHistogramFilter->GetOutput();

  InternalRealType magnitudeThreshold = histogram->Quantile(0, m_Percentile);

  clock2.Stop();
  if(m_Debug)
  {
      std::cerr << "Computing histogram took:\t" << clock2.GetMean() << std::endl;
      std::cerr << "Magnitude Threshold:\t" << magnitudeThreshold << std::endl;
  }

  unsigned int maxThreads = 256;
  m_Threads = (m_NumberOfPixels < maxThreads*2) ? this->NextPow2((m_NumberOfPixels + 1)/ 2) : maxThreads;
  m_Blocks = (m_NumberOfPixels + m_Threads -1 ) / ( m_Threads );

  m_cpuFixedGradientSamples = (InternalRealType*)malloc(m_Blocks * m_Threads * 4 * sizeof(InternalRealType));
  memset(m_cpuFixedGradientSamples, (InternalRealType)0, 4 * m_Blocks * m_Threads * sizeof(InternalRealType));

  m_cpuFixedLocationSamples = (InternalRealType*)malloc(m_Blocks * m_Threads * 4 * sizeof(InternalRealType));
  memset(m_cpuFixedLocationSamples, (InternalRealType)0, 4 * m_Blocks * m_Threads * sizeof(InternalRealType));

  unsigned int numberOfSamples = m_Blocks * m_Threads;

  // process full sampling separately
  if ( m_SamplingStrategy == FULL )
      {
      typename FixedImageType::RegionType region = m_FixedImage->GetRequestedRegion();
      typename FixedImageType::SizeType size = region.GetSize();
      typename FixedImageType::IndexType start = region.GetIndex();
      region.SetIndex(start);
      region.SetSize(size);

      FixedImageIteratorType imageIterator( m_FixedImage, region );
      unsigned int pixelCntr = 0;

      for (imageIterator.GoToBegin(); !imageIterator.IsAtEnd() & (pixelCntr < numberOfSamples); ++imageIterator)
         {
         unsigned int idx = static_cast<unsigned int>(m_FixedImage->ComputeOffset(imageIterator.GetIndex()));

         if(!m_ComputeMask || (m_ComputeMask && (cpuFixedGradientBuffer[idx*4 + 3] > (InternalRealType)0.0)))
            {

            InternalRealType magnitudeValue = 0;

            for ( unsigned int d = 0; d < FixedImageDimension; ++d )
                {
                magnitudeValue += pow( cpuFixedGradientBuffer[idx*4 + d] /  kernelNorms[d], 2.0);
                }
            magnitudeValue = sqrt(magnitudeValue);

            if(magnitudeValue > magnitudeThreshold)
               {
               typename FixedImageType::PointType fixedLocation;
               this->m_FixedImage->TransformIndexToPhysicalPoint(imageIterator.GetIndex(), fixedLocation);
               for (unsigned int d = 0; d < FixedImageDimension; ++d)
                  {
                  m_cpuFixedGradientSamples[4*pixelCntr+d] = cpuFixedGradientBuffer[idx*4 + d];
                  m_cpuFixedLocationSamples[4*pixelCntr+d] = (InternalRealType)fixedLocation[d];
                  }
               m_cpuFixedLocationSamples[4*pixelCntr+3] = (InternalRealType)1.0;
               pixelCntr++;
               }
            }
         }
      }
   else
      {
      typename ImageSamplerType::Pointer imageSampler = 0;
      typename SampleContainerType::Pointer imageSampleContainer = SampleContainerType::New();
      SampleType imageSample;
      typename FixedImageType::RegionType bufferedRegion = m_FixedImage->GetBufferedRegion();
      typename FixedImageType::IndexType imageIndex;

      if ( m_SamplingStrategy == RANDOM )
         {
         typename RandomImageSamplerType::Pointer temporaryImageSampler = RandomImageSamplerType::New();
         temporaryImageSampler->SetNumberOfSamples( m_NumberOfPixels );
         imageSampler = temporaryImageSampler;
         }
      else if ( m_SamplingStrategy == GRID )
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
      catch (itk::ExceptionObject & err)
         {
         std::cerr << err << std::endl;
         std::cerr << "Cannot grid sample the image" << std::endl;
         }

      imageSampleContainer = imageSampler->GetOutput();

      unsigned int pixelCntr = 0;
      for (unsigned int i = 0; i < imageSampleContainer->Size(); ++i)
         {
         if (imageSampleContainer->GetElementIfIndexExists( i, &imageSample ) )
            {
            m_FixedImage->TransformPhysicalPointToIndex( imageSample.m_ImageCoordinates, imageIndex);
            if (bufferedRegion.IsInside(imageIndex))
               {
               unsigned int idx = static_cast<unsigned int>(m_FixedImage->ComputeOffset(imageIndex));
               if(!m_ComputeMask || (m_ComputeMask && (cpuFixedGradientBuffer[idx*4 + 3] > (InternalRealType)0.0)))
                  {
                  InternalRealType magnitudeValue = 0.0;
                  for (unsigned int d = 0; d < FixedImageDimension; ++d)
                     {
                     magnitudeValue += pow( cpuFixedGradientBuffer[idx*4 + d] /  kernelNorms[d], 2.0);
                     }
                  magnitudeValue = sqrt(magnitudeValue);
                  if(magnitudeValue > magnitudeThreshold)
                     {
                     typename FixedImageType::PointType fixedLocation;
                     this->m_FixedImage->TransformIndexToPhysicalPoint(m_FixedImage->ComputeIndex(idx), fixedLocation);
                     for (unsigned int d = 0; d < FixedImageDimension; ++d)
                        {
                        m_cpuFixedGradientSamples[4*pixelCntr+d] = cpuFixedGradientBuffer[idx*4 + d];
                        m_cpuFixedLocationSamples[4*pixelCntr+d] = (InternalRealType)fixedLocation[d];
                        }
                     m_cpuFixedLocationSamples[4*pixelCntr+3] = (InternalRealType)1.0;
                     pixelCntr++;
                     }
                  }
               }
            }
         }
      }

  clock.Stop();
  if(m_Debug)
    std::cerr << "Post-Processing Fixed Image Gradient took:\t" << clock.GetMean() << std::endl;

  clReleaseMemObject(m_FixedImageGradientGPUBuffer);
  clReleaseMemObject(m_FixedImageGPUBuffer);

  for (int d = 0; d < FixedImageDimension; ++d)
  {
    clReleaseMemObject(m_GPUDerivOperatorBuffers[d]);
  }  
  delete[] cpuFixedGradientBuffer;
}

/**
 * Compute Gradients of Fixed and Moving Image
 */
template< class TFixedImage, class TMovingImage >
void
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::ComputeMovingImageGradient(void)
{
  if( !m_MovingImage)
  {
    itkExceptionMacro(<< "Moving Image is not set" );
  }
  if(m_Debug)
    std::cout << "Computing Moving Image Gradient" << std::endl;
  /*Create Moving Image Buffer */

  unsigned int nbrOfPixelsInMovingImage = m_MovingImage->GetBufferedRegion().GetNumberOfPixels();

  cl_int errid;

  m_MovingImageGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(MovingImagePixelType)*nbrOfPixelsInMovingImage,
                          m_MovingImage->GetBufferPointer(), &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  int imgSize[3];
  imgSize[0] = m_MovingImage->GetLargestPossibleRegion().GetSize()[0];
  imgSize[1] = m_MovingImage->GetLargestPossibleRegion().GetSize()[1];
  imgSize[2] = m_MovingImage->GetLargestPossibleRegion().GetSize()[2];

  m_MovingImageGradientGPUBuffer = clCreateBuffer(m_Context, CL_MEM_WRITE_ONLY , 
                          4 *  sizeof(MovingImagePixelType) * nbrOfPixelsInMovingImage,
                          nullptr, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);      


  /* Create Gauss Derivative Operator and Populate GPU Buffer */
  std::vector<MovingDerivativeOperatorType> opers;
  opers.resize(MovingImageDimension);
  m_GPUDerivOperatorBuffers.resize(MovingImageDimension);

  std::vector<InternalRealType> kernelNorms;
  kernelNorms.resize(MovingImageDimension);
  for( int dim=0; dim < MovingImageDimension; dim++ )
    {
    // Set up the operator for this dimension      
    opers[dim].SetDirection(dim);
    opers[dim].SetOrder(1);
    // convert the variance from physical units to pixels
    double s = m_MovingImage->GetSpacing()[dim];
    s = s*s;
    opers[dim].SetVariance(m_GradientScale / s);
    
    opers[dim].CreateDirectional();  

    unsigned int numberOfElements = 1;  
      for( int j=0; j < MovingImageDimension; j++ )
      {
        numberOfElements *= opers[dim].GetSize()[j];
      }

    kernelNorms[dim] = 0;
    for(unsigned int k = 0; k < opers[dim].GetSize(0); k++)
      {
      kernelNorms[dim] += pow(opers[dim].GetElement(k),2);
      }
    kernelNorms[dim] = sqrt(kernelNorms[dim]);      

    m_GPUDerivOperatorBuffers[dim] = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          sizeof(InternalRealType)*numberOfElements,
                          (InternalRealType *)opers[dim].Begin(), &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  
  }

  if(m_ComputeMask)
    {
      clReleaseKernel(m_GradientKernel);
      m_GradientKernel = CreateKernelFromString(GPUDiscreteGaussianGradientImageFilter,
                                                          "#define DIM_3\n", "SeparableNeighborOperatorFilter","");

    }


  int radius[3];  
  radius[0] = radius[1] = radius[2] = 0;
  for(int i=0; i<MovingImageDimension; i++)
    {
    radius[i]  = opers[i].GetRadius(i);
    }

  size_t localSize[3], globalSize[3];
  localSize[0] = localSize[1] = localSize[2] = OpenCLGetLocalBlockSize(MovingImageDimension);
  for(int i=0; i<MovingImageDimension; i++)
    {
    globalSize[i] = localSize[i]*(unsigned int)ceil( (float)imgSize[i]/(float)localSize[i]);
    }

  int argidx = 0;
  clSetKernelArg(m_GradientKernel, argidx++, sizeof(cl_mem), (void *)&m_MovingImageGPUBuffer);
  clSetKernelArg(m_GradientKernel, argidx++, sizeof(cl_mem), (void *)&m_MovingImageGradientGPUBuffer);

  for(int i=0; i<MovingImageDimension; i++)
    {
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(cl_mem), (void *)&m_GPUDerivOperatorBuffers[i]);
    }  

  for(int i=0; i<MovingImageDimension; i++)
    {
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(int),  &(radius[i]));
    }

  for(int i=0; i<MovingImageDimension; i++)
    {
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(int),  &(imgSize[i]) );
    }

  InternalRealType spacing[3];
  for(int i=0; i<MovingImageDimension; i++)
    {
    spacing[i] = m_MovingImage->GetSpacing()[ i ];
    clSetKernelArg(m_GradientKernel, argidx++, sizeof(int),  &(spacing[i]) );
    } 

  // create N-D range object with work-item dimensions and execute kernel
  clEnqueueNDRangeKernel(m_CommandQueue[0], m_GradientKernel, MovingImageDimension, nullptr, globalSize, localSize, 0, nullptr, nullptr);
  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  cl_image_format gpu_gradient_image_format;
  gpu_gradient_image_format.image_channel_order = CL_RGBA;
  gpu_gradient_image_format.image_channel_data_type = CL_FLOAT;

  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  desc.image_width = imgSize[0];
  desc.image_height = imgSize[1];
  desc.image_depth = imgSize[2];
  desc.image_array_size = 0;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;

  m_cpuMovingGradientImageBuffer = nullptr;
  m_MovingImageGradientGPUImage = clCreateImage(m_Context,
                                  CL_MEM_READ_ONLY, &(gpu_gradient_image_format), &desc,
                                  m_cpuMovingGradientImageBuffer, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = { (size_t)(imgSize[0]), (size_t)(imgSize[1]), (size_t)(imgSize[2])};
  errid = clEnqueueCopyBufferToImage(m_CommandQueue[0], m_MovingImageGradientGPUBuffer, m_MovingImageGradientGPUImage,
                                     0, origin, region, 0, nullptr, nullptr );
  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);


  clReleaseKernel(m_GradientKernel);
  clReleaseMemObject(m_MovingImageGradientGPUBuffer);
  clReleaseMemObject(m_MovingImageGPUBuffer);
  for (int d = 0; d < MovingImageDimension; ++d)
  {
    clReleaseMemObject(m_GPUDerivOperatorBuffers[d]);
  }

  this->CreateGPUVariablesForCostFunction();

  /* Build Orientation Matching Kernel */
  std::ostringstream defines2;
  defines2 << "#define SEL " << m_N << std::endl;
  defines2 << "#define N " << m_Blocks * m_Threads << std::endl;
  defines2 << "#define LOCALSIZE " << m_Threads << std::endl;

  m_OrientationMatchingKernel = CreateKernelFromString( GPUOrientationMatchingMatrixTransformationSparseMaskKernel,  
    defines2.str().c_str(), "OrientationMatchingMetricSparseMask","");  
}

/**
 * Update Metric Value
 */
template< class TFixedImage, class TMovingImage >
void
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::CreateGPUVariablesForCostFunction(void)
{
  MovingImageDirectionType movingIndexToLocation = m_MovingImage->GetDirection();

  MovingImageDirectionType scale;       
 
  for ( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
      scale[i][i]= m_MovingImage->GetSpacing()[i];
    }      
  movingIndexToLocation = movingIndexToLocation*scale;

  MovingImageDirectionType movingLocationToIndex = 
                    MovingImageDirectionType(movingIndexToLocation.GetInverse());
  MovingImagePointType movingOrigin = m_MovingImage->GetOrigin();  

  for ( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
    m_mOrigin[i] = movingOrigin[i];
    for ( unsigned int j = 0; j < MovingImageDimension; j++ )
      {    
      m_locToIdx[i][j] = movingLocationToIndex[i][j];
      }          
    }     

  cl_int errid;
  unsigned int dummySize = 24;
  m_cpuDummy = (InternalRealType*)malloc( dummySize* sizeof(InternalRealType));
  memset(m_cpuDummy, 0, dummySize * sizeof(InternalRealType));
  m_gpuDummy = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          dummySize * sizeof(InternalRealType),
                          m_cpuDummy, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

  m_gpuMetricAccum = clCreateBuffer(m_Context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 
                          m_Blocks * sizeof(InternalRealType),
                          nullptr, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    


  m_gpuFixedGradientSamples = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          m_Blocks * m_Threads * 4 * sizeof(InternalRealType),
                          m_cpuFixedGradientSamples, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

  m_gpuFixedLocationSamples = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          m_Blocks * m_Threads * 4 * sizeof(InternalRealType),
                          m_cpuFixedLocationSamples, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  
}


/**
 * Update Metric Value
 */
template< class TFixedImage, class TMovingImage >
void
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::UpdateGPUTransformVariables(void)
{
    m_TransformMatrix = m_Transform->GetMatrix();
    m_TransformOffset = m_Transform->GetOffset();

    m_matrixDummy = m_locToIdx * m_TransformMatrix.GetVnlMatrix();
    m_matrixTranspose = m_TransformMatrix.GetVnlMatrix().transpose();
    m_OffsetOrigin = m_TransformOffset.GetVnlVector() - m_mOrigin;
    m_vectorDummy = m_locToIdx * m_OffsetOrigin;
    for ( unsigned int i = 0; i < MovingImageDimension; i++ )
       {
       for (unsigned int j=0; j < MovingImageDimension; j++)
          {
          m_cpuDummy[4*i+j] = m_matrixDummy[i][j];
          m_cpuDummy[4*i+j+12] = m_matrixTranspose[i][j];
          }
       m_cpuDummy[4*i+3] = m_vectorDummy[i];
       }

    cl_int errid;
    errid = clEnqueueWriteBuffer(m_CommandQueue[0], m_gpuDummy, CL_TRUE, 0,
    24 * sizeof(InternalRealType), m_cpuDummy, 0, nullptr, nullptr);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
}

/**
 * Update Metric Value
 */
template< class TFixedImage, class TMovingImage >
void
GPUOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::Update(void)
{

    if(!m_Transform)
    {
    itkExceptionMacro(<< "Transform is not set." );
    }

    if(m_UseFixedImageMask)
    {
    if(!m_FixedImageMaskSpatialObject)
       {
       itkWarningMacro(<< "m_FixedImageMaskSpatialObject was not found, UseImageMask is set to OFF" );
       m_UseFixedImageMask = false;
       }
    //m_ComputeMask = true;
    }

    if(!m_MovingImageGradientGPUImage)
    {
       if(m_Debug)
          std::cout << "Preparing to Compute Gradients.." << std::endl;
       this->ComputeFixedImageGradient();
       this->ComputeMovingImageGradient();
    }

    if(m_TransformMatrix == m_Transform->GetMatrix() && m_TransformOffset == m_TransformOffset)
       {
       return;
       }

    this->UpdateGPUTransformVariables();

    size_t globalSize[1];
    size_t localSize[1];

    globalSize[0] = m_Blocks * m_Threads;
    localSize[0] = m_Threads;

    int argidx = 0;

    clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(cl_mem), (void *)&m_gpuDummy);
    clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(cl_mem), (void *)&m_gpuFixedGradientSamples);
    clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(cl_mem), (void *)&m_gpuFixedLocationSamples);
    clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(cl_mem), (void *)&m_MovingImageGradientGPUImage);
    clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(cl_mem), (void *)&m_gpuMetricAccum);
    clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(InternalRealType) * m_Threads, nullptr);

    clEnqueueNDRangeKernel(m_CommandQueue[0], m_OrientationMatchingKernel, 1, nullptr, globalSize, localSize, 0, nullptr, nullptr);

    cl_int errid;
    errid = clFinish(m_CommandQueue[0]);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    m_cpuMetricAccum = (InternalRealType *)clEnqueueMapBuffer( m_CommandQueue[0], m_gpuMetricAccum, CL_TRUE,
                                  CL_MAP_READ, 0, m_Blocks * sizeof(InternalRealType),
                                  0, nullptr, nullptr, &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);


    InternalRealType metricSum = 0;
    for(unsigned int i=0; i<m_Blocks; i++)
        {
        metricSum += m_cpuMetricAccum[i];
        }

    m_MetricValue = 0;
    if( metricSum >0 )
        m_MetricValue = (InternalRealType) (metricSum/((InternalRealType)globalSize[0]));
  
}


} // end namespace itk

#endif
