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

#ifndef __itkCombinationGOACostFunction_hxx
#define __itkCombinationGOACostFunction_hxx

#include "itkCombinationGOACostFunction.h"

namespace itk
{

// Constructor with default arguments
template < class TScalarType, unsigned int NInputDimensions>
CombinationGOACostFunction< TScalarType, NInputDimensions >
::CombinationGOACostFunction()
{
  m_CostFunctions.clear();
  m_CostFunctionWeights.clear();
}


// Destructor
template < class TScalarType, unsigned int NInputDimensions>
CombinationGOACostFunction< TScalarType, NInputDimensions >
::~CombinationGOACostFunction()
{

}

// AddCostFunction
template < class TScalarType, unsigned int NInputDimensions>
void 
CombinationGOACostFunction< TScalarType, NInputDimensions >
::AddCostFunction( CostFunctionPointer costFunction, double weight )
{
  m_CostFunctions.push_back( costFunction );
  m_CostFunctionWeights.push_back( weight );
}



//GetValue
template < class TScalarType, unsigned int NInputDimensions>
typename CombinationGOACostFunction< TScalarType, NInputDimensions >::MeasureType
CombinationGOACostFunction< TScalarType, NInputDimensions >
::GetValue(const ParametersType & parameters) const
{
 
  MeasureType measure = 0;
  for (int i = 0; i < m_CostFunctions.size(); ++i)
    {
    //std::cout << "GetValue() Evaluating Cost Function" << std::endl;
    //std::cout << m_CostFunctions[i] << std::endl;
    //std::cout << m_CostFunctionWeights[i] << std::endl;
    //std::cout << m_CostFunctions[i]->GetValue(parameters) << std::endl;
    measure += m_CostFunctionWeights[i] * m_CostFunctions[i]->GetValue( parameters );
    } 
  return measure;
}

//GetDerivative
template < class TScalarType, unsigned int NInputDimensions>
void
CombinationGOACostFunction< TScalarType, NInputDimensions >
::GetDerivative( const ParametersType & parameters, DerivativeType & derivative) const
{

  DerivativeType tmpDerivative = DerivativeType( this->GetNumberOfParameters() );
  derivative.fill(0);
  for (int i = 0; i < m_CostFunctions.size(); ++i)
    {
    m_CostFunctions[i]->GetDerivative( parameters, tmpDerivative );
    derivative += m_CostFunctionWeights[i] * tmpDerivative;
    } 
}


} // namespace


#endif
