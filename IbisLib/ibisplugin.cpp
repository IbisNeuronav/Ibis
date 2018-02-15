/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "ibisplugin.h"
#include "ibisapi.h"
#include <QSettings>

IbisPlugin::IbisPlugin()
{
    m_ibisAPI = 0;
}

void IbisPlugin::BaseLoadSettings( QSettings & s )
{
    s.beginGroup( this->GetPluginName() );
    PluginTypeLoadSettings( s );
    LoadSettings( s );
    s.endGroup();
}

void IbisPlugin::BaseSaveSettings( QSettings & s )
{
    s.beginGroup( this->GetPluginName() );
    PluginTypeSaveSettings( s );
    SaveSettings( s );
    s.endGroup();
}

QString IbisPlugin::GetPluginTypeAsString()
{
    IbisPluginTypes type = this->GetPluginType();
    QString ret = "Unknown Type";
    switch (type)
    {
    case IbisPluginTypeTool:
        ret = "Tool";
        break;
    case IbisPluginTypeObject:
        ret = "Object";
        break;
    case IbisPluginTypeGlobalObject:
        ret = "Global Object";
        break;
    case IbisPluginTypeImportExport:
        ret = "Import/Export";
        break;
    case IbisPluginTypeHardwareModule:
        ret = "Hardware Module";
        break;
    case IbisPluginTypeGenerator:
        ret = "Generator";
        break;
    }
    return ret;
}

QString IbisPlugin::GetPluginDescription()
{
    return QString("This plugin didn't provide any description");
}
