#include "plusserverinterface.h"

#include <vtkObjectFactory.h>

#include <QApplication>
#include <QFileInfo>
#include <QThread>
#include <QTime>

#include "logger.h"

vtkStandardNewMacro( PlusServerInterface );

PlusServerInterface::PlusServerInterface() : m_serverLogLevel( 3 )
{
    m_CurrentServerInstance = nullptr;
    m_state                 = Idle;
    m_logger                = nullptr;
}

PlusServerInterface::~PlusServerInterface() { StopServer(); }

//-----------------------------------------------------------------------------
bool PlusServerInterface::StartServer( const QString & configFilePath )
{
    if( m_CurrentServerInstance != nullptr )
    {
        StopServer();
    }

    // Make sure executable exists and is executable
    QFileInfo execInfo( m_plusServerExecutable );
    if( !execInfo.exists() )
    {
        SetLastErrorMessage( QString( "PlusServer executable %1 not found" ).arg( m_plusServerExecutable ) );
        return false;
    }
    if( !execInfo.isExecutable() )
    {
        SetLastErrorMessage( QString( "PlusServer executable %1 is not executable" ).arg( m_plusServerExecutable ) );
        return false;
    }

    // Make sure we have a valid config file before trying to launch a server
    QFileInfo configInfo( configFilePath );
    if( !configInfo.exists() )
    {
        SetLastErrorMessage( QString( "PlusServer config (%1) not found" ).arg( configFilePath ) );
        return false;
    }
    if( !configInfo.isReadable() )
    {
        SetLastErrorMessage( QString( "PlusServer config (%1) is not readable" ).arg( configFilePath ) );
        return false;
    }

    // Create a process
    m_CurrentServerInstance = new QProcess();
    if( !m_plusServerWorkingDirectory.isEmpty() )
        m_CurrentServerInstance->setWorkingDirectory( m_plusServerWorkingDirectory );

    connect( m_CurrentServerInstance, SIGNAL( readyReadStandardOutput() ), this, SLOT( StdOutMsgReceived() ) );
    connect( m_CurrentServerInstance, SIGNAL( readyReadStandardError() ), this, SLOT( StdErrMsgReceived() ) );
    connect( m_CurrentServerInstance, SIGNAL( error( QProcess::ProcessError ) ), this,
             SLOT( ErrorReceived( QProcess::ProcessError ) ) );
    connect( m_CurrentServerInstance, SIGNAL( finished( int, QProcess::ExitStatus ) ), this,
             SLOT( ServerExecutableFinished( int, QProcess::ExitStatus ) ) );

    // PlusServerLauncher wants at least LOG_LEVEL_INFO to parse status information from the PlusServer executable
    // Un-requested log entries that are captured from the PlusServer executable are parsed and dropped from output
    QString cmdLine = QString( "\"%1\" --config-file=\"%2\" --verbose=%3" )
                          .arg( m_plusServerExecutable )
                          .arg( configFilePath )
                          .arg( m_serverLogLevel );
    m_CurrentServerInstance->start( cmdLine );
    m_CurrentServerInstance->waitForStarted();

    // During waitForFinished an error signal may be emitted, which may delete m_CurrentServerInstance,
    // therefore we need to check if m_CurrentServerInstance is still not NULL
    if( m_CurrentServerInstance && m_CurrentServerInstance->state() == QProcess::Running )
    {
        SetState( Starting );
    }
    else
    {
        LogMessage( "Failed to start server process" );
        return false;
    }

    // Wait for the server to be listening or fail
    QThread * thread = QThread::currentThread();
    QTime timeWaited;
    timeWaited.start();
    while( timeWaited.elapsed() < 20000 && m_state == Starting )
    {
        thread->msleep( 100 );
        QApplication::processEvents();
    }

    return GetState() == Running;
}

bool PlusServerInterface::StopServer()
{
    if( !m_CurrentServerInstance )
    {
        // already stopped
        return true;
    }

    disconnect( m_CurrentServerInstance, SIGNAL( readyReadStandardOutput() ), this, SLOT( StdOutMsgReceived() ) );
    disconnect( m_CurrentServerInstance, SIGNAL( readyReadStandardError() ), this, SLOT( StdErrMsgReceived() ) );
    disconnect( m_CurrentServerInstance, SIGNAL( error( QProcess::ProcessError ) ), this,
                SLOT( ErrorReceived( QProcess::ProcessError ) ) );
    disconnect( m_CurrentServerInstance, SIGNAL( finished( int, QProcess::ExitStatus ) ), this,
                SLOT( ServerExecutableFinished( int, QProcess::ExitStatus ) ) );

    bool forcedShutdown = false;
    if( m_CurrentServerInstance->state() == QProcess::Running )
    {
        m_CurrentServerInstance->terminate();
        if( m_CurrentServerInstance->state() == QProcess::Running )
        {
            LogMessage( "Server process stop request sent successfully" );
        }
        const int totalTimeoutSec         = 15;
        const double retryDelayTimeoutSec = 0.3;
        double timePassedSec              = 0;
        while( !m_CurrentServerInstance->waitForFinished( static_cast<int>( retryDelayTimeoutSec * 1000 ) ) )
        {
            m_CurrentServerInstance
                ->terminate();  // in release mode on Windows the first terminate request may go unnoticed
            timePassedSec += retryDelayTimeoutSec;
            if( timePassedSec > totalTimeoutSec )
            {
                // graceful termination was not successful, force the process to quit
                QString msg = QString( "Server process did not stop on request for %1 seconds, force it to quit now." )
                                  .arg( timePassedSec );
                LogMessage( msg );
                m_CurrentServerInstance->kill();
                forcedShutdown = true;
                break;
            }
        }
        LogMessage( "Server process stopped successfully" );
    }
    delete m_CurrentServerInstance;
    m_CurrentServerInstance = nullptr;

    SetState( Idle );
    return ( !forcedShutdown );
}

//-----------------------------------------------------------------------------
void PlusServerInterface::StdOutMsgReceived()
{
    QByteArray strData = m_CurrentServerInstance->readAllStandardOutput();
    ParseServerOutput( strData );
}

//-----------------------------------------------------------------------------
void PlusServerInterface::StdErrMsgReceived()
{
    QByteArray strData = m_CurrentServerInstance->readAllStandardError();
    ParseServerOutput( strData );
}

//-----------------------------------------------------------------------------
void PlusServerInterface::ErrorReceived( QProcess::ProcessError errorCode )
{
    const char * errorString = "unknown";
    switch( static_cast<QProcess::ProcessError>( errorCode ) )
    {
        case QProcess::FailedToStart:
            errorString = "FailedToStart";
            break;
        case QProcess::Crashed:
            errorString = "Crashed";
            break;
        case QProcess::Timedout:
            errorString = "Timedout";
            break;
        case QProcess::WriteError:
            errorString = "WriteError";
            break;
        case QProcess::ReadError:
            errorString = "ReadError";
            break;
        case QProcess::UnknownError:
            errorString = "UnknownError";
            break;
    }
    std::cout << "Server process error: " << errorString << std::endl;
    LogMessage( QString( "Server process error: %1" ).arg( errorString ) );
}

void PlusServerInterface::ServerExecutableFinished( int returnCode, QProcess::ExitStatus )
{
    if( returnCode == 0 )
    {
        std::cout << "Server Terminated normally" << std::endl;
        LogMessage( "Server process terminated." );
    }
    else
    {
        std::cout << "Server terminated with error code " << returnCode << std::endl;
        LogMessage( QString( "Server stopped unexpectedly. Return code: %1" ).arg( returnCode ) );
    }
    SetState( Idle );
}

void PlusServerInterface::SetState( ServerState s )
{
    if( s != m_state )
    {
        m_state = s;
        emit StateChanged();
    }
}

void PlusServerInterface::LogMessage( const QString & s )
{
    if( m_logger )
    {
        m_logger->AddLog( s );
    }
}

void PlusServerInterface::ParseServerOutput( const QByteArray & strData )
{
    QString s( strData );

    std::cout << "Server output: " << s.toUtf8().data() << std::endl;

    if( s.contains( "Server status: Server(s) are running." ) )
    {
        SetState( Running );
    }

    LogMessage( s );
}
