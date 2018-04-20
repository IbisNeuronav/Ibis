#ifndef __PlusServerInterface_h_
#define __PlusServerInterface_h_

#include <QObject>
#include "vtkObject.h"
#include <QProcess>

class PlusServerInterface : public QObject, public vtkObject
{
    Q_OBJECT

public:

    static PlusServerInterface *New();
    vtkTypeMacro(PlusServerInterface, vtkObject);

    void SetServerExecutable( QString e ) { m_plusServerExecutable = e; }
    void SetServerWorkingDirectory( QString wd ) { m_plusServerWorkingDirectory = wd; }
    void SetServerLogLevel( int logLevel ) { m_serverLogLevel = logLevel; }

    /*! Start server process, connect outputs to logger. Returns with true on success. */
    bool StartServer(const QString& configFilePath);

    /*! Stop server process, disconnect outputs. Returns with true on success (shutdown on request was successful, without forcing). */
    bool StopServer();

    void SetLastErrorMessage( QString msg ) { m_lastErrorMessage = msg; }
    QString GetLastErrorMessage() { return m_lastErrorMessage; }

signals:

    void ServerOutputReceived( QString msg );

protected slots:

    void StdOutMsgReceived();
    void StdErrMsgReceived();
    void ErrorReceived(QProcess::ProcessError);
    void ServerExecutableFinished(int returnCode, QProcess::ExitStatus status);

protected:

    void LogInfo( const QString l );
    void LogWarning( const QString l );
    void LogError( const QString l );

    /*! Receive standard output or error and send it to the log */
    void SendServerOutputToLogger(const QByteArray& strData);

    /*! Parse a given log line for salient information from the PlusServer */
    //void ParseContent(const std::string& message);

private:

    PlusServerInterface();
    virtual ~PlusServerInterface();
    PlusServerInterface(const PlusServerInterface&); // Not implemented
    void operator=(const PlusServerInterface&); // Not implemented

    QString m_plusServerExecutable;
    QString m_plusServerWorkingDirectory;
    int m_serverLogLevel;

    /*! List of active ports for PlusServers */
    QString m_Suffix;

    /*! PlusServer instance that is responsible for all data collection and network transfer */
    QProcess * m_CurrentServerInstance;

    QString m_lastErrorMessage;

};

#endif
