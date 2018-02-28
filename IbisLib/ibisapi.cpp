#include "ibisapi.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "usacquisitionobject.h"

#include <QString>

const int IbisAPI::InvalidId = SceneManager::InvalidId;


IbisAPI::IbisAPI()
{

}

IbisAPI::~IbisAPI()
{
    disconnect( m_sceneManager, SIGNAL( ObjectAdded(int) ), this, SLOT( ObjectAddedSlot(int) ) );
    disconnect( m_sceneManager, SIGNAL( ObjectRemoved(int) ), this, SLOT( ObjectRemovedSlot(int) ) );
}

// from SceneManager
void IbisAPI::SetApplication( Application * app )
{
    m_application = app;
    m_sceneManager = m_application->GetSceneManager();

    connect( m_sceneManager, SIGNAL( ObjectAdded(int) ), this, SLOT( ObjectAddedSlot(int) ) );
    connect( m_sceneManager, SIGNAL( ObjectRemoved(int) ), this, SLOT( ObjectRemovedSlot(int) ) );
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

// slots
void IbisAPI::ObjectAddedSlot( int id )
{
    emit ObjectAdded( id );
}

void IbisAPI::ObjectRemovedSlot( int id )
{
    emit ObjectRemoved( id );
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
