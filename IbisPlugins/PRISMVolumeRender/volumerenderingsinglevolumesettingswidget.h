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

#ifndef VOLUMERENDERINGSINGLEVOLUMESETTINGSWIDGET_H
#define VOLUMERENDERINGSINGLEVOLUMESETTINGSWIDGET_H

#include <QWidget>

class VolumeRenderingObject;
class VolumeShaderEditorWidget;

namespace Ui {
class VolumeRenderingSingleVolumeSettingsWidget;
}

class VolumeRenderingSingleVolumeSettingsWidget : public QWidget
{
    Q_OBJECT
    
public:

    explicit VolumeRenderingSingleVolumeSettingsWidget(QWidget *parent = 0);
    ~VolumeRenderingSingleVolumeSettingsWidget();

    void SetVolumeRenderingObject( VolumeRenderingObject * vr, int volumeIndex );
    void UpdateUi();
    
private slots:

    // custom slots
    void OnVolumeShaderEditorWidgetClosed();

    // Auto-connected slots
    void on_enableVolumeCheckBox_toggled(bool checked);
    void on_volumeComboBox_currentIndexChanged(int index);
    void on_contributionTypeComboBox_currentIndexChanged(int index);
    void on_shaderPushButton_toggled(bool checked);
    void on_use16BitsVolumeCheckBox_toggled(bool checked);
    void on_useLinearSamplingCheckBox_toggled(bool checked);
    void on_addShaderContribTypeButton_clicked();

    void on_removeVolumeShaderButton_clicked();

private:

    VolumeRenderingObject * m_vr;
    int m_volumeIndex;
    Ui::VolumeRenderingSingleVolumeSettingsWidget *ui;
    VolumeShaderEditorWidget * m_shaderWidget;
};

#endif
