/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "trackertoolsdisplaymanager.h"
#include "tracker.h"
#include "trackedvideosource.h"
#include "scenemanager.h"
#include "vtkTrackerTool.h"
#include "vtkTransform.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"
#include "generictrackedobject.h"

vtkCxxSetObjectMacro( TrackerToolDisplayManager, tracker, Tracker );
vtkCxxSetObjectMacro( TrackerToolDisplayManager, videoSource, TrackedVideoSource );


TrackerToolDisplayManager::TrackerToolDisplayManager()
{
    this->tracker = 0;
    this->sceneManager = 0;
    this->videoSource = 0;
}

TrackerToolDisplayManager::~TrackerToolDisplayManager()
{
    Q_ASSERT( this->DisplayedObjects.size() == 0 );
    
    if( this->tracker )
        this->tracker->UnRegister( this );
    if( this->videoSource )
        this->videoSource->UnRegister( this );
}

void TrackerToolDisplayManager::Initialize()
{
    this->SetupObjects();
    this->SetupCallbacks(); 
}

void TrackerToolDisplayManager::ShutDown()
{
    this->ClearAllObjects();
    this->ClearCallbacks();
}

void TrackerToolDisplayManager::ToolDeactivated( int toolIndex )
{
    this->ClearObject( toolIndex );
}

void TrackerToolDisplayManager::ToolActivated( int toolIndex )
{
    this->SetupObject( toolIndex );
}

void TrackerToolDisplayManager::TrackerUpdated( )
{
    DisplayedObjectsVec::iterator it = DisplayedObjects.begin();
    for( ; it != DisplayedObjects.end(); ++it )
    {
        (*it)->NotifyTransformChanged();
    }
}

void TrackerToolDisplayManager::ToolUseChanged( int toolIndex )
{
    Q_ASSERT( this->tracker );

    if( this->tracker->IsToolActive( toolIndex ) )
    {
        this->ClearObject( toolIndex );
        this->SetupObject( toolIndex );
    }
}

void TrackerToolDisplayManager::ToolNameChanged( int toolIndex, QString prevName )
{
    DisplayedObjectsVec::iterator it = this->GetSceneObjectByName( prevName );
    if( it != this->DisplayedObjects.end() )
    {
        (*it)->SetName( this->tracker->GetToolName( toolIndex ) );
    }
}

void TrackerToolDisplayManager::ToolRemoved( int toolIndex )
{
    this->SetupObjects();
}

void TrackerToolDisplayManager::TrackerInitialized()
{
    this->SetupObjects();
}

void TrackerToolDisplayManager::SetupCallbacks()
{
    Q_ASSERT_X( this->tracker, "TrackerToolDisplayManager::SetupCallbacks()", "Need to set tracker before calling this function." );
    connect( this->tracker, SIGNAL( ToolActivated(int) ), this, SLOT( ToolActivated(int) ) );
    connect( this->tracker, SIGNAL( ToolDeactivated(int) ), this, SLOT( ToolDeactivated(int) ) );
    connect( this->tracker, SIGNAL( TrackerInitialized() ), this, SLOT( TrackerInitialized() ) );
    connect( this->tracker, SIGNAL( ToolUseChanged( int ) ), this, SLOT( ToolUseChanged( int ) ) );
    connect( this->tracker, SIGNAL( ToolNameChanged( int, QString ) ), this, SLOT( ToolNameChanged( int, QString ) ) );
    connect( this->tracker, SIGNAL( ToolRemoved(int) ), this, SLOT( ToolRemoved( int ) ) );
}

void TrackerToolDisplayManager::ClearCallbacks()
{
    Q_ASSERT_X( this->tracker, "TrackerToolDisplayManager::ClearCallbacks()", "Need to set tracker before calling this function." );
    this->tracker->disconnect( this );
}

void TrackerToolDisplayManager::SetupObjects()
{
    Q_ASSERT_X( this->sceneManager && this->tracker, "TrackerToolDisplayManager::SetupObjects()", "sceneManager and tracker must be set before calling this function." );

    if (this->tracker->IsInitialized())
    {
        int numberOfTools = this->tracker->GetNumberOfTools();
        for( int i = 0; i < numberOfTools; i++ )
        {
            this->SetupObject( i );
        }
    }
}

void TrackerToolDisplayManager::SetupObject( int index )
{
    Q_ASSERT_X( this->sceneManager && this->tracker, "TrackerToolDisplayManager::SetupObject()", "sceneManager and tracker must be set before calling this function." );
    if( this->tracker->IsToolActive( index ) )
    {
        // if object is already there, SceneManager will not add it to the scene
        // reference is not a scene object although axis represent currently used one

        int id = this->tracker->GetToolObjectId( index );
        SceneObject * obj = this->sceneManager->GetObjectByID( id );
        bool objExists = obj != 0;
        if( this->tracker->GetToolUse( index ) == Pointer )
        {
            PointerObject * pObj = 0;
            if( obj )
                pObj = PointerObject::SafeDownCast( obj );
            if( !pObj )
                pObj = PointerObject::New();
            pObj->SetCalibrationMatrix( this->tracker->GetToolCalibrationMatrix( index ) );
            pObj->SetTrackerToolIndex( index );
            obj = pObj;
        }
        else if ( this->tracker->GetToolUse( index ) == UsProbe )
        {
            UsProbeObject * usObj = 0;
            if( obj )
                usObj = UsProbeObject::SafeDownCast( obj );
            if( !usObj )
                usObj = UsProbeObject::New();
            usObj->SetHidden( true );
            if( this->videoSource )
                usObj->InitialSetMask( this->videoSource->GetDefaultMask() );
            obj = usObj;
        }
        else if( this->tracker->GetToolUse( index ) == Camera )
        {
            CameraObject * camObj = 0;
            if( obj )
                camObj = CameraObject::SafeDownCast( obj );
            if( !camObj )
                camObj = CameraObject::New();
            camObj->SetCameraTrackable( true );
            obj = camObj;
        }
        else if( this->tracker->GetToolUse( index ) == Generic )
        {
            GenericTrackedObject * genericObj = 0;
            if( obj )
                genericObj = GenericTrackedObject::SafeDownCast( obj );
            if( !genericObj )
                genericObj = GenericTrackedObject::New();
            genericObj->SetTrackerToolIndex( index );
            obj = genericObj;
        }

        if( obj )
        {
            if( !CameraObject::SafeDownCast(obj) )
                obj->SetLocalTransform( this->tracker->GetTool( index )->GetTransform() );
            obj->SetName( this->tracker->GetToolName( index ) );
            obj->SetObjectManagedBySystem( true );
            obj->SetCanAppendChildren( false );
            obj->SetObjectManagedByTracker(true);
            SceneObject * root = this->sceneManager->GetSceneRoot();
            if( !objExists )
            {
                this->sceneManager->AddObject( obj, root );
                this->tracker->SetToolObjectId(index, obj->GetObjectID());
                this->DisplayedObjects.push_back( obj );
            }
        }
    }
}

void TrackerToolDisplayManager::ClearObject( int toolIndex )
{
    DisplayedObjectsVec::iterator it = this->GetSceneObjectByName( this->tracker->GetToolName( toolIndex ) );
    if( it != this->DisplayedObjects.end() )
    {
        SceneObject * obj = *it;
        this->sceneManager->RemoveObject( obj );
        obj->Delete();
        this->DisplayedObjects.erase( it );
    }
}

void TrackerToolDisplayManager::ClearAllObjects()
{
    Q_ASSERT( this->sceneManager || this->DisplayedObjects.size() == 0 );

    DisplayedObjectsVec::iterator it = this->DisplayedObjects.begin();
    for( ; it != this->DisplayedObjects.end(); ++it )
    {
        this->sceneManager->RemoveObject( (*it) );
        (*it)->Delete();
    }

    this->DisplayedObjects.clear();
}

void TrackerToolDisplayManager::AddAllObjectsToScene()
{
    if( !this->sceneManager )
        return;

    DisplayedObjectsVec::iterator it = this->DisplayedObjects.begin();
    for( ; it != this->DisplayedObjects.end(); ++it )
    {
        this->sceneManager->AddObject( (*it) );
    }
}

void TrackerToolDisplayManager::RemoveAllObjectsFromScene()
{
    if( !this->sceneManager )
        return;

    DisplayedObjectsVec::iterator it = this->DisplayedObjects.begin();
    for( ; it != this->DisplayedObjects.end(); ++it )
    {
        this->sceneManager->RemoveObject( (*it) );
    }
}

TrackerToolDisplayManager::DisplayedObjectsVec::iterator TrackerToolDisplayManager::GetSceneObjectByName( QString name )
{
    DisplayedObjectsVec::iterator it = this->DisplayedObjects.begin();
    for( ; it != this->DisplayedObjects.end(); ++it )
    {
        if( (*it)->GetName() == name )
        {
            return it;
        }
    }
    return this->DisplayedObjects.end();
}
