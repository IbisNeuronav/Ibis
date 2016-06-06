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

#ifndef __itkGOAGradientAscent_h
#define __itkGOAGradientAscent_h

#include "itkSingleValuedNonLinearOptimizer.h"
#include "itkGradientOrientationAlignmentCostFunction.h"

#include "itkTransformFileWriter.h"

#include "itkImageDuplicator.h"

namespace itk 
{
  template< typename TScalar, unsigned int NInputDimensions = 3 >
  class ITK_EXPORT GOAGradientAscent : public SingleValuedNonLinearOptimizer
  {
  public: 
    /** Standard ITK-stuff. */
    typedef GOAGradientAscent                          Self;
    typedef SingleValuedNonLinearOptimizer      Superclass;
    typedef SmartPointer<Self>                        Pointer;
    typedef SmartPointer<const Self>                  ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );

    /** Run-time type information (and related methods). */
    itkTypeMacro( GOAGradientAscent, SingleValuedNonLinearOptimizer );

    /** Spatial Dimension */
    itkStaticConstMacro(SpaceDimension, unsigned int, NInputDimensions);
 
    /** Number of Coefficients */
    itkStaticConstMacro(NumberOfRigidCoefficients, unsigned int, SpaceDimension );
  
    typedef Superclass::Pointer                       SingleValuedNonLinearOptimizerPointer;

    typedef itk::GradientOrientationAlignmentCostFunction<TScalar, NInputDimensions>
                                                      CostFunctionType;
    typedef typename CostFunctionType::Pointer        CostFunctionPointer;

    typedef typename CostFunctionType::ParametersType
                                                      ParametersType;
    typedef typename CostFunctionType::DerivativeType
                                                      DerivativeType;                                                      

    typedef typename CostFunctionType::VectorImageType  VectorImageType;
    typedef typename VectorImageType::Pointer           VectorImagePointer;

    typedef typename CostFunctionType::TransformType
                                                      TransformType;
    typedef typename TransformType::Pointer           TransformPointer;

    typedef typename CostFunctionType::RequestedIndicesContainer 
                                                      IndicesContainer;

    typedef std::vector< std::vector< ParametersType> >
                                                      LoggedParametersContainer;

    typedef itk::ImageDuplicator<VectorImageType>     VectorImageDuplicator;

    itkSetMacro( InitialParameters, ParametersType );

    itkGetMacro( OptimizedParameters, ParametersType );

    itkSetMacro( Verbose, bool );
    itkSetMacro( NumberOfIterations, unsigned int );
    itkSetMacro( NumberOfNodeIterations, unsigned int );

    itkSetObjectMacro( CostFunction, CostFunctionType );
    itkGetObjectMacro( CostFunction, CostFunctionType );

    itkSetMacro( ValueTolerance, double );


    typedef enum SamplingModeType
    {
       FullSampling,
       SubsetSampling,
       StochasticSubsetSampling
    } SamplingModeType;


    itkSetMacro( SamplingMode, SamplingModeType );

    itkSetMacro( NumberOfSamples, unsigned int );

    void StartOptimization();

    IndicesContainer RandomSelection( IndicesContainer fullSet, unsigned int nbrOfSamples );

   protected:
       /** The constructor. */
    GOAGradientAscent();
    /** The destructor. */
    virtual ~GOAGradientAscent();

    /** PrintSelf. */
    void PrintSelf( std::ostream & os, Indent indent ) const;

    ParametersType OptimizeNodeParameters( unsigned int nodeId );
    ParametersType OptimizeNodes( std::vector<unsigned int> nodes, unsigned int threadId );


  private:
    CostFunctionPointer                             m_CostFunction;
    ParametersType                                  m_OptimizedParameters;
    ParametersType                                  m_InitialParameters;
    bool                                            m_Verbose;
    ScalesType                                      m_OptimizerScales;
    unsigned int                                    m_NumberOfThreads;
    unsigned int                                    m_NumberOfIterations;
    unsigned int                                    m_NumberOfNodeIterations;
    double                                          m_ValueTolerance;
    unsigned int                                    m_NumberOfSamples;
    SamplingModeType                                m_SamplingMode;

  };
}

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGOAGradientAscent.hxx"
#endif

#endif
