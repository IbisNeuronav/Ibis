#include "ibispreferences.h"
#include "preferencewidget.h"
#include "application.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QRect>

IbisPreferences::IbisPreferences(QObject *parent) : QObject(parent)
{
    m_prefWidget = nullptr;
}

void IbisPreferences::LoadSettings( QSettings & settings )
{
    settings.beginGroup( "CustomPaths" );
    m_customPaths.clear();
    QStringList names = settings.allKeys();
    for( int i = 0; i < names.count(); i++ )
    {
        m_customPaths.insert( names[i], settings.value( names[i] ).toString() );
    }
    settings.endGroup();
}

void IbisPreferences::SaveSettings( QSettings & settings )
{
    settings.beginGroup( "CustomPaths" );
    settings.remove( "" );
    if( m_customPaths.count() > 0 )
    {
        QMap< QString, QString >::iterator it;
        for( it = m_customPaths.begin(); it != m_customPaths.end(); ++it )
        {
            settings.setValue( it.key(), it.value() );
        }
    }
    settings.endGroup();
}

void IbisPreferences::RegisterPath( const QString  & name, const QString & pathName )
{
    m_customPaths.insert( name, pathName );
}

void IbisPreferences::UnRegisterPath( const QString & pathName )
{
    m_customPaths.remove( pathName );
}

const QString IbisPreferences::GetPath( const QString & pathName )
{
    QMap< QString, QString >::const_iterator it = m_customPaths.find( pathName );
    if( it != m_customPaths.end() )
       return it.value();
    return QString::null;
}

bool IbisPreferences::IsPathRegistered( const QString &pathName )
{
    QMap< QString, QString >::const_iterator it = m_customPaths.find( pathName );
    if( it != m_customPaths.end() )
       return true;
    return false;
}

void IbisPreferences::ShowPreferenceDialog()
{
    m_prefWidget = new PreferenceWidget;
    m_prefWidget->SetPreferences( this );
    const QRect screenGeometry = QApplication::desktop()->screenGeometry();
    m_prefWidget->move( screenGeometry.width() / 3, screenGeometry.height() / 3 );
    connect( m_prefWidget, SIGNAL(destroyed()), this, SLOT(OnPreferenceWidgetClosed()) );
    Application::GetInstance().ShowFloatingDock( m_prefWidget );
}

void IbisPreferences::OnPreferenceWidgetClosed()
{
    m_prefWidget = nullptr;
}
