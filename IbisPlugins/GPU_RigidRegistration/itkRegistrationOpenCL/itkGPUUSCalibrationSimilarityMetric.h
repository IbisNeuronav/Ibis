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

#include "itkUSCalibrationTransform.h"
#include "itkEuler3DTransform.h"
#include "itkGPUOrientationMatchingUSCalibration.h"
#include "itkSingleValuedCostFunction.h"

namespace itk
{

template< class TFixedImage, class TMovingImage>
class GPUUSCalibrationSimilarityMetric : public itk::SingleValuedCostFunction
{
public:


  typedef GPUUSCalibrationSimilarityMetric        Self;
  typedef itk::SingleValuedCostFunction     Superclass;
  typedef itk::SmartPointer<Self>           Pointer;
  typedef itk::SmartPointer<const Self>     ConstPointer;
  itkNewMacro( Self );
  itkTypeMacro( GPUUSCalibrationSimilarityMetric, SingleValuedCostFunction );

  enum { ParameterDimension=8 };

  typedef Superclass::ParametersType              ParametersType;
  typedef Superclass::DerivativeType              DerivativeType;
  typedef Superclass::MeasureType                 MeasureType;

  typedef vnl_vector<double>                      VectorType;
  
  typedef itk::GPUOrientationMatchingUSCalibration<
  			TFixedImage, TMovingImage>		                      GPUMetricType;
  typedef typename GPUMetricType::Pointer                   GPUMetricPointer;  												  
  typedef typename GPUMetricType::MatrixTransformType       MetricTransformType; 
  typedef typename MetricTransformType::Pointer             MetricTransformPointer;
  typedef typename MetricTransformType::ConstPointer        MetricTransformConstPointer;
  typedef typename GPUMetricType::SampleLocationsType       SampleLocationsType;

  typedef itk::USCalibrationTransform<double> 		          USCalibrationTransformType;  		  
  typedef USCalibrationTransformType::Pointer               USCalibrationTransformPointer;
  typedef USCalibrationTransformType::InputPointType        PointType;
  
  itkSetObjectMacro(GPUMetric, GPUMetricType);

  itkSetMacro(XScaleSigma, double);
  itkSetMacro(YScaleSigma, double);

  itkSetMacro(XScaleCenter, double);
  itkSetMacro(YScaleCenter, double);

  itkSetMacro(ScalePenaltyWeight, double);

  GPUUSCalibrationSimilarityMetric()
   {
    m_GPUMetric = NULL;    
    m_USCalibrationTransform = USCalibrationTransformType::New();
    m_ScalePenaltyWeight = 1.0;
    }

  double GetValue( const ParametersType & parameters ) const
    {
   	if(!m_GPUMetric)
   		itkExceptionMacro(<<"GPUMetric has not been set!");

    PointType center;
    center = m_GPUMetric->GetCenter();
   	m_USCalibrationTransform->SetCenter(center);
   	m_USCalibrationTransform->SetParameters( parameters );

   	MetricTransformPointer tempTransform = MetricTransformType::New();
   	tempTransform->SetMatrix(m_USCalibrationTransform->GetMatrix());
   	tempTransform->SetOffset(m_USCalibrationTransform->GetOffset());
   	m_GPUMetric->SetUSCalibrationTransform(tempTransform);

    m_GPUMetric->Update();	

    double meanGradientOrientationAlignmentMetric = m_GPUMetric->GetMetricValue();

    double mahDist = std::pow( (m_XScaleCenter-parameters[6]) / m_XScaleSigma , 2.0 ) + 
                    std::pow( (m_YScaleCenter-parameters[7]) / m_YScaleSigma , 2.0 );
    double heuristicPenaltyOnScales = mahDist; 

    return -(meanGradientOrientationAlignmentMetric - m_ScalePenaltyWeight * heuristicPenaltyOnScales);
    }

  PointType GetCenter(void) const
    {
      PointType temp;
      temp = m_GPUMetric->GetCenter();
      return temp;
    }  

  SampleLocationsType GetSampleLocations(void) const
    {
      SampleLocationsType sampleLocations = m_GPUMetric->GetSampleLocations();
      return sampleLocations;
    }

  void GetDerivative( const ParametersType & parameters,
                            DerivativeType & derivative ) const
    {
    	derivative.fill(0);
    }

  unsigned int GetNumberOfParameters(void) const
    {
    return ParameterDimension;
    }


private:
  
  typename GPUMetricType::Pointer        m_GPUMetric;
  USCalibrationTransformPointer          m_USCalibrationTransform; 
  PointType                              m_Center;

  double                                 m_XScaleSigma, m_YScaleSigma;
  double                                 m_XScaleCenter, m_YScaleCenter;

  double                                 m_ScalePenaltyWeight;
  
};

}
