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

#ifndef __GPU_SliceRigidRegistration_h_
#define __GPU_SliceRigidRegistration_h_


#include "imageobject.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"

#include "itkGPUSliceRigidSimilarityMetric.h"
#include "itkEuler3DTransform.h"

#include "itkAmoebaOptimizer.h"
#include "itkSPSAOptimizer.h"
#include "itkCMAEvolutionStrategyOptimizer.h"

#include "itkImageMaskSpatialObject.h"
#include "usacquisitionobject.h"

class USAcquisitionObject;

class GPU_SliceRigidRegistration
{

public:

    typedef itk::CMAEvolutionStrategyOptimizer            OptimizerType;

    typedef itk::GPUSliceRigidSimilarityMetric<IbisItkFloat3ImageType,IbisItkFloat3ImageType>
                                                        GPUCostFunctionType;
    typedef GPUCostFunctionType::Pointer                GPUCostFunctionPointer;

    typedef  GPUCostFunctionType::GPUMetricType         GPUMetricType;
    typedef  GPUCostFunctionType::GPUMetricPointer      GPUMetricPointer;

    typedef itk::Euler3DTransform<double>                ItkRigidTransformType;

    using ImageMaskType = itk::ImageMaskSpatialObject< 3 >;
    using ImageMaskPointer = ImageMaskType::Pointer;

    explicit GPU_SliceRigidRegistration();
    ~GPU_SliceRigidRegistration();

    void runRegistration();

    void SetItkSourceImage(IbisItkFloat3ImageType::Pointer image) { this->m_itkSourceImage = image; }
    void SetItkTargetImage(IbisItkFloat3ImageType::Pointer image) { this->m_itkTargetImage = image; }
    void SetSourceVtkTransform(vtkTransform * transform) { this->m_sourceVtkTransform = transform; } // WorldTransform of the ImageObject
    void SetTargetVtkTransform(vtkTransform * transform) { this->m_targetVtkTransform = transform; } // WorldTransform of the ImageObject
    void SetVtkTransform(vtkTransform * transform) { this->m_resultTransform = transform; } // LocalTransform of the ImageObject
    void SetPercentile(double percentile) { this->m_percentile = percentile; }
    void SetInitialSigma(double initialSigma) { this->m_initialSigma = initialSigma; }
    void SetNumberOfPixels(unsigned int numberOfPixels) { this->m_numberOfPixels = numberOfPixels; }
    void SetOrientationSelectivity(unsigned int orientationSelectivity) { m_orientationSelectivity = orientationSelectivity; }
    void SetPopulationSize(unsigned int populationSize) { this->m_populationSize = populationSize; }
    void SetParentVtkTransform( vtkTransform * transform ) { this->m_parentVtkTransform = transform; }
    void SetDebugOn() { m_debug = true; }
    void SetDebugOff() { m_debug = false; }
    void SetDebug(bool debug) { this->m_debug = debug; }
    void SetUseMask(bool usemask) { this->m_useMask = usemask; }

    double GetPercentile() { return m_percentile; }
    double GetInitialSigma() { return m_initialSigma; }
    unsigned int GetNumberOfPixels() { return m_numberOfPixels; }
    unsigned int GetOrientationSelectivity() { return m_orientationSelectivity; }
    unsigned int GetPopulationSize() { return m_populationSize; }
    vtkTransform * GetResultTransform() { return m_resultTransform; }
    bool GetUseMask() { return m_useMask; }

    using SamplingStrategy = GPUMetricType::SamplingStrategyType;
    void SetSamplingStrategyToRandom()    { this->m_samplingStrategy = SamplingStrategy::RANDOM; }
    void SetSamplingStrategyToGrid()      { this->m_samplingStrategy = SamplingStrategy::GRID; }
    void SetSamplingStrategyToFull()      { this->m_samplingStrategy = SamplingStrategy::FULL; }
    SamplingStrategy GetSamplingStrategy() { return this->m_samplingStrategy; }

    void SetTargetMask(ImageMaskPointer mask) { this->m_targetSpatialObjectMask = mask;}

    void SetTargetUSAcquisition(USAcquisitionObject * acq) {this->m_targetUSAcquisition = acq;}
    USAcquisitionObject * SetTargetUSAcquisition() { return this->m_targetUSAcquisition; }

private:

    void updateTagsDistance();
    vtkTransform * GetVtkTransformFromItkImage(IbisItkFloat3ImageType::Pointer, bool load_translation=false);

    bool m_OptimizationRunning;
    bool m_debug;
    bool m_useMask;

    IbisItkFloat3ImageType::Pointer m_itkSourceImage;
    IbisItkFloat3ImageType::Pointer m_itkTargetImage;
    USAcquisitionObject             * m_targetUSAcquisition;

    ImageMaskPointer m_targetSpatialObjectMask;

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
    SamplingStrategy m_samplingStrategy;

};

#endif