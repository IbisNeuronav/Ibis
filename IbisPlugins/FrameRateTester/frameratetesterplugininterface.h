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

#ifndef __FrameRateTesterPluginInterface_h_
#define __FrameRateTesterPluginInterface_h_

#include "toolplugininterface.h"

//class vtkEventQtSlotConnect;
class QTimer;
class QTime;

class FrameRateTesterPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.FrameRateTesterPluginInterface" )

public:

    FrameRateTesterPluginInterface();
    virtual ~FrameRateTesterPluginInterface();
    virtual QString GetPluginName() { return QString("FrameRateTester"); }
    virtual bool CanRun();
    virtual QString GetMenuEntryString() { return QString("Test Frame Rate"); }

    virtual QWidget * CreateTab();
    virtual bool WidgetAboutToClose();

    void SetRunning( bool run );
    bool IsRunning();

    void SetNumberOfFrames( int nb );
    int GetNumberOfFrames() { return m_numberOfFrames; }

    int GetLastNumberOfFrames() { return m_lastNumberOfFrames; }
    double GetLastPeriod() { return m_lastPeriod; }
    double GetLastFrameRate();

    void SetCurrentViewId( int id );
    int GetCurrentViewID() { return m_currentViewID; }

private slots:

    void OnTimerTriggered();

signals:

    void PeriodicSignal();

protected:

    void SetRenderingEnabled( bool enabled );

    int m_numberOfFrames;
    QTimer * m_timer;
    QTime * m_time;

    int m_lastNumberOfFrames;
    double m_lastPeriod;
    int m_currentViewID;

    // temp var used when running
    int m_accumulatedFrames;

};

#endif
