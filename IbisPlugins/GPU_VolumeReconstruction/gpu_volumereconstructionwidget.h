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
#include "gpu_volumereconstruction.h"

#include "ibisitkvtkconverter.h"
#include "vtkSmartPointer.h"

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

    Ui::GPU_VolumeReconstructionWidget * ui;
    Application * m_application;
    QFutureWatcher<void>        m_futureWatcher;
    VolumeReconstructionPointer m_Reconstructor;
    QElapsedTimer               m_ReconstructionTimer;
    GPU_VolumeReconstruction   * m_VolumeReconstructor;

private slots:

    void on_startButton_clicked();
    void slot_finished();

};

#endif
