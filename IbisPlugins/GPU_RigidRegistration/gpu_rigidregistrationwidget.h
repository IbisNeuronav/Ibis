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
#ifndef __GPU_RigidRegistrationWidget_h_
#define __GPU_RigidRegistrationWidget_h_

#include <QWidget>
#include <QtGui>
#include <QMessageBox>
#include "qdebugstream.h"
#include "ui_gpu_rigidregistrationwidget.h"
#include "application.h"
#include "scenemanager.h"
#include "sceneobject.h"
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

class Application;

namespace Ui
{
    class GPU_RigidRegistrationWidget;
}


class GPU_RigidRegistrationWidget : public QWidget
{

    Q_OBJECT

public:

    explicit GPU_RigidRegistrationWidget(QWidget *parent = 0);
    ~GPU_RigidRegistrationWidget();

    void SetApplication( Application * app );

private:

    void UpdateUi();

    void updateTagsDistance();

    Ui::GPU_RigidRegistrationWidget * ui;
    Application * m_application;
    bool          m_OptimizationRunning;

private slots:

    void on_startButton_clicked();
    void on_sourceImageComboBox_activated(int index);
    void on_debugCheckBox_clicked();

//    void on_targetImageComboBox_activated(int index);
//    void on_transformObjectComboBox_activated(int index);

//    void on_mriTagsComboBox_activated(int index);
//    void on_usTagsComboBox_activated(int index);

protected:
    void closeEvent(QCloseEvent *event)
     {
        if( m_OptimizationRunning )
        {
            event->ignore();
            QMessageBox::information( this, "Optimization in process..", "Please wait until end of optimization." );
        }
        else
        {
            event->accept();
        }
     }

};

#endif
