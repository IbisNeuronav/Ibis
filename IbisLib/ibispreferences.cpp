#include "ibispreferences.h"
#include "preferencewidget.h"

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

void IbisPreferences::RegisterPath(QString name, QString path )
{
    m_customPaths.insert( name, path );
}

void IbisPreferences::UnRegisterPath( QString & pathName )
{
    m_customPaths.remove( pathName );
}

QString IbisPreferences::GetPath( QString & pathName )
{
    QMap< QString, QString >::const_iterator it = m_customPaths.find( pathName );
    if( it != m_customPaths.end() )
       return it.value();
    return QString::null;
}

void IbisPreferences::ShowPreferenceDialog()
{
    m_prefWidget = new PreferenceWidget;
    m_prefWidget->SetPreferences( this );
    const QRect screenGeometry = QApplication::desktop()->screenGeometry();
    m_prefWidget->move( screenGeometry.width() / 3, screenGeometry.height() / 3 );
    m_prefWidget->show();
    connect( m_prefWidget, SIGNAL(destroyed()), this, SLOT(OnPreferenceWidgetClosed()) );
}

void IbisPreferences::OnPreferenceWidgetClosed()
{
    m_prefWidget = nullptr;
}
