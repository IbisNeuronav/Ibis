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

#include <vtkMatrix4x4.h>
#include <vtkTransform.h>

#include <QMessageBox>
#include <QWidget>
#include <QtGui>

#include "gpu_rigidregistration.h"
#include "imageobject.h"
#include "qdebugstream.h"
#include "sceneobject.h"
#include "ui_gpu_rigidregistrationwidget.h"

class GPU_RigidRegistrationPluginInterface;
class GPU_RigidRegistration;

namespace Ui
{
class GPU_RigidRegistrationWidget;
}

class GPU_RigidRegistrationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GPU_RigidRegistrationWidget( QWidget * parent = 0 );
    ~GPU_RigidRegistrationWidget();

    void SetPluginInterface( GPU_RigidRegistrationPluginInterface * ifc );

private:
    void UpdateUi();

    void updateTagsDistance();

    Ui::GPU_RigidRegistrationWidget * ui;
    GPU_RigidRegistrationPluginInterface * m_pluginInterface;
    QElapsedTimer m_registrationTimer;
    bool m_OptimizationRunning;

private slots:

    void on_startButton_clicked();
    void on_sourceImageComboBox_activated( int index );
    void on_debugCheckBox_clicked();

protected:
    void closeEvent( QCloseEvent * event )
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
