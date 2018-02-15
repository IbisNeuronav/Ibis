#ifndef IBISAPI_H
#define IBISAPI_H

#include <QObject>
#include "application.h"

class SceneManager;
class SceneObject;

class IbisAPI
{

public:
    IbisAPI();
    ~IbisAPI();

    static const int InvalidId;

    void AddObject( SceneObject * object, SceneObject * attachTo = 0 );
    void RemoveObject( SceneObject * object , bool viewChange = true);
    void SetCurrentObject( SceneObject * cur  );
    SceneObject * GetCurrentObject( );
    SceneObject * GetObjectByID( int id );
    SceneObject * GetSceneRoot();


    void ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex );

    bool IsLoadingScene();


private:
    Application  * m_application;
    SceneManager * m_sceneManager;

    friend class Application;
    void SetApplication( Application * app );
};

#endif // IBISAPI_H
