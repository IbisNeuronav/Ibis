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

#ifndef __itkGradientOrientationAlignmentCostFunction_h
#define __itkGradientOrientationAlignmentCostFunction_h

#include "itkSingleValuedCostFunction.h"
#include "itkVelocityFieldTransformCustom.h"
#include "itkImage.h"
#include "itkCovariantVector.h"
#include "itkVectorLinearInterpolateImageFunction.h"
#include "itkGradientImageFilter.h"
#include "itkGradientOrientationDifferenceFilter.h"
#include "itkMultiThreader.h"


namespace itk
{

template < typename TScalar, unsigned int NInputDimensions = 3 >
class ITK_EXPORT GradientOrientationAlignmentCostFunction :
  public SingleValuedCostFunction
{
public:
  itkStaticConstMacro(SpaceDimension, unsigned int, NInputDimensions);

  /** Standard class typedefs. */
  typedef GradientOrientationAlignmentCostFunction         Self;
  typedef SingleValuedCostFunction                        Superclass;
  typedef SmartPointer<Self>                               Pointer;
  typedef SmartPointer<const Self>                         ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(GradientOrientationAlignmentCostFunction, SingleValuedCostFunction);

  /** implement type-specific clone method*/
  itkNewMacro(Self);


  /** Number of Rigid Coefficients */
  itkStaticConstMacro(NumberOfRigidCoefficients, unsigned int, SpaceDimension );

  /** Type of the scalar representing coordinate and vector elements. */
  typedef  TScalar ScalarType;

  /** Some convenient typedefs. */
  typedef CovariantVector<ScalarType, SpaceDimension>      VectorType;
  typedef Image<VectorType, SpaceDimension>       VectorImageType;
  typedef typename VectorImageType::Pointer       VectorImagePointer;
  typedef typename VectorImageType::ConstPointer  VectorImageConstPointer;
  typedef typename VectorImageType::RegionType    VectorImageRegionType;
  typedef typename VectorImageType::PointType     PointType;
  typedef typename VectorImageType::IndexType     IndexType;

  typedef Image<ScalarType, SpaceDimension>         DistanceImageType;
  typedef typename DistanceImageType::Pointer       DistanceImagePointer;
  typedef typename DistanceImageType::ConstPointer  DistanceImageConstPointer;


  typedef typename Superclass::ParametersType     ParametersType;
  typedef typename Superclass::MeasureType        MeasureType;
  typedef typename Superclass::DerivativeType     DerivativeType;

  typedef VelocityFieldTransformCustom<ScalarType,
                          SpaceDimension>         TransformType;
  typedef typename TransformType::Pointer         TransformPointer;

  typedef std::vector<IndexType>                 RequestedIndicesContainer;


  typedef VectorLinearInterpolateImageFunction<VectorImageType>
                                                  VectorInterpolatorType;
  typedef typename VectorInterpolatorType::Pointer    VectorInterpolatorPointer;

  typedef LinearInterpolateImageFunction<DistanceImageType>
                                                  DistanceInterpolatorType;
  typedef typename DistanceInterpolatorType::Pointer    DistanceInterpolatorPointer;  

  typedef GradientImageFilter< DistanceImageType > 
                                                        GradientImageFilterType;
  typedef typename GradientImageFilterType::Pointer     GradientImageFilterPointer;
  typedef typename GradientImageFilterType::OutputImageType
                                                        DistanceGradientImageType;
  typedef typename DistanceGradientImageType::PixelType DistanceGradientType;   
  
  typedef VectorLinearInterpolateImageFunction<DistanceGradientImageType>
                                                  DistanceGradientInterpolatorType;
  typedef typename DistanceGradientInterpolatorType::Pointer    DistanceGradientInterpolatorPointer;                                                       


  typedef GradientOrientationDifferenceFilter< VectorImageType > OrientationGradientFilterType;
  typedef typename OrientationGradientFilterType::Pointer        OrientationGradientFilterPointer;
 

  //GetSet Methods
  itkSetMacro( FiniteDifference, bool );
  itkGetMacro( FiniteDifference, bool );

  itkSetMacro( N, unsigned int );
  itkGetMacro( N, unsigned int );

  itkSetMacro( DistanceVariance, double );
  itkGetMacro( DistanceVariance, double );  

  itkGetObjectMacro( FixedVectorImage, VectorImageType );
  void SetFixedVectorImage(VectorImagePointer vectorImage)
  {
    m_FixedVectorImage = vectorImage;
    m_FixedVectorInterpolator->SetInputImage(m_FixedVectorImage);

    // m_FixedOrientationGradient->SetInput( m_FixedVectorImage );
    // m_FixedOrientationGradient->Update();
    // m_FixedThetaGradientInterpolator->SetInputImage( m_FixedOrientationGradient->GetOutput1() );
    // m_FixedPhiGradientInterpolator->SetInputImage( m_FixedOrientationGradient->GetOutput2() );
  }

  itkGetObjectMacro( MovingVectorImage, VectorImageType );
  void SetMovingVectorImage(VectorImagePointer vectorImage)
  {
    m_MovingVectorImage = vectorImage;
    m_MovingVectorInterpolator->SetInputImage(m_MovingVectorImage);

    // m_MovingOrientationGradient->SetInput( m_MovingVectorImage );
    // m_MovingOrientationGradient->Update();
    // m_MovingThetaGradientInterpolator->SetInputImage( m_MovingOrientationGradient->GetOutput1() );
    // m_MovingPhiGradientInterpolator->SetInputImage( m_MovingOrientationGradient->GetOutput2() );    
  }

  itkGetObjectMacro( FixedDistanceImage, DistanceImageType );
  void SetFixedDistanceImage(DistanceImagePointer distanceImage)
  {
    m_FixedDistanceImage = distanceImage;
    m_FixedDistanceInterpolator->SetInputImage(m_FixedDistanceImage);
    m_FixedDistanceGradient->SetInput( m_FixedDistanceImage );
    m_FixedDistanceGradient->Update();
    m_FixedDistanceGradientInterpolator->SetInputImage( m_FixedDistanceGradient->GetOutput() );
  }

  itkGetObjectMacro( MovingDistanceImage, DistanceImageType );
  void SetMovingDistanceImage(DistanceImagePointer distanceImage)
  {
    m_MovingDistanceImage = distanceImage;
    m_MovingDistanceInterpolator->SetInputImage(m_MovingDistanceImage);
    m_MovingDistanceGradient->SetInput( m_MovingDistanceImage );
    m_MovingDistanceGradient->Update();
    m_MovingDistanceGradientInterpolator->SetInputImage( m_MovingDistanceGradient->GetOutput() );    
  }


  itkSetObjectMacro(Transform, TransformType);
  itkGetObjectMacro(Transform, TransformType);

  unsigned int GetNumberOfNodes() const 
  {
    return m_Transform->GetNumberOfNodes();
  }

  //itkGetMacro(FixedIndices, RequestedIndicesContainer);
  virtual RequestedIndicesContainer GetFixedIndices()
  {
    return m_FixedIndices;  
  }


  virtual void SetFixedIndices(RequestedIndicesContainer indices)
  {
    m_FixedIndices.resize( indices.size() );
    typename RequestedIndicesContainer::iterator inputIterator = indices.begin();
    typename RequestedIndicesContainer::iterator outputIterator = m_FixedIndices.begin();
    while(inputIterator != indices.end() && outputIterator != m_FixedIndices.end())
      {
        (*outputIterator) = (*inputIterator);
        ++inputIterator;
        ++outputIterator;
      }
  }

  // itkGetMacro(MovingIndices, RequestedIndicesContainer);
  virtual RequestedIndicesContainer GetMovingIndices()
  {
    return m_MovingIndices;  
  }
  
  virtual void SetMovingIndices(RequestedIndicesContainer indices)
  {
    m_MovingIndices.resize( indices.size() );
    typename RequestedIndicesContainer::iterator inputIterator = indices.begin();
    typename RequestedIndicesContainer::iterator outputIterator = m_MovingIndices.begin();
    while(inputIterator != indices.end() && outputIterator != m_MovingIndices.end())
      {
        (*outputIterator) = (*inputIterator);
        ++inputIterator;
        ++outputIterator;
      }
  }

  virtual unsigned int GetNumberOfParameters() const 
  {
    return GetNumberOfNodes() * NumberOfRigidCoefficients;
  }

  ParametersType GetTransformParameters() const
  {
    ParametersType parameters =  ParametersType( m_Transform->GetParameters() );
    return parameters;
  }

  virtual void SetTransformParameters( ParametersType parameters )
  {
   m_Transform->SetParameters(parameters);
  }

  void UpdateNodeParameters( unsigned int nodeId, const ParametersType & parameters ) const
  {
    m_Transform->UpdateNodeParameters( nodeId, parameters );
  }

  //Cost Function Methods
  virtual MeasureType GetValue(const ParametersType & parameters) const;
  MeasureType GetValueFullParameters(const ParametersType & parameters) const;
  void GetDerivative( const ParametersType & parameters, DerivativeType & derivative) const;
  void GetValueAndDerivative( const ParametersType & parameters, MeasureType & value, DerivativeType & derivative) const { }

  

  //Multi-Threading 
  typedef MultiThreader MultiThreaderType;
//  /** Get the Threader. */
//  itkGetModifiableObjectMacro(Threader, MultiThreaderType);

  /** Set/Get number of threads to use for computations. */
  void SetNumberOfThreads(ThreadIdType numberOfThreads);
  itkGetConstReferenceMacro(NumberOfThreads, ThreadIdType);

  struct MultiThreaderParameterType {
    Self *metric;
  };

  /**
   *  Initialize the Metric by
   *  (1) making sure that all the components are present and plugged
   *      together correctly,
   *  (2) uniformly select NumberOfSpatialSamples within
   *      the FixedImageRegion, and
   *  (3) allocate memory for pdf data structures. */
  void Initialize(void)
  throw ( ExceptionObject );

protected:
  GradientOrientationAlignmentCostFunction();
  virtual ~GradientOrientationAlignmentCostFunction();
  void PrintSelf(std::ostream & os, Indent indent) const;

  MeasureType ComputeValue() const;
  bool EvaluateMetricValue(IndexType imageIndex, MeasureType & metricValue, TransformPointer transform ) const;
  bool EvaluateMovingIndexMetricValue(IndexType imageIndex, MeasureType & metricValue, TransformPointer transform) const;

  void ComputeIndicesValueThread(unsigned int threadId) const;
  void ComputeIndicesDerivativeThread(unsigned int threadId) const;


  ParametersType ComputeDerivative() const;
  bool EvaluateMetricGradient(IndexType imageIndex, ParametersType & metricGradient, TransformPointer transform) const;
  bool EvaluateMovingIndexMetricGradient(IndexType imageIndex, ParametersType & metricGradient, TransformPointer transform) const;  

  void EvaluateFixedVectorMetricValue(VectorType fixedVector, PointType movingPoint, MeasureType & metricValue ) const;
  void EvaluateMovingVectorMetricValue(VectorType movingVector, PointType fixedPoint, MeasureType & metricValue ) const;

  void MultiThreadingInitialize(void) throw ( ExceptionObject );
  static ITK_THREAD_RETURN_TYPE  GetValueMultiThreaded(void *arg);
  static ITK_THREAD_RETURN_TYPE  GetDerivativeMultiThreaded(void *arg);

private:
  GradientOrientationAlignmentCostFunction(const Self &); // purposely not implemented
  void operator=(const Self &);   // purposely not implemented

  VectorImagePointer              m_FixedVectorImage;
  VectorImagePointer              m_MovingVectorImage;
  RequestedIndicesContainer       m_FixedIndices;
  RequestedIndicesContainer       m_MovingIndices;
  TransformPointer              m_Transform;
  VectorInterpolatorPointer     m_FixedVectorInterpolator;
  VectorInterpolatorPointer     m_MovingVectorInterpolator;
  unsigned int                  m_N;

  DistanceImagePointer              m_FixedDistanceImage;
  DistanceImagePointer              m_MovingDistanceImage;
  DistanceInterpolatorPointer       m_FixedDistanceInterpolator;
  DistanceInterpolatorPointer       m_MovingDistanceInterpolator;  

  GradientImageFilterPointer        m_FixedDistanceGradient;
  GradientImageFilterPointer        m_MovingDistanceGradient;
  DistanceGradientInterpolatorPointer         m_FixedDistanceGradientInterpolator;
  DistanceGradientInterpolatorPointer         m_MovingDistanceGradientInterpolator;  

  OrientationGradientFilterPointer  m_FixedOrientationGradient;
  VectorInterpolatorPointer         m_FixedThetaGradientInterpolator;
  VectorInterpolatorPointer         m_FixedPhiGradientInterpolator; 

  OrientationGradientFilterPointer  m_MovingOrientationGradient;
  VectorInterpolatorPointer         m_MovingThetaGradientInterpolator;
  VectorInterpolatorPointer         m_MovingPhiGradientInterpolator; 

  double                            m_DistanceVariance;
  unsigned int                      m_NumberOfThreads; 
  std::vector<TransformPointer>     m_ThreaderTransform;


  MultiThreaderType::Pointer        m_Threader;
  MultiThreaderParameterType        m_ThreaderParameter;  


  bool                              m_FiniteDifference;

  struct PerThreadS
  {
    ParametersType        fixedGradient;
    MeasureType           fixedValue;
    unsigned int          fixedValidCount;
    ParametersType        movingGradient;
    MeasureType           movingValue;
    unsigned int          movingValidCount;    
  };

//  itkAlignedTypedef( 64, PerThreadS, AlignedPerThreadType );
  typedef PerThreadS AlignedPerThreadType;
  AlignedPerThreadType * m_PerThread;



};
}

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGradientOrientationAlignmentCostFunction.hxx"
#endif


#endif
