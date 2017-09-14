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

#ifndef __GPU_VolumeReconstructionWidget_h_
#define __GPU_VolumeReconstructionWidget_h_
#include <QWidget>
#include <QtGui>
#include <QFutureWatcher>

#include "ui_gpu_volumereconstructionwidget.h"
#include "application.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "usacquisitionobject.h"
#include "vtkTransform.h"
#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"

#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkImageLuminance.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"

#include "itkGPUVolumeReconstruction.h"
#include "itkEuler3DTransform.h"

typedef itk::GPUVolumeReconstruction<IbisItkFloat3ImageType>
                                                    VolumeReconstructionType;
typedef VolumeReconstructionType::Pointer           VolumeReconstructionPointer;

typedef itk::Euler3DTransform<float>                ItkRigidTransformType;

class Application;

namespace Ui
{
    class GPU_VolumeReconstructionWidget;
}


class GPU_VolumeReconstructionWidget : public QWidget
{

    Q_OBJECT

public:

    explicit GPU_VolumeReconstructionWidget(QWidget *parent = 0);
    ~GPU_VolumeReconstructionWidget();

    void SetApplication( Application * app );

private:

    void UpdateUi();
    void VtkToItkImage( vtkImageData * vtkImage, IbisItkFloat3ImageType * itkOutputImage, vtkSmartPointer<vtkMatrix4x4> transformMatrix );

    Ui::GPU_VolumeReconstructionWidget * ui;
    Application * m_application;
    QFutureWatcher<void>        m_futureWatcher;
    VolumeReconstructionPointer m_Reconstructor;
    QElapsedTimer               m_ReconstructionTimer;

private slots:

    void on_startButton_clicked();
    void slot_finished();

};

#endif
