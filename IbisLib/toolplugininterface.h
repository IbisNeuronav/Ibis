/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TOOLPLUGININTERFACE_H
#define TOOLPLUGININTERFACE_H

#include <QPoint>
#include <QSize>
#include <QString>

#include "ibisplugin.h"
#include "serializer.h"

class SceneManager;
class QSettings;
class QWidget;

/**
 * @class   ToolPluginInterface
 * @brief   Create a tool
 *
 * Create a tool to process data in the scene, eg. registration.
 *
 * @sa IbisPlugin
 */
class ToolPluginInterface : public IbisPlugin
{
public:
    vtkTypeMacro( ToolPluginInterface, IbisPlugin );

    ToolPluginInterface() {}
    virtual ~ToolPluginInterface() override {}

    /** Implementation of IbisPlugin interface. */
    IbisPluginTypes GetPluginType() override { return IbisPluginTypeTool; }

    /** Serialize plugin parameters. */
    virtual void Serialize( Serializer * serializer ) override;

    struct Settings
    {
        Settings() : winPos( 0, 0 ), winSize( -1, -1 ), active( false ) {}
        QPoint winPos;
        QSize winSize;
        bool active;
    };

    /** Plugin settings. */
    Settings & GetSettings() { return m_settings; }

    /** Check if the plugin is activated, used to manage plugin widgets. */
    bool IsPluginActive() { return m_settings.active; }

    /** @name Function that should be overriden in plugins.
     */
    ///@{
    /** Check if the conditions to execute the plugin are satisfied. */
    virtual bool CanRun() = 0;
    /** Return the name of the plugin to display in the Plugins menu. */
    virtual QString GetMenuEntryString() = 0;
    ///@}
    ///
    /** @name Plugin Widgets.
     * Plugins should create one (and only one) of those widgets. Widgets are managed by the system.
     */
    virtual QWidget * CreateFloatingWidget() { return nullptr; }
    virtual QWidget * CreateTab() { return nullptr; }
    ///@}
    /** Give plugin a chance to react when the widget (either tab or floating widget) is about to be closed
     * Returning false will cause the plugin not to close.
     */
    virtual bool WidgetAboutToClose() { return true; }

protected:
    virtual void PluginTypeLoadSettings( QSettings & s ) override;
    virtual void PluginTypeSaveSettings( QSettings & s ) override;

    Settings m_settings;
};

ObjectSerializationHeaderMacro( ToolPluginInterface );

#endif
