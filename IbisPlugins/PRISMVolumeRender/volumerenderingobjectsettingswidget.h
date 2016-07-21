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

#ifndef VOLUMERENDERINGOBJECTSETTINGSWIDGET_H
#define VOLUMERENDERINGOBJECTSETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
    class VolumeRenderingObjectSettingsWidget;
}

class VolumeRenderingObject;
class VolumeRenderingSingleVolumeSettingsWidget;
class VolumeShaderEditorWidget;

class VolumeRenderingObjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:

    explicit VolumeRenderingObjectSettingsWidget( QWidget * parent = 0 );
    ~VolumeRenderingObjectSettingsWidget();

    void SetVolumeRenderingObject( VolumeRenderingObject * vr );

public slots:

    void UpdateUI();
    void VolumeSlotModified();

private:

    void SetupVolumeWidgets();
    int GetVolumeWidgetInsertionIndex();

    QList<VolumeRenderingSingleVolumeSettingsWidget*> m_volumeWidgets;

    VolumeShaderEditorWidget * m_shaderInitWidget;

    Ui::VolumeRenderingObjectSettingsWidget * ui;
    VolumeRenderingObject * m_vr;

private slots:

    void on_animateCheckBox_toggled(bool checked);
    void on_addVolumeButton_clicked();
    void on_removeVolumeButton_clicked();
    void on_samplingSpinBox_valueChanged(double arg1);
    void on_showLightCheckBox_toggled(bool checked);
    void on_trackLightCheckBox_toggled(bool checked);
    void on_interactionPointTypeButtonGroup_buttonClicked( int );
    void on_shaderInitButton_toggled(bool checked);
    void OnShaderInitEditorWidgetClosed();
    void on_pickPosCheckBox_toggled(bool checked);
    void on_pickValueSpinBox_valueChanged(double arg1);
    void on_addRayInitShaderButton_clicked();
    void on_removeRayInitShaderButton_clicked();
    void on_rayInitComboBox_currentIndexChanged(int index);
    void on_multFactorSlider_valueChanged(int value);
    void on_multFactorSpinBox_valueChanged(double arg1);

protected:

    bool m_needVolumeSlotUpdate;
};

#endif
