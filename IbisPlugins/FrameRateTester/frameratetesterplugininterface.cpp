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

#include "frameratetesterplugininterface.h"
#include "frameratetesterwidget.h"
#include "ibisapi.h"
#include "view.h"
#include <QTimer>
#include <QTime>
#include <QMap>
#include <vtkRenderer.h>

FrameRateTesterPluginInterface::FrameRateTesterPluginInterface()
{
    m_numberOfFrames = 200;
    m_timer = 0;
    m_lastNumberOfFrames = 0;
    m_lastPeriod = 0.0;
    m_currentViewID = THREED_VIEW_ID;
    m_time = new QTime;
    m_accumulatedFrames = 0;
}

FrameRateTesterPluginInterface::~FrameRateTesterPluginInterface()
{
    delete m_time;
}

bool FrameRateTesterPluginInterface::CanRun()
{
    return true;
}

QWidget * FrameRateTesterPluginInterface::CreateTab()
{
    FrameRateTesterWidget * widget = new FrameRateTesterWidget;
    widget->SetPluginInterface( this );

    return widget;
}

bool FrameRateTesterPluginInterface::WidgetAboutToClose()
{
    return true;
}

void FrameRateTesterPluginInterface::SetRunning( bool run )
{
    Q_ASSERT( run != IsRunning() );
    if( run )
    {
        // init frame rate vars
        m_lastNumberOfFrames = 0;
        m_lastPeriod = 0.0;
        m_accumulatedFrames = 0;

        // Make sure other views are not rendering
        SetRenderingEnabled( false );

        // Start a timer
        m_timer = new QTimer;
        connect( m_timer, SIGNAL(timeout()), this, SLOT(OnTimerTriggered()) );
        m_timer->start(0);
    }
    else
    {
        // Kill timer
        m_timer->stop();
        delete m_timer;
        m_timer = 0;

        // Reenable rendering everywhere
        SetRenderingEnabled( true );
    }
    emit PluginModified();
}

bool FrameRateTesterPluginInterface::IsRunning()
{
    return m_timer != 0;
}

void FrameRateTesterPluginInterface::SetNumberOfFrames( int nb )
{
    m_numberOfFrames = nb;
}

double FrameRateTesterPluginInterface::GetLastFrameRate()
{
    if( m_lastPeriod > 0.0 )
        return m_lastNumberOfFrames / m_lastPeriod;
    return 0.0;
}

void FrameRateTesterPluginInterface::SetCurrentViewId( int id )
{
    m_currentViewID = id;
    emit PluginModified();
}

void FrameRateTesterPluginInterface::OnTimerTriggered()
{
    if( m_lastNumberOfFrames == 0 )
        m_time->restart();

    // Render
    GetIbisAPI()->GetViewByID( m_currentViewID )->NotifyNeedRender();

    // Increment stats
    m_lastPeriod = ((double)m_time->elapsed()) * 0.001;
    m_lastNumberOfFrames++;
    m_accumulatedFrames++;
    if( m_accumulatedFrames > 20 )
    {
        m_accumulatedFrames = 0;
        emit PeriodicSignal();
    }

    if( m_lastNumberOfFrames == m_numberOfFrames )
        SetRunning( false );
}

void FrameRateTesterPluginInterface::SetRenderingEnabled( bool enabled )
{
    QMap<View*, int> allViews = GetIbisAPI()->GetAllViews();
    foreach( int id, allViews.values() )
    {
        if( id != m_currentViewID )
            GetIbisAPI()->GetViewByID( id )->SetRenderingEnabled( enabled );
    }
}
