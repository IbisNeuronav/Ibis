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
#include "application.h"
#include "scenemanager.h"
#include "view.h"
#include <QTimer>
#include <QTime>

FrameRateTesterPluginInterface::FrameRateTesterPluginInterface()
{
    m_numberOfFrames = 200;
    m_timer = 0;
    m_lastNumberOfFrames = 0;
    m_lastPeriod = 0.0;
    m_currentViewIndex = 1;
    m_time = new QTime;
    m_accumulatedFrames = 0;
}

FrameRateTesterPluginInterface::~FrameRateTesterPluginInterface()
{
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
    emit Modified();
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

void FrameRateTesterPluginInterface::SetCurrentViewIndex( int index )
{
    Q_ASSERT( index < GetSceneManager()->GetNumberOfViews() && index >= 0 );
    m_currentViewIndex = index;
    emit Modified();
}

void FrameRateTesterPluginInterface::OnTimerTriggered()
{
    if( m_lastNumberOfFrames == 0 )
        m_time->restart();

    // Render
    GetSceneManager()->GetViewByIndex( m_currentViewIndex )->Render();

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
    int nbViews = GetSceneManager()->GetNumberOfViews();
    for( int i = 0; i < nbViews; ++i )
    {
        if( i != m_currentViewIndex )
            GetSceneManager()->GetViewByIndex( i )->SetRenderingEnabled( enabled );
    }
}
