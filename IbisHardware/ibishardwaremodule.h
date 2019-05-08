/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __IbisHardwareModule_h_
#define __IbisHardwareModule_h_

#include "hardwaremodule.h"

class Tracker;
class TrackedVideoSource;
class TrackerToolDisplayManager;
class QMenu;

class IbisHardwareModule : public HardwareModule
{
    
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA(IID "Ibis.IbisHardwareModule" )
    
public:

    vtkTypeMacro( IbisHardwareModule, HardwareModule );

    static IbisHardwareModule * New() { return new IbisHardwareModule; }
    IbisHardwareModule();
    ~IbisHardwareModule();

    // Implementation of IbisPlugin interface
    virtual QString GetPluginName() override { return QString("IbisHardwareModule"); }

    // Implementation of the HardwareModule interface
    virtual void AddSettingsMenuEntries( QMenu * menu ) override;
    virtual void Init() override;
    virtual void Update() override;
    virtual bool ShutDown() override;

    virtual void AddToolObjectsToScene() override;
    virtual void RemoveToolObjectsFromScene() override;

    virtual vtkTransform * GetReferenceTransform() override;

    virtual bool IsTransformFrozen( TrackedSceneObject * obj ) override;
    virtual void FreezeTransform( TrackedSceneObject * obj, int nbSamples ) override;
    virtual void UnFreezeTransform( TrackedSceneObject * obj ) override;

    virtual void AddTrackedVideoClient( TrackedSceneObject * obj ) override;
    virtual void RemoveTrackedVideoClient( TrackedSceneObject * obj) override;

    // Local methods
    TrackedVideoSource * GetVideoSource( int index ) { return m_trackedVideoSource; } // simtodo : allow more than one source

private slots:

    void OpenVideoSettingsDialog();
    void OpenTrackerSettingsDialog();
    void OnWriteHardwareSettingsMenuActivated();

protected:

    void ReadHardwareConfig();
    void WriteHardwareConfig();
    void InternalWriteHardwareConfig( QString filename );

    TrackedVideoSource          * m_trackedVideoSource;
    Tracker                     * m_tracker;
};

#endif
