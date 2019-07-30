#include "ibispreferences.h"
#include "preferencewidget.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QRect>
#include <QFileInfo>

IbisPreferences::IbisPreferences(QObject *parent) : QObject(parent)
{
    m_prefWidget = nullptr;
}

void IbisPreferences::LoadSettings( QSettings & settings )
{
    settings.beginGroup( "CustomPaths" );
    m_customVariables.clear();
    QStringList names = settings.allKeys();
    for( int i = 0; i < names.count(); i++ )
    {
        QString customVariable = settings.value( names[i] ).toString();
        QFileInfo f(customVariable);
        VARIABLE_TYPE varType = VARIABLE_TYPE::DIRECTORY_VARIABLE_TYPE;
        if( f.isFile() )
            varType = VARIABLE_TYPE::FILE_VARIABLE_TYPE;
        TypedVariable tvar;
        tvar.name = customVariable;
        tvar.varType = varType;
        m_customVariables.insert( names[i], tvar );
    }
    settings.endGroup();
}

void IbisPreferences::SaveSettings( QSettings & settings )
{
    settings.beginGroup( "CustomPaths" );
    settings.remove( "" );
    if( m_customVariables.count() > 0 )
    {
        QMap< QString, TypedVariable >::iterator it;
        for( it = m_customVariables.begin(); it != m_customVariables.end(); ++it )
        {
            TypedVariable tvar = it.value();
            settings.setValue( it.key(), tvar.name );
        }
    }
    settings.endGroup();
}

void IbisPreferences::RegisterCustomVariable(const QString  & varName, const QString & customVariable, VARIABLE_TYPE varType )
{
    TypedVariable tvar;
    tvar.name = customVariable;
    QFileInfo fi(customVariable);
        tvar.varType = varType;
    m_customVariables.insert( varName, tvar );
}

void IbisPreferences::UnRegisterCustomVariable( const QString & varName )
{
    m_customVariables.remove( varName );
}

const QString IbisPreferences::GetCustomVariable(const QString & varName )
{
    QMap< QString, TypedVariable >::const_iterator it = m_customVariables.find( varName );
    if( it != m_customVariables.end() )
    {
       TypedVariable tvar = it.value();
       return tvar.name;
    }
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
