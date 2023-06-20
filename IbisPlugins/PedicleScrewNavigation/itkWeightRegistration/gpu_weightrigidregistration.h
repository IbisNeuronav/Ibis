/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Adapted from Dante De Nigris GPU Registration plugin

#ifndef __GPU_WeightRigidRegistration_h_
#define __GPU_WeightRigidRegistration_h_

#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include "imageobject.h"

#include <itkEuler3DTransform.h>
#include "itkGPU3DRigidSimilarityWeightMetric.h"

#include <itkCMAEvolutionStrategyOptimizer.h>

class GPU_WeightRigidRegistration
{
public:
    typedef itk::CMAEvolutionStrategyOptimizer OptimizerType;

    typedef itk::GPU3DRigidSimilarityWeightMetric<IbisItkFloat3ImageType, IbisItkFloat3ImageType> GPUCostFunctionType;
    typedef GPUCostFunctionType::Pointer GPUCostFunctionPointer;

    typedef itk::Euler3DTransform<double> ItkRigidTransformType;

    typedef itk::ImageMaskSpatialObject<3> ImageMaskType;
    typedef ImageMaskType::Pointer ImageMaskPointer;
    typedef GPUCostFunctionType::SamplingStrategy OrientationSamplingStrategy;
    typedef GPUCostFunctionType::RegistrationMetricToUseType RegistrationMetricToUseType;

    explicit GPU_WeightRigidRegistration();
    ~GPU_WeightRigidRegistration();

    void runRegistration();

    void SetItkSourceImage( IbisItkFloat3ImageType::Pointer image ) { this->m_itkSourceImage = image; }
    void SetItkTargetImage( IbisItkFloat3ImageType::Pointer image ) { this->m_itkTargetImage = image; }
    void SetSourceVtkTransform( vtkTransform * transform )
    {
        this->m_sourceVtkTransform = transform;
    }  // WorldTransform of the ImageObject
    void SetTargetVtkTransform( vtkTransform * transform )
    {
        this->m_targetVtkTransform = transform;
    }  // WorldTransform of the ImageObject
    void SetVtkTransform( vtkTransform * transform )
    {
        this->m_resultTransform = transform;
    }  // LocalTransform of the ImageObject
    void SetInitialSigma( double initialSigma ) { this->m_initialSigma = initialSigma; }
    void SetPopulationSize( unsigned int populationSize ) { this->m_populationSize = populationSize; }
    void SetParentVtkTransform( vtkTransform * transform ) { this->m_parentVtkTransform = transform; }
    void SetDebugOn() { m_debug = true; }
    void SetDebugOff() { m_debug = false; }
    void SetDebug( bool debug ) { this->m_debug = debug; }

    void SetOrientationPercentile( double percentile ) { this->m_orientationPercentile = percentile; }
    void SetOrientationNumberOfPixels( unsigned int numberOfPixels )
    {
        this->m_orientationNumberOfPixels = numberOfPixels;
    }
    void SetOrientationSelectivity( unsigned int orientationSelectivity )
    {
        m_orientationSelectivity = orientationSelectivity;
    }
    void SetUseMask( bool usemask ) { this->m_useMask = usemask; }
    void SetLambdaMetricBalance( double lambda ) { this->m_lambdaMetricBalance = lambda; }

    double GetOrientationPercentile() { return m_orientationPercentile; }
    unsigned int GetOrientationNumberOfPixels() { return m_orientationNumberOfPixels; }
    unsigned int GetOrientationSelectivity() { return m_orientationSelectivity; }
    bool GetUseMask() { return m_useMask; }
    double GetLambdaMetricBalance() { return m_lambdaMetricBalance; }

    void SetSamplingStrategyToRandom() { this->m_orientationSamplingStrategy = OrientationSamplingStrategy::RANDOM; }
    void SetSamplingStrategyToGrid() { this->m_orientationSamplingStrategy = OrientationSamplingStrategy::GRID; }
    void SetSamplingStrategyToFull() { this->m_orientationSamplingStrategy = OrientationSamplingStrategy::FULL; }
    OrientationSamplingStrategy GetSamplingStrategy() { return this->m_orientationSamplingStrategy; }

    void SetRegistrationMetricToIntensity()
    {
        this->m_registrationMetricToUse = RegistrationMetricToUseType::INTENSITY;
    }
    void SetRegistrationMetricToGradientOrientation()
    {
        this->m_registrationMetricToUse = RegistrationMetricToUseType::GRADIENT;
    }
    void SetRegistrationMetricToCombination()
    {
        this->m_registrationMetricToUse = RegistrationMetricToUseType::COMBINATION;
    }
    RegistrationMetricToUseType GetRegistrationMetricToUse() { return this->m_registrationMetricToUse; }

    void SetTargetMask( ImageMaskPointer mask ) { this->m_targetSpatialObjectMask = mask; }

    double GetInitialSigma() { return m_initialSigma; }
    unsigned int GetPopulationSize() { return m_populationSize; }
    vtkTransform * GetResultTransform() { return m_resultTransform; }

private:
    void updateTagsDistance();

    bool m_OptimizationRunning;
    bool m_debug;

    IbisItkFloat3ImageType::Pointer m_itkSourceImage;
    IbisItkFloat3ImageType::Pointer m_itkTargetImage;

    vtkTransform * m_sourceVtkTransform;
    vtkTransform * m_targetVtkTransform;
    vtkTransform * m_resultTransform;

    double m_initialSigma;
    unsigned int m_populationSize;

    vtkTransform * m_parentVtkTransform;

    ImageMaskPointer m_targetSpatialObjectMask;

    bool m_useMask;
    double m_orientationPercentile;
    unsigned int m_orientationNumberOfPixels;
    unsigned int m_orientationSelectivity;
    double m_lambdaMetricBalance;
    OrientationSamplingStrategy m_orientationSamplingStrategy;

    RegistrationMetricToUseType m_registrationMetricToUse;
};

#endif
