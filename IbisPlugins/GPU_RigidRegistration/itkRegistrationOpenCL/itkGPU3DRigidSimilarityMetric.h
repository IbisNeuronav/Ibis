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

#include <itkEuler3DTransform.h>
#include <itkSingleValuedCostFunction.h>
#include "itkGPUOrientationMatchingMatrixTransformationSparseMask.h"

namespace itk
{

template< class TFixedImage, class TMovingImage>
class GPU3DRigidSimilarityMetric : public itk::SingleValuedCostFunction
{
public:


  typedef GPU3DRigidSimilarityMetric        Self;
  typedef itk::SingleValuedCostFunction     Superclass;
  typedef itk::SmartPointer<Self>           Pointer;
  typedef itk::SmartPointer<const Self>     ConstPointer;
  itkNewMacro( Self );
  itkTypeMacro( GPU3DRigidSimilarityMetric, SingleValuedCostFunction );

  itkSetMacro(Debug, bool);

  enum { SpaceDimension=6 };

  typedef Superclass::ParametersType              ParametersType;
  typedef Superclass::DerivativeType              DerivativeType;
  typedef Superclass::MeasureType                 MeasureType;

  typedef vnl_vector<double>                      VectorType;
  
  typedef itk::GPUOrientationMatchingMatrixTransformationSparseMask<
  			TFixedImage, TMovingImage>		                      GPUMetricType;
  typedef typename GPUMetricType::Pointer                   GPUMetricPointer;  												  
  typedef typename GPUMetricType::MatrixTransformType       MetricTransformType; 
  typedef typename MetricTransformType::Pointer             MetricTransformPointer;


  typedef itk::Euler3DTransform<double>    	                EulerTransformType;  		  
  typedef EulerTransformType::Pointer                       EulerTransformPointer;
  typedef EulerTransformType::InputPointType                PointType;
  
  itkSetObjectMacro(GPUMetric, GPUMetricType);

  GPU3DRigidSimilarityMetric()
   {
    m_GPUMetric = NULL;    
    m_EulerTransform = EulerTransformType::New();
    m_Debug = true;
    }

  double GetValue( const ParametersType & parameters ) const override
    {
   	if(!m_GPUMetric)
   		itkExceptionMacro(<<"GPUMetric has not been set!");

    PointType center;
    center[0] = m_GPUMetric->GetFixedImage()->GetOrigin()[0] + m_GPUMetric->GetFixedImage()->GetSpacing()[0] * m_GPUMetric->GetFixedImage()->GetBufferedRegion().GetSize()[0] / 2.0;
    center[1] = m_GPUMetric->GetFixedImage()->GetOrigin()[1] + m_GPUMetric->GetFixedImage()->GetSpacing()[1] * m_GPUMetric->GetFixedImage()->GetBufferedRegion().GetSize()[1] / 2.0;
    center[2] = m_GPUMetric->GetFixedImage()->GetOrigin()[2] + m_GPUMetric->GetFixedImage()->GetSpacing()[2] * m_GPUMetric->GetFixedImage()->GetBufferedRegion().GetSize()[2] / 2.0;  

   	m_EulerTransform->SetCenter(center);
   	m_EulerTransform->SetParameters( parameters );

   	MetricTransformPointer tempTransform = MetricTransformType::New();
   	tempTransform->SetMatrix(m_EulerTransform->GetMatrix());
   	tempTransform->SetOffset(m_EulerTransform->GetOffset());

   	m_GPUMetric->SetTransform(tempTransform);
    m_GPUMetric->Update();	

    return -m_GPUMetric->GetMetricValue();
    }

  PointType GetCenter(void) const
    {
      PointType temp;
      temp[0] = m_GPUMetric->GetFixedImage()->GetOrigin()[0] + m_GPUMetric->GetFixedImage()->GetSpacing()[0] * m_GPUMetric->GetFixedImage()->GetBufferedRegion().GetSize()[0] / 2.0;
      temp[1] = m_GPUMetric->GetFixedImage()->GetOrigin()[1] + m_GPUMetric->GetFixedImage()->GetSpacing()[1] * m_GPUMetric->GetFixedImage()->GetBufferedRegion().GetSize()[1] / 2.0;
      temp[2] = m_GPUMetric->GetFixedImage()->GetOrigin()[2] + m_GPUMetric->GetFixedImage()->GetSpacing()[2] * m_GPUMetric->GetFixedImage()->GetBufferedRegion().GetSize()[2] / 2.0;  

      return temp;
    }  

  void GetDerivative( const ParametersType & parameters,
                            DerivativeType & derivative ) const override
    {
    	derivative.fill(0);
    }

  unsigned int GetNumberOfParameters(void) const override
    {
    return SpaceDimension;
    }


private:
  
  typename GPUMetricType::Pointer        m_GPUMetric;
  EulerTransformPointer                  m_EulerTransform; 
  PointType                              m_Center;

  bool                                   m_Debug;

  
};

}
