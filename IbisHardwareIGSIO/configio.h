#ifndef __ConfigIO_h_
#define __ConfigIO_h_

#include <QList>
#include <string>
#include "serializer.h"

class ServerConfig
{

public:

    ServerConfig();
    void Serialize( Serializer * ser );

    std::string m_serverName;
    std::string m_ipAddress;
    int m_port;
    bool m_startAuto;
    bool m_connectAuto;
    QString m_plusConfigFile;
};

ObjectSerializationHeaderMacro( ServerConfig );

class ConfigIO
{

public:

    ConfigIO( QString configDir );
    int GetNumberOfServers() { return m_servers.size(); }
    std::string GetServerName( int index );
    std::string GetServerIPAddress( int index );
    int GetServerPort( int index );
    bool GetStartAuto( int index );
    bool GetConnectAuto( int index );
    QString GetPlusConfigFile( int index );

protected:

    bool ReadConfig( QString configDir );

    QList<ServerConfig> m_servers;
};

#endif
