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

#ifndef __itkGPUOrientationMatchingUSCalibration_h
#define __itkGPUOrientationMatchingUSCalibration_h

#include "itkObject.h"
#include "itkOpenCLUtil.h"
#include "itkCovariantVector.h"
#include "itkMatrixOffsetTransformBase.h"
#include "itkGaussianDerivativeOperator.h"
#include "itkImage.h"
#include "itkSampleToHistogramFilter.h"
#include "itkListSample.h"
#include "itkHistogram.h"

#include "vnl/vnl_matrix_fixed.h"
#include "vnl/vnl_inverse.h"
#include "vnl/vnl_matrix.h"

//#define DEBUG

#ifdef DEBUG
  #include "itkImageDuplicator.h"
  #include "itkImageFileWriter.h"
#endif

namespace itk
{

template< class TFixedImage, class TMovingImage >
class ITK_EXPORT GPUOrientationMatchingUSCalibration :
  public Object
{
public:
  /** Standard class typedefs. */
  typedef GPUOrientationMatchingUSCalibration               Self;
  typedef Object                     Superclass;
  typedef SmartPointer< Self >       Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  typedef  float                    InternalRealType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(GPUOrientationMatchingUSCalibration,
               Object);

  /** FixedImage image type. */
  typedef TFixedImage                           FixedImageType;
  typedef typename FixedImageType::PixelType    FixedImagePixelType;
  typedef typename FixedImageType::Pointer      FixedImagePointer;
  typedef typename FixedImageType::ConstPointer FixedImageConstPointer;
  typedef typename FixedImageType::PointType    FixedImagePointType;

  typedef std::vector<FixedImagePointType>       SampleLocationsType;

  itkGetObjectMacro(FixedSliceMask, FixedImageType);
  itkSetObjectMacro(FixedSliceMask, FixedImageType);


  /** MovingImage image type. */
  typedef TMovingImage                           MovingImageType;
  typedef typename MovingImageType::PixelType    MovingImagePixelType;
  typedef typename MovingImageType::Pointer      MovingImagePointer;
  typedef typename MovingImageType::ConstPointer MovingImageConstPointer;
  typedef typename MovingImageType::DirectionType MovingImageDirectionType;
  typedef typename MovingImageType::PointType     MovingImagePointType;

  itkGetObjectMacro(MovingImage, MovingImageType);
  itkSetObjectMacro(MovingImage, MovingImageType);  


  /** Extract dimension from input image. */
  itkStaticConstMacro(FixedImageDimension, unsigned int,
                      TFixedImage::ImageDimension);
  itkStaticConstMacro(MovingImageDimension, unsigned int,
                      TMovingImage::ImageDimension);

  typedef Image<InternalRealType, FixedImageDimension>    RealImageType;
  typedef typename RealImageType::Pointer                 RealImagePointer;

  typedef CovariantVector< InternalRealType, 2 >
                                                         SliceGradientType;

  typedef CovariantVector< InternalRealType,itkGetStaticConstMacro(FixedImageDimension) >
                                                         FixedGradientType;

  typedef Image< FixedGradientType, itkGetStaticConstMacro(FixedImageDimension) >
                                                         FixedImageGradientType;
  typedef typename FixedImageGradientType::Pointer       FixedImageGradientPointer;
  typedef typename FixedImageGradientType::ConstPointer  FixedImageGradientConstPointer;

  typedef CovariantVector< InternalRealType,itkGetStaticConstMacro(MovingImageDimension) >
                                                         MovingGradientType;


  typedef Image< MovingGradientType , itkGetStaticConstMacro(MovingImageDimension) >
                                                          MovingImageGradientType;
  typedef typename MovingImageGradientType::Pointer       MovingImageGradientPointer;
  typedef typename MovingImageGradientType::ConstPointer  MovingImageGradientConstPointer;

  typedef double   TransformRealType;

  typedef MatrixOffsetTransformBase< TransformRealType, 
                                FixedImageDimension, MovingImageDimension>
                                                      MatrixTransformType;
  typedef typename MatrixTransformType::Pointer       MatrixTransformPointer;
  typedef typename MatrixTransformType::ConstPointer  MatrixTransformConstPointer;

  typedef typename MatrixTransformType::MatrixType          TransformMatrixType;
  typedef typename MatrixTransformType::InverseMatrixType   TransformInverseMatrixType;
  typedef typename MatrixTransformType::OutputVectorType    TransformOffsetType;

  itkGetConstObjectMacro(Transform, MatrixTransformType);
  itkSetObjectMacro(Transform, MatrixTransformType);

  itkGetConstObjectMacro(USCalibrationTransform, MatrixTransformType);
  itkSetObjectMacro(USCalibrationTransform, MatrixTransformType);  

  itkGetConstMacro(MetricValue, InternalRealType);

  itkSetMacro(NumberOfPixels, unsigned int);
  itkSetMacro(N, unsigned int);
  itkSetMacro(Percentile, double);

  itkSetMacro(Debug, bool);
  itkSetMacro(MaskThreshold, double);

  itkSetMacro(GradientScale, double);

  itkSetMacro(FixedGradientScale, double);  
  itkSetMacro(MovingGradientScale, double);    

  itkGetMacro(SampleLocations, SampleLocationsType)

  itkGetConstMacro(Center, FixedImagePointType);  
  itkSetMacro(Center,      FixedImagePointType);

  typedef GaussianDerivativeOperator<InternalRealType, FixedImageDimension> FixedDerivativeOperatorType;
  typedef GaussianDerivativeOperator<InternalRealType, MovingImageDimension> MovingDerivativeOperatorType;

  typedef itk::Vector<InternalRealType, 1>                       MeasurementVectorType;
  typedef itk::Statistics::ListSample< MeasurementVectorType >   FixedGradientMagnitudeSampleType;

  typedef itk::Statistics::ListSample< itk::Vector<unsigned int, 1> >   IdxSampleType;

  typedef itk::Statistics::Histogram< float,
        itk::Statistics::DenseFrequencyContainer2 >         HistogramType;

  typedef itk::Statistics::SampleToHistogramFilter<FixedGradientMagnitudeSampleType, HistogramType> 
                                                            SampleToHistogramFilterType;


#ifdef DEBUG                                                       
  typedef itk::ImageDuplicator< FixedImageType >            DuplicatorType;
  typedef typename DuplicatorType::Pointer                  DuplicatorPointer;

  typedef itk::ImageFileWriter< FixedImageType  >  WriterType;  
  typedef typename WriterType::Pointer        WriterPointer;
#endif

  void Update(void);  

  unsigned int NextPow2( unsigned int x );

  void SetFixedSlice( unsigned int sliceIdx, FixedImagePointer sliceImage);

  void SetNumberOfSlices( unsigned int numberOfSlices );

  void UpdateSampleLocations();

protected:
  GPUOrientationMatchingUSCalibration();
  ~GPUOrientationMatchingUSCalibration();

  void InitializeGPUContext(void);

  void PrintSelf(std::ostream & os, Indent indent) const;  

  void ComputeSliceMaskWithoutBoundaries(void);
  void Compute2DImageGradient( FixedImagePointer sliceImage, InternalRealType ** imageGradients);

  void ComputeFixedImageGradient(void);
  void ComputeMovingImageGradient(void);
  void CreateGPUVariablesForCostFunction(void);
  void UpdateGPUTransformVariables(void);

  void Compute2DUSIndexToWorldTransformations(void);

  cl_kernel CreateKernelFromFile(const char * filename, const char * cPreamble, const char * kernelname, const char * cOptions);
  cl_kernel CreateKernelFromString(const char * cOriginalSourceString, const char * cPreamble, const char * kernelname, const char * cOptions, cl_program * program);

  bool CheckAllSlicesDefined( void );

  bool              m_Debug;

  unsigned int      m_NumberOfPixels;
  double            m_Percentile;
  unsigned int      m_N;
  double            m_GradientScale;

  double            m_FixedGradientScale;  
  double            m_MovingGradientScale;

  double            m_MaskThreshold;

  unsigned int      m_Blocks;
  unsigned int      m_Threads;

  unsigned int      m_NumberOfSlices;

  FixedImagePointType  m_Center;

  FixedImagePointer               m_FixedSliceMask;
  FixedImagePointer               m_FixedSliceMaskWithoutBoundaries;
  cl_mem                          m_FixedImageGPUBuffer;
  MovingImagePointer              m_MovingImage;
  cl_mem                          m_MovingImageGPUBuffer;
  std::vector<FixedImagePointer>  m_FixedSlices;

  std::vector<unsigned int>       m_SliceValidIdxs;

  SampleLocationsType             m_SampleLocations;

  std::vector<InternalRealType*>  m_CPUDerivOperatorBuffers;
  std::vector<cl_mem>             m_GPUDerivOperatorBuffers; 

  cl_mem                      m_FixedImageGradientGPUBuffer;
  cl_mem                      m_MovingImageGradientGPUBuffer;
  cl_mem                      m_MovingImageGradientGPUImage;

  InternalRealType              m_MetricValue;

  vnl_matrix_fixed<double, 4, 4>   m_FixedLocationToMovingLocationMatrix;
  vnl_matrix_fixed<double, 4, 4>   m_MovingIndexToMovingLocationMatrix;  
  vnl_matrix_fixed<double, 4, 4>   m_MovingLocationToMovingIndexMatrix;
  vnl_matrix_fixed<double, 4, 4>   m_FixedLocationToMovingIndexMatrix;

  MatrixTransformConstPointer   m_Transform;
  MatrixTransformConstPointer   m_USCalibrationTransform;

  TransformMatrixType           m_TransformMatrix, m_TransformMatrixTranspose;
  TransformOffsetType           m_TransformOffset;  

  InternalRealType*             m_cpuDummy;
  cl_mem                        m_gpuDummy;

  unsigned int                  m_NbrPixelsInSlice;

  InternalRealType *  m_cpuFixedGradientSamples;
  cl_mem              m_gpuFixedGradientSamples;  

  unsigned int  *     m_cpuFixedIndicesSamples;
  cl_mem              m_gpuFixedIndicesSamples;  


  InternalRealType *  m_cpuFixedPixelToWorldTransformations;
  cl_mem              m_gpuFixedPixelToWorldTransformations;

  InternalRealType *  m_cpuMovingGradientImageBuffer;

  InternalRealType*   m_cpuMetricAccum;
  cl_mem              m_gpuMetricAccum;

  cl_program          m_OrientationMatchingProgram;
  cl_kernel           m_OrientationMatchingKernel;

  cl_program          m_3DGradientProgram;
  cl_kernel           m_3DGradientKernel;  

  cl_program          m_2DGradientProgram;
  cl_kernel           m_2DGradientKernel;  

  cl_program          m_2DAllGradientProgram;
  cl_kernel           m_2DAllGradientKernel;    

  cl_platform_id      m_Platform;
  cl_context          m_Context;
  cl_device_id*       m_Devices;
  cl_command_queue*   m_CommandQueue;
  cl_program          m_Program;
  
  cl_uint             m_NumberOfDevices, m_NumberOfPlatforms;

private:
  GPUOrientationMatchingUSCalibration(const Self &);   //purposely not implemented
  void operator=(const Self &); //purposely not implemented

};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGPUOrientationMatchingUSCalibration.hxx"
#endif

#endif
