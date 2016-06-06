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

#ifndef __itkCombinationGOACostFunction_h
#define __itkCombinationGOACostFunction_h

#include "itkGradientOrientationAlignmentCostFunction.h"


namespace itk
{

template < typename TScalar, unsigned int NInputDimensions = 3 >
class ITK_EXPORT CombinationGOACostFunction :
  public GradientOrientationAlignmentCostFunction<TScalar, NInputDimensions>
{
public:

  itkStaticConstMacro(SpaceDimension, unsigned int, NInputDimensions);

  /** Standard class typedefs. */
  typedef CombinationGOACostFunction         				    Self;
  typedef GradientOrientationAlignmentCostFunction<TScalar, NInputDimensions>    Superclass;
  typedef SmartPointer<Self>                          Pointer;
  typedef SmartPointer<const Self>                    ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(CombinationGOACostFunction, GradientOrientationAlignmentCostFunction);

  /** implement type-specific clone method*/
  itkNewMacro(Self);

  typedef itk::GradientOrientationAlignmentCostFunction<TScalar, NInputDimensions>
                                                  CostFunctionType;
  typedef typename CostFunctionType::Pointer               CostFunctionPointer;


  typedef typename Superclass::ParametersType     ParametersType;
  typedef typename Superclass::MeasureType        MeasureType;
  typedef typename Superclass::DerivativeType     DerivativeType;  

  //Cost Function Methods
  virtual MeasureType GetValue(const ParametersType & parameters) const;
  MeasureType GetValueFullParameters(const ParametersType & parameters) const;
  void GetDerivative( const ParametersType & parameters, DerivativeType & derivative) const;

  void AddCostFunction( CostFunctionPointer costFunction, double weight );

  virtual unsigned int GetNumberOfParameters() const 
  {
    if( m_CostFunctions.size() > 0 )
    {
      return m_CostFunctions[0]->GetNumberOfParameters();
    }
    else
    {
      return 0;
    }
  }

  typename CostFunctionType::TransformType * GetTransform()
  {
    if( m_CostFunctions.size() > 0 )
    {
      return m_CostFunctions[0]->GetTransform();
    }
    else
    {
      return NULL;
    }
  }

protected:
  CombinationGOACostFunction();
  virtual ~CombinationGOACostFunction();
  // void PrintSelf(std::ostream & os, Indent indent) const;


private:
  CombinationGOACostFunction(const Self &); // purposely not implemented
  void operator=(const Self &);   // purposely not implemented

  std::vector<CostFunctionPointer>              m_CostFunctions;
  std::vector<double>                           m_CostFunctionWeights;                                                

};
}

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkCombinationGOACostFunction.hxx"
#endif


#endif