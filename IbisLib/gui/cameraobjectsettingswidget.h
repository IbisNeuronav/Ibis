/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef CAMERAOBJECTSETTINGSWIDGET_H
#define CAMERAOBJECTSETTINGSWIDGET_H

#include <QWidget>

class vtkQtMatrixDialog;

namespace Ui {
    class CameraObjectSettingsWidget;
}

class CameraObject;

class CameraObjectSettingsWidget : public QWidget
{

    Q_OBJECT

public:

    explicit CameraObjectSettingsWidget(QWidget *parent = 0);
    ~CameraObjectSettingsWidget();

    void SetCamera( CameraObject * cam );

private:

    void UpdateUI();
    void UpdateExtrinsicTransform();

    CameraObject * m_camera;
    vtkQtMatrixDialog * m_calibrationMatrixDialog;

    Ui::CameraObjectSettingsWidget *ui;

private slots:

    void CameraModified();

    void on_trackCameraPushButton_toggled(bool checked);
    void on_freezeButton_toggled(bool checked);
    void on_snapshotButton_clicked();

    void on_imageDistanceSpinBox_valueChanged( double );

    void on_calibrationMatrixButton_toggled( bool checked);
    void CalibrationMatrixDialogClosed();

    void on_xImageCenterSpinBox_valueChanged(double arg1);
    void on_yImageCenterSpinBox_valueChanged(double arg1);
    void on_distortionSpinBox_valueChanged(double arg1);

    void on_opacitySlider_valueChanged(int value);

    void on_targetTransparencyGroupBox_toggled(bool arg1);
    void on_useGradientCheckBox_toggled(bool checked);
    void on_showMaskCheckBox_toggled(bool checked);
    void on_xTransparencyCenterSlider_valueChanged(int value);
    void on_yTransparencyCenterSlider_valueChanged(int value);
    void on_minTransparencyRadiusSlider_valueChanged(int value);
    void on_maxTransparencyRadiusSlider_valueChanged(int value);
    void on_recordButton_clicked();
    void on_currentFrameSlider_valueChanged(int value);

    void on_intrinsicParamsGroupBox_toggled(bool arg1);
    void on_extrinsicParamsGroupBox_toggled(bool arg1);
    void on_xtransSpinBox_valueChanged(double arg1);
    void on_ytransSpinBox_valueChanged(double arg1);
    void on_ztransSpinBox_valueChanged(double arg1);
    void on_xrotSpinBox_valueChanged(double arg1);
    void on_yrotSpinBox_valueChanged(double arg1);
    void on_zrotSpinBox_valueChanged(double arg1);
    void on_verticalAngleSpinBox_valueChanged(double arg1);
    void on_trackTransparencyCenterCheckBox_toggled(bool checked);
    void on_saturationSlider_valueChanged(int value);
    void on_brightnessSlider_valueChanged(int value);
    void on_lensDisplacementSlider_valueChanged(int value);
    void on_lensDisplacementSpinBox_valueChanged(double arg1);
};

#endif
