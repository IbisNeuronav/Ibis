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

#ifndef PLUSSERVERINTERFACE_H
#define PLUSSERVERINTERFACE_H

#include <vtkObject.h>
#include <vtkSmartPointer.h>

#include <QObject>
#include <QProcess>

class Logger;

typedef vtkSmartPointer<class PlusServerInterface> PlusServerInterfacePtr;

class PlusServerInterface : public QObject, public vtkObject
{
    Q_OBJECT

public:
    enum ServerState
    {
        Idle,
        Starting,
        Running,
        Terminated,
        Crashed,
        NbOfServerStates
    };

    static PlusServerInterface * New();
    vtkTypeMacro( PlusServerInterface, vtkObject );

    void SetServerExecutable( QString e ) { m_plusServerExecutable = e; }
    void SetServerWorkingDirectory( QString wd ) { m_plusServerWorkingDirectory = wd; }
    void SetServerLogLevel( int logLevel ) { m_serverLogLevel = logLevel; }
    void SetLogger( Logger * l ) { m_logger = l; }

    // Start server process, connect outputs to logger. Returns with true on success.
    bool StartServer( const QString & configFilePath );

    // Stop server process, disconnect outputs. Returns with true on success (shutdown on request was successful,
    // without forcing).
    bool StopServer();

    ServerState GetState() { return m_state; }

    void SetLastErrorMessage( QString msg ) { m_lastErrorMessage = msg; }
    QString GetLastErrorMessage() { return m_lastErrorMessage; }

signals:

    void StateChanged();

protected slots:

    void StdOutMsgReceived();
    void StdErrMsgReceived();
    void ErrorReceived( QProcess::ProcessError );
    void ServerExecutableFinished( int returnCode, QProcess::ExitStatus status );

protected:
    void SetState( ServerState s );

    void LogMessage( const QString & s );

    // Receive standard output or error and send it to the log
    void ParseServerOutput( const QByteArray & strData );

private:
    PlusServerInterface();
    virtual ~PlusServerInterface() override;
    PlusServerInterface( const PlusServerInterface & );  // Not implemented
    void operator=( const PlusServerInterface & );       // Not implemented

    QString m_plusServerExecutable;
    QString m_plusServerWorkingDirectory;
    int m_serverLogLevel;

    // Used to store all outputs of the server
    Logger * m_logger;

    ServerState m_state;

    // PlusServer instance that is responsible for all data collection and network transfer
    QProcess * m_CurrentServerInstance;

    QString m_lastErrorMessage;
};

#endif
