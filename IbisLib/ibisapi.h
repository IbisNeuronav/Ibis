#ifndef IBISAPI_H
#define IBISAPI_H

#include <QObject>
#include <QList>

class Application;
class SceneManager;
class SceneObject;
class USAcquisitionObject;
class UsProbeObject;
class PointerObject;
class CameraObject;
class ImageObject;
class ToolPluginInterface;
class View;

class QProgressDialog;
class QString;

class IbisAPI : public QObject
{

    Q_OBJECT

public:
    IbisAPI();
    ~IbisAPI();

    static const int InvalidId;

    // from SceneManager:
    void AddObject( SceneObject * object, SceneObject * attachTo = 0 );
    void RemoveObject( SceneObject * object , bool viewChange = true);
    void SetCurrentObject( SceneObject * cur  );
    SceneObject * GetCurrentObject( );
    SceneObject * GetObjectByID( int id );
    SceneObject * GetSceneRoot();
    PointerObject *GetNavigationPointerObject( );
    int  GetNumberOfUserObjects();
    void GetAllImageObjects( QList<ImageObject*> & objects );
    const QList< SceneObject* > & GetAllObjects();
    void GetAllUSAcquisitionObjects( QList<USAcquisitionObject*> & all );
    void GetAllUsProbeObjects( QList<UsProbeObject*> & all );
    void GetAllCameraObjects( QList<CameraObject*> & all );

    View * GetMain3DView();
    View * GetMainCoronalView();
    View * GetMainSagittalView();
    View * GetMainTransverseView();

    void ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex );

    bool IsLoadingScene();
    const QString GetSceneDirectory();

    static QString FindUniqueName( QString wantedName, QStringList & otherNames );


    // from Application
    QProgressDialog * StartProgress( int max, const QString & caption = QString() );
    void StopProgress( QProgressDialog * progressDialog);
    void UpdateProgress( QProgressDialog*, int current );

    bool IsViewerOnly();
    QString GetWorkingDirectory();
    QString GetConfigDirectory();
    QString GetOpenFileName( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetSaveFileName( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetExistingDirectory( const QString & caption = QString(), const QString & dir = QString() );
    QString GetGitHashShort();
    ToolPluginInterface * GetToolPluginByName( QString name );

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
