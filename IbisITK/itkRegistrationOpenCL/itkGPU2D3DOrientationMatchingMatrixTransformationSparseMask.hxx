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

#ifndef __itkGPU2D3DOrientationMatchingMatrixTransformationSparseMask_hxx
#define __itkGPU2D3DOrientationMatchingMatrixTransformationSparseMask_hxx

#include "itkMacro.h"
#include "itkGPU2D3DOrientationMatchingMatrixTransformationSparseMask.h"
#include "itkTimeProbe.h"
#include "itkMatrix.h"
#include "vnl/vnl_matrix.h"
#include "itkGaussianDerivativeOperator.h"
#include "itkOpenCLUtil.h"

#include "GPUDiscreteGaussianGradientImageFilter.h"
#include "GPU2D3DOrientationMatchingMatrixTransformationSparseMaskKernel.h"


namespace itk
{
/**
 * Default constructor
 */
template< class TFixedImage, class TMovingImage >
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::GPU2D3DOrientationMatchingMatrixTransformationSparseMask()
{

  if(!itk::IsGPUAvailable())
  {
    itkExceptionMacro(<< "OpenCL-enabled GPU is not present.");
  }

  /* Initialize GPU Context */
  this->InitializeGPUContext();

  m_Percentile = 0.9;
  m_N = 2;

  m_GradientScale = 1.0;
  m_FixedGradientScale = 1.0;  
  m_MovingGradientScale = 1.0;

  m_MaskThreshold = 0.0;

  m_MovingImage = NULL;

  m_MovingImageGradientGPUImage = NULL;

  m_Transform = NULL;
  m_MetricValue = 0;

  m_NumberOfSlices = 0;  

  m_FixedSliceMaskWithoutBoundaries = NULL;

  m_Center.Fill(0.0);

  m_Debug = false;

}


template< class TFixedImage, class TMovingImage >
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::~GPU2D3DOrientationMatchingMatrixTransformationSparseMask()
{
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
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::InitializeGPUContext(void)
{
  cl_int errid;

  // Get the platforms
  errid = clGetPlatformIDs(0, NULL, &m_NumberOfPlatforms);
  OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

  // Get NVIDIA platform by default
  m_Platform = OpenCLSelectPlatform("NVIDIA");
  assert(m_Platform != NULL);

  cl_device_type devType = CL_DEVICE_TYPE_GPU;
  m_Devices = OpenCLGetAvailableDevices(m_Platform, devType, &m_NumberOfDevices);

  // create context
  m_Context = clCreateContext(0, m_NumberOfDevices, m_Devices, NULL, NULL, &errid);
  OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );

  // create command queues
  m_CommandQueue = (cl_command_queue *)malloc(m_NumberOfDevices * sizeof(cl_command_queue) );
  for(unsigned int i=0; i<m_NumberOfDevices; i++)
    {
    m_CommandQueue[i] = clCreateCommandQueueWithProperties(m_Context, m_Devices[i], 0, &errid);
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
    }


  // Build All Necessary Kernels (Except Orientation Matching Kernel)
  m_2DGradientKernel = CreateKernelFromString(GPUDiscreteGaussianGradientImageFilter,
                                                      "#define DIM_2\n","SeparableNeighborOperatorFilter", "", &m_2DGradientProgram);    

  m_2DAllGradientKernel = CreateKernelFromString(GPUDiscreteGaussianGradientImageFilter,
                                                      "#define DIM_2_ALL\n", "SeparableNeighborOperatorFilter", "", &m_2DAllGradientProgram);    

  m_3DGradientKernel = CreateKernelFromString(GPUDiscreteGaussianGradientImageFilter,
                                                          "#define DIM_3\n", "SeparableNeighborOperatorFilter","", &m_3DGradientProgram);


}

/**
 * Standard "PrintSelf" method.
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template< class TFixedImage, class TMovingImage >
unsigned int
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
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
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::CreateKernelFromFile(const char * filename, const char * cPreamble, const char * kernelname, const char * cOptions)
{

  FILE*  pFileStream = NULL;
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

  cl_program program;
  cl_kernel kernel = CreateKernelFromString(OriginalSourceString, cPreamble, kernelname, cOptions, &program);

}


template< class TFixedImage, class TMovingImage >
cl_kernel
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::CreateKernelFromString(const char * cOriginalSourceString, const char * cPreamble, const char * kernelname, const char * cOptions, cl_program * program)
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

  *program = clCreateProgramWithSource( m_Context, 1, (const char **)&cSourceString, &szFinalLength, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  errid = clBuildProgram(*program, 0, NULL, cOptions, NULL, NULL);
  if(errid != CL_SUCCESS)
    {
    size_t paramValueSize = 0;
    clGetProgramBuildInfo(m_Program, 0, CL_PROGRAM_BUILD_LOG, 0, NULL, &paramValueSize);
    char *paramValue;
    paramValue = new char[paramValueSize+1];
    clGetProgramBuildInfo(m_Program, 0, CL_PROGRAM_BUILD_LOG, paramValueSize, paramValue, NULL);
    paramValue[paramValueSize] = '\0';
    std::cerr << paramValue << std::endl;
    free( paramValue );
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
    itkExceptionMacro(<<"Cannot Build Program");
    }

  free(cSourceString);
  cl_kernel kernel = clCreateKernel(*program, kernelname, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

  return kernel;
}

template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::SetNumberOfSlices( unsigned int numberOfSlices )
{
  if(m_NumberOfSlices != numberOfSlices)
  {
    m_NumberOfSlices = numberOfSlices;
    m_FixedSlices.resize( m_NumberOfSlices );
  }

  for(unsigned int i=0; i<m_NumberOfSlices; i++)
  {
    m_FixedSlices[i] = NULL;
  }

}

template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::SetFixedSlice( unsigned int sliceIdx, FixedImagePointer sliceImage )
{

  m_FixedSlices[sliceIdx] = sliceImage;
}

template< class TFixedImage, class TMovingImage >
bool
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::CheckAllSlicesDefined( void )
{
  bool allSlicesDefined = true;
  for(unsigned int i=0; i<m_NumberOfSlices; i++)
  {
    if( !m_FixedSlices[i] )
    {
      allSlicesDefined = false;
    }
  }
  return allSlicesDefined;
}


/**
 * Compute Gradients of Fixed and Moving Image
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::ComputeSliceMaskWithoutBoundaries(void)
{
  if(!m_FixedSliceMask)
  {
    itkExceptionMacro(<< "Slice Mask has not been set." );
  }
  m_FixedSliceMask->Update();
  m_NbrPixelsInSlice = m_FixedSliceMask->GetBufferedRegion().GetNumberOfPixels();  

  InternalRealType * imageGradients = NULL;
  Compute2DImageGradient(m_FixedSliceMask, &imageGradients);

  m_SliceValidIdxs.resize(0);

  for(unsigned int i=0; i<m_NbrPixelsInSlice; i++)
    {
    InternalRealType magnitudeValue = 0;
    for (unsigned int d = 0; d < 2; ++d)
      {
      magnitudeValue += pow( imageGradients[i*2 + d], 2.0);
      }
     magnitudeValue = sqrt(magnitudeValue);

    if(m_FixedSliceMask->GetPixel(m_FixedSliceMask->ComputeIndex(i)) > 0.0 && magnitudeValue <= 0.001)
     {
      m_SliceValidIdxs.push_back(i);
     }

    }     
  free(imageGradients);

}

/**
 * Compute Gradients of Fixed and Moving Image
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::Compute2DImageGradient( FixedImagePointer sliceImage, InternalRealType ** imageGradients) 
//::Compute2DImageGradient( FixedImagePointer sliceImage, std::vector<SliceGradientType>& vecGradients);
{

  /* Create Gauss Derivative Operator and Populate GPU Buffer */
  std::vector<FixedDerivativeOperatorType> opers;
  opers.resize(2);

  std::vector<cl_mem> gpuOpers;
  gpuOpers.resize(2);

  std::vector<InternalRealType> kernelNorms;
  kernelNorms.resize(2);

  cl_int errid;

  for( int dim=0; dim < 2; dim++ )
    {
    // Set up the operator for this dimension      
    opers[dim].SetDirection(dim);
    opers[dim].SetOrder(1);
    // convert the variance from physical units to pixels
    double s = sliceImage->GetSpacing()[dim];
    s = s*s;
    //opers[dim].SetVariance(m_GradientScale / s);
    opers[dim].SetVariance(m_FixedGradientScale / s);
    
    opers[dim].SetMaximumKernelWidth(15);
    opers[dim].CreateDirectional();  

    unsigned int numberOfElements = 1;  
      for( int j=0; j < 2; j++ )
      {
        numberOfElements *= opers[dim].GetSize()[j];
      }

    kernelNorms[dim] = 0;
    //for(unsigned int k = 0; k < opers[dim].GetSize(0); k++)
    for(unsigned int k = 0; k < opers[dim].GetSize(dim); k++)
      {
      kernelNorms[dim] += pow(opers[dim].GetElement(k),2);
      }
    kernelNorms[dim] = sqrt(kernelNorms[dim]);      

    gpuOpers[dim] = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          sizeof(InternalRealType)*numberOfElements,
                          (InternalRealType *)opers[dim].Begin(), &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  
  }

  int radius[2];
  radius[0] = radius[1] = 0;

  for(int i=0; i<2; i++)
    {
    radius[i]  = opers[i].GetRadius(i);
    }

  size_t localSize[2], globalSize[2];
  localSize[0] = localSize[1] = OpenCLGetLocalBlockSize(2);

  if(localSize[0] == 0)
  {
    itkExceptionMacro(<<"localSize == 0");
  }
                                                                                        
  int imgSize[2];
  imgSize[0] = sliceImage->GetLargestPossibleRegion().GetSize()[0];
  imgSize[1] = sliceImage->GetLargestPossibleRegion().GetSize()[1];

  for(unsigned int i=0; i<2; i++)
    {
    globalSize[i] = localSize[i]*(unsigned int)ceil( (float)imgSize[i]/(float)localSize[i]);
    }

  unsigned int nbrOfPixelsInFixedImage = sliceImage->GetBufferedRegion().GetNumberOfPixels();    

  cl_mem inputImageGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(FixedImagePixelType)*nbrOfPixelsInFixedImage,
                          (void *)sliceImage->GetBufferPointer(), &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
  
  *imageGradients = (InternalRealType*)malloc(2 *
                                  nbrOfPixelsInFixedImage * sizeof(InternalRealType));
  memset(*imageGradients, (InternalRealType)0, 2 * nbrOfPixelsInFixedImage * sizeof(InternalRealType) );

  cl_mem imageGradientGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                          2 * sizeof(FixedImagePixelType) * nbrOfPixelsInFixedImage,
                          *imageGradients, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);      

  int argidx = 0;
  errid = clSetKernelArg(m_2DGradientKernel, argidx++, sizeof(cl_mem), (void *)&inputImageGPUBuffer);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  errid = clSetKernelArg(m_2DGradientKernel, argidx++, sizeof(cl_mem), (void *)&imageGradientGPUBuffer);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  for(unsigned int i=0; i<2; i++)
    {
    errid = clSetKernelArg(m_2DGradientKernel, argidx++, sizeof(cl_mem), (void *)&gpuOpers[i]);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
    }  

  for(unsigned int i=0; i<2; i++)
    {
    errid = clSetKernelArg(m_2DGradientKernel, argidx++, sizeof(int),  &(radius[i]));
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
    }

  for(unsigned int i=0; i<2; i++)
    {
    errid = clSetKernelArg(m_2DGradientKernel, argidx++, sizeof(int),  &(imgSize[i]) );
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
    }

  InternalRealType spacing[2];
  for(unsigned int i=0; i<2; i++)
    {
    spacing[i] = sliceImage->GetSpacing()[ i ];
    errid = clSetKernelArg(m_2DGradientKernel, argidx++, sizeof(int),  &(spacing[i]) );
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
    } 

  errid = clEnqueueNDRangeKernel(m_CommandQueue[0], m_2DGradientKernel, 2, NULL, globalSize, localSize, 0, NULL, NULL);

  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  errid = clEnqueueReadBuffer(m_CommandQueue[0], imageGradientGPUBuffer, CL_TRUE, 0,
    nbrOfPixelsInFixedImage * 2 * sizeof(InternalRealType), *imageGradients, 0, NULL, NULL);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

  clReleaseMemObject(imageGradientGPUBuffer);
  clReleaseMemObject(inputImageGPUBuffer);

  for (int d = 0; d < 2; ++d)
  {
    clReleaseMemObject(gpuOpers[d]);
  }  

}


/**
 * Compute Gradients of Fixed and Moving Image
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::ComputeFixedImageGradient(void)
{

  if( !CheckAllSlicesDefined() )
  {
    itkExceptionMacro(<< "All Fixed Slices have not been set." );
  }

  if( !m_FixedSliceMask )
  {
    itkExceptionMacro(<< "Slice Mask has not been set." );
  }
  m_FixedSliceMask->Update();

  ComputeSliceMaskWithoutBoundaries();


  itk::TimeProbe clockGradientKernel;
  clockGradientKernel.Start();

  cl_ulong global_mem_size;
  clGetDeviceInfo(m_Devices[0] , CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);

  cl_ulong gpu512mb = 536900000;

  unsigned int maxNbrOfSlices = round((float)global_mem_size / (float)gpu512mb) * 64;

  unsigned int size_slice = m_NbrPixelsInSlice * sizeof(InternalRealType);
  unsigned int sliceCntr = 0; // Counts the number of slices that have been processed.

  /* Create Gauss Derivative Operator and Populate GPU Buffer */
  std::vector<FixedDerivativeOperatorType> opers;
  opers.resize(2);

  std::vector<cl_mem> gpuOpers;
  gpuOpers.resize(2);

  std::vector<InternalRealType> kernelNorms;
  kernelNorms.resize(2);

  cl_int errid;

  for( int dim=0; dim < 2; dim++ )
    {
    // Set up the operator for this dimension
    opers[dim].SetDirection(dim);
    opers[dim].SetOrder(1);
    // convert the variance from physical units to pixels
    double s = m_FixedSliceMask->GetSpacing()[dim];
    s = s*s;
    //opers[dim].SetVariance(m_GradientScale / s);
    opers[dim].SetVariance(m_FixedGradientScale / s);

    opers[dim].SetMaximumKernelWidth(15);
    opers[dim].CreateDirectional();

    unsigned int numberOfElements = 1;
      for( int j=0; j < 2; j++ )
      {
        numberOfElements *= opers[dim].GetSize()[j];
      }

    kernelNorms[dim] = 0;
    //for(unsigned int k = 0; k < opers[dim].GetSize(0); k++)
    for(unsigned int k = 0; k < opers[dim].GetSize(dim); k++)
      {
      kernelNorms[dim] += pow(opers[dim].GetElement(k),2);
      }
    kernelNorms[dim] = sqrt(kernelNorms[dim]);

    gpuOpers[dim] = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(InternalRealType)*numberOfElements,
                          (InternalRealType *)opers[dim].Begin(), &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
  }

  int radius[2];
  radius[0] = radius[1] = 0;

  for(int i=0; i<2; i++)
    {
    radius[i]  = opers[i].GetRadius(i);
    }

  size_t localSize[3], globalSize[3];
  localSize[0] = localSize[1] = localSize[2] = OpenCLGetLocalBlockSize(3);

  InternalRealType * allImageGradients = new InternalRealType[2* m_NumberOfSlices *m_NbrPixelsInSlice];

  do
  {
    unsigned int nbrOfSlicesToProcess = std::min(m_NumberOfSlices - sliceCntr, maxNbrOfSlices);
    unsigned int size_input = nbrOfSlicesToProcess * size_slice;
    unsigned int size_output = 2 * size_input;

    FixedImagePixelType * allSlicesPixels = new FixedImagePixelType[nbrOfSlicesToProcess*m_NbrPixelsInSlice];

    for(unsigned int sliceIdx=sliceCntr; sliceIdx<sliceCntr+nbrOfSlicesToProcess ; sliceIdx++)
    {
      FixedImagePointer sliceImage = m_FixedSlices[sliceIdx];
      sliceImage->Update();

      memcpy((void *)&allSlicesPixels[(sliceIdx-sliceCntr)*m_NbrPixelsInSlice],
          (void *)sliceImage->GetBufferPointer(), size_slice );

    }

    int imgSize[3];
    imgSize[0] = m_FixedSliceMask->GetLargestPossibleRegion().GetSize()[0];
    imgSize[1] = m_FixedSliceMask->GetLargestPossibleRegion().GetSize()[1];
    imgSize[2] = nbrOfSlicesToProcess;

    for(unsigned int i=0; i<3; i++)
      {
      globalSize[i] = localSize[i]*(unsigned int)ceil( (float)imgSize[i]/(float)localSize[i]);
      }

    cl_mem inputImageGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            size_input, (void *)allSlicesPixels, &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    cl_mem imageGradientGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_WRITE,
                            size_output, NULL, &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    int argidx = 0;
    errid = clSetKernelArg(m_2DAllGradientKernel, argidx++, sizeof(cl_mem), (void *)&inputImageGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    errid = clSetKernelArg(m_2DAllGradientKernel, argidx++, sizeof(cl_mem), (void *)&imageGradientGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    for(unsigned int i=0; i<2; i++)
      {
      errid = clSetKernelArg(m_2DAllGradientKernel, argidx++, sizeof(cl_mem), (void *)&gpuOpers[i]);
      OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
      }

    for(unsigned int i=0; i<2; i++)
      {
      errid = clSetKernelArg(m_2DAllGradientKernel, argidx++, sizeof(int),  &(radius[i]));
      OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
      }

    for(unsigned int i=0; i<3; i++)
      {
      errid = clSetKernelArg(m_2DAllGradientKernel, argidx++, sizeof(int),  &(imgSize[i]) );
      OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
      }

    InternalRealType spacing[2];
    for(unsigned int i=0; i<2; i++)
      {
      spacing[i] = m_FixedSliceMask->GetSpacing()[ i ];
      errid = clSetKernelArg(m_2DAllGradientKernel, argidx++, sizeof(int),  &(spacing[i]) );
      OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
      }


    errid = clEnqueueNDRangeKernel(m_CommandQueue[0], m_2DAllGradientKernel, 3, NULL, globalSize, localSize, 0, NULL, NULL);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);


    errid = clFinish(m_CommandQueue[0]);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    errid = clEnqueueReadBuffer(m_CommandQueue[0], imageGradientGPUBuffer, CL_TRUE, 0,
                                size_output, (void *)&allImageGradients[sliceCntr*2*m_NbrPixelsInSlice],
                                0, NULL, NULL);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    errid = clFinish(m_CommandQueue[0]);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    errid = clReleaseMemObject(inputImageGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    errid = clReleaseMemObject(imageGradientGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    sliceCntr += nbrOfSlicesToProcess;

  }while(sliceCntr < m_NumberOfSlices);


  for (int d = 0; d < 2; ++d)
  {
    errid = clReleaseMemObject(gpuOpers[d]);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
  }

  clockGradientKernel.Stop();
  if(m_Debug)
    std::cout << "Time to 2D gradients of US Slices:\t" << clockGradientKernel.GetMean() << std::endl;

#ifdef DEBUG
  //Output US Slice Gradient Magnitude of First Slice
  unsigned int sliceIdx = 50;

  DuplicatorPointer duplicator = DuplicatorType::New();
  duplicator->SetInputImage(m_FixedSlices[sliceIdx]);
  duplicator->Update();

  FixedImagePointer sliceGradientMagnitudeImage = duplicator->GetOutput();
  sliceGradientMagnitudeImage->FillBuffer(0.0);
  for(unsigned int idx=0; idx < m_SliceValidIdxs.size(); idx++)
  {
    SliceGradientType sliceGradient;

    unsigned int imageIndex = m_SliceValidIdxs[idx];
    sliceGradient[0] = allImageGradients[2*sliceIdx*m_NbrPixelsInSlice +  imageIndex*2 ];
    sliceGradient[1] = allImageGradients[2*sliceIdx*m_NbrPixelsInSlice + imageIndex*2 +1];

    FixedImagePixelType gradientMagnitude; 
    gradientMagnitude = sliceGradient.GetNorm();
    sliceGradientMagnitudeImage->SetPixel( sliceGradientMagnitudeImage->ComputeIndex(imageIndex), gradientMagnitude );
  }

  WriterPointer writer = WriterType::New();
  writer->SetInput(sliceGradientMagnitudeImage);
  writer->SetFileName( "usSlice50GradientMagnitude.mhd" );
  writer->Update();
#endif  

  FixedGradientMagnitudeSampleType::Pointer sample = FixedGradientMagnitudeSampleType::New();

  itk::TimeProbe clock;
  clock.Start();

  unsigned int nbrOfPixelsForHistogram =  100000;

  //if(nbrOfPixelsForHistogram < m_AllValidSliceGradients.size())
  if( nbrOfPixelsForHistogram < m_NumberOfSlices * m_SliceValidIdxs.size() )
    {
    unsigned int pixelCntr = 0;
    while(pixelCntr<nbrOfPixelsForHistogram)
      {
      unsigned int idx = ( rand() % m_SliceValidIdxs.size() );

      unsigned int imageIndex = m_SliceValidIdxs[idx];
      unsigned int sliceIndex = ( rand() % m_NumberOfSlices );

      SliceGradientType sliceGradient;
      sliceGradient[0] = allImageGradients[2*sliceIndex*m_NbrPixelsInSlice +  imageIndex*2 ];
      sliceGradient[1] = allImageGradients[2*sliceIndex*m_NbrPixelsInSlice + imageIndex*2 +1];

      MeasurementVectorType tempSample;

      tempSample[0] = sliceGradient.GetNorm();

      sample->PushBack(tempSample);
      pixelCntr++;
      }
    }
  else
    {

    for(unsigned int sliceIndex = 0; sliceIndex < m_NumberOfSlices; sliceIndex++)
      {
      for(unsigned int idx = 0; idx < m_SliceValidIdxs.size(); idx++)
        {
        unsigned int imageIndex = m_SliceValidIdxs[idx];
        SliceGradientType sliceGradient;
        sliceGradient[0] = allImageGradients[2*sliceIndex*m_NbrPixelsInSlice +  imageIndex*2 ];
        sliceGradient[1] = allImageGradients[2*sliceIndex*m_NbrPixelsInSlice + imageIndex*2 +1];   

        MeasurementVectorType tempSample;
        tempSample[0] = sliceGradient.GetNorm();
        sample->PushBack(tempSample);     
        }        
      }  
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
    std::cout << "Computing histogram took:\t" << clock2.GetMean() << std::endl;
    std::cout << "Magnitude Threshold:\t" << magnitudeThreshold << std::endl;
  }


  unsigned int maxThreads = 256;
  m_Threads = (m_NumberOfPixels < maxThreads*2) ? this->NextPow2((m_NumberOfPixels + 1)/ 2) : maxThreads;
  m_Blocks = (m_NumberOfPixels + m_Threads -1 ) / ( m_Threads );

  //Version C
  m_cpuFixedGradientSamples = (InternalRealType*)malloc(m_Blocks * m_Threads * 2 * sizeof(InternalRealType));
  memset(m_cpuFixedGradientSamples, (InternalRealType)0, 2 * m_Blocks * m_Threads * sizeof(InternalRealType));

  m_cpuFixedLocationSamples = (InternalRealType*)malloc(m_Blocks * m_Threads * 4 * sizeof(InternalRealType));
  memset(m_cpuFixedLocationSamples, (InternalRealType)0, 4 * m_Blocks * m_Threads * sizeof(InternalRealType));

#ifdef DEBUG
  double lowerBounds[3];
  double upperBounds[3];
#endif  

  unsigned int pixelCntr = 0;
  while(pixelCntr < m_NumberOfPixels)
  {

    unsigned int idx = ( rand() % m_SliceValidIdxs.size() );

    unsigned int imageIdx = m_SliceValidIdxs[idx];
    unsigned int sliceIdx = ( rand() % m_NumberOfSlices );

    SliceGradientType sliceGradient;
    sliceGradient[0] = allImageGradients[2*sliceIdx*m_NbrPixelsInSlice +  imageIdx*2 ];
    sliceGradient[1] = allImageGradients[2*sliceIdx*m_NbrPixelsInSlice + imageIdx*2 +1];


    if(sliceGradient.GetNorm() > magnitudeThreshold)
    {
      typename FixedImageType::PointType fixedLocation;
      
      m_FixedSlices[sliceIdx]->TransformIndexToPhysicalPoint(m_FixedSlices[sliceIdx]->ComputeIndex(imageIdx), fixedLocation);  

#ifdef DEBUG  
      if( pixelCntr == 0 )
      {
        lowerBounds[0] = fixedLocation[0]; upperBounds[0] = fixedLocation[0];
        lowerBounds[1] = fixedLocation[1]; upperBounds[1] = fixedLocation[1];
        lowerBounds[2] = fixedLocation[2]; upperBounds[2] = fixedLocation[2];
      }
      else
      {
        if( fixedLocation[0] < lowerBounds[0] )
          lowerBounds[0] = fixedLocation[0];

        if( fixedLocation[1] < lowerBounds[1] )
          lowerBounds[1] = fixedLocation[1];

        if( fixedLocation[2] < lowerBounds[2] )
          lowerBounds[2] = fixedLocation[2];


        if( fixedLocation[0] > upperBounds[0] )
          upperBounds[0] = fixedLocation[0];

        if( fixedLocation[1] > upperBounds[1] )
          upperBounds[1] = fixedLocation[1];

        if( fixedLocation[2] > upperBounds[2] )
          upperBounds[2] = fixedLocation[2];        

      }

      m_SampleLocations.push_back(fixedLocation);
#endif    

      //Version C
      m_cpuFixedLocationSamples[4*pixelCntr+0] = (InternalRealType)fixedLocation[0];
      m_cpuFixedLocationSamples[4*pixelCntr+1] = (InternalRealType)fixedLocation[1];
      m_cpuFixedLocationSamples[4*pixelCntr+2] = (InternalRealType)fixedLocation[2];
      m_cpuFixedLocationSamples[4*pixelCntr+3] = (InternalRealType)sliceIdx;

      m_cpuFixedGradientSamples[2*pixelCntr+0] = sliceGradient[0];
      m_cpuFixedGradientSamples[2*pixelCntr+1] = sliceGradient[1];   

      pixelCntr++;
    }
  }

#ifdef DEBUG
  FixedImagePointer samplesImage = FixedImageType::New();

  typename FixedImageType::PointType origin;
  origin[0] = lowerBounds[0]; // first index on X
  origin[1] = lowerBounds[1]; // first index on Y
  origin[2] = lowerBounds[2]; // first index on Z  

  typename FixedImageType::IndexType start;
  start[0] = 0; // first index on X
  start[1] = 0; // first index on Y
  start[2] = 0; // first index on Z

  typename FixedImageType::SpacingType spacing;
  spacing[0] = 0.5; // first index on X
  spacing[1] = 0.5; // first index on Y
  spacing[2] = 0.5; // first index on Z  

  typename FixedImageType::SizeType size;
  size[0] = ceil(upperBounds[0] - lowerBounds[0])/spacing[0]; // size along X
  size[1] = ceil(upperBounds[1] - lowerBounds[1])/spacing[1]; // size along Y
  size[2] = ceil(upperBounds[2] - lowerBounds[2])/spacing[2]; // size along Z

  typename FixedImageType::RegionType region;
  region.SetSize( size );
  region.SetIndex( start );

  samplesImage->SetRegions( region );
  samplesImage->SetOrigin( origin );
  samplesImage->SetSpacing( spacing );
  samplesImage->Allocate();
  samplesImage->FillBuffer(0.0);

  std::cerr << "Setting Pixels..." << std::endl;

  for(unsigned int n=0; n < m_SampleLocations.size(); n++)
  {
    typename FixedImageType::IndexType fixedIndex;
    samplesImage->TransformPhysicalPointToIndex( m_SampleLocations[n], fixedIndex );
    samplesImage->SetPixel(fixedIndex, 1.0);
  }

  std::cerr << "Setting Pixels...DONE" << std::endl;

  std::cerr << "Writing Samples Image..." << std::endl;
  WriterPointer samplesImageWriter = WriterType::New();
  samplesImageWriter->SetInput( samplesImage );
  samplesImageWriter->SetFileName( "selectedLocations.mhd" );
  samplesImageWriter->Update();
  std::cerr << "Writing Samples Image...DONE" << std::endl;
#endif  

  m_cpuFixedGradientBase = (InternalRealType*)malloc(m_NumberOfSlices * 6 * sizeof(InternalRealType));
  memset(m_cpuFixedGradientBase, (InternalRealType)0, m_NumberOfSlices * 6 * sizeof(InternalRealType));
  //Version C
  for(unsigned int sliceIdx = 0; sliceIdx < m_NumberOfSlices; sliceIdx++)
  {
    FixedGradientType dummyGradient, base1, base2;

    dummyGradient[0] = 1.0; dummyGradient[1] = 0.0; dummyGradient[2] = 0.0;
    m_FixedSlices[sliceIdx]->TransformLocalVectorToPhysicalVector(dummyGradient, base1);

    dummyGradient[0] = 0.0; dummyGradient[1] = 1.0; dummyGradient[2] = 0.0;
    m_FixedSlices[sliceIdx]->TransformLocalVectorToPhysicalVector(dummyGradient, base2);

    m_cpuFixedGradientBase[6*sliceIdx+0] = base1[0];
    m_cpuFixedGradientBase[6*sliceIdx+1] = base1[1];
    m_cpuFixedGradientBase[6*sliceIdx+2] = base1[2];
    m_cpuFixedGradientBase[6*sliceIdx+3] = base2[0];
    m_cpuFixedGradientBase[6*sliceIdx+4] = base2[1];
    m_cpuFixedGradientBase[6*sliceIdx+5] = base2[2];

#ifdef DEBUG
    //std::cerr << "Fixed Gradient Bases for slice " << sliceIdx << std::endl;
    //std::cerr << base1 << std::endl;
    //std::cerr << base2 << std::endl << std::endl;
#endif    

  }

  delete[] allImageGradients;
  clock.Stop();
  if(m_Debug)
    std::cout << "Random selection of "<< m_NumberOfPixels << " voxels took :\t" << clock.GetMean() << std::endl;

}

/**
 * Compute Gradients of Fixed and Moving Image
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::ComputeMovingImageGradient(void)
{
  if( !m_MovingImage)
  {
    itkExceptionMacro(<< "Moving Image is not set" );
  }

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
                          NULL, &errid);
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
    //opers[dim].SetVariance(m_GradientScale / s);
    opers[dim].SetVariance(m_MovingGradientScale / s);
    
    opers[dim].SetMaximumKernelWidth(9);
    opers[dim].CreateDirectional();  

    unsigned int numberOfElements = 1;  
      for( int j=0; j < MovingImageDimension; j++ )
      {
        numberOfElements *= opers[dim].GetSize()[j];
      }

    kernelNorms[dim] = 0;
    //for(unsigned int k = 0; k < opers[dim].GetSize(0); k++)
    for(unsigned int k = 0; k < opers[dim].GetSize(dim); k++)
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
  clSetKernelArg(m_3DGradientKernel, argidx++, sizeof(cl_mem), (void *)&m_MovingImageGPUBuffer);
  clSetKernelArg(m_3DGradientKernel, argidx++, sizeof(cl_mem), (void *)&m_MovingImageGradientGPUBuffer);

  for(int i=0; i<MovingImageDimension; i++)
    {
    clSetKernelArg(m_3DGradientKernel, argidx++, sizeof(cl_mem), (void *)&m_GPUDerivOperatorBuffers[i]);
    }  

  for(int i=0; i<MovingImageDimension; i++)
    {
    clSetKernelArg(m_3DGradientKernel, argidx++, sizeof(int),  &(radius[i]));
    }

  for(int i=0; i<MovingImageDimension; i++)
    {
    clSetKernelArg(m_3DGradientKernel, argidx++, sizeof(int),  &(imgSize[i]) );
    }

  InternalRealType spacing[3];
  for(int i=0; i<MovingImageDimension; i++)
    {
    spacing[i] = m_MovingImage->GetSpacing()[ i ];
    clSetKernelArg(m_3DGradientKernel, argidx++, sizeof(int),  &(spacing[i]) );
    } 

  // create N-D range object with work-item dimensions and execute kernel
  clEnqueueNDRangeKernel(m_CommandQueue[0], m_3DGradientKernel, MovingImageDimension, NULL, globalSize, localSize, 0, NULL, NULL);
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
  desc.buffer = NULL;

  m_MovingImageGradientGPUImage = clCreateImage(m_Context,
                                  CL_MEM_READ_ONLY, &(gpu_gradient_image_format), &desc,
                                   m_cpuMovingGradientImageBuffer, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {imgSize[0], imgSize[1], imgSize[2]};
  errid = clEnqueueCopyBufferToImage(m_CommandQueue[0], m_MovingImageGradientGPUBuffer, m_MovingImageGradientGPUImage,
                                     0, origin, region, 0, NULL, NULL );
  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);


  clReleaseKernel(m_3DGradientKernel);
  clReleaseMemObject(m_MovingImageGradientGPUBuffer);
  clReleaseMemObject(m_MovingImageGPUBuffer);
  for (int d = 0; d < MovingImageDimension; ++d)
  {
    clReleaseMemObject(m_GPUDerivOperatorBuffers[d]);
  }

  this->CreateGPUVariablesForCostFunction();

  /* Build Orientation Matching Kernel */

  //Version C
  std::ostringstream defines2;
  defines2 << "#define SEL " << m_N << std::endl;
  defines2 << "#define N " << m_Blocks * m_Threads << std::endl;
  defines2 << "#define LOCALSIZE " << m_Threads << std::endl;
  defines2 << "#define SIZE_OF_GRADIENT_BASE " << m_NumberOfSlices * 6 << std::endl;
  defines2 << "#define VERSION_C " << std::endl;

  m_OrientationMatchingKernel = CreateKernelFromString( GPU2D3DOrientationMatchingMatrixTransformationSparseMaskKernel,
    defines2.str().c_str(), "OrientationMatchingMetricSparseMask","", &m_OrientationMatchingProgram);


}

/**
 * Update Metric Value
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::CreateGPUVariablesForCostFunction(void)
{

  MovingImageDirectionType scale;       
  for ( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
      scale[i][i]= m_MovingImage->GetSpacing()[i];
    }      
  MovingImageDirectionType movingIndexToLocation = m_MovingImage->GetDirection()*scale;

  MovingImageDirectionType movingLocationToIndex = 
                    MovingImageDirectionType(movingIndexToLocation.GetInverse());


  m_MovingIndexToMovingLocationMatrix.set_identity();
  for ( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
    m_MovingIndexToMovingLocationMatrix[i][3] =  m_MovingImage->GetOrigin()[i]; 
    for ( unsigned int j = 0; j < MovingImageDimension; j++ )
      {    
      m_MovingIndexToMovingLocationMatrix[i][j] = movingLocationToIndex[i][j];
      }          
    }  
   m_MovingLocationToMovingIndexMatrix =  vnl_inverse(m_MovingIndexToMovingLocationMatrix);


  cl_int errid;


  m_gpuMetricAccum = clCreateBuffer(m_Context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 
                          m_Blocks * sizeof(InternalRealType),
                          NULL, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);   


  unsigned int dummySize = 24;
  m_cpuDummy = (InternalRealType*)malloc( dummySize* sizeof(InternalRealType));
  memset(m_cpuDummy, 0, dummySize * sizeof(InternalRealType));
  m_gpuDummy = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                          dummySize * sizeof(InternalRealType),
                          m_cpuDummy, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

  //Version C
  m_gpuFixedGradientSamples = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          m_Blocks * m_Threads * 2 * sizeof(InternalRealType),
                          m_cpuFixedGradientSamples, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  m_gpuFixedLocationSamples = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          m_Blocks * m_Threads * 4 * sizeof(InternalRealType),
                          m_cpuFixedLocationSamples, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  m_gpuFixedGradientBase = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          m_NumberOfSlices * 6 * sizeof(InternalRealType),
                          m_cpuFixedGradientBase, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

}


/**
 * Update Metric Value
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::UpdateGPUTransformVariables(void)
{
  m_TransformMatrix = m_Transform->GetMatrix();
  m_TransformOffset = m_Transform->GetOffset();  
  m_TransformMatrixTranspose = m_TransformMatrix.GetVnlMatrix().transpose();

  m_FixedLocationToMovingLocationMatrix.set_identity();
  //New Code
  for ( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
      m_FixedLocationToMovingLocationMatrix[i][3] = m_TransformOffset[i];
      for (unsigned int j=0; j < MovingImageDimension; j++)
      {
        m_FixedLocationToMovingLocationMatrix[i][j] = m_TransformMatrix[i][j];
      }
    }  
  m_FixedLocationToMovingIndexMatrix = m_MovingLocationToMovingIndexMatrix * m_FixedLocationToMovingLocationMatrix;

  for ( unsigned int i = 0; i < MovingImageDimension; i++ )
    {
      for (unsigned int j=0; j < MovingImageDimension; j++)
      {
        m_cpuDummy[4*i+j] = m_FixedLocationToMovingIndexMatrix[i][j]; // Fixed Location to Moving Index Matrix
        m_cpuDummy[4*i+j+12] = m_TransformMatrixTranspose[i][j]; //Spatial Jacobian (Rotation Matrix)

      }
      m_cpuDummy[4*i+3] = m_FixedLocationToMovingIndexMatrix[i][3];
    }

  cl_int errid;
  errid = clEnqueueWriteBuffer(m_CommandQueue[0], m_gpuDummy, CL_TRUE, 0, 
    24 * sizeof(InternalRealType), m_cpuDummy, 0, NULL, NULL);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
}


/**
 * Update Metric Value
 */
template< class TFixedImage, class TMovingImage >
void
GPU2D3DOrientationMatchingMatrixTransformationSparseMask< TFixedImage, TMovingImage >
::Update(void)
{

  if(!m_Transform)
  {
    itkExceptionMacro(<< "Transform is not set." );
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
  clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(InternalRealType) * m_Threads, NULL);
  //Version C
  clSetKernelArg(m_OrientationMatchingKernel, argidx++, sizeof(cl_mem), (void *)&m_gpuFixedGradientBase);

  clEnqueueNDRangeKernel(m_CommandQueue[0], m_OrientationMatchingKernel, 1, NULL, globalSize, localSize, 0, NULL, NULL);

  cl_int errid;
  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  m_cpuMetricAccum = (InternalRealType *)clEnqueueMapBuffer( m_CommandQueue[0], m_gpuMetricAccum, CL_TRUE,
                                  CL_MAP_READ, 0, m_Blocks * sizeof(InternalRealType),
                                  0, NULL, NULL, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
  errid = clFinish(m_CommandQueue[0]);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
 
  InternalRealType metricSum = 0;  
  for(unsigned int i=0; i<m_Blocks; i++)
  {
    metricSum += m_cpuMetricAccum[i]; 
  }

  clEnqueueUnmapMemObject( m_CommandQueue[0], m_gpuMetricAccum, m_cpuMetricAccum, 0, NULL, NULL);

  m_MetricValue = 0;

  if( metricSum >0 )
    m_MetricValue = (InternalRealType) (metricSum/((InternalRealType)globalSize[0]));

}


} // end namespace itk

#endif
