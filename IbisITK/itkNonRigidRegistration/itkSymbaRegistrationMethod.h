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

#ifndef __itkSymbaRegistrationMethod_h
#define __itkSymbaRegistrationMethod_h

#include "itkProcessObject.h"
#include "itkImage.h"
#include "itkImageToImageMetric.h"
#include "itkSingleValuedNonLinearOptimizer.h"
#include "itkDataObjectDecorator.h"

#include "itkGOAGradientAscent.h"
#include "itkCombinationGOACostFunction.h"
#include "itkRescaledGaussianGradientImageFilter.h"
#include "itkRescaledImageFilter.h"
#include "itkCannyEdgeDetectionImageFilter2.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkResampleImageFilter.h"

#include "itkImageFileWriter.h"

#define itkSetListObjectElementMacro(name, type)                   \
  virtual void Set##name (type * _arg, unsigned int index)                \
    {                                                   \
    itkDebugMacro("setting " << #name " to " << _arg); \
  if ( this->m_ ## name ## List[index] != _arg )         \
      {                                                 \
      this->m_ ## name ## List[index] = _arg;                 \
      this->Modified();                                 \
      }                                                 \
    }

#define itkSetListElementMacro(name, type)                   \
  virtual void Set##name (type _arg, unsigned int index)                \
    {                                                   \
    itkDebugMacro("setting " << #name " to " << _arg); \
  if ( this->m_ ## name ## List[index] != _arg )         \
      {                                                 \
      this->m_ ## name ## List[index] = _arg;                 \
      this->Modified();                                 \
      }                                                 \
    }


namespace itk
{

template< unsigned int NDimensions >
class SymbaRegistrationMethod:public ProcessObject
{
public:
  /** Standard class typedefs. */
  typedef SymbaRegistrationMethod    Self;
  typedef ProcessObject              Superclass;
  typedef SmartPointer< Self >       Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(SymbaRegistrationMethod, ProcessObject);

  /** The dimension of the image. */
  itkStaticConstMacro(ImageDimension, unsigned int, NDimensions);

  /** Define Types **/
  typedef   double                                           PixelType;
  typedef   itk::Image< PixelType, ImageDimension >         ImageType;
  typedef   typename ImageType::ConstPointer                ImageConstPointer;  
  typedef   typename ImageType::Pointer                     ImagePointer;

  typedef   double                                          MaskPixelType;
  typedef   itk::Image< MaskPixelType, ImageDimension >     MaskImageType;
  typedef   typename MaskImageType::Pointer                 MaskImagePointer;

  typedef   itk::GOAGradientAscent<double, ImageDimension>            GOAOptimizationType;
  typedef   typename GOAOptimizationType::Pointer                    GOAOptimizationPointer;
  typedef   typename GOAOptimizationType::SamplingModeType           SamplingModeType;


  typedef   typename GOAOptimizationType::CostFunctionType           CostFunctionType;
  typedef   typename CostFunctionType::Pointer                       CostFunctionPointer;

  typedef   typename CostFunctionType::VectorImageType               VectorImageType;
  typedef   typename VectorImageType::PixelType                      VectorType;
  typedef   typename VectorImageType::Pointer                        VectorImagePointer;

  typedef   typename CostFunctionType::TransformType                 NonRigidTransformType;
  typedef   typename NonRigidTransformType::Pointer                  NonRigidTransformPointer;
  typedef   typename NonRigidTransformType::ParametersType           ParametersType;

  typedef   typename CostFunctionType::RequestedIndicesContainer     IndicesContainer;

  typedef   itk::CombinationGOACostFunction<double, ImageDimension>
                                                            CombinationCostFunctionType;
  typedef   typename CombinationCostFunctionType::Pointer   CombinationCostFunctionPointer;


  typedef   typename NonRigidTransformType::Euler3DTransformType    RigidTransformType;
  typedef   typename RigidTransformType::Pointer                    RigidTransformPointer;

  typedef  itk::RescaledGaussianGradientImageFilter<ImageType, VectorImageType>
                                                            GradientFilterType;
  typedef  typename GradientFilterType::Pointer             GradientFilterPointer;

  typedef  itk::RescaledImageFilter<ImageType, ImageType>   RescaleFilterType;
  typedef  typename RescaleFilterType::Pointer              RescalePointer;

  typedef  itk::ResampleImageFilter<ImageType, ImageType>    ResampleFilterType;

  typedef  itk::ResampleImageFilter<MaskImageType, MaskImageType>    MaskResampleFilterType;  

  typedef itk::CannyEdgeDetectionImageFilter2<ImageType, MaskImageType>
                                                            CannyEdgeFilterType;
  typedef typename CannyEdgeFilterType::Pointer             CannyEdgePointer;

  typedef itk::SignedMaurerDistanceMapImageFilter<MaskImageType, ImageType>
                                                            DistanceFilterType;
  typedef typename DistanceFilterType::Pointer              DistanceFilterPointer;


  /** Type for the output: Using Decorator pattern for enabling
   *  the Transform to be passed in the data pipeline */
  typedef  DataObjectDecorator< NonRigidTransformType >      TransformOutputType;
  typedef typename TransformOutputType::Pointer              TransformOutputPointer;
  typedef typename TransformOutputType::ConstPointer         TransformOutputConstPointer;


  /** Initialize by setting the interconnects between the components. */
  void Initialize();

  /** Returns the transform resulting from the registration process  */
  const TransformOutputType * GetOutput() const;

  void SetNumberOfLevels( unsigned int nbrOfLevels );

  void SetNumberOfImages( unsigned int nbrOfImages );



  itkSetListObjectElementMacro(FixedImage, ImageType);
  itkSetListObjectElementMacro(MovingImage, ImageType);
  itkSetListElementMacro(NumberOfVoxels, unsigned int);
  itkSetListElementMacro(NumberOfIterations, unsigned int);
  itkSetListElementMacro(SamplingMode, SamplingModeType);
  itkSetListElementMacro(GradientScale, double);
  itkSetListElementMacro(ImageSpacing, double);
  itkSetListElementMacro(SmoothingSigma, double);
  itkSetListElementMacro(DistanceVariance, double);
  itkSetListElementMacro(KnotSpacing, double);  

  itkSetMacro( Selectivity, unsigned int );

  itkSetMacro( SymmetryEnabled, bool );

  itkSetObjectMacro( RigidTransform, RigidTransformType );

  itkSetMacro( Verbose, bool );

  itkSetObjectMacro( FixedMask, MaskImageType );

  /* Start the Optimization */
  void StartOptimization(void);

  NonRigidTransformPointer GetFinalTransform()
  {
    if( m_TransformList.size() )
      {
        return m_TransformList.back();
      }
    else
      {
        return NULL;
      }
  }

protected:
  SymbaRegistrationMethod();
  virtual ~SymbaRegistrationMethod() {}
  void PrintSelf(std::ostream & os, Indent indent) const;
  /** Method invoked by the pipeline in order to trigger the computation of
   * the registration. */
  void  GenerateData();




  void PreProcessImages(unsigned int level, unsigned int index);
  void InitializeTransform(unsigned int level);
  void InitializeCostFunction(unsigned int level, unsigned int index);

private:
  SymbaRegistrationMethod(const Self &); //purposely not implemented
  void operator=(const Self &);          //purposely not implemented

  bool                                    m_Verbose;

  unsigned int                            m_NumberOfLevels;
  unsigned int                            m_NumberOfImages;

  CombinationCostFunctionPointer            m_CombinationCost;
  std::vector<CostFunctionPointer>          m_CostList;

  GOAOptimizationPointer                    m_Optimizer;

  MaskImagePointer                          m_FixedMask;

  std::vector<ImageConstPointer>            m_FixedImageList;
  std::vector<ImageConstPointer>            m_MovingImageList;

  std::vector<VectorImagePointer>           m_FixedGradientImageList;
  std::vector<VectorImagePointer>           m_MovingGradientImageList;

  std::vector<MaskImagePointer>             m_FixedCannyImageList;
  std::vector<MaskImagePointer>             m_MovingCannyImageList;
  std::vector<ImagePointer>                 m_FixedDistanceImageList;
  std::vector<ImagePointer>                 m_MovingDistanceImageList;

  std::vector<unsigned int>                 m_NumberOfVoxelsList;
  std::vector<unsigned int>                 m_NumberOfIterationsList;
  std::vector<double>                       m_GradientScaleList;
  std::vector<double>                       m_ImageSpacingList;
  std::vector<double>                       m_SmoothingSigmaList;
  std::vector<double>                       m_DistanceVarianceList;
  std::vector<SamplingModeType>             m_SamplingModeList;
  std::vector<double>                       m_KnotSpacingList;
  
  std::vector<IndicesContainer>             m_FixedIndicesList;
  std::vector<IndicesContainer>             m_MovingIndicesList;

  unsigned int                            m_Selectivity;
  bool                                    m_SymmetryEnabled;


  RigidTransformPointer                   m_RigidTransform;
  std::vector<NonRigidTransformPointer>   m_TransformList;





};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkSymbaRegistrationMethod.hxx"
#endif

#endif
