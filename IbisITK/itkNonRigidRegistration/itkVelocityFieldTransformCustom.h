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

#ifndef __itkVelocityFieldTransformCustom_h
#define __itkVelocityFieldTransformCustom_h

#include "itkVelocityFieldTransformCustom.h"
#include "itkEuler3DTransform.h"
#include "itkTransform.h"
#include "itkImage.h"
#include "itkVectorNearestNeighborInterpolateImageFunction.h"
#include "itkVectorLinearInterpolateImageFunction2.h"

#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkMatrix.h"
#include "vnl_sd_matrix_tools.h"
#include "itkImageRegionConstIterator.h"

#include "itkDisplacementFieldTransform.h"
#include "itkTransformFileWriter.h"
#include "itkImageFileWriter.h"

namespace itk
{

template <class TScalarType = double, unsigned int NDimensions = 3,
          unsigned int VSplineOrder = 3>
class ITK_EXPORT VelocityFieldTransformCustom :
    public Transform<TScalarType, NDimensions, NDimensions>
{
public:
  /** Standard class typedefs. */
  typedef VelocityFieldTransformCustom                           Self;
  typedef Transform<TScalarType, NDimensions, NDimensions>           Superclass;
  typedef SmartPointer<Self>                                         Pointer;
  typedef SmartPointer<const Self>                                   ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro( VelocityFieldTransformCustom, Transform );

  /** New macro for creation of through the object factory. */
  itkNewMacro( Self );

  /** Dimension of the domain space. */
  itkStaticConstMacro( SpaceDimension, unsigned int, NDimensions );

  /** The BSpline order. */
  itkStaticConstMacro( SplineOrder, unsigned int, VSplineOrder );


  typedef typename Superclass::ScalarType             ScalarType;
  typedef typename Superclass::ParametersType         ParametersType;
  typedef typename Superclass::ParametersValueType    ParametersValueType;
  typedef typename Superclass::JacobianType           JacobianType;
  typedef typename Superclass::NumberOfParametersType NumberOfParametersType;
  typedef typename Superclass::InputVectorType        InputVectorType;
  typedef typename Superclass::OutputVectorType       OutputVectorType;
  typedef typename Superclass::Pointer                SuperclassPointer;

  typedef typename Superclass::InputCovariantVectorType  InputCovariantVectorType;
  typedef typename Superclass::OutputCovariantVectorType OutputCovariantVectorType;

  typedef typename Superclass::InputVnlVectorType       InputVnlVectorType;
  typedef typename Superclass::OutputVnlVectorType      OutputVnlVectorType;

  typedef typename Superclass::InputPointType  InputPointType;
  typedef typename Superclass::OutputPointType OutputPointType;

  /** Parameter index array type. */
  typedef Array<unsigned long>                         ParameterIndexArrayType;

  //typedef OutputPointType           VelocityType;
  typedef Vector<ScalarType, SpaceDimension>           VelocityType;
  typedef Image<VelocityType, itkGetStaticConstMacro( SpaceDimension )>
                                                        VelocityImageType;
  typedef typename VelocityImageType::Pointer           VelocityImagePointer;
  typedef typename VelocityImageType::RegionType        VelocityImageRegionType;
  typedef typename VelocityImageType::RegionType        RegionType;  
  typedef typename VelocityImageType::SpacingType       SpacingType;
  typedef typename VelocityImageType::SizeType          SizeType;    
  typedef typename VelocityImageType::IndexType         IndexType;     
  typedef typename VelocityImageType::DirectionType     DirectionType;  

  typedef ImageRegionConstIterator<VelocityImageType>         VelocityConstIteratorType;
  typedef ImageRegionIterator<VelocityImageType>              VelocityIteratorType;  


  typedef Image<OutputPointType, itkGetStaticConstMacro( SpaceDimension )>
                                                       DisplacementImageType;
  typedef typename DisplacementImageType::Pointer      DisplacementImagePointer;
  typedef ImageRegionIteratorWithIndex< DisplacementImageType > DispImageIteratorType;

  typedef ImageBase< itkGetStaticConstMacro( SpaceDimension )> 
                                                       ImageBaseType;
  typedef typename ImageBaseType::Pointer              ImageBasePointer;


  typedef DisplacementFieldTransform<TScalarType, SpaceDimension>
                                                                  DisplacementFieldTransformType;
  typedef typename DisplacementFieldTransformType::Pointer        DisplacementFieldTransformPointer;  
  typedef typename DisplacementFieldTransformType::DisplacementFieldType        DisplacementFieldType;  
  typedef typename DisplacementFieldTransformType::DisplacementFieldPointer     DisplacementFieldPointer;    

  typedef Euler3DTransform<ScalarType>                 Euler3DTransformType;
  typedef typename Euler3DTransformType::Pointer       Euler3DTransformPointer;

  typedef ImageRegionIteratorWithIndex< DisplacementFieldType > DispFieldIteratorType;

  typedef TransformFileWriter                              TransformWriterType;
  typedef ImageFileWriter< DisplacementFieldType >         DisplacementFieldImageWriterType;

  /** Smoothing functions */
  typedef SmoothingRecursiveGaussianImageFilter< VelocityImageType > SmoothingFilterType;


  /** Interpolation weights function type. */
  typedef VectorLinearInterpolateImageFunction2< VelocityImageType > VectorInterpolationFunctionType;
  typedef typename VectorInterpolationFunctionType::WeightsContainer WeightsContainer;
  typedef typename VectorInterpolationFunctionType::IndicesContainer IndicesContainer;

  typedef VectorLinearInterpolateImageFunction< DisplacementImageType, ScalarType> DisplacementInterpolationFunctionType;


  itkSetObjectMacro( RigidTransform, Euler3DTransformType );
  itkGetObjectMacro( RigidTransform, Euler3DTransformType );

  itkGetObjectMacro( VelocityImage, VelocityImageType );
  void SetVelocityImage( VelocityImageType * image )
  {
    m_VelocityImage->SetOrigin( image->GetOrigin() );
    m_VelocityImage->SetRegions( image->GetLargestPossibleRegion() );
    m_VelocityImage->SetDirection( image->GetDirection() );
    m_VelocityImage->SetSpacing( image->GetSpacing() );
    m_VelocityImage->Allocate();

    VelocityConstIteratorType refIt = VelocityConstIteratorType( image, image->GetLargestPossibleRegion() );
    VelocityIteratorType tarIt = VelocityIteratorType( m_VelocityImage, m_VelocityImage->GetLargestPossibleRegion() );

    while( !refIt.IsAtEnd() )
    {
      tarIt.Set( refIt.Get() );
      ++refIt;
      ++tarIt;      
    }
    m_VectorInterpolationFunction->SetInputImage( m_VelocityImage );
  }

  void  EvaluateDisplacementField( ImageBasePointer referenceImage, DisplacementFieldPointer displacementImage );
  void  WriteDisplacementFieldTransform(  ImageBasePointer referenceImage, const std::string filename );
  void  WriteDisplacementFieldImage(  ImageBasePointer referenceImage, const std::string filename );

  void SetFixedParameters( const ParametersType & parameters );
  void SetParameters( const ParametersType & parameters );
  void SetIdentity();

  /** Get the Transformation Parameters. */
  const ParametersType & GetParameters(void) const
  {
    return * m_InputParametersPointer;
  }


  unsigned int GetNumberOfNodes()
  {
    return this->m_VelocityImage->GetLargestPossibleRegion().GetNumberOfPixels();
  }

  OutputPointType  TransformPoint( const InputPointType & point ) const;  
  OutputPointType  InverseTransformPoint( const InputPointType & point ) const;


  /** Transform Orientation Vector */
  OutputCovariantVectorType TransformOrientationVector(const InputPointType &, const InputCovariantVectorType &) const;
  OutputCovariantVectorType InverseTransformOrientationVector(const InputPointType &, const InputCovariantVectorType &) const;

  /** Transform Orientation Vector */
  void TransformPointAndOrientationVector(const InputPointType & point, const InputCovariantVectorType & inputVector, OutputPointType & outputPoint, OutputCovariantVectorType & outputVector) const;
  void InverseTransformPointAndOrientationVector(const InputPointType & point, const InputCovariantVectorType & inputVector, OutputPointType & outputPoint, OutputCovariantVectorType & outputVector) const;

  /** Method to transform a vector -
   *  not applicable for this type of transform. */
  using Superclass::TransformVector;
  virtual OutputVectorType TransformVector( const InputVectorType & ) const
  {
    itkExceptionMacro( << "Method not applicable for deformable transform." );
    return OutputVectorType();
  }

  /** Method to transform a vnl_vector -
   *  not applicable for this type of transform */
  virtual OutputVnlVectorType TransformVector( const InputVnlVectorType & ) const
  {
    itkExceptionMacro( << "Method not applicable for deformable transform. " );
    return OutputVnlVectorType();
  }

  /** Method to transform a CovariantVector -
   *  not applicable for this type of transform */
  using Superclass::TransformCovariantVector;
  virtual OutputCovariantVectorType TransformCovariantVector(
    const InputCovariantVectorType & ) const
  {
    itkExceptionMacro( << "Method not applicable for deformable transfrom. " );
    return OutputCovariantVectorType();
  }

  void ComputeJacobianWithRespectToParameters( const InputPointType &, std::map<unsigned int,ScalarType> & ) const;
  void ComputeInverseJacobianWithRespectToParameters( const InputPointType &, std::map<unsigned int,ScalarType> & ) const;

  void ComputeJacobianWithRespectToParameters( const InputPointType &, std::vector< std::pair<unsigned int,ScalarType> > & ) const;
  void ComputeInverseJacobianWithRespectToParameters( const InputPointType &, std::vector< std::pair<unsigned int,ScalarType> > & ) const;


  /** Return the number of parameters that completely define the Transfom */
  NumberOfParametersType GetNumberOfParameters() const;

  /** Return the number of parameters per dimension */
  NumberOfParametersType GetNumberOfParametersPerDimension() const;

  itkGetMacro( NumberOfSquareCompositions, unsigned int );
  void SetNumberOfSquareCompositions( unsigned int number )
  {
      m_NumberOfSquareCompositions = number;
      m_Pow2N = pow(2, m_NumberOfSquareCompositions);
  }


  InputPointType GetNodeLocation( unsigned int nodeId )
  {
    InputPointType point;
    m_VelocityImage->TransformIndexToPhysicalPoint( m_VelocityImage->ComputeIndex(nodeId), point );
    return point;
  }

  SpacingType GetNodeSpacing()
  {
    return m_VelocityImage->GetSpacing();
  }

  SizeType GetNodeImageSize()
  {
    return m_VelocityImage->GetLargestPossibleRegion().GetSize();
  }

  void UpdateNodeParameters( unsigned int nodeId, const ParametersType & nodeParameters );
  
  ParametersType UpdateParameters( ParametersType & parametersGradient, double weight );

  ParametersType InitializeVelocityField( Pointer referenceTransform );

  void Duplicate( Pointer originalTransform )
  {
    this->SetNumberOfSquareCompositions( originalTransform->GetNumberOfSquareCompositions() );
    this->SetFixedParameters( originalTransform->GetFixedParameters() );
    this->SetVelocityImage( originalTransform->GetVelocityImage() );
    this->SetSmoothingSigma( originalTransform->GetSmoothingSigma() );
    this->SetRigidTransform( originalTransform->GetRigidTransform() );

  }

  void ComputeVelocity( const InputPointType & point, VelocityType & velocity ) const;

  void ComputeVelocity( const InputPointType & point, VelocityType & velocity, WeightsContainer & weights, IndicesContainer & indices ) const;

  itkGetMacro( SmoothingSigma, double );
  itkSetMacro( SmoothingSigma, double );

  itkGetMacro( IntegrationEndTime, double );
  itkSetMacro( IntegrationEndTime, double );

  void InvertTransform()
  {

    VelocityIteratorType velocityIt = VelocityIteratorType( m_VelocityImage, m_VelocityImage->GetLargestPossibleRegion() );

    while( !velocityIt.IsAtEnd() )
    {
      velocityIt.Set( -velocityIt.Get() );
      ++velocityIt;      
    }
    m_VectorInterpolationFunction->SetInputImage( m_VelocityImage );    

    // TODO: IF RIGID TRANSFORM
  }

protected:
  VelocityFieldTransformCustom();
  virtual ~VelocityFieldTransformCustom();

  /** Print contents of an VelocityFieldTransformCustom.h. */
  void PrintSelf( std::ostream & os, Indent indent ) const;

  /** Wrap flat array into images of coefficients. */
  void WrapAsImages();
  void WrapAsVelocityImage( const ParametersType & parameters, VelocityImagePointer velocityImage );
  void UnwrapAsParameters( VelocityImagePointer velocityImage,  ParametersType & parameters );
  

  OutputPointType TransformPointDelta( const OutputPointType & point ) const;  
  OutputPointType InverseTransformPointDelta( const OutputPointType & point ) const;


  OutputPointType ComputeRecursiveTransform( const InputPointType & point ) const;
  OutputPointType ComputeInverseRecursiveTransform( const InputPointType & point ) const;

  void ComputeInverseRecursiveJacobian(const InputPointType & point, std::map<unsigned int,ScalarType> & ) const;
  void ComputeRecursiveJacobian(const InputPointType & point, std::map<unsigned int,ScalarType> &) const;

  void ComputeInverseRecursiveJacobian(const InputPointType & point, std::vector< std::pair< unsigned int,ScalarType> > & ) const;
  void ComputeRecursiveJacobian(const InputPointType & point, std::vector< std::pair<unsigned int,ScalarType> > &) const;


  OutputPointType AccumulateInverseJacobian(const OutputPointType & point, std::map<unsigned int,ScalarType> &) const;
  OutputPointType AccumulateJacobian(const OutputPointType & point, std::map<unsigned int,ScalarType> &) const;  

  bool IsInside(const InputPointType & point) const;

private:

  /** Keep a pointer to the input parameters. */
  const ParametersType *m_InputParametersPointer;

  /** Internal parameters buffer. */
  ParametersType m_InternalParametersBuffer;

  /** Pointer to function used to compute Bspline interpolation weights. */
  typename VectorInterpolationFunctionType::Pointer m_VectorInterpolationFunction;

  /** Velocity Image **/
  VelocityImagePointer    m_VelocityImage;
  VelocityImagePointer    m_GradientVelocityImage;

  unsigned int      m_NumberOfSquareCompositions;
  double            m_Pow2N;

  typename SmoothingFilterType::Pointer m_SmoothingFilter;
  double                                m_SmoothingSigma;

  Euler3DTransformPointer               m_RigidTransform;

  double                                m_IntegrationEndTime;

}; // class VelocityFieldTransformCustom.h
}  // namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkVelocityFieldTransformCustom.hxx"
#endif

#endif /* __itkVelocityFieldTransformCustom_h */
