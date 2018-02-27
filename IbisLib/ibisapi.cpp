#include "ibisapi.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "pointerobject.h"

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
