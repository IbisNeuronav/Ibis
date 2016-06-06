/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "toolplugininterface.h"
#include <QSettings>
#include "application.h"

ObjectSerializationMacro( ToolPluginInterface );

void ToolPluginInterface::BaseLoadSettings( QSettings & s )
{
    s.beginGroup( this->GetPluginName() );
    m_settings.winPos = s.value( "WindowPosition", QPoint( 0, 0 ) ).toPoint();
    m_settings.winSize = s.value( "WindowSize", QSize( -1, -1 ) ).toSize();
    m_settings.active = s.value( "Active", false ).toBool();
    LoadSettings( s );
    s.endGroup();
}

void ToolPluginInterface::BaseSaveSettings( QSettings & s )
{
    s.beginGroup( this->GetPluginName() );
    s.setValue( "WindowPosition", m_settings.winPos );
    s.setValue( "WindowSize", m_settings.winSize );
    s.setValue( "Active", m_settings.active );
    SaveSettings( s );
    s.endGroup();
}

SceneManager * ToolPluginInterface::GetSceneManager()
{
    Q_ASSERT( m_application );
    return m_application->GetSceneManager();
}

void ToolPluginInterface::Serialize( Serializer * ser )
{
}
