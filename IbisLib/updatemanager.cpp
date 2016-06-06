/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "updatemanager.h"
#include "application.h"
#include <QTimer>
#include <QTime>

UpdateManager::UpdateManager()
{
    m_timer = new QTimer( NULL );
    connect( m_timer, SIGNAL(timeout()), this, SLOT(TimerCallback()) );
    m_timerPeriod = 66;  // 66 ms => 15 fps
    m_lastUpdateTime = new QTime;
}

UpdateManager::~UpdateManager()
{
    delete m_timer;
    delete m_lastUpdateTime;
}

void UpdateManager::SetUpdatePeriod( int msecPeriod )
{
    m_timerPeriod = msecPeriod;
}

void UpdateManager::Start()
{
    Q_ASSERT( !m_timer->isActive() );

    // Make the timer fire on when system is idle
    m_timer->start( 0 );
    m_lastUpdateTime->start();
}

bool UpdateManager::IsRunning()
{
    return m_timer->isActive();
}

void UpdateManager::Stop()
{
    m_timer->stop();
}

void UpdateManager::TimerCallback()
{
    if( m_lastUpdateTime->elapsed() >= m_timerPeriod )
    {
        Application::GetInstance().TickIbisClock();
        m_lastUpdateTime->restart();
    }
}
