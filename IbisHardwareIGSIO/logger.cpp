#include "logger.h"

const int Logger::m_maxNumberOfLogs = 3000;

Logger::Logger( QObject * parent ) : QObject( parent ) {}

void Logger::AddLog( const QString & log )
{
    m_log.push_back( log );
    if( m_log.size() > m_maxNumberOfLogs )
    {
        m_log.pop_front();
    }
    emit LogAdded( log );
}

void Logger::ClearLog()
{
    m_log.clear();
    emit LogCleared();
}

QString Logger::GetAll() const { return m_log.join( "" ); }

QString Logger::GetLast() const { return m_log.back(); }
