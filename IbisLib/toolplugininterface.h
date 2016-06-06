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
#include <QtPlugin>
#include "serializer.h"

class Application;
class SceneManager;
class QSettings;
class QWidget;

class ToolPluginInterface
{

public:
    
    ToolPluginInterface() : m_application(0) {}
    virtual ~ToolPluginInterface() {}

    virtual void Serialize( Serializer * serializer );

    // System functions
    struct Settings
    {
        Settings() : winPos( 0, 0 ), winSize( -1, -1 ), active( false ) {}
        QPoint winPos;
        QSize winSize;
        bool active;
    };
    
    void BaseLoadSettings( QSettings & s );
    void BaseSaveSettings( QSettings & s );
    Settings & GetSettings() { return m_settings; }
    void SetApplication( Application * app ) { m_application = app; }
    Application * GetApplication() { return m_application; }  
    SceneManager * GetSceneManager();
    bool IsPluginActive() { return m_settings.active; }

    // Give a chance to plugin to initialize things right after construction
    // but with pointer to application and SceneManager available.
    virtual void InitPlugin() {}

    // Functions that should be overriden in plugins
    virtual QString GetPluginName() = 0;
    virtual bool CanRun() = 0;
    virtual QString GetMenuEntryString() = 0;

    // Plugins should create one (and only one) of those widgets. Widgets are managed by the system.
    virtual QWidget * CreateFloatingWidget() { return 0; }
    virtual QWidget * CreateTab() { return 0; }

    // Give plugin a chance to react when the widget (either tab or floating widget) is about to be closed
    // Returning false will cause the plugin not to close.
    virtual bool WidgetAboutToClose() { return true; }

    // Give plugin a chance to react before/after scene loading/saving
    virtual void SceneAboutToLoad() {}
    virtual void SceneFinishedLoading() {}
    virtual void SceneAboutToSave() {}
    virtual void SceneFinishedSaving() {}
    
protected:

    // Override this function to save settings for your plugin
    virtual void LoadSettings( QSettings & s ) {}
    virtual void SaveSettings( QSettings & s ) {}

    Application * m_application;
    Settings m_settings;

};

Q_DECLARE_INTERFACE( ToolPluginInterface, "ca.mcgill.mni.bic.Ibis.ToolPluginInterface/1.0" );

ObjectSerializationHeaderMacro( ToolPluginInterface );

#endif
