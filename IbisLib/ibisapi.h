#ifndef IBISAPI_H
#define IBISAPI_H

#include <QObject>
#include <QList>
#include "application.h"

class SceneManager;
class SceneObject;
class QString;
class PointerObject;

class IbisAPI : public QObject
{

    Q_OBJECT

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
    PointerObject *GetNavigationPointerObject( );
    void GetAllImageObjects( QList<ImageObject*> & objects );


    void ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex );

    bool IsLoadingScene();
    const QString GetSceneDirectory();

    static QString FindUniqueName( QString wantedName, QStringList & otherNames );

public slots:
    void ObjectAddedSlot( int id );
    void ObjectRemovedSlot( int id );

signals:

    void ObjectAdded( int );
    void ObjectRemoved( int );


private:
    Application  * m_application;
    SceneManager * m_sceneManager;

    friend class Application;
    void SetApplication( Application * app );
};

#endif // IBISAPI_H
