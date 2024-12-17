/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <vtkObject.h>

#include <QObject>

class QTimer;

// Description:
// This is the main clock of Ibis. It is here to trigger
// hardware update and keep data fixed until next update
// Warning: the update period we talk about here has nothing
// to do with the rate at which the application acquires data
// internally. The update is just for refreshing the info in the
// gui.
class UpdateManager : public QObject, public vtkObject
{
    Q_OBJECT

public:
    static UpdateManager * New() { return new UpdateManager; }

    vtkTypeMacro( UpdateManager, vtkObject );

    UpdateManager();
    ~UpdateManager();

    void SetUpdatePeriod( int msecPeriod );

    void Start();
    bool IsRunning();
    void Stop();

public slots:

    void TimerCallback();

private:
    QTimer * m_timer;
    int m_timerPeriod;
};

#endif
