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

#ifndef __TimelineWidget_h_
#define __TimelineWidget_h_

#include <QWidget>

namespace Ui {
class TimelineWidget;
}

class AnimatePluginInterface;

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:

    explicit TimelineWidget(QWidget *parent = 0);
    ~TimelineWidget();

    void SetPluginInterface( AnimatePluginInterface * interf );

private slots:

    void on_currentFrameSpinBox_valueChanged( int arg1 );
    void on_frameSlider_valueChanged( int value );

    void UpdateUi();

    void on_playButton_toggled(bool checked);

private:

    AnimatePluginInterface * m_pluginInterface;
    Ui::TimelineWidget *ui;

};

#endif
