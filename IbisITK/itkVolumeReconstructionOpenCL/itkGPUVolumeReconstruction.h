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

#ifndef __itkGPUVolumeReconstruction_h
#define __itkGPUVolumeReconstruction_h

#include "itkObject.h"
#include "itkOpenCLUtil.h"
#include <itkImage.h>

#include "vnl/vnl_matrix_fixed.h"
#include "vnl/vnl_inverse.h"
#include "vnl/vnl_matrix.h"

#include "itkEuler3DTransform.h"

#include "itkImageFileWriter.h" //ImageFileWriter used for debugging
namespace itk
{

template< class TImage >
class ITK_EXPORT GPUVolumeReconstruction :
  public Object
{
public:
  /** Standard class typedefs. */
  typedef GPUVolumeReconstruction               Self;
  typedef Object                     Superclass;
  typedef SmartPointer< Self >       Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  typedef  float                    InternalRealType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(GPUVolumeReconstruction,
               Object);

  /** Image type. */
  typedef TImage                                ImageType;
  typedef typename ImageType::PixelType         ImagePixelType;
  typedef typename ImageType::Pointer           ImagePointer;
  typedef typename ImageType::ConstPointer      ImageConstPointer;
  typedef typename ImageType::PointType         ImagePointType;
  typedef typename ImageType::DirectionType     ImageDirectionType;

  typedef itk::Euler3DTransform<float>          TransformType;
  typedef typename TransformType::Pointer       TransformPointer;

  itkGetObjectMacro(FixedSliceMask, ImageType);
  itkSetObjectMacro(FixedSliceMask, ImageType);

  itkGetObjectMacro(ReconstructedVolume, ImageType);

  itkSetMacro(USSearchRadius, unsigned int);

  itkSetMacro(KernelStdDev, float);  

  itkSetMacro(VolumeSpacing, float);    

  itkGetObjectMacro(Transform, TransformType);
  itkSetObjectMacro(Transform, TransformType);  

  /** Extract dimension from input image. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TImage::ImageDimension);

  typedef Image<InternalRealType, ImageDimension>         RealImageType;
  typedef typename RealImageType::Pointer                 RealImagePointer;

  itkSetMacro(Debug, bool);

  //ImageFileWriter used for debugging
  typedef itk::ImageFileWriter< ImageType  >  WriterType;  
  typedef typename WriterType::Pointer        WriterPointer;

  void ReconstructVolume(void);  

  void SetFixedSlice( unsigned int sliceIdx, ImagePointer sliceImage);

  void SetNumberOfSlices( unsigned int numberOfSlices );


protected:
  GPUVolumeReconstruction();
  ~GPUVolumeReconstruction();

  void InitializeGPUContext(void);

  void PrintSelf(std::ostream & os, Indent indent) const override;

  cl_kernel CreateKernelFromFile(const char * filename, const char * cPreamble, const char * kernelname, const char * cOptions);
  cl_kernel CreateKernelFromString(const char * cOriginalSourceString, const char * cPreamble, const char * kernelname, const char * cOptions, cl_program * program);

  void CreateReconstructedVolume(void);
  void CreateMatrices(void);

  bool CheckAllSlicesDefined( void );

  bool              m_Debug;

  unsigned int      m_NumberOfSlices;
  unsigned int      m_NbrPixelsInSlice;

  unsigned int      m_USSearchRadius;
  float             m_KernelStdDev;
  float             m_VolumeSpacing;  

  TransformPointer  m_Transform;

  ImagePointer               m_FixedSliceMask;

  cl_mem                     m_FixedImageGPUBuffer;

  std::vector<ImagePointer>  m_FixedSlices;

  std::vector<unsigned int>  m_SliceValidIdxs;

  ImagePointer               m_ReconstructedVolume;

  cl_program          m_VolumeReconstructionPopulatingProgram;
  cl_kernel           m_VolumeReconstructionPopulatingKernel;

  cl_platform_id      m_Platform;
  cl_context          m_Context;
  cl_device_id*       m_Devices;
  cl_command_queue*   m_CommandQueue;
  cl_program          m_Program;
  
  cl_uint             m_NumberOfDevices, m_NumberOfPlatforms;

  cl_mem              m_gpuAllMatrices;  

  InternalRealType *  m_VolumeIndexToSliceIndexMatrices;
  InternalRealType *  m_VolumeIndexToLocationMatrix;
  InternalRealType *  m_SliceIndexToLocationMatrices;


private:
  GPUVolumeReconstruction(const Self &);   //purposely not implemented
  void operator=(const Self &); //purposely not implemented

};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGPUVolumeReconstruction.hxx"
#endif

#endif
