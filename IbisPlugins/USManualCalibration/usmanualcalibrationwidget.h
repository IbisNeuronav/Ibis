/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __USManualCalibrationWidget_h_
#define __USManualCalibrationWidget_h_

#include <QWidget>
#include "usprobeobject.h"

namespace Ui
{
    class USManualCalibrationWidget;
}

class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkImageActor;
class vtkImageData;
class vtkMatrix4x4;
class vtkRenderer;
class vtkNShapeCalibrationWidget;
class vtkEventQtSlotConnect;
class USManualCalibrationPluginInterface;

class USManualCalibrationWidget : public QWidget
{

    Q_OBJECT

public:

    explicit USManualCalibrationWidget( QWidget * parent = 0 );
    ~USManualCalibrationWidget();

    void SetPluginInterface( USManualCalibrationPluginInterface * pluginInterface );

protected slots:

    void NewFrameSlot();
    void UpdateDisplay();

private slots:

    void OnManipulatorsModified();
    void on_freezeVideoButton_toggled(bool checked);
    void on_resetButton_clicked();
    void on_depthComboBox_currentIndexChanged(int);

private:

    void ComputeCalibration();
    void UpdateUSProbeStatus();

    vtkRenderWindow * GetRenderWindow();
    vtkRenderWindowInteractor * GetInteractor();
    void RenderFirst();
    void UpdateUi();

    Ui::USManualCalibrationWidget * ui;

    vtkImageActor * m_imageActor;
    vtkRenderer * m_renderer;

    bool m_imageFrozen;
    vtkImageData * m_frozenImage;
    vtkMatrix4x4 * m_frozenMatrix;
    TrackerToolState m_frozenState;

    // N shape widget manipulators
    void EnableManipulators( bool on );
    void UpdateManipulators();
    vtkNShapeCalibrationWidget * m_manipulators[4];
    vtkEventQtSlotConnect * m_manipulatorsCallbacks;

    USManualCalibrationPluginInterface * m_pluginInterface;

};

#endif
