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

#ifndef __itkGPU3DRigidSimilarityWeightMetric_h__
#define __itkGPU3DRigidSimilarityWeightMetric_h__

#include <itkEuler3DTransform.h>
#include "itkGPUWeightMatchingMatrixTransformationSparseMask.h"
#include "itkGPUOrientationMatchingMatrixTransformationSparseMask.h"
#include <itkSingleValuedCostFunction.h>

namespace itk
{

class MetricMonitor
{
public:
    MetricMonitor()
    {
        m_CurrentGradientMetricValue = 0;
        m_CurrentIntensityMetricValue = 0;
    }
    ~MetricMonitor(){}

    void SetIntensityMetric(double value)
    {
        m_CurrentIntensityMetricValue = value;
    }

    void SetGradientMetric(double value)
    {
        m_CurrentGradientMetricValue = value;
    }

    double GetIntesityMetric(){ return m_CurrentIntensityMetricValue; }
    double GetGradientMetric(){ return m_CurrentGradientMetricValue; }

private:
    double m_CurrentGradientMetricValue;
    double m_CurrentIntensityMetricValue;
};

template< class TFixedImage, class TMovingImage>
class GPU3DRigidSimilarityWeightMetric : public itk::SingleValuedCostFunction
{

public:

  typedef GPU3DRigidSimilarityWeightMetric  Self;
  typedef itk::SingleValuedCostFunction     Superclass;
  typedef itk::SmartPointer<Self>           Pointer;
  typedef itk::SmartPointer<const Self>     ConstPointer;
  itkNewMacro( Self );
  itkTypeMacro( GPU3DRigidSimilarityWeightMetric, SingleValuedCostFunction );

  typedef TFixedImage                           FixedImageType;
  typedef typename FixedImageType::Pointer      FixedImagePointer;
  typedef TMovingImage                          MovingImageType;
  typedef typename MovingImageType::Pointer     MovingImagePointer;

  enum RegistrationMetricToUseType { INTENSITY = 0, GRADIENT = 1, COMBINATION = 2 };

  itkSetMacro(Debug, bool);

  enum { SpaceDimension=6 };

  typedef Superclass::ParametersType              ParametersType;
  typedef Superclass::DerivativeType              DerivativeType;
  typedef Superclass::MeasureType                 MeasureType;

  typedef vnl_vector<double>                      VectorType;
  
  typedef itk::GPUWeightMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>
                                                                    GPUIntensityMetricType;

  typedef typename GPUIntensityMetricType::MatrixTransformType      MetricTransformType;
  typedef typename MetricTransformType::Pointer                     MetricTransformPointer;

  typedef itk::GPUOrientationMatchingMatrixTransformationSparseMask<TFixedImage, TMovingImage>
                                                                    GPUOrientationMetricType;

  typedef itk::ImageMaskSpatialObject< 3 >                          ImageMaskType;
  typedef ImageMaskType::Pointer                                    ImageMaskPointer;
  typedef typename GPUOrientationMetricType::SamplingStrategyType   SamplingStrategy;

  typedef itk::Euler3DTransform<double>    	                EulerTransformType;  		  
  typedef EulerTransformType::Pointer                       EulerTransformPointer;
  typedef EulerTransformType::InputPointType                PointType;
  
  itkSetObjectMacro(GPUIntensityMetric, GPUIntensityMetricType);
  itkSetObjectMacro(FixedImage, FixedImageType);
  itkSetObjectMacro(MovingImage, MovingImageType);
  itkSetObjectMacro(MetricTransform, MetricTransformType);
  itkSetObjectMacro(FixedSpatialObjectImageMask, ImageMaskType);

  itkSetMacro(OrientationNumberOfPixels, unsigned int);
  itkSetMacro(OrientationPercentile, double);
  itkSetMacro(OrientationSelectivity, unsigned int);
  itkSetMacro(OrientationUseMask, bool);
  itkSetMacro(OrientationSamplingStrategy, SamplingStrategy);

  itkSetMacro(RegistrationMetricToUse, RegistrationMetricToUseType);

  itkSetMacro(Lambda, double);

  GPU3DRigidSimilarityWeightMetric()
   {
    m_GPUIntensityMetric = NULL;
    m_GPUOrientationMetric = NULL;
    m_EulerTransform = EulerTransformType::New();
    m_Debug = true;
    m_FixedImage = 0;
    m_MovingImage = 0;
    m_MetricTransform = 0;

    m_FixedSpatialObjectImageMask = 0;
    m_OrientationNumberOfPixels = 16000;
    m_OrientationPercentile = 0.8;
    m_OrientationSelectivity = 32;
    m_OrientationUseMask = true;
    m_OrientationSamplingStrategy = SamplingStrategy::RANDOM;

    m_RegistrationMetricToUse = COMBINATION;
    m_Lambda = 0.5;
    m_MetricMonitor = new MetricMonitor;
    }

  double GetValue( const ParametersType & parameters ) const override
    {
    if(!m_GPUIntensityMetric)
        itkExceptionMacro(<<"GPUIntensityMetric has not been set!");

    if(!m_GPUOrientationMetric)
        itkExceptionMacro(<<"GPUOrientationMetric has not been set!");

    m_EulerTransform->SetParameters( parameters );
    double metricValue;
    double CurrentIntensityMetricValue = 0;
    double CurrentGradientMetricValue = 0;

    if ((m_RegistrationMetricToUse == INTENSITY) | (m_RegistrationMetricToUse == COMBINATION))
      {
        MetricTransformPointer intensityTransform = MetricTransformType::New();
        intensityTransform->SetMatrix(m_EulerTransform->GetMatrix());
        intensityTransform->SetOffset(m_EulerTransform->GetOffset());
        intensityTransform->GetInverse(intensityTransform);

        m_GPUIntensityMetric->SetTransform(intensityTransform);
        m_GPUIntensityMetric->Update();
        CurrentIntensityMetricValue = (double) m_GPUIntensityMetric->GetMetricValue();
      }

    if ((m_RegistrationMetricToUse == GRADIENT) | (m_RegistrationMetricToUse == COMBINATION))
      {
        MetricTransformPointer gradientTransform = MetricTransformType::New();
        gradientTransform->SetMatrix(m_EulerTransform->GetMatrix());
        gradientTransform->SetOffset(m_EulerTransform->GetOffset());

        m_GPUOrientationMetric->SetTransform(gradientTransform);
        m_GPUOrientationMetric->Update();

        CurrentGradientMetricValue = (double) std::pow(m_GPUOrientationMetric->GetMetricValue(), 1.0 / m_OrientationSelectivity);
//        CurrentGradientMetricValue = (double) m_GPUOrientationMetric->GetMetricValue() * 1000.0;
      }

//    if (m_RegistrationMetricToUse == COMBINATION)
//        metricValue = (1 + CurrentIntensityMetricValue) * (1 + CurrentGradientMetricValue);
//    else
//
    metricValue = (1-m_Lambda) * CurrentIntensityMetricValue + (m_Lambda) * CurrentGradientMetricValue;
    m_MetricMonitor->SetIntensityMetric(CurrentIntensityMetricValue);
    m_MetricMonitor->SetGradientMetric(CurrentGradientMetricValue);

    //
    return -metricValue;
    }

  PointType GetCenter(void) const
    {
      PointType temp;
      temp[0] = m_FixedImage->GetOrigin()[0] + m_FixedImage->GetSpacing()[0] * m_FixedImage->GetBufferedRegion().GetSize()[0] / 2.0;
      temp[1] = m_FixedImage->GetOrigin()[1] + m_FixedImage->GetSpacing()[1] * m_FixedImage->GetBufferedRegion().GetSize()[1] / 2.0;
      temp[2] = m_FixedImage->GetOrigin()[2] + m_FixedImage->GetSpacing()[2] * m_FixedImage->GetBufferedRegion().GetSize()[2] / 2.0;

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

  double GetCurrentIntensityMetricValue() const
  {
      return m_MetricMonitor->GetIntesityMetric();
  }

  double GetCurrentGradientMetricValue() const
  {
      return m_MetricMonitor->GetGradientMetric();
  }

  void UpdateMetrics()
    {
      if(!m_FixedImage)
          itkExceptionMacro(<< "Fixed Image has not been set!");

      if(!m_MovingImage)
          itkExceptionMacro(<< "Moving Image has not been set!");

      if(!m_MetricTransform)
          itkExceptionMacro(<< "Metric Transform has not been set!");

      if(!m_GPUIntensityMetric)
          m_GPUIntensityMetric = GPUIntensityMetricType::New();

      if(!m_GPUOrientationMetric)
          m_GPUOrientationMetric = GPUOrientationMetricType::New();

      if ((m_RegistrationMetricToUse == INTENSITY) | (m_RegistrationMetricToUse == COMBINATION))
        {
          m_EulerTransform->SetCenter(m_MetricTransform->GetCenter());

          m_GPUIntensityMetric->SetFixedImage(m_FixedImage);
          m_GPUIntensityMetric->SetMovingImage(m_MovingImage);
          m_GPUIntensityMetric->SetTransform(m_MetricTransform);
          m_GPUIntensityMetric->Update();
        }

      if ((m_RegistrationMetricToUse == GRADIENT) | (m_RegistrationMetricToUse == COMBINATION))
        {
          m_EulerTransform->SetCenter(m_MetricTransform->GetCenter());

          m_GPUOrientationMetric->SetFixedImage(m_FixedImage);
          m_GPUOrientationMetric->SetMovingImage(m_MovingImage);
          m_GPUOrientationMetric->SetTransform(m_MetricTransform);
          if (m_FixedSpatialObjectImageMask)
             {
             m_GPUOrientationMetric->SetFixedImageMaskSpatialObject(m_FixedSpatialObjectImageMask);
             m_GPUOrientationMetric->SetUseImageMask(true);
             }
          m_GPUOrientationMetric->SetSamplingStrategy(m_OrientationSamplingStrategy);
          m_GPUOrientationMetric->SetNumberOfPixels( m_OrientationNumberOfPixels );
          m_GPUOrientationMetric->SetPercentile( m_OrientationPercentile );
          m_GPUOrientationMetric->SetN( m_OrientationSelectivity );
          m_GPUOrientationMetric->SetComputeMask( m_OrientationUseMask );
          m_GPUOrientationMetric->SetMaskThreshold( 0.5 );
          m_GPUOrientationMetric->SetGradientScale( 1.0 );
          m_GPUOrientationMetric->Update();
        }

    }


private:
  
  typename GPUIntensityMetricType::Pointer          m_GPUIntensityMetric;
  typename GPUOrientationMetricType::Pointer        m_GPUOrientationMetric;
  EulerTransformPointer                             m_EulerTransform;
  PointType                                         m_Center;

  bool                                              m_Debug;

  FixedImagePointer                                 m_FixedImage;
  MovingImagePointer                                m_MovingImage;
  MetricTransformPointer                            m_MetricTransform;

  ImageMaskPointer                                  m_FixedSpatialObjectImageMask;
  unsigned int                                      m_OrientationNumberOfPixels;
  double                                            m_OrientationPercentile;
  unsigned int                                      m_OrientationSelectivity;
  bool                                              m_OrientationUseMask;
  SamplingStrategy                                  m_OrientationSamplingStrategy;

  RegistrationMetricToUseType                       m_RegistrationMetricToUse;
  double                                            m_Lambda;
  MetricMonitor                                     *m_MetricMonitor;
//  double                                            *m_CurrentIntensityMetricValue;
//  double                                            *m_CurrentGradientMetricValue;

};

}

#endif

