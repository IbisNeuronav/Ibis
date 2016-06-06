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
#include "volumerenderingobject.h"
#include "worldobject.h"
#include "ignsmsg.h"
#include "pointsobject.h"
#include "cameraobject.h"
#include "usacquisitionobject.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "filereader.h"
#include "sceneinfo.h"
#include "ignsconfig.h"
#include "application.h"
#include "hardwaremodule.h"
#include "objectplugininterface.h"
#include "toolplugininterface.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPluginLoader>
#include <QProcess>
#include <QProgressDialog>
#include <QApplication>
#include "octants.h"

ObjectSerializationMacro( SceneManager );

SceneManager::SceneManager()
{
    m_viewFollowsReferenceObject = true;
    this->CurrentView = -1;
    this->ExpandedView = -1;
    this->ViewBackgroundColor[0] = 0;
    this->ViewBackgroundColor[1] = 0;
    this->ViewBackgroundColor[2] = 0;
    this->CameraViewAngle3D = 30.0;
    this->SupportedSceneSaveVersion = IGNS_SCENE_SAVE_VERSION;
    this->NavigationPointerID = SceneObject::InvalidObjectId;
    this->IsNavigating = false;

    this->Init();
}

SceneManager::~SceneManager()
{
}

// for debugging only
int SceneManager::GetRefCount()
{
    int n = this->GetReferenceCount();
    return n;
}

void SceneManager::Destroy()
{
    disconnect(this);
    this->blockSignals( true );  // at this point, we don't want signals to be emited.
    this->RemoveObject( this->SceneRoot );

    this->ReleaseAllViews();
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        (*it)->Delete();
    }
    Views.clear();
    for( int i = 0; i < TemporaryFiles.count(); i++ )
    {
        QString program( "rm" );
        QStringList arguments;
        arguments << "-rf" << TemporaryFiles[i];
        QProcess *removeProcess = new QProcess(0);
        removeProcess->start(program, arguments);
        if( removeProcess->waitForStarted() )
            removeProcess->waitForFinished();
        delete removeProcess;
    }
    TemporaryFiles.clear();
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
    connect( this->MainCutPlanes, SIGNAL(PlaneMoved(int)), this, SLOT(OnCutPlanesPositionChanged()) );

    // Volume Renderer
    this->MainVolumeRenderer = VolumeRenderingObject::New();
    this->MainVolumeRenderer->SetCanChangeParent( false );
    this->MainVolumeRenderer->SetName( "Volume Renderer" );
    this->MainVolumeRenderer->SetNameChangeable( false );
    this->MainVolumeRenderer->SetCanAppendChildren( false );
    this->MainVolumeRenderer->SetCanEditTransformManually( false );
    this->MainVolumeRenderer->SetHidden( true );
    this->MainVolumeRenderer->SetObjectManagedBySystem( true );
    this->MainVolumeRenderer->SetObjectDeletable(false);
    AddObject( this->MainVolumeRenderer, this->SceneRoot );


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

    this->OctantsOfInterest = Octants::New();
    this->InteractorStyle3D = InteractorStyleTrackball;
    this->CurrentSceneInfo = SceneInfo::New();
    m_sceneLoadSaveProgressDialog = 0;
}

void SceneManager::Clear()
{
    for( int i = 0; i < TemporaryFiles.count(); i++ )
    {
        QString program( "rm" );
        QStringList arguments;
        arguments << "-rf" << TemporaryFiles[i];
        QProcess *removeProcess = new QProcess(0);
        removeProcess->start(program, arguments);
        if( removeProcess->waitForStarted() )
            removeProcess->waitForFinished();
        delete removeProcess;
    }
    TemporaryFiles.clear();
    disconnect( this->GetMainImagePlanes(), SIGNAL(PlaneMoved(int)), this, SLOT(OnCutPlanesPositionChanged()) );
    this->RemoveAllSceneObjects();
    Q_ASSERT_X( AllObjects.size() == 0, "SceneManager::~SceneManager()", "Objects are left in the global list.");
    this->OctantsOfInterest->Delete();
    this->CurrentSceneInfo->Delete();
    this->SceneRoot->Delete();
}

void SceneManager::ClearScene()
{
    SetRenderingEnabled( false );

    NotifyPluginsSceneAboutToLoad();

    // Start progress
    QProgressDialog * progressDialog = Application::GetInstance().StartProgress(4, tr("Removing All Scene Objects..."));

    // Do the clear
    Application::GetHardwareModule()->RemoveObjectsFromScene();
    InternalClearScene();
    Application::GetHardwareModule()->AddObjectsToScene();

    // Make sure everything is updated
    Application::GetInstance().UpdateProgress(progressDialog, 4);
    QApplication::processEvents();

    NotifyPluginsSceneFinishedLoading();

    // Stop progress
    Application::GetInstance().StopProgress(progressDialog);

    SetRenderingEnabled( true );
}

void SceneManager::InternalClearScene()
{
    QString saveSceneDir(this->CurrentSceneInfo->GetSceneDirectory());
    this->Clear();
    this->Init();
    this->SetSceneDirectory(saveSceneDir);
}

void SceneManager::RemoveAllSceneObjects()
{
    this->RemoveObject( this->SceneRoot );

#ifdef USE_NEW_IMAGE_PLANES
    this->MainImagePlanes = 0;
#else
    this->MainCutPlanes = 0;
#endif
    this->MainVolumeRenderer = 0;
}

void SceneManager::RemoveAllChildrenObjects(SceneObject *obj)
{
    int n;
    while( (n = obj->GetNumberOfChildren()) > 0)
    {
        SceneObject * o1 = obj->GetChild( n - 1 );
        this->RemoveAllChildrenObjects( o1 );
        this->RemoveObject( o1 );
    }
}

void SceneManager::LoadScene( QString & fileName, bool interactive )
{
    SetRenderingEnabled( false );

    // Set scene directory
    QFileInfo info( fileName );
    QString newDir = info.dir().absolutePath();
    SetSceneDirectory( newDir );

    NotifyPluginsSceneAboutToLoad();

    // Remove hardware module objects from scene before loading
    Application::GetHardwareModule()->RemoveObjectsFromScene();

    // Clear the scene
    InternalClearScene();

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
        return;
    }
    else if( reader.FileVersionIsLowerThan( QString::number(5.0) ) )
    {
        QString message = "This scene version is older than 5.0. This is not supported anymore. Scene may not be correctly restored.\n";
        QMessageBox::warning( 0, "Error", message, 1, 0 );
    }
    ::Serialize( &reader, "SceneInfo", this->CurrentSceneInfo);
    int numberOfSceneObjects;
    ::Serialize( &reader, "NumberOfSceneObjects", numberOfSceneObjects );
    ::Serialize( &reader, "NextObjectID", this->NextObjectID);

    // Start Progress dialog
    if( interactive )
        m_sceneLoadSaveProgressDialog = Application::GetInstance().StartProgress( numberOfSceneObjects*2+3, tr("Loading Scene...") );

    this->ObjectReader(&reader, interactive);
    if( interactive && !this->UpdateProgress(numberOfSceneObjects+1) )
        return;
    ::Serialize( &reader, "SceneManager", this );
    SceneObject *currentObject = this->GetCurrentObject();
    if( interactive && !this->UpdateProgress(numberOfSceneObjects+1) )
        return;

    // Read other params of the scene
    bool axesHidden;
    bool cursorVisible;
    ::Serialize( &reader, "AxesHidden", axesHidden);
    ::Serialize( &reader, "CursorVisible", cursorVisible);
    this->SceneRoot->SetAxesHidden(axesHidden);
    this->SceneRoot->SetCursorVisible(cursorVisible);
    reader.EndSection();
    reader.Finish();
    this->CurrentSceneInfo->SetDirectorySet(true);
    this->CurrentSceneInfo->SetSessionFile(fileName);
    if( interactive && !this->UpdateProgress(numberOfSceneObjects+3) )
        return;

    // Give a chance to all objects to react after scene is read
    this->PostSceneRead(numberOfSceneObjects+3);

    // Stop progress
    if( interactive )
        Application::GetInstance().StopProgress(m_sceneLoadSaveProgressDialog);
    m_sceneLoadSaveProgressDialog = 0;

    // Put back hardware module objects after loading
    Application::GetHardwareModule()->AddObjectsToScene();

    // Tell plugins new scene finished loading
    NotifyPluginsSceneFinishedLoading();

    SetRenderingEnabled( true );
    this->SetCurrentObject( currentObject);
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
    QApplication::processEvents();
    Application::GetInstance().UpdateProgress(m_sceneLoadSaveProgressDialog, value);
    return true;
}

void SceneManager::SaveScene(QString & fileName)
{
    SceneObject *currentObject = this->GetCurrentObject();
    NotifyPluginsSceneAboutToSave();

    // Remove hardware module objects so that they don't get saved in the scene
    Application::GetHardwareModule()->RemoveObjectsFromScene();

    QList< SceneObject* > listedObjects;
    this->GetAllListableNonTrackedObjects(listedObjects);
    int numberOfSceneObjects = listedObjects.count();
    m_sceneLoadSaveProgressDialog = Application::GetInstance().StartProgress(numberOfSceneObjects+3, tr("Saving Scene..."));
    m_sceneLoadSaveProgressDialog->setCancelButton(0);
    SerializerWriter writer;
    writer.SetFilename( fileName.toUtf8().data() );
    writer.Start();
    writer.BeginSection("SaveScene");
    QString version(IGNS_SCENE_SAVE_VERSION);
    ::Serialize( &writer, "Version", version);
    ::Serialize( &writer, "SceneInfo", this->CurrentSceneInfo);
    ::Serialize( &writer, "NextObjectID", this->NextObjectID);
    this->UpdateProgress(1);
    this->ObjectWriter(&writer);
    ::Serialize( &writer, "SceneManager", this);
    this->UpdateProgress(numberOfSceneObjects+2);
    bool axesHidden = this->SceneRoot->AxesHidden();
    bool cursorVisible = this->SceneRoot->GetCursorVisible();
    ::Serialize( &writer, "AxesHidden", axesHidden);
    ::Serialize( &writer, "CursorVisible", cursorVisible);
    writer.EndSection();
    writer.Finish();
    Application::GetInstance().StopProgress(m_sceneLoadSaveProgressDialog);
    m_sceneLoadSaveProgressDialog = 0;

    // Put back hardware module objects after saving
    Application::GetHardwareModule()->AddObjectsToScene();

    NotifyPluginsSceneFinishedSaving();
    this->SetCurrentObject( currentObject);
}

void SceneManager::SetSceneDirectory(const QString &wDir)
{
   this->CurrentSceneInfo->SetSceneDirectory(wDir);
}

const QString SceneManager::GetSceneDirectory()
{
    return this->CurrentSceneInfo->GetSceneDirectory();
}

void SceneManager::Serialize( Serializer * ser )
{
    int id = SceneObject::InvalidObjectId;
    int refObjID = SceneObject::InvalidObjectId;
    if (!ser->IsReader() && this->CurrentObject)
    {
        id = this->CurrentObject->GetObjectID();
        if( this->ReferenceDataObject )
            refObjID = this->ReferenceDataObject->GetObjectID();
    }
    ::Serialize( ser, "CurrentObjectID", id );
    ::Serialize( ser, "ReferenceObjectID", refObjID );

    ser->BeginSection("Views");
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        View * view  = (*it);
        QString viewName(view->GetName());
        ::Serialize( ser, viewName.toUtf8().data(), view );
    }
    int oldExpandedView = ExpandedView;
    ::Serialize( ser, "ExpandedView", ExpandedView );
    int newExpandedView = ExpandedView;
    if (ser->IsReader())
    {
        if (oldExpandedView > -1)
        {
            emit ExpandView();
        }
        if (newExpandedView > -1)
        {
            CurrentView = newExpandedView;
            emit ExpandView();
        }
        this->SetCurrentObject(this->GetObjectByID(id));
        if( refObjID != SceneObject::InvalidObjectId )
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

View * SceneManager::CreateView( int type, QString name )
{
    View * res = this->GetView( type );

    if( res )
        return res;

    res = View::New();
    res->SetName( name );
    res->SetManager( this );
    res->SetType( type );
    if( type == THREED_VIEW_TYPE )
    {
        AssignInteractorStyleToView( this->InteractorStyle3D, res );
        res->SetViewAngle( this->CameraViewAngle3D );
    }

    // Reset cameras for the colin27 in talairach space. This should be more or less good
    // for any kind of brain data.
    double bounds[6] = { -90.0, 91.0, -126.0, 91.0, -72.0, 109.0 };
    res->ResetCamera( bounds );

    this->Views.push_back( res );

    return res;
}

View * SceneManager::GetView( int type )
{
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        if( (*it)->GetType() == type )
        {
            return (*it);
        }
    }
    return NULL;
}


// Todo : do something smarter. Main 3D view is not any 3D view!
View * SceneManager::GetMain3DView()
{
    return GetView( THREED_VIEW_TYPE );
}

View * SceneManager::GetView( const QString & name )
{
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        if( (*it)->GetName() == name )
        {
            return (*it);
        }
    }
    return NULL;
}

View * SceneManager::GetViewFromInteractor( vtkRenderWindowInteractor * interactor )
{
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        if( (*it)->GetInteractor() == interactor )
        {
            return (*it);
        }
    }
    return NULL;
}

void SceneManager::SetViewBackgroundColor( double * color )
{
    this->ViewBackgroundColor[0] = color[0];
    this->ViewBackgroundColor[1] = color[1];
    this->ViewBackgroundColor[2] = color[2];
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        (*it)->SetBackgroundColor( color );
    }
}

void SceneManager::UpdateBackgroundColor( )
{
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        (*it)->SetBackgroundColor( this->ViewBackgroundColor );
    }
}

void SceneManager::AddObject( SceneObject * object, SceneObject * attachTo )
{
    this->AddObjectUsingID(object, attachTo, SceneObject::InvalidObjectId);
}

void SceneManager::AddObjectUsingID( SceneObject * object, SceneObject * attachTo, int objID )
{
    if( object )
    {
        // Register and add to the global list
        object->Register( this );
        this->AllObjects.push_back( object );

        if( !attachTo )
            attachTo = GetSceneRoot();

        // Notify clients we start adding an object
        int position = attachTo->GetNumberOfListableChildren();
        emit StartAddingObject( attachTo, position );

        // Setting object id and manager
        int id = objID;  // if we get a valid id, use it
        if( id == SceneObject::InvalidObjectId )
        {
            // if the object has already been assigned an id, keep it
            if( object->GetObjectID() != SceneObject::InvalidObjectId )
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

        // Notify clients the object has been added
        if( object->IsListable() )
            this->SetCurrentObject(object);
        emit FinishAddingObject();
        emit ObjectAdded( id );
    }
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

    // remove the object from the global list
    this->AllObjects.removeAt( indexAll );

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


    // Release from all views
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        object->Release( (*it) );
    }

    // Detach from parent
	if( parent )
    {
        parent->RemoveChild( object );
    }

    // Tell other this object is being removed
    object->RemoveFromScene();

    object->UnRegister( this );

    if (object == this->ReferenceDataObject)
        this->ReferenceDataObject  = 0;
    if( object->IsListable() )
        emit FinishRemovingObject();
    emit ObjectRemoved( objId );
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
    GetChildrenImageObjects( GetSceneRoot(), objects );
}


void SceneManager::GetChildrenImageObjects( SceneObject * obj, QList<ImageObject*> & all )
{
    for( int i = 0; i < obj->GetNumberOfChildren(); ++i )
    {
        SceneObject * child = obj->GetChild(i);
        ImageObject * imChild = ImageObject::SafeDownCast( child );
        if( imChild )
            all.push_back( imChild );
        GetChildrenImageObjects( child, all );
    }
}

void SceneManager::GetAllPolydataObjects( QList<PolyDataObject*> & objects )
{
    GetChildrenPolydataObjects( GetSceneRoot(), objects );
}

void SceneManager::GetChildrenPolydataObjects( SceneObject * obj, QList<PolyDataObject*> & all )
{
    for( int i = 0; i < obj->GetNumberOfChildren(); ++i )
    {
        SceneObject * child = obj->GetChild(i);
        PolyDataObject * imChild = PolyDataObject::SafeDownCast( child );
        if( imChild )
            all.push_back( imChild );
        GetChildrenPolydataObjects( child, all );
    }
}

void SceneManager::GetAllPointsObjects( QList<PointsObject*> & objects )
{
    GetChildrenPointsObjects( this->GetSceneRoot(), objects );
}

void SceneManager::GetChildrenPointsObjects( SceneObject * obj, QList<PointsObject*> & all )
{
    for( int i = 0; i < obj->GetNumberOfChildren(); ++i )
    {
        SceneObject * child = obj->GetChild(i);
        PointsObject * imChild = PointsObject::SafeDownCast( child );
        if( imChild )
            all.push_back( imChild );
        GetChildrenPointsObjects( child, all );
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
    if( id == SceneObject::InvalidObjectId )
        return 0;
    for( int i = 0; i < this->AllObjects.size(); ++i )
    {
        if( this->AllObjects[i]->GetObjectID() == id )
            return this->AllObjects[i];
    }
    return 0;
}

bool SceneManager::ObjectExists(SceneObject *obj)
{
    if (obj && this->GetObjectByID(obj->GetObjectID()))
        return true;
    return false;
}

void SceneManager::SetCurrentObject( SceneObject * obj )
{
    // SetCurrentObject(0) is called to force refreshing of the object list in the left pane
    if (obj)
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
        if( this->ReferenceDataObject )
        {
            connect( this->ReferenceDataObject, SIGNAL(WorldTransformChangedSignal()), this, SLOT(ReferenceTransformChangedSlot()));
            this->ResetAllCameras();
        }
        emit ReferenceTransformChanged();

        // define octants of interest
        double bounds[6];
        double origin[3];
        this->ReferenceDataObject->GetImage()->GetBounds(bounds);
        this->GetMainImagePlanes()->GetPlanesPosition( origin );
        this->OctantsOfInterest->SetBounds(bounds);
        this->OctantsOfInterest->SetOrigin(origin);

//        vtkMINCImageAttributes2 * attributes = this->ReferenceDataObject->GetAttributes();
//        if (attributes)
//        {
//            this->CurrentSceneInfo->SetPatientName(attributes->GetAttributeValueAsString(MIpatient, MIfull_name));
//            this->CurrentSceneInfo->SetPatientIdentification(attributes->GetAttributeValueAsString(MIpatient, MIidentification));
//        }
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
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        (*it)->ResetCamera();
    }
}

void SceneManager::ResetAllCameras( double bounds[6] )
{
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        (*it)->ResetCamera( bounds );
    }
}

void SceneManager::ZoomAllCameras(double factor)
{
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        (*it)->ZoomCamera(factor);
    }
}

void SceneManager::SetupAllObjects( View * v )
{
    SetupOneObject( v, GetSceneRoot() );
}

void SceneManager::ReleaseAllObjects( View * v )
{
    GetSceneRoot()->Release( v );
    this->MainVolumeRenderer->Release( v );
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
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        (*it)->ReleaseView();
    }
}

void SceneManager::SetupInAllViews( SceneObject * object )
{
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
		object->Setup( (*it) );
    }
}

void SceneManager::Set3DInteractorStyle( InteractorStyle style )
{
    this->InteractorStyle3D = style;
    for( ViewList::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        if( (*it)->GetType() == THREED_VIEW_TYPE )
            AssignInteractorStyleToView( style, (*it) );
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
    View * v3d = GetView( THREED_VIEW_TYPE );
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



vtkRenderer *SceneManager::GetViewRenderer(int viewType)
{
    View *v = this->GetView(viewType);
    return v->GetRenderer();
}

void SceneManager::SetRenderingEnabled( bool r )
{
    for( int i = 0; i < this->Views.size(); ++i )
        this->Views[i]->SetRenderingEnabled( r );
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

void SceneManager::OnCutPlanesPositionChanged()
{
    emit CursorPositionChanged();
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

void SceneManager::GetChildrenUserObjects( SceneObject * obj, QList<SceneObject*> & list )
{
    for( int i = 0; i < obj->GetNumberOfChildren(); ++i )
    {
        SceneObject * child = obj->GetChild(i);
        if( !child->IsManagedByTracker() && !child->IsManagedBySystem() && child->IsListable() )
            list.append(child);
        GetChildrenUserObjects( child, list );
    }
}

int SceneManager::GetNumberOfUserObjects()
{
    QList<SceneObject*> allUserObjects;
    GetAllUserObjects( allUserObjects );
    return allUserObjects.size();
}

void SceneManager::GetAllUserObjects(QList<SceneObject*> &list)
{
    GetChildrenUserObjects( GetSceneRoot(), list );
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
    bool sceneOlderThanVersion6_0 = ser->FileVersionIsLowerThan( QString::number(6.0) );

    // needed for backward compatibility:
    int preopId = SceneObject::InvalidObjectId;
    int intraopId = SceneObject::InvalidObjectId;
    int oldPreopId = SceneObject::InvalidObjectId;
    int oldIntraopId = SceneObject::InvalidObjectId;
    for (i = 0; i < numberOfSceneObjects; i++)
    {
        QString objectName;
        QString sectionName = QString( "ObjectInScene_%1" ).arg(i);
        ser->BeginSection( sectionName.toUtf8().data() );
        ::Serialize( ser, "ObjectClass", className );
        ::Serialize( ser, "FullFileName", filePath );
        ::Serialize( ser, "ObjectID", oldId );
        ::Serialize( ser, "ParentID", oldParentId );
        if( sceneOlderThanVersion6_0  && oldId < 0)
        {
            ::Serialize( ser, "ObjectName", objectName );
            if( ( objectName == QString(PREOP_ROOT_OBJECT_NAME) ) ||
                ( objectName == QString(INTRAOP_ROOT_OBJECT_NAME) ) )
            {
                SceneObject *obj = SceneObject::New();
                obj->Serialize(ser);
                this->AddObjectUsingID(obj, this->GetSceneRoot(), this->NextObjectID);
                obj->SetObjectDeletable( true );
                obj->SetObjectManagedBySystem( false );
                if( objectName == QString(PREOP_ROOT_OBJECT_NAME) )
                {
                    preopId = obj->GetObjectID();
                    oldPreopId = oldId;
                }
                else
                {
                    intraopId = obj->GetObjectID();
                    oldIntraopId = oldId;
                }
                obj->Delete();
            }
        }
        parentObject = 0;
        if (oldParentId != SceneObject::InvalidObjectId) // only World does not have parent
        {
            if( sceneOlderThanVersion6_0 )
            {
                if( oldParentId == oldPreopId )
                {
                    parentObject = this->GetObjectByID(preopId);
                }
                else if( oldParentId == oldIntraopId )
                {
                    parentObject = this->GetObjectByID(intraopId);
                }
                else
                    parentObject = this->GetObjectByID(oldParentId);
            }
            else
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
            fileReader->SetFileNames( fileToOpen );
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
//                QString oldName(obj->GetName());
//                emit ObjectNameChanged(oldName, obj->GetName());
                if( filePath != obj->GetFullFileName() )
                    TemporaryFiles.push_back( obj->GetFullFileName() );
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
                // pt->SetObjectDeletable(true);  todo : check if this is needed - yes, in old scenes

                pt->Delete();
            }
            else if (QString::compare(className, QString("USAcquisitionObject"), Qt::CaseSensitive) == 0)
            {
                USAcquisitionObject *usObj = USAcquisitionObject::New();
                usObj->ObjectID = oldId;  // serialization needs the id
                usObj->Serialize(ser);
                //usObj->SetObjectDeletable(true);  todo : check if this is needed - yes, in old scenes
                this->AddObjectUsingID(usObj, parentObject, oldId);
                usObj->Delete();
            }
            else if (QString::compare(className, QString("VolumeRenderingObject"), Qt::CaseSensitive) == 0) // one and only
            {
                this->GetMainVolumeRenderer()->Serialize(ser);
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
            else if (QString::compare(className, QString("SceneObject"), Qt::CaseSensitive) == 0)// PreopRoot, IntraopRoot, Transform object or whatever scene object
            {
                if( !(sceneOlderThanVersion6_0 && oldId < 0) )
                {
                    SceneObject *obj = SceneObject::New();
                    obj->Serialize(ser);
                    // obj->SetObjectDeletable(true); todo : check if this is needed
                    this->AddObjectUsingID(obj, parentObject, oldId);
                    obj->Delete();
                }
            }
            else
            {
                foreach( QObject * plugin, QPluginLoader::staticInstances() )
                {
                    ObjectPluginInterface * objectPlugin = qobject_cast< ObjectPluginInterface* >( plugin );
                    if( objectPlugin )
                    {
                       QString name = objectPlugin->GetObjectClassName();
                       if ( QString::compare(name, className) == 0 )
                       {
                           objectPlugin->CreateObject();
                           this->GetCurrentObject()->Serialize(ser);
                           // ignore the id given by SceneManager
                           this->GetCurrentObject()->SetObjectID(oldId);
                       }
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
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
        {
            bool sectionFound = ser->BeginSection( toolModule->GetPluginName().toUtf8().data() );
            toolModule->Serialize(ser);
            if( sectionFound )
                ser->EndSection();
            if( interactive && !this->UpdateProgress(++i) )
                return;
        }
    }
    ser->EndSection();
    if( sceneOlderThanVersion6_0 )
    {
        if( ser->BeginSection("LandmarkRegistration"))
        {
            int registered;
            int sourcePointsID = SceneObject::InvalidObjectId;
            int targetPointsID = SceneObject::InvalidObjectId;
            int targetObjectID = SceneObject::InvalidObjectId;
            ::Serialize( ser, "Registered", registered );
            ::Serialize( ser, "SourcePointsObjectId", sourcePointsID );
            ::Serialize( ser, "TargetPointsObjectId", targetPointsID );
            ser->EndSection();
            if( registered == 1 )
            {
                 QMessageBox::warning( 0, "Error", "Loaded scene version 5.0 or earlier - registration cannot be changed" , 1, 0 );
            }
            else
            {
                if( ( sourcePointsID != SceneObject::InvalidObjectId ) && ( targetPointsID != SceneObject::InvalidObjectId ))
                {
                    foreach( QObject * plugin, QPluginLoader::staticInstances() )
                    {
                        ObjectPluginInterface * objectPlugin = qobject_cast< ObjectPluginInterface* >( plugin );
                        if( objectPlugin )
                        {
                           QString name = objectPlugin->GetObjectClassName();
                           if ( name == "LandmarkRegistrationObject" )
                           {
                               int ret = QMessageBox::warning(0, "Load Scene",
                                                              tr("Loaded scene version 5.0 or earlier.\nWould you like to create LandmarkRegistrationObject using PointsObjects from loaded scene?"),
                                                              QMessageBox::Yes | QMessageBox::Default,
                                                              QMessageBox::No | QMessageBox::Escape);
                               if (ret == QMessageBox::Yes)
                               {
                                   objectPlugin->CreateObject();
                                   this->GetCurrentObject()->Serialize(ser);
                               }
                           }
                        }
                    }
                }
            }
        }
    }
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
    int id, parentId = SceneObject::InvalidObjectId;
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
        {// we only have to create a MINC file, other objects will be saved within scene, used only for reconstructed volumes
            if (obj->IsA("ImageObject"))
            {
                dataFileName.append("mnc");
                newPath.append(dataFileName);
                ImageObject *image = ImageObject::SafeDownCast(obj);
                image->WriteFile(newPath);
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
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
        {
            ser->BeginSection( toolModule->GetPluginName().toUtf8().data() );
            toolModule->Serialize(ser);
            ser->EndSection();
            this->UpdateProgress(++i);
        }
    }
    ser->EndSection();
}

void SceneManager::NotifyPluginsSceneAboutToLoad()
{
    // Tell plugins new scene is about to load
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
            toolModule->SceneAboutToLoad();
    }
}

void SceneManager::NotifyPluginsSceneFinishedLoading()
{
    // Tell plugins new scene finished loading
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
            toolModule->SceneFinishedLoading();
    }
}

void SceneManager::NotifyPluginsSceneAboutToSave()
{
    // Tell plugins new scene finished loading
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
            toolModule->SceneAboutToSave();
    }
}

void SceneManager::NotifyPluginsSceneFinishedSaving()
{
    // Tell plugins new scene finished loading
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
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
}

PointerObject *SceneManager::GetNavigationPointerObject( )
{
    if( this->NavigationPointerID == SceneObject::InvalidObjectId )
        return 0;
    SceneObject *obj =  this->GetObjectByID( this->NavigationPointerID );
    if( obj )
    {
        return PointerObject::SafeDownCast( obj );
    }
    return 0;
}

