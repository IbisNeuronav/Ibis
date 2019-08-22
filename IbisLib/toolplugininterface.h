/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __ToolPluginInterface_h_
#define __ToolPluginInterface_h_

#include <QString>
#include <QPoint>
#include <QSize>
#include "serializer.h"
#include "ibisplugin.h"

class SceneManager;
class QSettings;
class QWidget;

class ToolPluginInterface : public IbisPlugin
{

public:

    vtkTypeMacro( ToolPluginInterface, IbisPlugin );
    
    ToolPluginInterface() {}
    virtual ~ToolPluginInterface() override {}

    // Implementation of IbisPlugin interface
    IbisPluginTypes GetPluginType()  override { return IbisPluginTypeTool; }

    virtual void Serialize( Serializer * serializer ) override;

    // System functions
    struct Settings
    {
        Settings() : winPos( 0, 0 ), winSize( -1, -1 ), active( false ) {}
        QPoint winPos;
        QSize winSize;
        bool active;
    };
    
    Settings & GetSettings() { return m_settings; }

    bool IsPluginActive() { return m_settings.active; }

    // Functions that should be overriden in plugins
    virtual bool CanRun() = 0;
    virtual QString GetMenuEntryString() = 0;

    // Plugins should create one (and only one) of those widgets. Widgets are managed by the system.
    virtual QWidget * CreateFloatingWidget() { return nullptr; }
    virtual QWidget * CreateTab() { return nullptr; }

    // Give plugin a chance to react when the widget (either tab or floating widget) is about to be closed
    // Returning false will cause the plugin not to close.
    virtual bool WidgetAboutToClose() { return true; }

protected:

    virtual void PluginTypeLoadSettings( QSettings & s ) override;
    virtual void PluginTypeSaveSettings( QSettings & s ) override;

    Settings m_settings;

};

ObjectSerializationHeaderMacro( ToolPluginInterface );

#endif
