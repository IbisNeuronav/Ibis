/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_VIDEOSETTINGSDIALOG_H
#define TAG_VIDEOSETTINGSDIALOG_H

#include <QObject>
#include "ui_videosettingsdialog.h"

class TrackedVideoSource;
class AutomaticGui;

class VideoSettingsDialog : public QWidget, public Ui::VideoSettingsDialog
{
    Q_OBJECT

public:
    
    VideoSettingsDialog( QWidget* parent = 0 );
    virtual ~VideoSettingsDialog();
    
    virtual void SetSource(TrackedVideoSource * source);
    
public slots:

    virtual void VideoOutputChanged(int outputType);

protected:
    
    TrackedVideoSource * m_source;
    AutomaticGui * m_automaticGui;  // device type specific gui that is automatically generated based on device generic params
    
    virtual void Update();

private slots:

    void on_m_deviceTypeComboBox_activated(int index);
    void on_timeShiftSpinBox_valueChanged(double arg1);
};

#endif
