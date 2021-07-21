/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "ibisapi.h"
#include "application.h"
#include "filereader.h"
#include "scenemanager.h"
#include "mainwindow.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "polydataobject.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"
#include "ibispreferences.h"
#include "view.h"
#include "usacquisitionobject.h"
#include "toolplugininterface.h"
#include "objectplugininterface.h"

#include <QString>
#include <QProgressDialog>

const int IbisAPI::InvalidId = SceneManager::InvalidId;

IbisAPI::IbisAPI(Application *app)
{
    this->SetApplication( app );
}

IbisAPI::~IbisAPI()
{
    disconnect( m_sceneManager, SIGNAL( ObjectAdded(int) ), this, SLOT( ObjectAddedSlot(int) ) );
    disconnect( m_sceneManager, SIGNAL( ObjectRemoved(int) ), this, SLOT( ObjectRemovedSlot(int) ) );
    disconnect( m_sceneManager, SIGNAL( ReferenceTransformChanged() ), this, SLOT( ReferenceTransformChangedSlot() ) );
    disconnect( m_sceneManager, SIGNAL( ReferenceObjectChanged() ), this, SLOT( ReferenceObjectChangedSlot() ) );
    disconnect( m_sceneManager, SIGNAL( CursorPositionChanged() ), this, SLOT( CursorPositionChangedSlot() ) );
    disconnect( m_sceneManager, SIGNAL( NavigationPointerChanged() ), this, SLOT( NavigationPointerChangedSlot() ) );
    disconnect( m_application, SIGNAL( IbisClockTick() ), this, SLOT( IbisClockTickSlot() ) );
}

void IbisAPI::SetApplication( Application * app )
{
    m_application = app;
    m_sceneManager = m_application->GetSceneManager();

    connect( m_sceneManager, SIGNAL( ObjectAdded(int) ), this, SLOT( ObjectAddedSlot(int) ) );
    connect( m_sceneManager, SIGNAL( ObjectRemoved(int) ), this, SLOT( ObjectRemovedSlot(int) ) );
    connect( m_sceneManager, SIGNAL( ReferenceTransformChanged() ), this, SLOT( ReferenceTransformChangedSlot() ) );
    connect( m_sceneManager, SIGNAL( ReferenceObjectChanged() ), this, SLOT( ReferenceObjectChangedSlot() ) );
    connect( m_sceneManager, SIGNAL( CursorPositionChanged() ), this, SLOT( CursorPositionChangedSlot() ) );
    connect( m_sceneManager, SIGNAL( NavigationPointerChanged() ), this, SLOT( NavigationPointerChangedSlot() ) );
    connect( m_application, SIGNAL( IbisClockTick() ), this, SLOT( IbisClockTickSlot() ) );
}

// from SceneManager
void IbisAPI::AddObject(SceneObject * object, SceneObject * parenObject )
{
    m_sceneManager->AddObject( object, parenObject );
}

void IbisAPI::RemoveObject(SceneObject * object )
{
    m_sceneManager->RemoveObject( object );
}

void IbisAPI::RemoveAllChildrenObjects( SceneObject *obj )
{
    m_sceneManager->RemoveAllChildrenObjects( obj );
}

SceneObject *IbisAPI::GetCurrentObject( )
{
    return m_sceneManager->GetCurrentObject();
}

void IbisAPI::SetCurrentObject( SceneObject * cur  )
{
    m_sceneManager->SetCurrentObject( cur );
}

SceneObject * IbisAPI::GetObjectByID( int id )
{
    return m_sceneManager->GetObjectByID( id );
}

void IbisAPI::RemoveObjectByID( int id )
{
    m_sceneManager->RemoveObjectById( id );
}

SceneObject * IbisAPI::GetSceneRoot()
{
    return m_sceneManager->GetSceneRoot();
}

PointerObject * IbisAPI::GetNavigationPointerObject( )
{
    return m_sceneManager->GetNavigationPointerObject();
}

int  IbisAPI::GetNumberOfUserObjects()
{
    return m_sceneManager->GetNumberOfUserObjects();
}

void IbisAPI::GetAllUserObjects(QList<SceneObject*> &list)
{
    m_sceneManager->GetAllUserObjects( list );
}

ImageObject * IbisAPI::GetReferenceDataObject( )
{
    return m_sceneManager->GetReferenceDataObject();
}

void IbisAPI::GetAllImageObjects( QList<ImageObject*> & objects )
{
    m_sceneManager->GetAllImageObjects( objects );
}

void IbisAPI::GetAllPolyDataObjects( QList<PolyDataObject*> & objects )
{
    m_sceneManager->GetAllPolydataObjects( objects );
}

const QList<SceneObject *> &IbisAPI::GetAllObjects()
{
    return m_sceneManager->GetAllObjects();
}

void IbisAPI::GetAllUSAcquisitionObjects( QList<USAcquisitionObject*> & all )
{
    return m_sceneManager->GetAllUSAcquisitionObjects( all );
}

void IbisAPI::GetAllUsProbeObjects( QList<UsProbeObject*> & all )
{
    return m_sceneManager->GetAllUsProbeObjects( all );
}

void IbisAPI::GetAllPointerObjects( QList<PointerObject*> & all )
{
    return m_sceneManager->GetAllPointerObjects( all );
}

void IbisAPI::GetAllTrackedObjects( QList<TrackedSceneObject*> & all )
{
    return m_sceneManager->GetAllTrackedObjects( all );
}

void IbisAPI::GetAllListableNonTrackedObjects( QList<SceneObject*> & all )
{
    return m_sceneManager->GetAllListableNonTrackedObjects( all );
}

void IbisAPI::GetAllPointsObjects( QList<PointsObject*> & objects )
{
    return m_sceneManager->GetAllPointsObjects( objects );
}

void IbisAPI::GetAllObjectsOfType( const char * typeName, QList<SceneObject*> & all )
{
    return m_sceneManager->GetAllObjectsOfType( typeName, all );
}

void IbisAPI::GetAllCameraObjects( QList<CameraObject*> & all )
{
    return m_sceneManager->GetAllCameraObjects( all );
}

View * IbisAPI::GetViewByID( int id )
{
    return m_sceneManager->GetViewByID( id );
}

View * IbisAPI::GetMain3DView()
{
    return m_sceneManager->GetMain3DView( );
}

View * IbisAPI::GetMainCoronalView()
{
    return m_sceneManager->GetMainCoronalView( );
}

View * IbisAPI::GetMainSagittalView()
{
    return m_sceneManager->GetMainSagittalView( );
}
View * IbisAPI::GetMainTransverseView()
{
    return m_sceneManager->GetMainTransverseView( );
}

QMap<View*, int> IbisAPI::GetAllViews( )
{
    return m_sceneManager->GetAllViews();
}

void IbisAPI::SetRenderingEnabled( bool enable )
{
    m_sceneManager->SetRenderingEnabled( enable );
}

double * IbisAPI::GetViewBackgroundColor()
{
    return m_sceneManager->GetViewBackgroundColor();
}

double * IbisAPI::GetView3DBackgroundColor()
{
    return m_sceneManager->GetView3DBackgroundColor();
}

void IbisAPI::SetViewBackgroundColor( double * color )
{
    m_sceneManager->SetViewBackgroundColor( color );
}

void IbisAPI::SetView3DBackgroundColor( double * color )
{
    m_sceneManager->SetView3DBackgroundColor( color );
}

void IbisAPI::ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex )
{
    m_sceneManager->ChangeParent( object, newParent, newChildIndex );
}

void IbisAPI::NewScene()
{
    m_sceneManager->NewScene();
}

void IbisAPI::LoadScene( QString & filename, bool interactive )
{
    m_sceneManager->LoadScene( filename, interactive );
}

bool IbisAPI::IsLoadingScene()
{
    return m_sceneManager->IsLoadingScene();
}

const QString IbisAPI::GetSceneDirectory()
{
    return m_sceneManager->GetSceneDirectory();
}

QString IbisAPI::FindUniqueName( QString wantedName, QStringList & otherNames )
{
    return SceneManager::FindUniqueName( wantedName, otherNames );
}

void IbisAPI::GetCursorPosition( double pos[3] )
{
    m_sceneManager->GetCursorPosition( pos );
}

void IbisAPI::SetCursorPosition( double *pos )
{
    m_sceneManager->SetCursorPosition( pos );
}

void IbisAPI::GetCursorWorldPosition( double pos[3] )
{
    double curPos[3];
    m_sceneManager->GetCursorPosition( curPos );
    m_sceneManager->ReferenceToWorld( curPos, pos );
}

void IbisAPI::SetCursorWorldPosition( double * pos )
{
    m_sceneManager->SetCursorWorldPosition( pos );
}

// slots
void IbisAPI::ObjectAddedSlot( int id )
{
    emit ObjectAdded( id );
}

void IbisAPI::ObjectRemovedSlot( int id )
{
    emit ObjectRemoved( id );
}

void IbisAPI::ReferenceTransformChangedSlot()
{
    emit ReferenceTransformChanged();
}

void IbisAPI::ReferenceObjectChangedSlot()
{
    emit ReferenceObjectChanged();
}

void IbisAPI::CursorPositionChangedSlot()
{
    emit CursorPositionChanged();
}

void IbisAPI::NavigationPointerChangedSlot()
{
    emit NavigationPointerChanged();
}

void IbisAPI::IbisClockTickSlot()
{
    emit IbisClockTick();
}

// from Application
QProgressDialog * IbisAPI::StartProgress( int max, const QString & caption )
{
    return m_application->StartProgress( max, caption );
}

void IbisAPI::StopProgress( QProgressDialog * progressDialog )
{
    m_application->StopProgress( progressDialog );
}

void IbisAPI::UpdateProgress( QProgressDialog* progressDialog, int current )
{
    m_application->UpdateProgress( progressDialog, current );
}

void IbisAPI::Warning( const QString &title, const QString & text )
{
    m_application->Warning( title, text );
}

bool IbisAPI::IsViewerOnly()
{
    return m_application->IsViewerOnly();
}

QString IbisAPI::GetWorkingDirectory()
{
    return m_application->GetSettings()->WorkingDirectory;
}

QString IbisAPI::GetConfigDirectory()
{
    return m_application->GetConfigDirectory();
}

QString IbisAPI::GetFileNameOpen( const QString & caption, const QString & dir, const QString & filter )
{
    return m_application->GetFileNameOpen( caption, dir, filter );
}
QString IbisAPI::GetFileNameSave( const QString & caption, const QString & dir, const QString & filter )
{
    return m_application->GetFileNameSave( caption, dir, filter );
}
QString IbisAPI::GetExistingDirectory( const QString & caption, const QString & dir )
{
    return m_application->GetExistingDirectory( caption, dir );
}

QString IbisAPI::GetGitHashShort()
{
    return m_application->GetGitHashShort();
}

ToolPluginInterface * IbisAPI::GetToolPluginByName( QString name )
{
    return m_application->GetToolPluginByName( name );
}

ObjectPluginInterface * IbisAPI::GetObjectPluginByName( QString className )
{
    return m_application->GetObjectPluginByName( className );
}

SceneObject * IbisAPI::GetGlobalObjectInstance( const QString & className )
{
    return m_application->GetGlobalObjectInstance( className );
}

void IbisAPI::AddGlobalEventHandler( GlobalEventHandler * h )
{
    m_application->AddGlobalEventHandler( h );
}

void IbisAPI::RemoveGlobalEventHandler( GlobalEventHandler * h )
{
    m_application->RemoveGlobalEventHandler( h );
}

bool IbisAPI::OpenTransformFile( const QString & filename, vtkMatrix4x4 * mat )
{
    return m_application->OpenTransformFile( filename.toUtf8().data(), mat );
}

bool IbisAPI::OpenTransformFile( const QString & filename, SceneObject * obj )
{
    return m_application->OpenTransformFile( filename.toUtf8().data(), obj );
}

void IbisAPI::OpenFiles( OpenFileParams * params, bool addToScene )
{
    m_application->OpenFiles( params, addToScene );
}

void IbisAPI::SetMainWindowFullscreen( bool f )
{
    MainWindow * mw = m_application->GetMainWindow();
    if( f )
    {
        mw->showFullScreen();
    }
    else
    {
        mw->showNormal();
    }
}

void IbisAPI::SetToolbarVisibility( bool v )
{
    MainWindow * mw = m_application->GetMainWindow();
    mw->SetShowToolbar( v );
}

void IbisAPI::SetLeftPanelVisibility( bool v )
{
    MainWindow * mw = m_application->GetMainWindow();
    mw->SetShowLeftPanel( v );
}

void IbisAPI::SetRightPanelVisibility( bool v )
{
    MainWindow * mw = m_application->GetMainWindow();
    mw->SetShowRightPanel( v );
}

void IbisAPI::AddBottomWidget( QWidget * w )
{
    m_application->AddBottomWidget( w );
}

void IbisAPI::RemoveBottomWidget( QWidget * w )
{
    m_application->RemoveBottomWidget( w );
}

void IbisAPI::ShowFloatingDock( QWidget * w, QFlags<QDockWidget::DockWidgetFeature> features )
{
    m_application->ShowFloatingDock( w, features );
}

IbisPreferences * IbisAPI::GetIbisPreferences()
{
    return m_application->GetIbisPreferences();
}

//Custom paths
void IbisAPI::RegisterCustomPath( const QString & pathName, const QString & directoryPath )
{
    m_application->GetIbisPreferences()->RegisterPath( pathName, directoryPath );
}

void IbisAPI::UnRegisterCustomPath( const QString & pathName )
{
    m_application->GetIbisPreferences()->UnRegisterPath( pathName );
}

const QString IbisAPI::GetCustomPath( const QString &pathName )
{
    return m_application->GetIbisPreferences()->GetPath( pathName );
}

bool IbisAPI::IsCustomPathRegistered( const QString &pathName )
{
    return m_application->GetIbisPreferences()->IsPathRegistered( pathName );
}
