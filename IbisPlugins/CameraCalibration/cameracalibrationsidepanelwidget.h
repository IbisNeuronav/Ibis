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

#ifndef CAMERACALIBRATIONSIDEPANELWIDGET_H
#define CAMERACALIBRATIONSIDEPANELWIDGET_H

#include <QWidget>

namespace Ui {
class CameraCalibrationSidePanelWidget;
}

class QListWidgetItem;
class CameraCalibrationPluginInterface;

class CameraCalibrationSidePanelWidget : public QWidget
{
    Q_OBJECT
    
public:

    explicit CameraCalibrationSidePanelWidget(QWidget *parent = 0);
    ~CameraCalibrationSidePanelWidget();

    void SetPluginInterface( CameraCalibrationPluginInterface * interface );
    
private slots:

    void InterfaceModified();
    void UpdateUi();
    void on_gridWidthSpinBox_valueChanged(int arg1);
    void on_gridHeightSpinBox_valueChanged(int arg1);
    void on_gridSquareSizeSpinBox_valueChanged(int arg1);
    void on_calibrateButton_toggled( bool checked );
    void on_currentCameraComboBox_currentIndexChanged(int index);
    void on_exportCalibrationDataButton_clicked();
    void on_importDataButton_clicked();
    void on_showGridCheckBox_toggled(bool checked);
    void on_showAllCapturedViewsButton_clicked();
    void on_hideAllCapturedViewsButton_clicked();
    void on_validationButton_clicked();
    void on_translationScaleSpinBox_valueChanged(double arg1);
    void on_rotationScaleSpinBox_valueChanged(double arg1);
    void on_updateCalibrationButton_clicked();
    void on_viewListWidget_itemChanged(QListWidgetItem *item);

private:

    CameraCalibrationPluginInterface * m_pluginInterface;
    Ui::CameraCalibrationSidePanelWidget * ui;
};

#endif
