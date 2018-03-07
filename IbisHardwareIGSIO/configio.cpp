#include "configio.h"
#include "serializer.h"
#include <QFileInfo>

ObjectSerializationMacro( ServerConfig );

ServerConfig::ServerConfig()
{
    m_serverName = "Local";
    m_ipAddress = "localhost";
    m_port = 18944;
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
    reader.Finish();

    return true;
}
