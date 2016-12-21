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

#ifndef __AnimateWidget_h_
#define __AnimateWidget_h_

#include <QWidget>

class Application;
class AnimatePluginInterface;

namespace Ui
{
    class AnimateWidget;
}


class AnimateWidget : public QWidget
{

    Q_OBJECT

public:

    explicit AnimateWidget(QWidget *parent = 0);
    ~AnimateWidget();

    void SetPluginInterface( AnimatePluginInterface * pi );

private slots:

    void UpdateUi();

    void on_domeModeButton_toggled(bool checked);
    void on_cubeTextureSizeSlider_valueChanged(int value);
    void on_cubeTextureSizeSpinBox_valueChanged(int arg1);
    void on_cameraKeyCheckBox_toggled(bool checked);
    void on_renderHDRadioButton_toggled(bool checked);
    void on_render1KradioButton_toggled(bool checked);
    void on_render2KRadioButton_toggled(bool checked);
    void on_render4KradioButton_toggled(bool checked);
    void on_renderSnapshotButton_clicked();
    void on_renderAnimationButton_clicked();
    void on_transferFuncKeyframeCheckBox_toggled(bool checked);
    void on_nbFramesSpinBox_valueChanged(int arg1);
    void on_cameraMinDistanceGroupBox_toggled(bool arg1);
    void on_cameraMinDistanceSpinBox_valueChanged(double arg1);
    void on_cameraMinDistCheckBox_toggled(bool checked);
    void on_nextKeyframeButton_clicked();
    void on_prevKeyframeButton_clicked();
    void on_domeViewAngleSlider_valueChanged(int value);
    void on_domeViewAngleSpinBox_valueChanged(int arg1);

private:

    AnimatePluginInterface * m_pluginInterface;
    Ui::AnimateWidget * ui;

};

#endif
