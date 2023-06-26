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

#ifndef __FrameRateTesterWidget_h_
#define __FrameRateTesterWidget_h_

#include <QWidget>

class Application;
class FrameRateTesterPluginInterface;

namespace Ui
{
class FrameRateTesterWidget;
}

class FrameRateTesterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FrameRateTesterWidget( QWidget * parent = 0 );
    ~FrameRateTesterWidget();

    void SetPluginInterface( FrameRateTesterPluginInterface * pi );

private slots:

    void UpdateUi();
    void UpdateStats();

    void on_currentViewComboBox_currentIndexChanged( int index );
    void on_periodSpinBox_valueChanged( int arg1 );
    void on_runButton_toggled( bool checked );

private:
    FrameRateTesterPluginInterface * m_pluginInterface;
    Ui::FrameRateTesterWidget * ui;
};

#endif
