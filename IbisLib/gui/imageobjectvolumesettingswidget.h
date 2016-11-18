/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef IMAGEOBJECTVOLUMESETTINGSWIDGET_H
#define IMAGEOBJECTVOLUMESETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
class ImageObjectVolumeSettingsWidget;
}

class ImageObject;

class ImageObjectVolumeSettingsWidget : public QWidget
{
    Q_OBJECT
    
public:

    explicit ImageObjectVolumeSettingsWidget(QWidget *parent = 0);
    ~ImageObjectVolumeSettingsWidget();

    void SetImageObject( ImageObject * img );
    
private slots:

    void on_enableVolumeRenderingCheckBox_toggled(bool checked);
    void on_shadingCheckBox_toggled(bool checked);
    void on_ambiantSpinBox_valueChanged(double arg1);
    void on_diffuseSpinBox_valueChanged(double arg1);
    void on_specularSpinBox_valueChanged(double arg1);
    void on_specularPowerSpinBox_valueChanged(double arg1);
    void on_gradientOpacityCheckBox_toggled(bool checked);
    void on_renderModeComboBox_currentIndexChanged(int index);
    void on_levelSpinBox_valueChanged(double arg1);
    void on_windowSpinBox_valueChanged(double arg1);

    void on_autoSampleDistanceCheckBox_toggled(bool checked);

    void on_sampleDistanceSpinBox_valueChanged(double arg1);

    void on_showClippingWidgetCheckBox_toggled(bool checked);

private:

    void UpdateUi();

    ImageObject * m_imageObject;

    Ui::ImageObjectVolumeSettingsWidget *ui;
};

#endif
