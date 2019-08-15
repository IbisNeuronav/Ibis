#include "configio.h"
#include "serializer.h"
#include <QFileInfo>

ObjectSerializationMacro( ServerConfig );
ObjectSerializationMacro( ToolConfig );
ObjectSerializationMacro( DeviceToolAssociation );

ServerConfig::ServerConfig()
{
    m_serverName = "Local";
    m_ipAddress = "localhost";
    m_port = 18944; // There should be a constant in IO for this
    m_startAuto = true;
    m_connectAuto = true;
}

void ServerConfig::Serialize( Serializer * ser )
{
    ::Serialize( ser, "ServerName", m_serverName );
    ::Serialize( ser, "IPAddress", m_ipAddress );
    ::Serialize( ser, "Port", m_port );
    ::Serialize( ser, "StartAuto", m_startAuto );
    ::Serialize( ser, "ConnectAuto", m_connectAuto );
    ::Serialize( ser, "PlusConfigFile", m_plusConfigFile );
}

void ToolConfig::Serialize( Serializer * ser )
{
    ::Serialize( ser, "ToolName", ToolName );
    ::Serialize( ser, "ToolType", ToolType );
    ::Serialize( ser, "ToolModelFile", ToolModelFile);
    ::Serialize(ser, "ToolParamFile", ToolParamFile);
}

void DeviceToolAssociation::Serialize( Serializer * ser )
{
    ::Serialize( ser, "DeviceName", deviceName );
    ::Serialize( ser, "ToolName", toolName );
    ::Serialize( ser, "ToolPart", toolPart );
}

void DeviceToolMap::ToolAndPartFromDevice( const QString & device, QString & tool, QString & part )
{
	QPair<QString, QString> defaultValue( QString(""), QString("") );
    QPair<QString,QString> val = value( device, defaultValue );
    tool = val.first;
    part = val.second;
}

ConfigIO::ConfigIO( QString configDir )
{
    ReadConfig( configDir );
}

std::string ConfigIO::GetServerName( int index )
{
    Q_ASSERT( index >=0 && index < m_servers.size() );
    return m_servers[index].m_serverName;
}

std::string ConfigIO::GetServerIPAddress( int index )
{
    Q_ASSERT( index >=0 && index < m_servers.size() );
    return m_servers[index].m_ipAddress;
}

int ConfigIO::GetServerPort( int index )
{
    Q_ASSERT( index >=0 && index < m_servers.size() );
    return m_servers[index].m_port;
}

bool ConfigIO::GetStartAuto( int index )
{
    Q_ASSERT( index >=0 && index < m_servers.size() );
    return m_servers[index].m_startAuto;
}

bool ConfigIO::GetConnectAuto( int index )
{
    Q_ASSERT( index >=0 && index < m_servers.size() );
    return m_servers[index].m_connectAuto;
}

QString ConfigIO::GetPlusConfigFile( int index )
{
    Q_ASSERT( index >=0 && index < m_servers.size() );
    return m_servers[index].m_plusConfigFile;
}

QString ConfigIO::GetToolName( int index )
{
    Q_ASSERT( index >=0 && index < m_tools.size() );
    return m_tools[index].ToolName;
}

QString ConfigIO::GetToolType( int index )
{
    Q_ASSERT( index >=0 && index < m_tools.size() );
    return m_tools[index].ToolType;
}

QString ConfigIO::GetToolModelFile(int index)
{
    Q_ASSERT(index >= 0 && index < m_tools.size());
    return m_tools[index].ToolModelFile;
}

QString ConfigIO::GetToolParamFile(int index)
{
    Q_ASSERT(index >= 0 && index < m_tools.size());
    return m_tools[index].ToolParamFile;
}

DeviceToolMap ConfigIO::GetAssociations()
{
    DeviceToolMap res;
    for( int i = 0; i < m_associations.size(); ++i )
    {
        res[ m_associations[i].deviceName ] = QPair<QString,QString>( m_associations[i].toolName, m_associations[i].toolPart );
    }
    return res;
}

#include <iostream>

bool ConfigIO::ReadConfig( QString configFile )
{
    // Make sure the config file exists
    QFileInfo info( configFile );
    if( !info.exists() || !info.isReadable() )
        return false;

    // Read config file
    SerializerReader reader;
    reader.SetFilename( configFile.toUtf8().data() );
    reader.Start();
    ::Serialize( &reader, "Servers", m_servers );
    ::Serialize( &reader, "Tools", m_tools );
    ::Serialize( &reader, "Associations", m_associations );
    reader.Finish();

    return true;
}
