/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __TripleCutPlaneObjectMixerWidget_h_
#define __TripleCutPlaneObjectMixerWidget_h_

#include <QWidget>

namespace Ui {
class TripleCutPlaneObjectMixerWidget;
}

class TripleCutPlaneObject;

class TripleCutPlaneObjectMixerWidget : public QWidget
{
    Q_OBJECT

public:

    explicit TripleCutPlaneObjectMixerWidget(QWidget *parent = 0);
    ~TripleCutPlaneObjectMixerWidget();

    void SetTripleCutPlaneObject( TripleCutPlaneObject * obj, int imageIndex );

private slots:

    void on_colorTableComboBox_currentIndexChanged(int index);
    void on_blendModeComboBox_currentIndexChanged(int index);
    void on_sliceMixComboBox_currentIndexChanged(int index);
    void on_imageIntensitySlider_valueChanged(int value);

    void UpdateUi();

private:

    Ui::TripleCutPlaneObjectMixerWidget *ui;
    TripleCutPlaneObject * m_cutPlaneObject;
    int m_imageIndex;
};

#endif
