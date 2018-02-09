#include "plusserverinterface.h"
#include <vtkObjectFactory.h>
#include <QFileInfo>

vtkStandardNewMacro(PlusServerInterface);

PlusServerInterface::PlusServerInterface() : m_serverLogLevel( 3 )
{
    m_CurrentServerInstance = 0;
}

PlusServerInterface::~PlusServerInterface()
{
    StopServer();
}

//-----------------------------------------------------------------------------
bool PlusServerInterface::StartServer(const QString& configFilePath)
{
    if (m_CurrentServerInstance != NULL)
    {
        StopServer();
    }

    // Make sure executable exists and is executable
    QFileInfo execInfo( m_plusServerExecutable );
    if( !execInfo.exists() )
    {
        SetLastErrorMessage( QString("PlusServer executable %1 not found").arg( m_plusServerExecutable) );
        return false;
    }
    if( !execInfo.isExecutable() )
    {
        SetLastErrorMessage( QString("PlusServer executable %1 is not executable").arg( m_plusServerExecutable) );
        return false;
    }

    // Make sure we have a valid config file before trying to launch a server
    QFileInfo configInfo( configFilePath );
    if( !configInfo.exists() )
    {
        SetLastErrorMessage( QString("PlusServer config () not found").arg( configFilePath ) );
        return false;
    }
    if( !configInfo.isReadable() )
    {
        SetLastErrorMessage( QString("PlusServer config () is not readable").arg( configFilePath ) );
        return false;
    }

    // Create a process
    m_CurrentServerInstance = new QProcess();
    if( !m_plusServerWorkingDirectory.isEmpty() )
        m_CurrentServerInstance->setWorkingDirectory( m_plusServerWorkingDirectory );

    connect(m_CurrentServerInstance, SIGNAL(readyReadStandardOutput()), this, SLOT(StdOutMsgReceived()));
    connect(m_CurrentServerInstance, SIGNAL(readyReadStandardError()), this, SLOT(StdErrMsgReceived()));
    connect(m_CurrentServerInstance, SIGNAL(error(QProcess::ProcessError)), this, SLOT(ErrorReceived(QProcess::ProcessError)));
    connect(m_CurrentServerInstance, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ServerExecutableFinished(int, QProcess::ExitStatus)));

    // PlusServerLauncher wants at least LOG_LEVEL_INFO to parse status information from the PlusServer executable
    // Un-requested log entries that are captured from the PlusServer executable are parsed and dropped from output
    QString cmdLine = QString("\"%1\" --config-file=\"%2\" --verbose=%3").arg(m_plusServerExecutable).arg(configFilePath).arg(m_serverLogLevel);
    m_CurrentServerInstance->start(cmdLine);
    m_CurrentServerInstance->waitForFinished(500);

    // During waitForFinished an error signal may be emitted, which may delete m_CurrentServerInstance,
    // therefore we need to check if m_CurrentServerInstance is still not NULL
    if (m_CurrentServerInstance && m_CurrentServerInstance->state() == QProcess::Running)
    {
        LogInfo("Server process started successfully");
        return true;
    }
    else
    {
        SetLastErrorMessage("Failed to start server process");
        LogError("Failed to start server process");
        return false;
    }
}

//-----------------------------------------------------------------------------
bool PlusServerInterface::StopServer()
{
    if (m_CurrentServerInstance == NULL)
    {
        // already stopped
        return true;
    }

    disconnect(m_CurrentServerInstance, SIGNAL(readyReadStandardOutput()), this, SLOT(StdOutMsgReceived()));
    disconnect(m_CurrentServerInstance, SIGNAL(readyReadStandardError()), this, SLOT(StdErrMsgReceived()));
    disconnect(m_CurrentServerInstance, SIGNAL(error(QProcess::ProcessError)), this, SLOT(ErrorReceived(QProcess::ProcessError)));
    disconnect(m_CurrentServerInstance, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ServerExecutableFinished(int, QProcess::ExitStatus)));

    bool forcedShutdown = false;
    if (m_CurrentServerInstance->state() == QProcess::Running)
    {
        m_CurrentServerInstance->terminate();
        if (m_CurrentServerInstance->state() == QProcess::Running)
        {
            LogInfo("Server process stop request sent successfully");
        }
        const int totalTimeoutSec = 15;
        const double retryDelayTimeoutSec = 0.3;
        double timePassedSec = 0;
        while (!m_CurrentServerInstance->waitForFinished(retryDelayTimeoutSec * 1000))
        {
            m_CurrentServerInstance->terminate(); // in release mode on Windows the first terminate request may go unnoticed
            timePassedSec += retryDelayTimeoutSec;
            if (timePassedSec > totalTimeoutSec)
            {
                // graceful termination was not successful, force the process to quit
                QString msg = QString("Server process did not stop on request for %1 seconds, force it to quit now.").arg( timePassedSec );
                LogWarning( msg );
                m_CurrentServerInstance->kill();
                forcedShutdown = true;
                break;
            }
        }
        LogInfo("Server process stopped successfully");
    }
    delete m_CurrentServerInstance;
    m_CurrentServerInstance = NULL;
    m_Suffix.clear();
    return (!forcedShutdown);
}



//-----------------------------------------------------------------------------
void PlusServerInterface::StdOutMsgReceived()
{
    QByteArray strData = m_CurrentServerInstance->readAllStandardOutput();
    SendServerOutputToLogger(strData);
}

//-----------------------------------------------------------------------------
void PlusServerInterface::StdErrMsgReceived()
{
    QByteArray strData = m_CurrentServerInstance->readAllStandardError();
    SendServerOutputToLogger(strData);
}

//-----------------------------------------------------------------------------
void PlusServerInterface::ErrorReceived(QProcess::ProcessError errorCode)
{
    const char* errorString = "unknown";
    switch ((QProcess::ProcessError)errorCode)
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
    default:
        errorString = "UnknownError";
    }
    LogError( QString("Server process error: %1").arg( errorString ) );
}

//-----------------------------------------------------------------------------
void PlusServerInterface::ServerExecutableFinished(int returnCode, QProcess::ExitStatus status)
{
    if (returnCode == 0)
    {
        LogInfo("Server process terminated.");
    }
    else
    {
        LogError( QString("Server stopped unexpectedly. Return code: %1").arg( returnCode ) );
    }
}

//----------------------------------------------------------------------------
/*void PlusServerInterface::ParseContent(const std::string& message)
{
    // Input is the format: message
    // Plus OpenIGTLink server listening on IPs:
    // 169.254.100.247
    // 169.254.181.13
    // 129.100.44.163
    // 192.168.199.1
    // 192.168.233.1
    // 127.0.0.1 -- port 18944
    if (message.find("Plus OpenIGTLink server listening on IPs:") != std::string::npos)
    {
        m_Suffix.append(message);
        m_Suffix.append("\n");
        m_DeviceSetSelectorWidget->SetDescriptionSuffix(QString(m_Suffix.c_str()));
    }
    else if (message.find("Server status: Server(s) are running.") != std::string::npos)
    {
        m_DeviceSetSelectorWidget->SetConnectionSuccessful(true);
        m_DeviceSetSelectorWidget->SetConnectButtonText(QString("Stop server"));
    }
    else if (message.find("Server status: ") != std::string::npos)
    {
        // pull off server status and display it
        this->m_DeviceSetSelectorWidget->SetDescriptionSuffix(QString(message.c_str()));
    }
}*/

void PlusServerInterface::LogInfo( const QString l )
{
    emit ServerOutputReceived( l );
}

void PlusServerInterface::LogWarning( const QString l )
{
    emit ServerOutputReceived( l );
}

void PlusServerInterface::LogError( const QString l )
{
    emit ServerOutputReceived( l );
}

void PlusServerInterface::SendServerOutputToLogger(const QByteArray& strData)
{
    QString s(strData);
    emit ServerOutputReceived( strData );

    /*typedef std::vector<std::string> StringList;

    QString string(strData);
    std::string logString(string.toLatin1().constData());

    if (logString.empty())
    {
        return;
    }

    // De-windows-ifiy
    ReplaceStringInPlace(logString, "\r\n", "\n");
    StringList tokens;

    if (logString.find('|') != std::string::npos)
    {
        PlusCommon::SplitStringIntoTokens(logString, '|', tokens, false);
        // Remove empty tokens
        for (StringList::iterator it = tokens.begin(); it != tokens.end(); ++it)
        {
            if (PlusCommon::Trim(*it).empty())
            {
                tokens.erase(it);
                it = tokens.begin();
            }
        }
        for (unsigned int index = 0; index < tokens.size(); ++index)
        {
            if (vtkPlusLogger::GetLogLevelType(tokens[index]) != vtkPlusLogger::LOG_LEVEL_UNDEFINED)
            {
                vtkPlusLogger::LogLevelType logLevel = vtkPlusLogger::GetLogLevelType(tokens[index++]);
                std::string timeStamp = tokens[index++];
                std::string message = tokens[index++];
                std::string location = tokens[index++];

                std::string file = location.substr(4, location.find_last_of('(') - 4);
                int lineNumber(0);
                std::stringstream lineNumberStr(location.substr(location.find_last_of('(') + 1, location.find_last_of(')') - location.find_last_of('(') - 1));
                lineNumberStr >> lineNumber;

                // Only parse for content if the line was successfully parsed for logging
                this->ParseContent(message);

                vtkPlusLogger::Instance()->LogMessage(logLevel, message.c_str(), file.c_str(), lineNumber, "SERVER");
            }
        }
    }
    else
    {
        PlusCommon::SplitStringIntoTokens(logString, '\n', tokens, false);
        for (StringList::iterator it = tokens.begin(); it != tokens.end(); ++it)
        {
            vtkPlusLogger::Instance()->LogMessage(vtkPlusLogger::LOG_LEVEL_INFO, *it, "SERVER");
            this->ParseContent(*it);
        }
        return;
    } */
}
