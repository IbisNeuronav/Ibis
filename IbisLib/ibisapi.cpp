#include "ibisapi.h"
#include "scenemanager.h"

const int IbisAPI::InvalidId = SceneManager::InvalidId;


IbisAPI::IbisAPI()
{

}

IbisAPI::~IbisAPI()
{

}


void IbisAPI::SetApplication( Application * app )
{
    m_application = app;
    m_sceneManager = m_application->GetSceneManager();
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

void IbisAPI::ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex )
{
    m_sceneManager->ChangeParent( object, newParent, newChildIndex );
}

bool IbisAPI::IsLoadingScene()
{
    return m_sceneManager->IsLoadingScene();
}
