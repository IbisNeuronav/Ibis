/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __IbisPlugin_h_
#define __IbisPlugin_h_

#include "ibistypes.h"
#include "vtkObject.h"
#include "viewinteractor.h"
#include "serializer.h"
#include <QString>
#include <QObject>

class IbisAPI;
class Application;
class QSettings;


class IbisPlugin : public QObject, public vtkObject, public ViewInteractor
{

    Q_OBJECT

public:

    vtkTypeMacro( IbisPlugin, vtkObject );

    IbisAPI * GetIbisAPI() { return m_ibisAPI; }

    virtual QString GetPluginName() = 0;
    virtual IbisPluginTypes GetPluginType() = 0;
    QString GetPluginTypeAsString();
    virtual QString GetPluginDescription();

    virtual void Serialize( Serializer * ser ) {}

    // Give plugin a chance to react before/after scene loading/saving
    virtual void SceneAboutToLoad() {}
    virtual void SceneFinishedLoading() {}
    virtual void SceneAboutToSave() {}
    virtual void SceneFinishedSaving() {}

signals:

    void PluginModified();

protected:

    IbisPlugin();
    virtual ~IbisPlugin() {}

    // Give a chance to plugin to initialize things right after construction
    // but with a valid pointer to m_ibiAPI and after settings have been loaded.
    // This function can be overriden by every plugin to initialize its internal data.
    virtual void InitPlugin() {}

    // These functions should be overriden only by the base class for each plugin type base classes.
    virtual void PluginTypeLoadSettings( QSettings & s ) {}
    virtual void PluginTypeSaveSettings( QSettings & s ) {}

    // Override this function to save settings for your plugin
    virtual void LoadSettings( QSettings & s ) {}
    virtual void SaveSettings( QSettings & s ) {}

private:

    Application * m_application;

    friend class Application;

    IbisAPI * m_ibisAPI;

    friend class IbisAPI;

    // Should only be called by Application at init and shutdown
    void SetIbisAPI( IbisAPI * api ) { m_ibisAPI = api; }
    void BaseLoadSettings( QSettings & s );
    void BaseSaveSettings( QSettings & s );

};

Q_DECLARE_INTERFACE( IbisPlugin, "Ibis.IbisPluginInterface/1.0" );

ObjectSerializationHeaderMacro( IbisPlugin );

#endif
