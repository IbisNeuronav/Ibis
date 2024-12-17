#ifndef CONFIGIO_H
#define CONFIGIO_H

#include <QList>
#include <QMap>
#include <QPair>
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

class ToolConfig
{
public:
    void Serialize( Serializer * ser );
    QString ToolName;
    QString ToolType;
    QString ToolModelFile;
    QString ToolParamFile;
};

ObjectSerializationHeaderMacro( ToolConfig );

class DeviceToolAssociation
{
public:
    void Serialize( Serializer * ser );
    QString deviceName;
    QString toolName;
    QString toolPart;
};

ObjectSerializationHeaderMacro( DeviceToolAssociation );

class DeviceToolMap : public QMap<QString, QPair<QString, QString> >
{
public:
    void ToolAndPartFromDevice( const QString & device, QString & tool, QString & part );
};

class ConfigIO
{
public:
    ConfigIO( QString configDir );

    // Servers
    int GetNumberOfServers() { return m_servers.size(); }
    std::string GetServerName( int index );
    std::string GetServerIPAddress( int index );
    int GetServerPort( int index );
    bool GetStartAuto( int index );
    bool GetConnectAuto( int index );
    QString GetPlusConfigFile( int index );

    // Tools
    int GetNumberOfTools() { return m_tools.size(); }
    QString GetToolName( int index );
    QString GetToolType( int index );
    QString GetToolModelFile( int index );
    QString GetToolParamFile( int index );

    // Associations
    DeviceToolMap GetAssociations();

protected:
    bool ReadConfig( QString configDir );

    QList<ServerConfig> m_servers;
    QList<ToolConfig> m_tools;
    QList<DeviceToolAssociation> m_associations;
};

#endif
