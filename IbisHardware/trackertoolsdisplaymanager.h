/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __TrackerToolDisplayManager_h_
#define __TrackerToolDisplayManager_h_

#include "vtkObject.h"
#include <QObject>
#include <vector>

class Tracker;
class SceneManager;
class TrackedVideoSource;
class UsProbeObject;
class SceneObject;

class TrackerToolDisplayManager : public QObject, public vtkObject
{
    
Q_OBJECT    
    
public:

    static TrackerToolDisplayManager * New() { return new TrackerToolDisplayManager; }
    TrackerToolDisplayManager();
    ~TrackerToolDisplayManager();        
    
    void Settracker( Tracker * track );
    void SetSceneManager( SceneManager * manager ) { this->sceneManager = manager; }
    void SetvideoSource( TrackedVideoSource * probe );
    void Initialize();
    void ShutDown();
    void ClearAllObjects();

    void AddAllObjectsToScene();
    void RemoveAllObjectsFromScene();
    
public slots:

    // Description:
    // The following functions are callback for events
    // emited by the Tracker class.
    void ToolDeactivated( int );
    void ToolActivated( int );
    void TrackerUpdated( );
    void ToolUseChanged( int );
    void ToolNameChanged( int , QString );
    void ToolRemoved( int );
    void TrackerInitialized();
    
protected:
    
    void SetupCallbacks();
    void ClearCallbacks();
    
    void SetupObjects();
    void SetupObject( int index );
    void ClearObject( int toolIndex );
    
    Tracker         * tracker;
    SceneManager    * sceneManager;
    TrackedVideoSource * videoSource;
    
    // List of objects added to the scene manager
    typedef std::vector< SceneObject * > DisplayedObjectsVec;
    DisplayedObjectsVec DisplayedObjects;

    DisplayedObjectsVec::iterator GetSceneObjectByName( QString name );
    
};

#endif
