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

class IbisHardwareModule : public QObject, public HardwareModule
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
    virtual QString GetPluginName() { return QString("IbisHardwareModule"); }

    // Implementation of the HardwareModule interface
    virtual void AddSettingsMenuEntries( QMenu * menu );
    virtual void Init();
    virtual void Update();
    virtual bool ShutDown();

    virtual void AddToolObjectsToScene();
    virtual void RemoveToolObjectsFromScene();

    virtual vtkTransform * GetReferenceTransform();

    virtual bool IsTransformFrozen( TrackedSceneObject * obj );
    virtual void FreezeTransform( TrackedSceneObject * obj, int nbSamples );
    virtual void UnFreezeTransform( TrackedSceneObject * obj );

    virtual void AddTrackedVideoClient( TrackedSceneObject * obj );
    virtual void RemoveTrackedVideoClient( TrackedSceneObject * obj);

    virtual void StartTipCalibration( PointerObject * p );
    virtual double DoTipCalibration( PointerObject * p, vtkMatrix4x4 * calibMat );
    virtual bool IsCalibratingTip( PointerObject * p );
    virtual void StopTipCalibration( PointerObject * p );

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
