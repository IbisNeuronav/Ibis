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

#ifndef __CameraCalibrationWidget_h_
#define __CameraCalibrationWidget_h_

#include <QWidget>

namespace Ui
{
    class CameraCalibrationWidget;
}

class TrackedVideoSource;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkImageActor;
class vtkRenderer;
class vtkActor;
class CameraCalibrationPluginInterface;

class CameraCalibrationWidget : public QWidget
{

    Q_OBJECT

public:

    explicit CameraCalibrationWidget( QWidget * parent = 0 );
    ~CameraCalibrationWidget();

    void SetPluginInterface( CameraCalibrationPluginInterface * pluginInterface );

protected slots:

    void UpdateDisplay();

private slots:

    void on_clearCalibrationViewsButton_clicked();
    void on_computeCenterCheckBox_toggled(bool checked);
    void on_computeDistortionCheckBox_toggled(bool checked);
    void on_computeExtrinsicCheckBox_toggled(bool checked);
    void on_accumulateCheckBox_toggled(bool checked);
    void on_captureViewButton_clicked();

private:

    vtkRenderWindow * GetRenderWindow();
    vtkRenderWindowInteractor * GetInteractor();
    void CreateGrid();
    void DeleteGrid();
    void ShowGrid( bool show );
    void RenderFirst();
    void UpdateUi();

    Ui::CameraCalibrationWidget * ui;

    vtkImageActor * m_imageActor;
    vtkRenderer * m_renderer;

    // grid point position
    bool m_imagePointsOn;
    std::vector< vtkActor * > m_markerActors;

    CameraCalibrationPluginInterface * m_pluginInterface;

    QTime * m_accumulationTime;

};

#endif
