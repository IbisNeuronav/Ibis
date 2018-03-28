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

#ifndef __itkGPUVolumeReconstruction_hxx
#define __itkGPUVolumeReconstruction_hxx

#include "itkMacro.h"
#include "itkGPUVolumeReconstruction.h"
#include "itkTimeProbe.h"
#include "itkMatrix.h"
#include "vnl/vnl_matrix.h"
#include "itkOpenCLUtil.h"

#include "GPUVolumeReconstructionKernel.h"


namespace itk
{
/**
 * Default constructor
 */
template< class TImage >
GPUVolumeReconstruction< TImage >
::GPUVolumeReconstruction()
{

  if(!itk::IsGPUAvailable())
  {
    itkExceptionMacro(<< "OpenCL-enabled GPU is not present.");
  }

  /* Initialize GPU Context */
  this->InitializeGPUContext();

  m_NumberOfSlices = 0;  

  m_Debug = false;

  m_USSearchRadius = 0;
  m_KernelStdDev   = 1.0;
  m_VolumeSpacing   = 1.0;

  m_ReconstructedVolume = 0;
  m_FixedSliceMask = 0;

  m_Transform = TransformType::New();
}


template< class TImage >
GPUVolumeReconstruction< TImage >
::~GPUVolumeReconstruction()
{
    
  if(m_VolumeReconstructionPopulatingKernel)
    clReleaseKernel(m_VolumeReconstructionPopulatingKernel);

}

template< class TImage >
void
GPUVolumeReconstruction< TImage >
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
#ifdef __APPLE__
    m_CommandQueue[i] = clCreateCommandQueue(m_Context, m_Devices[i], 0, &errid);
#else
    m_CommandQueue[i] = clCreateCommandQueueWithProperties(m_Context, m_Devices[i], 0, &errid);
#endif
    OpenCLCheckError( errid, __FILE__, __LINE__, ITK_LOCATION );
    }

#ifdef DEBUG
  std::cerr << "Creating Kernel.. " << std::endl;
#endif  

  m_VolumeReconstructionPopulatingKernel = CreateKernelFromString( GPUVolumeReconstructionKernel,
                                                     "", "VolumeReconstructionPopulating", "", &m_VolumeReconstructionPopulatingProgram);

#ifdef DEBUG
  std::cerr << "Creating Kernel.. DONE" << std::endl;
#endif    

}

/**
 * Standard "PrintSelf" method.
 */
template< class TImage >
void
GPUVolumeReconstruction< TImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template< class TImage >
unsigned int
GPUVolumeReconstruction< TImage >
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
template< class TImage >
cl_kernel
GPUVolumeReconstruction< TImage >
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


template< class TImage >
cl_kernel
GPUVolumeReconstruction< TImage >
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

template< class TImage >
void
GPUVolumeReconstruction< TImage >
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

template< class TImage >
void
GPUVolumeReconstruction< TImage >
::SetFixedSlice( unsigned int sliceIdx, ImagePointer sliceImage )
{

  m_FixedSlices[sliceIdx] = sliceImage;
}

template< class TImage >
bool
GPUVolumeReconstruction< TImage >
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

template< class TImage >
void
GPUVolumeReconstruction< TImage >
::CreateReconstructedVolume( void )
{
#ifdef DEBUG
  std::cerr << "Creating Empty Reconstructed Volume.." << std::endl;
#endif  

  itk::TimeProbe clockReconstruction;
  clockReconstruction.Start();
  //Find bounds for 1mm3 US volume
  typename ImageType::PointType corner1, corner2, corner3, corner4;
  typename ImageType::PointType trCorner1, trCorner2, trCorner3, trCorner4;
  typename ImageType::IndexType fixedIndex;
  typename ImageType::SizeType sliceSize = m_FixedSlices[1]->GetLargestPossibleRegion().GetSize();

  double m_LowerBound[3], m_UpperBound[3];

  m_LowerBound[0] = 10000.0; m_LowerBound[1] = 10000.0; m_LowerBound[2] = 10000.0;
  m_UpperBound[0] = -10000.0; m_UpperBound[1] = -10000.0; m_UpperBound[2] = -10000.0;
  for(unsigned int i=1; i<m_NumberOfSlices; i++)
  {

    fixedIndex[0] = 0; fixedIndex[1] = 0; fixedIndex[2] = 0;
    m_FixedSlices[i]->TransformIndexToPhysicalPoint(fixedIndex, corner1);

    fixedIndex[0] = 0+sliceSize[0]; fixedIndex[1] = 0; fixedIndex[2] = 0;
    m_FixedSlices[i]->TransformIndexToPhysicalPoint(fixedIndex, corner2);

    fixedIndex[0] = 0; fixedIndex[1] = 0+sliceSize[1]; fixedIndex[2] = 0;
    m_FixedSlices[i]->TransformIndexToPhysicalPoint(fixedIndex, corner3);

    fixedIndex[0] = 0+sliceSize[0]; fixedIndex[1] = 0+sliceSize[1]; fixedIndex[2] = 0;
    m_FixedSlices[i]->TransformIndexToPhysicalPoint(fixedIndex, corner4);

    trCorner1 = m_Transform->TransformPoint( corner1 );
    trCorner2 = m_Transform->TransformPoint( corner2 );
    trCorner3 = m_Transform->TransformPoint( corner3 );
    trCorner4 = m_Transform->TransformPoint( corner4 );
    
    m_LowerBound[0] = std::min( trCorner1[0], m_LowerBound[0] ); m_LowerBound[1] = std::min( trCorner1[1], m_LowerBound[1] ); m_LowerBound[2] = std::min( trCorner1[2], m_LowerBound[2] );
    m_LowerBound[0] = std::min( trCorner2[0], m_LowerBound[0] ); m_LowerBound[1] = std::min( trCorner2[1], m_LowerBound[1] ); m_LowerBound[2] = std::min( trCorner2[2], m_LowerBound[2] );
    m_LowerBound[0] = std::min( trCorner3[0], m_LowerBound[0] ); m_LowerBound[1] = std::min( trCorner3[1], m_LowerBound[1] ); m_LowerBound[2] = std::min( trCorner3[2], m_LowerBound[2] );
    m_LowerBound[0] = std::min( trCorner4[0], m_LowerBound[0] ); m_LowerBound[1] = std::min( trCorner4[1], m_LowerBound[1] ); m_LowerBound[2] = std::min( trCorner4[2], m_LowerBound[2] );

    m_UpperBound[0] = std::max( trCorner1[0], m_UpperBound[0] ); m_UpperBound[1] = std::max( trCorner1[1], m_UpperBound[1] ); m_UpperBound[2] = std::max( trCorner1[2], m_UpperBound[2] );
    m_UpperBound[0] = std::max( trCorner2[0], m_UpperBound[0] ); m_UpperBound[1] = std::max( trCorner2[1], m_UpperBound[1] ); m_UpperBound[2] = std::max( trCorner2[2], m_UpperBound[2] );
    m_UpperBound[0] = std::max( trCorner3[0], m_UpperBound[0] ); m_UpperBound[1] = std::max( trCorner3[1], m_UpperBound[1] ); m_UpperBound[2] = std::max( trCorner3[2], m_UpperBound[2] );
    m_UpperBound[0] = std::max( trCorner4[0], m_UpperBound[0] ); m_UpperBound[1] = std::max( trCorner4[1], m_UpperBound[1] ); m_UpperBound[2] = std::max( trCorner4[2], m_UpperBound[2] );    
  }

  typename ImageType::IndexType startIndex;
  startIndex[0] = 0; // first index on X
  startIndex[1] = 0; // first index on Y
  startIndex[2] = 0; // first index on Z

  typename ImageType::SpacingType spacing1;
  spacing1.Fill( m_VolumeSpacing );

  typename ImageType::SizeType size1;
  size1[0] = ceil(m_UpperBound[0] - m_LowerBound[0])/spacing1[0]; // size along X
  size1[1] = ceil(m_UpperBound[1] - m_LowerBound[1])/spacing1[1]; // size along Y
  size1[2] = ceil(m_UpperBound[2] - m_LowerBound[2])/spacing1[2]; // size along Z

  typename ImageType::RegionType region1;
  region1.SetSize( size1 );
  region1.SetIndex( startIndex );

  m_ReconstructedVolume = ImageType::New();
  m_ReconstructedVolume->SetRegions( region1 );
  m_ReconstructedVolume->SetOrigin( m_LowerBound );
  m_ReconstructedVolume->SetSpacing( spacing1 );
  m_ReconstructedVolume->Allocate();
  m_ReconstructedVolume->FillBuffer(0.0);

#ifdef DEBUG
  std::cerr << "Creating Emtpy Reconstructed Volume..DONE" << std::endl;
#endif    

}

template< class TImage >
void
GPUVolumeReconstruction< TImage >
::CreateMatrices(void)
{

#ifdef DEBUG
  std::cerr << "Creating Matrices.." << std::endl;
#endif  

  m_VolumeIndexToSliceIndexMatrices = new InternalRealType[m_NumberOfSlices * 12];
  m_VolumeIndexToLocationMatrix = new InternalRealType[12];
  m_SliceIndexToLocationMatrices = new InternalRealType[m_NumberOfSlices * 12];  

  ImageDirectionType volumeScale;       
  for ( unsigned int i = 0; i < ImageDimension; i++ )
    {
      volumeScale[i][i]= m_ReconstructedVolume->GetSpacing()[i];
    }      
  ImageDirectionType volumeIndexToLocation3x3 = m_ReconstructedVolume->GetDirection()*volumeScale;

  vnl_matrix_fixed<InternalRealType, 4, 4> volumeIndexToLocation4x4;
  volumeIndexToLocation4x4.set_identity();
  for ( unsigned int i = 0; i < ImageDimension; i++ )
    {
    volumeIndexToLocation4x4[i][3] =  m_ReconstructedVolume->GetOrigin()[i]; 
    m_VolumeIndexToLocationMatrix[4*i+3] = volumeIndexToLocation4x4[i][3];
    for ( unsigned int j = 0; j < ImageDimension; j++ )
      {    
      volumeIndexToLocation4x4[i][j] = volumeIndexToLocation3x3[i][j]; //Does this make sense??
      m_VolumeIndexToLocationMatrix[4*i+j] = volumeIndexToLocation3x3[i][j];
      }          
    }  


  vnl_matrix_fixed<InternalRealType, 4, 4> locationToTransformLocation4x4;
  locationToTransformLocation4x4.set_identity();
  for ( unsigned int i = 0; i < ImageDimension; i++ )
    {
    locationToTransformLocation4x4[i][3] = m_Transform->GetOffset()[i];
    for ( unsigned int j = 0; j < ImageDimension; j++ )
      {    
      locationToTransformLocation4x4[i][j] = m_Transform->GetMatrix()[i][j];
      }          
    }  


  for(unsigned int sliceIdx = 0; sliceIdx < m_NumberOfSlices; sliceIdx++)
  {
    ImageDirectionType scale;       
    for ( unsigned int i = 0; i < ImageDimension; i++ )
      {
        scale[i][i]= m_FixedSlices[sliceIdx]->GetSpacing()[i];
      }      
    ImageDirectionType sliceIndexToLocation3x3 = m_FixedSlices[sliceIdx]->GetDirection()*scale;

    vnl_matrix_fixed<InternalRealType, 4, 4> sliceIndexToLocation4x4, transSliceIndexToLocation4x4;
    sliceIndexToLocation4x4.set_identity();
    for ( unsigned int i = 0; i < ImageDimension; i++ )
      {
      sliceIndexToLocation4x4[i][3] =  m_FixedSlices[sliceIdx]->GetOrigin()[i]; 
      
      for ( unsigned int j = 0; j < ImageDimension; j++ )
        {    
        sliceIndexToLocation4x4[i][j] = sliceIndexToLocation3x3[i][j]; //Does this make sense??        
        }          
      }  

     transSliceIndexToLocation4x4 = locationToTransformLocation4x4 * sliceIndexToLocation4x4;
     vnl_matrix_fixed<InternalRealType, 4, 4> volumeIndexToSliceIndex = vnl_inverse(transSliceIndexToLocation4x4) * volumeIndexToLocation4x4;
    for ( unsigned int i = 0; i < ImageDimension; i++ )
      {
      m_VolumeIndexToSliceIndexMatrices[sliceIdx*12 + 4*i + 3] = volumeIndexToSliceIndex[i][3];
      m_SliceIndexToLocationMatrices[sliceIdx*12 + 4*i + 3] = transSliceIndexToLocation4x4[i][3];
      for ( unsigned int j = 0; j < ImageDimension; j++ )
        {    
        m_VolumeIndexToSliceIndexMatrices[sliceIdx*12 + 4*i +j] = volumeIndexToSliceIndex[i][j];  

        m_SliceIndexToLocationMatrices[sliceIdx*12 + 4*i +j] = transSliceIndexToLocation4x4[i][j];
        }
      }        
  }


  cl_int errid;

#ifdef DEBUG
  std::cerr << "Creating Matrix Buffers on GPU" << std::endl;
#endif  

#ifdef DEBUG
  std::cerr << "Creating Matrices..DONE" << std::endl;
#endif  


}


template< class TImage >
void
GPUVolumeReconstruction< TImage >
::ReconstructVolume(void)
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


  CreateReconstructedVolume();

  CreateMatrices();

  unsigned int maxNbrOfSlices = 8;    //No reason in particular.. seems to yield a good tradeoff

  itk::TimeProbe clockGPUKernel;
  clockGPUKernel.Start();

  m_NbrPixelsInSlice = m_FixedSliceMask->GetLargestPossibleRegion().GetNumberOfPixels();
  unsigned int nbrOfPixelsInVolume = m_ReconstructedVolume->GetLargestPossibleRegion().GetNumberOfPixels();
  unsigned int size_output = nbrOfPixelsInVolume*sizeof(ImagePixelType);
  unsigned int size_slice = m_NbrPixelsInSlice * sizeof(InternalRealType);

//  std::cout << "Volume Spacing:\t" << m_VolumeSpacing << std::endl;
//  std::cout << "Standard Deviation:\t" << m_KernelStdDev << std::endl;


  unsigned char * maskValues = new unsigned char[m_NbrPixelsInSlice];
  for(unsigned int n=0; n < m_NbrPixelsInSlice; n++)
  {
    maskValues[n] = (unsigned char)(m_FixedSliceMask->GetPixel( m_FixedSliceMask->ComputeIndex(n)) );
  }
  
  cl_int errid;
  cl_image_format mask_image_format;
  mask_image_format.image_channel_order = CL_R;
  mask_image_format.image_channel_data_type = CL_UNSIGNED_INT8;    
  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = m_FixedSliceMask->GetLargestPossibleRegion().GetSize()[0];
  desc.image_height = m_FixedSliceMask->GetLargestPossibleRegion().GetSize()[1];
  desc.image_depth = 0;
  desc.image_array_size = 0;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = NULL;
  cl_mem inputImageMaskGPUBuffer = clCreateImage(   m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                    &(mask_image_format),
                                                    &desc,
                                                    maskValues, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
  

  size_t localSize[3], globalSize[3];
  localSize[0] = localSize[1] = localSize[2] = OpenCLGetLocalBlockSize(3);


  InternalRealType * cpuAccumWeightAndWeightedValueBuffer = new InternalRealType[2*nbrOfPixelsInVolume];
  cl_mem accumWeightAndWeightedValueGPUBuffer = clCreateBuffer(m_Context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            2*size_output, cpuAccumWeightAndWeightedValueBuffer, &errid);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    


  int volumeSize[3];
  volumeSize[0] = m_ReconstructedVolume->GetLargestPossibleRegion().GetSize()[0];
  volumeSize[1] = m_ReconstructedVolume->GetLargestPossibleRegion().GetSize()[1];
  volumeSize[2] = m_ReconstructedVolume->GetLargestPossibleRegion().GetSize()[2];  

  for(unsigned int i=0; i<3; i++)
    {
    globalSize[i] = localSize[i]*(unsigned int)ceil( (float)volumeSize[i]/(float)localSize[i]);
    }

  itk::TimeProbe clockMemCpy;

  InternalRealType * allMatrices = new InternalRealType[12*(2*maxNbrOfSlices+1)];
  memcpy((void*)&allMatrices[0], (void *)m_VolumeIndexToLocationMatrix, 12*sizeof(InternalRealType));

  unsigned int sliceCntr = 0; // Counts the number of slices that have been processed.
  do
  {
    unsigned int nbrOfSlicesToProcess = std::min(m_NumberOfSlices - sliceCntr, maxNbrOfSlices);
    unsigned int size_input = nbrOfSlicesToProcess * size_slice;

    clockMemCpy.Start();
    ImagePixelType * allSlicesPixels = new ImagePixelType[nbrOfSlicesToProcess*m_NbrPixelsInSlice];

    for(unsigned int sliceIdx=sliceCntr; sliceIdx<sliceCntr+nbrOfSlicesToProcess ; sliceIdx++)
    {
      ImagePointer sliceImage = m_FixedSlices[sliceIdx];
      sliceImage->Update();
      memcpy((void *)&allSlicesPixels[(sliceIdx-sliceCntr)*m_NbrPixelsInSlice],
          (void *)sliceImage->GetBufferPointer(), size_slice );
    }
    clockMemCpy.Stop();

    int imgSize[3];
    imgSize[0] = m_FixedSliceMask->GetLargestPossibleRegion().GetSize()[0];
    imgSize[1] = m_FixedSliceMask->GetLargestPossibleRegion().GetSize()[1];
    imgSize[2] = nbrOfSlicesToProcess;

    // Create an OpenCL Image / texture and transfer data to the device

    cl_image_format gpu_image_format;
    gpu_image_format.image_channel_order = CL_R;
    gpu_image_format.image_channel_data_type = CL_FLOAT;    
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
    cl_mem inputImageGPUBuffer = clCreateImage(   m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                    &(gpu_image_format),
                                                    &desc,
                                                    (void *)allSlicesPixels, &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    memcpy((void*)&allMatrices[12], (void *)&m_VolumeIndexToSliceIndexMatrices[12*sliceCntr], 12*sizeof(InternalRealType)*nbrOfSlicesToProcess);    
    memcpy((void*)&allMatrices[12*(nbrOfSlicesToProcess+1)], (void *)&m_SliceIndexToLocationMatrices[12*sliceCntr], 12*sizeof(InternalRealType)*nbrOfSlicesToProcess);

    m_gpuAllMatrices = clCreateBuffer(m_Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            12 * sizeof(InternalRealType) * (2* nbrOfSlicesToProcess + 1),
                            allMatrices, &errid);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  


    int argidx = 0;
    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(cl_mem), (void *)&inputImageGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(cl_mem), (void *)&inputImageMaskGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    

    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(cl_mem), (void *)&accumWeightAndWeightedValueGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);       


    for(unsigned int i=0; i<ImageDimension; i++)
      {
      errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(int),  &(imgSize[i]) );
      OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
      }


    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(cl_mem), (void *)&m_gpuAllMatrices);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    


    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(InternalRealType) * 12 * (2*nbrOfSlicesToProcess + 1), NULL);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);  

    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(int),  &(sliceCntr) );
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    for(unsigned int i=0; i<ImageDimension; i++)
      {
      errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(int),  &(volumeSize[i]) );
      OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);
      }    

    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(int),  &(m_USSearchRadius) );
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);      

    errid = clSetKernelArg(m_VolumeReconstructionPopulatingKernel, argidx++, sizeof(float),  &(m_KernelStdDev) );
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);          

    errid = clEnqueueNDRangeKernel(m_CommandQueue[0], m_VolumeReconstructionPopulatingKernel, 3, NULL, globalSize, localSize, 0, NULL, NULL);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    

    errid = clFinish(m_CommandQueue[0]);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    

    errid = clReleaseMemObject(inputImageGPUBuffer);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

    errid = clReleaseMemObject(m_gpuAllMatrices);
    OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    

    sliceCntr += nbrOfSlicesToProcess;

    delete[] allSlicesPixels;
    

  }while(sliceCntr < m_NumberOfSlices);

  delete[] allMatrices;

  errid = clReleaseMemObject(inputImageMaskGPUBuffer);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);


  errid = clEnqueueReadBuffer(m_CommandQueue[0], accumWeightAndWeightedValueGPUBuffer, CL_TRUE, 0, 
    2*size_output, cpuAccumWeightAndWeightedValueBuffer, 0, NULL, NULL);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);    

  clockGPUKernel.Stop();

#ifdef DEBUG
    std::cerr << "Time to Populate and Accumulate Value in GPU:\t" << clockGPUKernel.GetMean() << std::endl;
    std::cerr << "Time to MemCpy:\t" << clockMemCpy.GetMean() << std::endl;
#endif  


  itk::TimeProbe clockSettingValue;
  clockSettingValue.Start();

#ifdef DEBUG
  unsigned int n = 0, m = 0;
#endif
  for(unsigned int i=0; i<nbrOfPixelsInVolume; i++)
  {
    InternalRealType weightedValue = cpuAccumWeightAndWeightedValueBuffer[2*i];
    InternalRealType weight =  cpuAccumWeightAndWeightedValueBuffer[2*i+1];

    if(weightedValue > 0)
    {
      ImagePixelType reconstructedValue = weightedValue / weight;
      m_ReconstructedVolume->SetPixel(m_ReconstructedVolume->ComputeIndex(i), reconstructedValue);
#ifdef DEBUG
      m++;
#endif
    }
    else
    {
      m_ReconstructedVolume->SetPixel(m_ReconstructedVolume->ComputeIndex(i), 0.0);
#ifdef DEBUG
      n++;
#endif
    }
  }
  clockSettingValue.Stop();

#ifdef DEBUG
    std::cout << "nbrOfPixelsInVolume = " << nbrOfPixelsInVolume << " m = " << m  << " n = " << n << std::endl;
    std::cerr << "Time to Pixel Values with for loop:\t" << clockSettingValue.GetMean() << std::endl;
#endif    

#ifdef DEBUG
  WriterPointer writer = WriterType::New();
  writer->SetInput(m_ReconstructedVolume);
  writer->SetFileName( "reconstructedVolume.mnc" );
  writer->Update();
#endif  

  errid = clReleaseMemObject(accumWeightAndWeightedValueGPUBuffer);
  OpenCLCheckError(errid, __FILE__, __LINE__, ITK_LOCATION);

  delete[] cpuAccumWeightAndWeightedValueBuffer;
  
}



} // end namespace itk

#endif
