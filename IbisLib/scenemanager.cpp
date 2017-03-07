/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <iostream>
#include <unistd.h>
#include "scenemanager.h"
#include "mainwindow.h"
#include "quadviewwindow.h"
#include "objecttreewidget.h"

#include "vtkInteractorStyleImage.h"
#include "vtkInteractorStyleTerrain.h"
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkTransform.h"
#include "vtkLinearTransform.h"
#include "vtkMultiImagePlaneWidget.h"
#include "vtkImageData.h"
#include "vtkAxes.h"

#include "view.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "imageobject.h"
#include "polydataobject.h"
#include "triplecutplaneobject.h"
#include "trackedsceneobject.h"
#include "worldobject.h"
#include "pointsobject.h"
#include "cameraobject.h"
#include "usacquisitionobject.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "filereader.h"
#include "application.h"
#include "hardwaremodule.h"
#include "objectplugininterface.h"
#include "toolplugininterface.h"
#include "trackerstatusdialog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPluginLoader>
#include <QProcess>
#include <QProgressDialog>
#include <QApplication>

ObjectSerializationMacro( SceneManager );

const int SceneManager::InvalidId = -1;

SceneManager::SceneManager()
{
    m_viewFollowsReferenceObject = true;
    this->ViewBackgroundColor[0] = 0;
    this->ViewBackgroundColor[1] = 0;
    this->ViewBackgroundColor[2] = 0;
    this->CameraViewAngle3D = 30.0;
    this->SupportedSceneSaveVersion = IBIS_SCENE_SAVE_VERSION;
    this->NavigationPointerID = SceneManager::InvalidId;
    this->IsNavigating = false;
    this->LoadingScene = false;

    m_referenceTransform = vtkTransform::New();
    m_invReferenceTransform = vtkTransform::New();

    this->Init();
}

SceneManager::~SceneManager()
{
    m_referenceTransform->Delete();
    m_invReferenceTransform->Delete();
    this->MainCutPlanes->Delete();
    this->SceneRoot->Delete();
}

// for debugging only
int SceneManager::GetRefCount()
{
    int n = this->GetReferenceCount();
    return n;
}

void SceneManager::Destroy()
{
    this->ReleaseAllViews();

    this->RemoveObject( this->SceneRoot );

    foreach( View* v, Views.keys() )
        v->Delete();

    Views.clear();

    this->Delete();
}

void SceneManager::OnStartMainLoop()
{
    connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(ClockTick()) );
}

void SceneManager::Init()
{
    this->NextObjectID = 0;
    this->NextSystemObjectID = -2;
    this->CurrentObject = 0;
    this->SceneRoot = 0;
    this->ReferenceDataObject = 0;

    // Root
    this->SceneRoot = WorldObject::New();
    this->SceneRoot->SetObjectManagedBySystem(true);
    this->SceneRoot->SetNameChangeable( false );
    this->SceneRoot->SetCanChangeParent( false );
    this->SceneRoot->SetObjectDeletable( false );
    this->SceneRoot->SetCanEditTransformManually( false );
    this->SceneRoot->Manager = this;
    this->SceneRoot->ObjectID = this->NextSystemObjectID--;

    CurrentObject = this->SceneRoot;
    AllObjects.push_back( this->SceneRoot );
    this->SceneRoot->Register(this);

    // Cut planes
    this->MainCutPlanes = TripleCutPlaneObject::New();
    this->MainCutPlanes->SetCanChangeParent( false );
    this->MainCutPlanes->SetName( "Image Planes" );
    this->MainCutPlanes->SetNameChangeable( false );
    this->MainCutPlanes->SetCanAppendChildren( false );
    this->MainCutPlanes->SetCanEditTransformManually( false );
    this->MainCutPlanes->SetObjectManagedBySystem( true );
    this->MainCutPlanes->SetHidable( false );
    this->MainCutPlanes->SetObjectDeletable(false);
    AddObject( this->MainCutPlanes, this->SceneRoot );
    connect( this->MainCutPlanes, SIGNAL(StartPlaneMoved(int)), this, SLOT(OnStartCutPlaneInteraction()) );
    connect( this->MainCutPlanes, SIGNAL(EndPlaneMove(int)), this, SLOT(OnEndCutPlaneInteraction()) );
    connect( this->MainCutPlanes, SIGNAL(PlaneMoved(int)), this, SLOT(OnCutPlanesPositionChanged()) );
    connect( this, SIGNAL(ReferenceObjectChanged()), this->MainCutPlanes, SLOT(AdjustAllImages()) );

    // Add all global objects from plugins
    QList<SceneObject*> globalObjects;
    Application::GetInstance().GetAllGlobalObjectInstances( globalObjects );
    for( int i = 0; i < globalObjects.size(); ++i )
    {
        AddObject( globalObjects[i], this->SceneRoot );
    }

    // Axes
    vtkAxes * axesSource = vtkAxes::New();
    axesSource->SetScaleFactor( 150 );
    axesSource->SymmetricOn();
    axesSource->ComputeNormalsOff();
    axesSource->Update();
    PolyDataObject * axesObject = PolyDataObject::New();
    axesObject->SetPolyData( axesSource->GetOutput() );
    axesObject->SetName("Axes");
    axesObject->SetScalarsVisible( true );
    axesObject->SetLutIndex( -1 );  // spectral
    axesObject->SetNameChangeable( false );
    axesObject->SetObjectDeletable( false );
    axesObject->SetCanChangeParent( false );
    axesObject->SetCanAppendChildren( false );
    axesObject->SetListable( false );
    axesObject->SetObjectManagedBySystem(true);
    axesObject->SetHidden( false );
    this->AddObject( axesObject );
    this->SetAxesObject( axesObject );
    axesSource->Delete();
    axesObject->Delete();

    this->SetCurrentObject( this->GetSceneRoot() );

    this->InteractorStyle3D = InteractorStyleTrackball;
    m_sceneLoadSaveProgressDialog = 0;
}

void SceneManager::Clear()
{
    disconnect( this->GetMainImagePlanes(), SIGNAL(PlaneMoved(int)), this, SLOT(OnCutPlanesPositionChanged()) );
    disconnect( this, SIGNAL(ReferenceObjectChanged()), this->MainCutPlanes, SLOT(AdjustAllImages()) );
    this->RemoveAllSceneObjects();
    Q_ASSERT_X( AllObjects.size() == 0, "SceneManager::~SceneManager()", "Objects are left in the global list.");
    this->SceneRoot->Delete();
    this->SceneRoot = 0;
}

void SceneManager::ClearScene()
{
    SetRenderingEnabled( false );

    NotifyPluginsSceneAboutToLoad();

    // Start progress
    QProgressDialog * progressDialog = Application::GetInstance().StartProgress(4, tr("Removing All Scene Objects..."));

    // Do the clear
    InternalClearScene();

    // Make sure everything is updated
    Application::GetInstance().UpdateProgress(progressDialog, 4);

    NotifyPluginsSceneFinishedLoading();

    // Stop progress
    Application::GetInstance().StopProgress(progressDialog);

    SetRenderingEnabled( true );
}

void SceneManager::InternalClearScene()
{
    Application::GetInstance().RemoveToolObjectsFromScene();
    QString saveSceneDir(this->SceneDirectory);
    this->Clear();
    this->Init();
    Application::GetInstance().AddToolObjectsToScene();
    this->SetSceneDirectory(saveSceneDir);
}

void SceneManager::RemoveAllSceneObjects()
{
    this->RemoveObject( this->SceneRoot );
    this->MainCutPlanes->Delete();
    this->MainCutPlanes = 0;
}

void SceneManager::RemoveAllChildrenObjects(SceneObject *obj)
{
    int n;
    while( (n = obj->GetNumberOfChildren()) > 0 )
    {
        SceneObject * o1 = obj->GetChild( n - 1 );
        this->RemoveAllChildrenObjects( o1 );
        this->RemoveObject( o1 );
    }
}

void SceneManager::LoadScene(QString & fileName, bool interactive )
{
    this->SetRenderingEnabled( false );

    this->LoadingScene = true;
    // Set scene directory
    QFileInfo info( fileName );
    QString newDir = info.dir().absolutePath();
    this->SetSceneDirectory( newDir );

    NotifyPluginsSceneAboutToLoad();

    // Read all objects in the scene
    SerializerReader reader;
    reader.SetFilename( fileName.toUtf8().data() );
    reader.SetSupportedVersion( this->SupportedSceneSaveVersion );
    reader.Start();
    reader.BeginSection("SaveScene");
    reader.ReadVersionFromFile();
    if( reader.FileVersionNewerThanSupported() )
    {
        reader.EndSection();
        reader.Finish();
        QString message = "SaveScene version from file (" + reader.GetVersionFromFile() + ") is more recent than supported (" + this->SupportedSceneSaveVersion + ")\n";
        QMessageBox::warning( 0, "Error", message, 1, 0 );
        SetRenderingEnabled( true );
        this->LoadingScene = false;
        return;
    }
    else if( reader.FileVersionIsLowerThan( QString::number(6.0) ) )
    {
        QString message = "This scene version is older than 6.0. This is not supported anymore. Scene may not be restored.\n";
        QMessageBox::warning( 0, "Error", message, 1, 0 );
        SetRenderingEnabled( true );
        this->LoadingScene = false;
        return;
    }
    int numberOfSceneObjects;
    ::Serialize( &reader, "NumberOfSceneObjects", numberOfSceneObjects );
    ::Serialize( &reader, "NextObjectID", this->NextObjectID);

    // Start Progress dialog
    if( interactive )
    {
        QApplication::processEvents();
        m_sceneLoadSaveProgressDialog = Application::GetInstance().StartProgress( numberOfSceneObjects*2+3, tr("Loading Scene...") );
    }

    this->ObjectReader(&reader, interactive);
    if( interactive )
        this->UpdateProgress(numberOfSceneObjects+1);
    ::Serialize( &reader, "SceneManager", this );
    SceneObject *currentObject = this->GetCurrentObject();
    if( interactive )
        this->UpdateProgress(numberOfSceneObjects+1);

    // Read other params of the scene
    bool axesHidden;
    bool cursorVisible;
    ::Serialize( &reader, "AxesHidden", axesHidden);
    ::Serialize( &reader, "CursorVisible", cursorVisible);
    this->SceneRoot->SetAxesHidden(axesHidden);
    this->SceneRoot->SetCursorVisible(cursorVisible);

    Application::GetInstance().GetMainWindow()->Serialize( &reader );
    reader.EndSection();
    reader.Finish();
    this->SetSceneFile( fileName );
    if( interactive )
        this->UpdateProgress(numberOfSceneObjects+3);

    // Give a chance to all objects to react after scene is read
    this->PostSceneRead(numberOfSceneObjects+3);

    // Stop progress
    if( interactive )
        Application::GetInstance().StopProgress(m_sceneLoadSaveProgressDialog);
    m_sceneLoadSaveProgressDialog = 0;

    // Tell plugins new scene finished loading
    NotifyPluginsSceneFinishedLoading();

    SetRenderingEnabled( true );
    this->SetCurrentObject( currentObject);
    this->LoadingScene = false;
}

void SceneManager::NewScene()
{
    SetRenderingEnabled( false );

    // Clear the scene
    InternalClearScene();

    SetRenderingEnabled( true );
}

void SceneManager::CancelProgress()
{
    Application::GetInstance().StopProgress(m_sceneLoadSaveProgressDialog);
    m_sceneLoadSaveProgressDialog = 0;
    this->ClearScene();
}

bool SceneManager::UpdateProgress(int value)
{
    if (!m_sceneLoadSaveProgressDialog)
        return false;
    if ( m_sceneLoadSaveProgressDialog->wasCanceled())
    {
        this->CancelProgress();
        return false;
    }
    Application::GetInstance().UpdateProgress(m_sceneLoadSaveProgressDialog, value);
    return true;
}

void SceneManager::SaveScene( QString & fileName )
{
    SceneObject *currentObject = this->GetCurrentObject();
    NotifyPluginsSceneAboutToSave();

    QList< SceneObject* > listedObjects;
    this->GetAllListableNonTrackedObjects(listedObjects);
    int numberOfSceneObjects = listedObjects.count();
    m_sceneLoadSaveProgressDialog = Application::GetInstance().StartProgress(numberOfSceneObjects+3, tr("Saving Scene..."));
    m_sceneLoadSaveProgressDialog->setCancelButton(0);
    SerializerWriter writer;
    writer.SetFilename( fileName.toUtf8().data() );
    writer.Start();
    writer.BeginSection("SaveScene");
    QString version(IBIS_SCENE_SAVE_VERSION);
    ::Serialize( &writer, "Version", version);
    ::Serialize( &writer, "NextObjectID", this->NextObjectID);
    this->UpdateProgress(1);
    this->ObjectWriter(&writer);
    ::Serialize( &writer, "SceneManager", this);
    this->UpdateProgress(numberOfSceneObjects+2);
    bool axesHidden = this->SceneRoot->AxesHidden();
    bool cursorVisible = this->SceneRoot->GetCursorVisible();
    ::Serialize( &writer, "AxesHidden", axesHidden);
    ::Serialize( &writer, "CursorVisible", cursorVisible);

    Application::GetInstance().GetMainWindow()->Serialize( &writer );

    writer.EndSection();
    writer.Finish();
    Application::GetInstance().StopProgress(m_sceneLoadSaveProgressDialog);
    m_sceneLoadSaveProgressDialog = 0;

    NotifyPluginsSceneFinishedSaving();
    this->SetCurrentObject( currentObject);
}

void SceneManager::Serialize( Serializer * ser )
{
    int id = SceneManager::InvalidId;
    int refObjID = SceneManager::InvalidId;
    if (!ser->IsReader() && this->CurrentObject)
    {
        id = this->CurrentObject->GetObjectID();
        if( this->ReferenceDataObject )
            refObjID = this->ReferenceDataObject->GetObjectID();
    }
    ::Serialize( ser, "CurrentObjectID", id );
    ::Serialize( ser, "ReferenceObjectID", refObjID );
    ::Serialize( ser, "ViewBackgroundColor", this->ViewBackgroundColor, 3 );

    ser->BeginSection("Views");
    foreach( View* view, Views.keys() )
    {
        QString viewName(view->GetName());
        ::Serialize( ser, viewName.toUtf8().data(), view );
    }
    if (ser->IsReader())
    {
        this->SetCurrentObject(this->GetObjectByID(id));
        this->SetViewBackgroundColor( this->ViewBackgroundColor );
        if( refObjID != SceneManager::InvalidId )
            this->SetReferenceDataObject( this->GetObjectByID(refObjID) );
    }
    ser->EndSection();
}

void SceneManager::PostSceneRead( int n )
{
    this->GetSceneRoot()->PostSceneRead();
    // Have to set up NextObjectId
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        if (this->AllObjects[i]->GetObjectID() > this->NextObjectID)
             this->NextObjectID = this->AllObjects[i]->GetObjectID();
        if (!this->UpdateProgress(n+i+1))
            return;
    }
    this->NextObjectID++;
}

QWidget * SceneManager::CreateQuadViewWindow( QWidget * parent )
{
    QuadViewWindow * res = 0;
    res = new QuadViewWindow( parent );
    res->setAttribute(Qt::WA_DeleteOnClose);
    res->SetSceneManager( this );
    return res;
}

QWidget * SceneManager::CreateObjectTreeWidget( QWidget * parent )
{
    ObjectTreeWidget * res = new ObjectTreeWidget( parent );
    res->setObjectName( QString("Object list") );
    res->setAttribute(Qt::WA_DeleteOnClose);
    res->SetSceneManager( this );
    return res;
}

QWidget * SceneManager::CreateTrackedToolsStatusWidget( QWidget * parent )
{
    TrackerStatusDialog * res = new TrackerStatusDialog( parent );
    res->setAttribute( Qt::WA_DeleteOnClose, true );
    res->SetSceneManager( this );
    return res;
}

View * SceneManager::GetViewByID( int id )
{
    return Views.key( id, NULL );
}

View * SceneManager::CreateView( int type, int id, QString name )
{
    View * res = this->GetViewByID( id );

    if( res )
        return res;

    res = View::New();
    res->SetName( name );
    res->SetManager( this );
    res->SetType( type );
    res->SetViewID( id );
    if( type == THREED_VIEW_TYPE )
    {
        AssignInteractorStyleToView( this->InteractorStyle3D, res );
        res->SetViewAngle( this->CameraViewAngle3D );
    }

    // Reset cameras for the colin27 in talairach space. This should be more or less good
    // for any kind of brain data.
    double bounds[6] = { -90.0, 91.0, -126.0, 91.0, -72.0, 109.0 };
    res->ResetCamera( bounds );

    this->Views.insert( res, id );

    return res;
}

View * SceneManager::GetMain3DView()
{
    return Views.key( this->Main3DViewID, NULL );
}

View * SceneManager::GetMainCoronalView()
{
    return Views.key( this->MainCoronalViewID, NULL );
}

View * SceneManager::GetMainSagittalView()
{
    return Views.key( this->MainSagittalViewID, NULL );
}

View * SceneManager::GetMainTransverseView()
{
    return Views.key( this->MainTransverseViewID, NULL );
}

View * SceneManager::GetViewFromInteractor( vtkRenderWindowInteractor * interactor )
{
    foreach( View* view, Views.keys() )
    {
        if( view->GetInteractor() == interactor )
        {
            return view;
        }
    }
    return NULL;
}

void SceneManager::SetViewBackgroundColor( double * color )
{
    this->ViewBackgroundColor[0] = color[0];
    this->ViewBackgroundColor[1] = color[1];
    this->ViewBackgroundColor[2] = color[2];
    foreach( View* view, Views.keys() )
    {
        view->SetBackgroundColor( color );
    }
}

void SceneManager::UpdateBackgroundColor( )
{
    foreach( View* view, Views.keys() )
    {
        view->SetBackgroundColor( this->ViewBackgroundColor );
    }
}

void SceneManager::AddObject( SceneObject * object, SceneObject * attachTo )
{
    this->AddObjectUsingID(object, attachTo, SceneManager::InvalidId);
}

void SceneManager::AddObjectUsingID( SceneObject * object, SceneObject * attachTo, int objID )
{
    Q_ASSERT( object );

    // Register and add to the global list
    object->Register( this );
    this->AllObjects.push_back( object );

    if( !attachTo )
        attachTo = GetSceneRoot();

    // Notify clients we start adding an object
    int position = attachTo->GetNumberOfListableChildren();
    if( object->IsListable() )
        emit StartAddingObject( attachTo, position );

    // Setting object id and manager
    int id = objID;  // if we get a valid id, use it
    if( id == SceneManager::InvalidId )
    {
        // if the object has already been assigned an id, keep it
        if( object->GetObjectID() != SceneManager::InvalidId )
            id = object->GetObjectID();
        else if( object->IsManagedBySystem() )
            id = this->NextSystemObjectID--;
        else
            id = this->NextObjectID++;
    }
    object->AddToScene( this, id );

    // Attach object to the hierarchy
    attachTo->AddChild( object );

    if( !GetReferenceDataObject() && CanBeReference( object ) )
        SetReferenceDataObject( object );

    // Setup in views
    this->SetupInAllViews( object );
    object->PreDisplaySetup();

    // adding first image object has to call ResetAllCameras,
    // at this point the object is already added to the list
    if( this->GetNumberOfUserObjects() == 1 && object->IsUserObject() )
        this->ResetAllCameras();

    // Notify clients the object has been added
    if( object->IsListable() )
    {
        emit FinishAddingObject();
    }
    ValidatePointerObject();
    emit ObjectAdded( id );
}

bool SceneManager::GetCursorVisible()
{
    Q_ASSERT( MainCutPlanes );
    return MainCutPlanes->GetCursorVisible();
}

void SceneManager::SetCursorVisibility( bool v )
{
    Q_ASSERT( MainCutPlanes );
    MainCutPlanes->SetCursorVisibility( v );
}

void SceneManager::SetCursorColor( const QColor & c )
{
    Q_ASSERT( MainCutPlanes );
    MainCutPlanes->SetCursorColor( c );
}

QColor SceneManager::GetCursorColor()
{
    Q_ASSERT( MainCutPlanes );
    return MainCutPlanes->GetCursorColor();
}

void SceneManager::ViewPlane( int index, bool show )
{
    Q_ASSERT( MainCutPlanes );
    MainCutPlanes->SetViewPlane( index, show );
}

bool SceneManager::IsPlaneVisible( int index )
{
    Q_ASSERT( MainCutPlanes );
    return MainCutPlanes->GetViewPlane( index );
}

void SceneManager::ViewAllPlanes( bool show )
{
    Q_ASSERT( MainCutPlanes );
    MainCutPlanes->SetViewAllPlanes( show );
}

void SceneManager::SetResliceInterpolationType( int type )
{
    Q_ASSERT( MainCutPlanes );
    MainCutPlanes->SetResliceInterpolationType( type );
}

int SceneManager::GetResliceInterpolationType()
{
    Q_ASSERT( MainCutPlanes );
    return MainCutPlanes->GetResliceInterpolationType();
}

void SceneManager::SetDisplayInterpolationType( int type )
{
    Q_ASSERT( MainCutPlanes );
    MainCutPlanes->SetDisplayInterpolationType( type );
}

int SceneManager::GetDisplayInterpolationType()
{
    Q_ASSERT( MainCutPlanes );
    return MainCutPlanes->GetDisplayInterpolationType();
}


void SceneManager::RemoveObjectById( int objectId )
{
    SceneObject * obj = GetObjectByID( objectId );
    if( obj )
        RemoveObject( obj );
}

void SceneManager::RemoveObject( SceneObject * object , bool viewChange)
{
    int objId = object->GetObjectID();

    // first, make sure we have a reference to this object
    int indexAll = this->AllObjects.indexOf( object );
    if( indexAll == -1 )
        return;

    object->ObjectAboutToBeRemovedFromScene();

    // remove all children
    this->RemoveAllChildrenObjects(object);

    // simtodo : assert position != -1
    int position = object->GetObjectListableIndex();

    SceneObject * parent = object->GetParent();

    if( this->CurrentObject == object )
    {
        if( parent )
            this->SetCurrentObject(parent);
        else
            this->SetCurrentObject( this->SceneRoot );
    }

    if( object->IsListable() )
        emit StartRemovingObject( object->GetParent(), position );

    // Set ReferenceDataObject if needed
    if (object == this->ReferenceDataObject)
    {
        this->ReferenceDataObject  = 0;
        m_referenceTransform->Identity();
        m_invReferenceTransform->Identity();
        QList< ImageObject* > imObjects;
        this->GetAllImageObjects( imObjects );
        int i = 0;
        bool refSet = false;
        while (i < imObjects.size() && !refSet )
        {
            if( imObjects[i] != object )
            {
                this->SetReferenceDataObject( imObjects[i] );
                refSet = true;
            }
            i++;
        }
    }

    // Release from all views
    foreach( View* view, Views.keys() )
    {
        object->Release( view );
    }

    // Detach from parent
	if( parent )
    {
        parent->RemoveChild( object );
    }

    // remove the object from the global list
    this->AllObjects.removeAt( indexAll );

    // Tell other this object is being removed
    object->RemoveFromScene();
    //at this moment object still exist, we send ObjectRemoved() signal
    //in order to let other objects to deal with the a still valid object,
    //unregister shoul trigger destruction of the object
    emit ObjectRemoved( objId );

    object->UnRegister( this );

    if( object->IsListable() )
        emit FinishRemovingObject();

    ValidatePointerObject();
}

void SceneManager::ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex )
{
    SceneObject * curParent = object->GetParent();
    int position = object->GetObjectListableIndex();

    emit StartRemovingObject( curParent, position );
    curParent->RemoveChild( object );
    emit FinishRemovingObject();

    emit StartAddingObject( newParent, newChildIndex );
    newParent->InsertChild( object, newChildIndex );
    emit FinishAddingObject();
}

void SceneManager::GetAllImageObjects( QList<ImageObject*> & objects )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        ImageObject * obj = ImageObject::SafeDownCast( this->AllObjects[i] );
        if( obj )
            objects.push_back( obj );
    }
}

void SceneManager::GetAllPolydataObjects( QList<PolyDataObject*> & objects )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        PolyDataObject * obj = PolyDataObject::SafeDownCast( this->AllObjects[i] );
        if( obj )
            objects.push_back( obj );
    }
}

void SceneManager::GetAllPointsObjects( QList<PointsObject*> & objects )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        PointsObject * obj = PointsObject::SafeDownCast( this->AllObjects[i] );
        if( obj )
            objects.push_back( obj );
    }
}

void SceneManager::GetAllCameraObjects( QList<CameraObject*> & all )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        CameraObject * obj = CameraObject::SafeDownCast( this->AllObjects[i] );
        if( obj )
            all.push_back( obj );
    }
}

void SceneManager::GetAllUSAcquisitionObjects( QList<USAcquisitionObject*> & all )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        USAcquisitionObject * obj = USAcquisitionObject::SafeDownCast( this->AllObjects[i] );
        if( obj )
            all.push_back( obj );
    }
}

void SceneManager::GetAllUsProbeObjects( QList<UsProbeObject*> & all )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        UsProbeObject * obj = UsProbeObject::SafeDownCast( this->AllObjects[i] );
        if( obj )
            all.push_back( obj );
    }
}

void SceneManager::GetAllPointerObjects( QList<PointerObject*> & all )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        PointerObject * obj = PointerObject::SafeDownCast( this->AllObjects[i] );
        if( obj )
            all.push_back( obj );
    }
}

void SceneManager::GetAllTrackedObjects( QList<TrackedSceneObject*> & all )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        TrackedSceneObject * obj = TrackedSceneObject::SafeDownCast( this->AllObjects[i] );
        if( obj && obj->IsDrivenByHardware() )
            all.push_back( obj );
    }
}

void SceneManager::GetAllObjectsOfType( const char * typeName, QList<SceneObject*> & all )
{
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        if( this->AllObjects[i]->IsA( typeName ) )
            all.push_back( this->AllObjects[i] );
    }
}

SceneObject * SceneManager::GetObjectByID( int id )
{
    if( id == SceneManager::InvalidId )
        return 0;
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        if( this->AllObjects[i]->GetObjectID() == id )
            return this->AllObjects[i];
    }
    return 0;
}

void SceneManager::SetCurrentObject( SceneObject * obj )
{
    // SetCurrentObject(0) is called to force refreshing of the object list in the left pane
    if (obj && obj->IsListable() )
    {
        this->CurrentObject = obj;
    }
    emit CurrentObjectChanged();
}

void SceneManager::SetReferenceDataObject( SceneObject *  refObject )
{
    if( refObject != this->ReferenceDataObject )
    {
        ImageObject * referenceObject = 0;
        if( refObject )
        {
            referenceObject = ImageObject::SafeDownCast( refObject );
            Q_ASSERT_X( referenceObject, "SceneManager::SetReferenceDataObject()", "Trying to set object of wrong type as reference" );
        }

        if( this->ReferenceDataObject )
        {
            disconnect( this->ReferenceDataObject, SIGNAL(WorldTransformChangedSignal()), this, SLOT(ReferenceTransformChangedSlot()));
        }
        this->ReferenceDataObject = referenceObject;
        m_referenceTransform->Identity();
        m_invReferenceTransform->Identity();
        if( this->ReferenceDataObject )
        {
            m_referenceTransform->SetInput( this->ReferenceDataObject->GetWorldTransform() );
            m_invReferenceTransform->SetInput( this->ReferenceDataObject->GetWorldTransform() );
            m_invReferenceTransform->Inverse();
            connect( this->ReferenceDataObject, SIGNAL(WorldTransformChangedSignal()), this, SLOT(ReferenceTransformChangedSlot()));
            emit ReferenceObjectChanged();
        }
        emit ReferenceTransformChanged();
    }
}
        
ImageObject * SceneManager::GetReferenceDataObject( )
{
    return this->ReferenceDataObject;
}

bool SceneManager::CanBeReference( SceneObject * obj )
{
    return obj->IsA("ImageObject");
}

void SceneManager::GetReferenceBounds( double bounds[6] )
{
    if( this->ReferenceDataObject )
        this->ReferenceDataObject->GetImage()->GetBounds(bounds);
    else
    {
        bounds[0] = 0.0; bounds[1] = 1.0;
        bounds[2] = 0.0; bounds[3] = 1.0;
        bounds[4] = 0.0; bounds[5] = 1.0;
    }
}

SceneObject * SceneManager::GetSceneRoot()
{
    return this->SceneRoot;
}

void SceneManager::SetAxesObject( PolyDataObject * obj )
{
    this->SceneRoot->SetAxesObject( obj );
}

PolyDataObject * SceneManager::GetAxesObject()
{
    return this->SceneRoot->GetAxesObject();
}

void SceneManager::PreDisplaySetup()
{
    GetSceneRoot()->PreDisplaySetup();
    this->ResetAllCameras();
}

void SceneManager::ResetAllCameras()
{
    foreach( View* view, Views.keys() )
    {
        view->ResetCamera();
    }
}

void SceneManager::ResetAllCameras( double bounds[6] )
{
    foreach( View* view, Views.keys() )
    {
        view->ResetCamera( bounds );
    }
}

void SceneManager::ZoomAllCameras(double factor)
{
    foreach( View* view, Views.keys() )
    {
        view->ZoomCamera(factor);
    }
}

void SceneManager::SetupAllObjects( View * v )
{
    SetupOneObject( v, GetSceneRoot() );
}

void SceneManager::ReleaseAllObjects( View * v )
{
    GetSceneRoot()->Release( v );
}

void SceneManager::SetupOneObject( View * v, SceneObject * obj )
{
    obj->Setup( v );
    for( int i = 0; i < obj->GetNumberOfChildren(); ++i )
    {
        SetupOneObject( v, obj->GetChild( i ) );
    }
}

void SceneManager::ReleaseAllViews()
{
    foreach( View* view, Views.keys() )
    {
        view->ReleaseView();
    }
}

void SceneManager::SetupInAllViews( SceneObject * object )
{
    foreach( View* view, Views.keys() )
    {
        object->Setup( view );
    }
}

void SceneManager::Set3DInteractorStyle( InteractorStyle style )
{
    this->InteractorStyle3D = style;
    foreach( View* view, Views.keys() )
    {
        if( view->GetType() == THREED_VIEW_TYPE )
            AssignInteractorStyleToView( style, view );
    }
}

bool SeparateBasenameAndDigit( QString orig, QString & baseName, int & digit )
{
    // Determine if there is a digit
    int nameSize = orig.size();
    int spaceIndex = orig.lastIndexOf( ' ' );
    QString digitString;
    digit = 0;
    bool hasDigit = false;
    if( spaceIndex > 0 && spaceIndex < nameSize - 1 )
    {
        digitString = orig.right( nameSize - spaceIndex - 1 );
        digit = digitString.toInt( &hasDigit );
    }

    if( hasDigit )
        baseName = orig.left( spaceIndex );
    else
        baseName = orig;

    return hasDigit;
}

QString SceneManager::FindUniqueName( QString wantedName, QStringList & otherNames )
{
    QString uniqueName = wantedName;
    while( otherNames.contains( uniqueName ) )
    {
        int digit = 0;
        QString baseName;
        SeparateBasenameAndDigit( uniqueName, baseName, digit );
        uniqueName = baseName + " " + QString::number( digit + 1 );
    }
    return uniqueName;
}

void SceneManager::AssignInteractorStyleToView( InteractorStyle style, View * v )
{
    vtkInteractorStyle * styleObject = 0;
    if( style == InteractorStyleTerrain )
        styleObject = vtkInteractorStyleTerrain::New();
    else if( style == InteractorStyleTrackball )
        styleObject = vtkInteractorStyleTrackballCamera::New();
    else if( style == InteractorStyleJoystick )
        styleObject = vtkInteractorStyleJoystickCamera::New();

    if( styleObject != 0 )
        v->SetInteractorStyle( styleObject );
    styleObject->Delete();
}

void SceneManager::SetStandardView(STANDARDVIEW type)
{
    View * v3d = GetMain3DView();
    if (v3d)
    {
        vtkCamera *camera = v3d->GetRenderer()->GetActiveCamera();
        double x, y, z;
        double s = camera->GetParallelScale();
        switch (type)
        {
        case SV_FRONT:
            x = 0.0;
            y = 1.0;
            z = 0.0;
            break;
        case SV_BACK:
            x = 0.0;
            y = -1.0;
            z = 0.0;
            break;
        case SV_LEFT:
            x = -1.0;
            y = 0.0;
            z = 0.0;
            break;
        case SV_RIGHT:
            x = 1.0;
            y = 0.0;
            z = 0.0;
            break;
        case SV_TOP:
            x = 0.0;
            y = 0.0;
            z = 1.0;
            break;
        case SV_BOTTOM:
            x = 0.0;
            y = 0.0;
            z = -1.0;
            break;
        default:
            return;
        }
        camera->SetFocalPoint(0.0, 0.0, 0.0);
        camera->SetPosition(x, y, z);
        if (x == 0.0)
        {
            camera->SetViewUp(0.0, 1.0, 0.0);
        }
        else if (y == 0.0)
        {
            camera->SetViewUp(0.0, 0.0, 1.0);
        }
        else
        {
            camera->SetViewUp(1.0, 0.0, 0.0);
        }
        v3d->ResetCamera();
        if (this->ReferenceDataObject) // TODO -check with Simon
        {
            vtkTransform * tr = this->ReferenceDataObject->GetWorldTransform();
            camera->ApplyTransform(tr);
        }
        v3d->ResetCamera();
        camera->SetParallelScale(s);
        v3d->NotifyNeedRender();
    }
}

void SceneManager::ReferenceTransformChangedSlot()
{
     emit ReferenceTransformChanged();
}



vtkRenderer *SceneManager::GetViewRenderer(int viewID )
{
    View *v = Views.key( viewID, NULL );
    if( v )
        return v->GetRenderer();
    return NULL;
}

void SceneManager::SetRenderingEnabled( bool r )
{
    foreach( View* view, Views.keys() )
        view->SetRenderingEnabled( r );
}

double SceneManager::Get3DCameraViewAngle()
{
    return this->CameraViewAngle3D;
}

void SceneManager::Set3DCameraViewAngle( double angle )
{
    this->CameraViewAngle3D = angle;
    if( this->GetMain3DView() )
        this->GetMain3DView()->SetViewAngle( angle );
}

void SceneManager::GetCursorPosition( double pos[3] )
{
    this->GetMainImagePlanes()->GetPlanesPosition( pos );
}

bool SceneManager::IsInPlane( VIEWTYPES planeType, double pos[3] )
{
    return this->GetMainImagePlanes()->IsInPlane( planeType, pos );
}

void SceneManager::ResetCursorPosition()
{
    this->GetMainImagePlanes()->ResetPlanes();
}

void SceneManager::SetCursorWorldPosition( double * pos )
{
    this->GetMainImagePlanes()->SetWorldPlanesPosition( pos );
}

void SceneManager::SetCursorPosition( double * pos )
{
    this->GetMainImagePlanes()->SetPlanesPosition( pos );
}

void SceneManager::OnStartCutPlaneInteraction()
{
    emit StartCursorInteraction();
}

void SceneManager::OnCutPlanesPositionChanged()
{
    emit CursorPositionChanged();
}

void SceneManager::OnEndCutPlaneInteraction()
{
    emit EndCursorInteraction();
}

void SceneManager::WorldToReference( double worldPoint[3], double referencePoint[3] )
{
    // Apply inverse of reference object world transform to worldPoint
    ImageObject * ref = GetReferenceDataObject();
    if( ref )
    {
        vtkLinearTransform * inverseReg = ref->GetWorldTransform()->GetLinearInverse();
        inverseReg->TransformPoint( worldPoint, referencePoint );
    }
    else
    {
        referencePoint[0] = worldPoint[0];
        referencePoint[1] = worldPoint[1];
        referencePoint[2] = worldPoint[2];
    }
}

void SceneManager::ReferenceToWorld( double referencePoint[3], double worldPoint[3] )
{
    // Apply reference object world transform to referencePoint
    // Apply inverse of reference object world transform to worldPoint
    ImageObject * ref = GetReferenceDataObject();
    if( ref )
    {
        vtkLinearTransform * transform = ref->GetWorldTransform();
        transform->TransformPoint( referencePoint, worldPoint );
    }
    else
    {
        worldPoint[0] = referencePoint[0];
        worldPoint[1] = referencePoint[1];
        worldPoint[2] = referencePoint[2];
    }
}

void SceneManager::GetReferenceOrientation( vtkMatrix4x4 * mat )
{
    ImageObject * im = GetReferenceDataObject();
    if( im )
    {
        double orient[4];
        im->GetWorldTransform()->GetOrientationWXYZ( orient );
        vtkTransform * t = vtkTransform::New();
        t->RotateWXYZ( orient[0], orient[1], orient[2], orient[3] );
        mat->DeepCopy( t->GetMatrix() );
        t->Delete();
    }
    else
        mat->Identity();
}

int SceneManager::GetNumberOfUserObjects()
{
    int count = 0;
    for( int i = 0; i < AllObjects.size(); ++i )
    {
        if( AllObjects[i]->IsUserObject() )
        {
            ++count;
        }
    }
    return count;
}

void SceneManager::GetAllUserObjects(QList<SceneObject*> &list)
{
    for( int i = 0; i < AllObjects.size(); ++i )
    {
        if( AllObjects[i]->IsUserObject() )
        {
            list.push_back( AllObjects[i] );
        }
    }
}

int SceneManager::GetNumberOfImageObjects()
{
    QList<ImageObject*> allImageObjects;
    GetAllImageObjects( allImageObjects );
    return allImageObjects.size();
}

void SceneManager::GetAllListableNonTrackedObjects(QList<SceneObject*> &list)
{
    list.push_back( this->SceneRoot );
    GetChildrenListableNonTrackedObjects( GetSceneRoot(), list );
}

void SceneManager::GetChildrenListableNonTrackedObjects( SceneObject * obj, QList<SceneObject*> &list )
{
    for( int i = 0; i < obj->GetNumberOfChildren(); ++i )
    {
        SceneObject * child = obj->GetChild(i);
        if (!child->IsManagedByTracker() && (child->IsListable() || !child->IsManagedBySystem()) )
            list.append(child);
        GetChildrenListableNonTrackedObjects( child, list );
    }
}
void SceneManager::EmitSignalObjectRenamed(QString oldName, QString newName)
{
    emit ObjectNameChanged(oldName, newName);
}

void SceneManager::EmitShowGenericLabel( bool show)
{
    emit ShowGenericLabel( show );
}

void SceneManager::EmitShowGenericLabelText()
{
    emit ShowGenericLabelText();
}

void SceneManager::ObjectReader( Serializer * ser, bool interactive )
{
    int i, numberOfSceneObjects = 0;
    int oldId, oldParentId;
    QString sectionName, className, filePath, objectName;
    ::Serialize( ser, "NumberOfSceneObjects", numberOfSceneObjects );
    ser->BeginSection("ObjectList");
    SceneObject *parentObject;

    for (i = 0; i < numberOfSceneObjects; i++)
    {
        QString sectionName = QString( "ObjectInScene_%1" ).arg(i);
        ser->BeginSection( sectionName.toUtf8().data() );
        ::Serialize( ser, "ObjectClass", className );
        ::Serialize( ser, "FullFileName", filePath );
        ::Serialize( ser, "ObjectID", oldId );
        ::Serialize( ser, "ParentID", oldParentId );
        parentObject = 0;
        if( oldParentId != SceneManager::InvalidId ) // only World does not have parent
        {
            parentObject = this->GetObjectByID(oldParentId);
            if (!parentObject)
            {
                parentObject = this->GetSceneRoot();
            }
        }
        if (filePath.at(0) == '.')
            filePath.replace(0, 1, this->GetSceneDirectory());

        // If there is a path, we open a file
        QStringList fileToOpen;
        fileToOpen.clear();
        if (!filePath.isEmpty() && QString::compare(filePath, "none"))
        {
            fileToOpen.append(filePath);
            FileReader *fileReader = new FileReader;
            bool labelImage = false;
            if( className == "ImageObject")
            {
                ::Serialize( ser, "LabelImage", labelImage );
            }
            OpenFileParams params;
            params.AddInputFile(filePath);
            params.defaultParent = parentObject;
            if( labelImage )
            {
                params.filesParams[0].isLabel = true;
            }
            fileReader->SetParams( &params );
            fileReader->start();
            fileReader->wait();
            QList<SceneObject*> loadedObject;
            fileReader->GetReadObjects( loadedObject );
            const QStringList & warnings = fileReader->GetWarnings();
            if (!loadedObject.empty())
            {
                SceneObject *obj = loadedObject.at(0);
                obj->Serialize(ser);
                this->AddObjectUsingID(obj, parentObject, oldId);
                obj->Delete();
            }
            else
            {
                // Display warnings if any
                if( warnings.size() )
                {
                    QString message;
                    for( int i = 0; i < warnings.size(); ++i )
                    {
                        message += warnings[i];
                        message += QString("\n");
                    }
                    QMessageBox::warning( 0, "Error", message );
                }
            }
            delete fileReader;
        }
        else
        {
            if (QString::compare(className, QString("WorldObject"), Qt::CaseSensitive) == 0)
            {
                this->GetSceneRoot()->Serialize(ser);
            }
            else if (QString::compare(className, QString("TripleCutPlaneObject"), Qt::CaseSensitive) == 0)
            {
                this->GetMainImagePlanes()->Serialize(ser);
            }
            else if (QString::compare(className, QString("PointsObject"), Qt::CaseSensitive) == 0)
            {
                PointsObject *pt = PointsObject::New();
                pt->Serialize(ser);
                this->AddObjectUsingID(pt, parentObject, oldId);
                pt->Delete();
            }
            else if (QString::compare(className, QString("USAcquisitionObject"), Qt::CaseSensitive) == 0)
            {
                USAcquisitionObject *usObj = USAcquisitionObject::New();
                usObj->ObjectID = oldId;  // serialization needs the id
                usObj->Serialize(ser);
                this->AddObjectUsingID(usObj, parentObject, oldId);
                usObj->Delete();
            }
            else if( className == QString("CameraObject") )
            {
                CameraObject * camObject = CameraObject::New();
                camObject->ObjectID = oldId; // serialization needs the id
                camObject->Serialize(ser);
                this->AddObjectUsingID(camObject, parentObject, oldId);
                camObject->Delete();
            }
            else if( className == QString("PolyDataObject") || className == QString("ImageObject"))
            {
                //ignore, we do not have a mechanism to create object
                SceneObject *obj = SceneObject::New();
                obj->Serialize(ser);
                QString msg("Incomplete data, ignoring object:  ");
                msg.append(obj->GetName());
                QMessageBox::warning( 0, "Error",msg , 1, 0 );
                obj->Delete();
            }
            else if (QString::compare(className, QString("SceneObject"), Qt::CaseSensitive) == 0) // Transform object or whatever scene object
            {
                if( oldId >= 0 )
                {
                    SceneObject *obj = SceneObject::New();
                    obj->Serialize(ser);
                    this->AddObjectUsingID(obj, parentObject, oldId);
                    obj->Delete();
                }
            }
            else
            {
                // Read object class from an ObjectPlugin
                bool found = false;
                ObjectPluginInterface * p = Application::GetInstance().GetObjectPluginByName( className );
                if( p )
                {
                    SceneObject *obj = p->CreateObject();
                    if( obj )
                    {
                        obj->Serialize(ser);
                        // ignore the id given by SceneManager
                        obj->SetObjectID(oldId);
                        found = true;
                    }
                }

                // Try global object plugins
                if( !found )
                {
                    QList<SceneObject*> globObj;
                    Application::GetInstance().GetAllGlobalObjectInstances( globObj );
                    for( int go = 0; go < globObj.size(); ++go )
                    {
                        if( globObj[go]->GetClassName() == className )
                            globObj[go]->Serialize( ser );
                    }
                }
            }
        }
        ser->EndSection();
        if ( interactive && !this->UpdateProgress(i+1) )
            return;
    }
    ser->EndSection();
    ser->BeginSection("Plugins");
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    for( int t = 0; t < allTools.size(); ++t )
    {
        ToolPluginInterface * toolModule = allTools[t];
        bool sectionFound = ser->BeginSection( toolModule->GetPluginName().toUtf8().data() );
        toolModule->Serialize(ser);
        if( sectionFound )
            ser->EndSection();
    }
    ser->EndSection();
}

void SceneManager::ObjectWriter( Serializer * ser )
{
    QList< SceneObject* > listedObjects;
    int i, numberOfSceneObjects = 0;
    this->GetAllListableNonTrackedObjects(listedObjects);
    numberOfSceneObjects = listedObjects.count();
    ::Serialize( ser, "NumberOfSceneObjects", numberOfSceneObjects );
    QList< SceneObject* >::iterator it = listedObjects.begin();
    ser->BeginSection("ObjectList");
    QString newPath;
    int id, parentId = SceneManager::InvalidId;
    for(i = 0 ; it != listedObjects.end(); ++it, i++ )
    {
        SceneObject * obj = (*it);
        QString sectionName= QString( "ObjectInScene_%1" ).arg(i);
        QString objectName = QString(obj->GetName());
        QString className = QString(obj->GetClassName());
        ser->BeginSection(sectionName.toUtf8().data());
        QString oldPath = obj->GetFullFileName();
        newPath = QString(this->GetSceneDirectory());
        newPath.append("/");
        QString dataFileName(QString::number(obj->GetObjectID()));
        dataFileName.append(".");
        if (!oldPath.isEmpty() && !obj->GetDataFileName().isEmpty())
        {
            if( !obj->IsA( "PointsObject" )) // from now on, we always save points in scene.xml
            {
                QFileInfo fi(obj->GetDataFileName());
                dataFileName.append(fi.completeSuffix());
                newPath.append(dataFileName);
                // Copy or move object to the scene directory
                if (!QFile::exists(newPath))
                {
                    QString program("cp");
                    QStringList arguments;
                    arguments << "-p" << oldPath << newPath;
                    QProcess *copyProcess = new QProcess(0);
                    copyProcess->start(program, arguments);
                    if (copyProcess->waitForStarted())
                        copyProcess->waitForFinished();
                }
            }
            else
            {
                obj->SetFullFileName("");
                obj->SetDataFileName("");
                newPath = QString("none");
            }
        }
        else
        {// we only have to create a MINC or PolyData (vtk) file, other objects will be saved within scene, used only for reconstructed volumes
            const char *className = obj->GetClassName();
            if (strcmp( className, "ImageObject") == 0 )
            {
                dataFileName.append("mnc");
                newPath.append(dataFileName);
                ImageObject *image = ImageObject::SafeDownCast(obj);
                image->SaveImageData(newPath);
            }
            else if( strcmp( className, "PolyDataObject") == 0 )
            {
                dataFileName.append("vtk");
                newPath.append(dataFileName);
                PolyDataObject *pObj = PolyDataObject::SafeDownCast(obj);
                pObj->SavePolyData(newPath);
            }
            else
                newPath = QString("none");
        }
        id = obj->GetObjectID();
        if(obj->GetParent())
            parentId = obj->GetParent()->GetObjectID();
        QString relPath(newPath);
        relPath.replace(this->GetSceneDirectory(), ".");
        //  FullFilename, ObjectID and ParentId are rather scene properties and are serialized outside of object
        // ObjectClass has to be known before in order to serialize correctly
        ::Serialize( ser, "ObjectClass", className);
        ::Serialize( ser, "FullFileName", relPath);
        ::Serialize( ser, "ObjectID", id );
        ::Serialize( ser, "ParentID", parentId );
        obj->Serialize(ser);
        ser->EndSection();
        this->UpdateProgress(i+1);
    }
    ser->EndSection();

    ser->BeginSection("Plugins");
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    for( int t = 0; t < allTools.size(); ++t )
    {
        ToolPluginInterface * toolModule = allTools[t];
        ser->BeginSection( toolModule->GetPluginName().toUtf8().data() );
        toolModule->Serialize(ser);
        ser->EndSection();
    }
    ser->EndSection();
}

void SceneManager::NotifyPluginsSceneAboutToLoad()
{
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    for( int i = 0; i < allTools.size(); ++i )
    {
        ToolPluginInterface * toolModule = allTools[i];
        toolModule->SceneAboutToLoad();
    }
}

void SceneManager::NotifyPluginsSceneFinishedLoading()
{
    // Tell plugins new scene finished loading
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    for( int i = 0; i < allTools.size(); ++i )
    {
        ToolPluginInterface * toolModule = allTools[i];
        toolModule->SceneFinishedLoading();
    }
}

void SceneManager::NotifyPluginsSceneAboutToSave()
{
    // Tell plugins new scene finished loading
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    for( int i = 0; i < allTools.size(); ++i )
    {
        ToolPluginInterface * toolModule = allTools[i];
        toolModule->SceneAboutToSave();
    }
}

void SceneManager::NotifyPluginsSceneFinishedSaving()
{
    // Tell plugins new scene finished loading
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    for( int i = 0; i < allTools.size(); ++i )
    {
        ToolPluginInterface * toolModule = allTools[i];
        toolModule->SceneFinishedSaving();
    }
}

void SceneManager::Register(vtkObjectBase* o)
{
    vtkObjectBase::Register(o);
}

void SceneManager::UnRegister(vtkObjectBase* o)
{
    vtkObjectBase::UnRegister(o);
}


void SceneManager::ClockTick()
{
    PointerObject * navPointer = this->GetNavigationPointerObject();
    if( navPointer  &&  this->IsNavigating )
    {
        this->SetCursorWorldPosition( navPointer->GetTipPosition() );
    }
}

// navigation
void SceneManager::EnablePointerNavigation( bool on )
{
    PointerObject * navPointer = this->GetNavigationPointerObject();
    if( navPointer )
        IsNavigating = on;
}

void SceneManager::SetNavigationPointerID( int id )
{
    this->NavigationPointerID = id;
    emit NavigationPointerChanged();
}

PointerObject *SceneManager::GetNavigationPointerObject( )
{
    if( this->NavigationPointerID == SceneManager::InvalidId )
        return 0;
    SceneObject *obj =  this->GetObjectByID( this->NavigationPointerID );
    if( obj )
    {
        return PointerObject::SafeDownCast( obj );
    }
    return 0;
}

void SceneManager::ValidatePointerObject()
{
    int pointerId = this->NavigationPointerID;
    if( pointerId != SceneManager::InvalidId )
        if( !GetObjectByID( pointerId ) )
            pointerId = SceneManager::InvalidId;
    if( pointerId == SceneManager::InvalidId )
    {
        QList<PointerObject*> allPointers;
        GetAllPointerObjects( allPointers );
        if( allPointers.size() > 0 )
            pointerId = allPointers[0]->GetObjectID();
    }
    if( pointerId != this->NavigationPointerID )
        SetNavigationPointerID( pointerId );
}
