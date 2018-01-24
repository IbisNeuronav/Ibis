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

#ifndef __GPURigidRegistration_h_
#define __GPURigidRegistration_h_


#include "imageobject.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"

#include "itkGPU3DRigidSimilarityMetric.h"
#include "itkEuler3DTransform.h"
#include "itkImageDuplicator.h"

#include "itkAmoebaOptimizer.h"
#include "itkSPSAOptimizer.h"
#include "itkCMAEvolutionStrategyOptimizer.h"

typedef itk::ImageDuplicator< IbisItkFloat3ImageType >  DuplicatorType;

typedef itk::CMAEvolutionStrategyOptimizer            OptimizerType;

typedef itk::GPU3DRigidSimilarityMetric<IbisItkFloat3ImageType,IbisItkFloat3ImageType>
                                                    GPUCostFunctionType;
typedef GPUCostFunctionType::Pointer                GPUCostFunctionPointer;

typedef  GPUCostFunctionType::GPUMetricType         GPUMetricType;
typedef  GPUCostFunctionType::GPUMetricPointer      GPUMetricPointer;

typedef itk::Euler3DTransform<double>                ItkRigidTransformType;


class GPURigidRegistration
{

public:

    explicit GPURigidRegistration();
    ~GPURigidRegistration();

    void runRegistration();

    void SetItkSourceImage(IbisItkFloat3ImageType::Pointer image) { this->m_itkSourceImage = image; }
    void SetItkTargetImage(IbisItkFloat3ImageType::Pointer image) { this->m_itkTargetImage = image; }
    void SetSourceVtkTransform(vtkTransform * transform) { this->m_sourceVtkTransform = transform; }
    void SetTargetVtkTransform(vtkTransform * transform) { this->m_targetVtkTransform = transform; }
    void SetPercentile(double percentile) { this->m_percentile = percentile; }
    void SetInitialSigma(double initialSigma) { this->m_initialSigma = initialSigma; }
    void SetNumberOfPixels(unsigned int numberOfPixels) { this->m_numberOfPixels = numberOfPixels; }
    void SetOrientationSelectivity(unsigned int orientationSelectivity) { m_orientationSelectivity = orientationSelectivity; }
    void SetPopulationSize(unsigned int populationSize) { this->m_populationSize = populationSize; }
    void SetParentVtkTransform( vtkTransform * transform ) { this->m_parentVtkTransform = transform; }
    void SetDebugOn() { m_debug = true; }
    void SetDebugOff() { m_debug = false; }

    double GetPercentile() { return m_percentile; }
    double GetInitialSigma() { return m_initialSigma; }
    unsigned int GetNumberOfPixels() { return m_numberOfPixels; }
    unsigned int GetOrientationSelectivity() { return m_orientationSelectivity; }
    unsigned int GetPopulationSize() { return m_populationSize; }
    vtkTransform * GetResultTransform() { return m_resultTransform; }

private:

    void updateTagsDistance();
    vtkTransform * GetVtkTransformFromItkImage(IbisItkFloat3ImageType::Pointer, bool load_translation=false);

    bool m_OptimizationRunning;
    bool m_debug;

    IbisItkFloat3ImageType::Pointer m_itkSourceImage;
    IbisItkFloat3ImageType::Pointer m_itkTargetImage;

    vtkTransform * m_sourceVtkTransform;
    vtkTransform * m_targetVtkTransform;
    vtkTransform * m_resultTransform;

    double m_percentile;
    double m_initialSigma;
    double m_gradientScale;
    unsigned int m_numberOfPixels;
    unsigned int m_orientationSelectivity;
    unsigned int m_populationSize;

    vtkTransform * m_parentVtkTransform;


};

#endif
