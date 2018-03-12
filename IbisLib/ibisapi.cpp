#include "ibisapi.h"
#include "application.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"
#include "view.h"
#include "usacquisitionobject.h"
#include "toolplugininterface.h"

#include <QString>

const int IbisAPI::InvalidId = SceneManager::InvalidId;


IbisAPI::IbisAPI()
{

}

IbisAPI::~IbisAPI()
{
    disconnect( m_sceneManager, SIGNAL( ObjectAdded(int) ), this, SLOT( ObjectAddedSlot(int) ) );
    disconnect( m_sceneManager, SIGNAL( ObjectRemoved(int) ), this, SLOT( ObjectRemovedSlot(int) ) );
    disconnect( m_sceneManager, SIGNAL( ReferenceTransformChanged() ), this, SLOT( ReferenceTransformChangedSlot() ) );
    disconnect( m_sceneManager, SIGNAL( CursorPositionChanged() ), this, SLOT( CursorPositionChangedSlot() ) );
}

// from SceneManager
void IbisAPI::SetApplication( Application * app )
{
    m_application = app;
    m_sceneManager = m_application->GetSceneManager();

    connect( m_sceneManager, SIGNAL( ObjectAdded(int) ), this, SLOT( ObjectAddedSlot(int) ) );
    connect( m_sceneManager, SIGNAL( ObjectRemoved(int) ), this, SLOT( ObjectRemovedSlot(int) ) );
    connect( m_sceneManager, SIGNAL( ReferenceTransformChanged() ), this, SLOT( ReferenceTransformChangedSlot() ) );
    connect( m_sceneManager, SIGNAL( CursorPositionChanged() ), this, SLOT( CursorPositionChangedSlot() ) );
}

void IbisAPI::AddObject( SceneObject * object, SceneObject * attachTo )
{
    m_sceneManager->AddObject( object, attachTo );
}

void IbisAPI::RemoveObject( SceneObject * object , bool viewChange )
{
    m_sceneManager->RemoveObject( object, viewChange );
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

ImageObject * IbisAPI::GetReferenceDataObject( )
{
    return m_sceneManager->GetReferenceDataObject();
}

void IbisAPI::GetAllImageObjects( QList<ImageObject*> & objects )
{
    m_sceneManager->GetAllImageObjects( objects );
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

void IbisAPI::ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex )
{
    m_sceneManager->ChangeParent( object, newParent, newChildIndex );
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

void IbisAPI::CursorPositionChangedSlot()
{
    emit CursorPositionChanged();
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

QString IbisAPI::GetOpenFileName( const QString & caption, const QString & dir, const QString & filter )
{
    return m_application->GetOpenFileName( caption, dir, filter );
}
QString IbisAPI::GetSaveFileName( const QString & caption, const QString & dir, const QString & filter )
{
    return m_application->GetSaveFileName( caption, dir, filter );
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
