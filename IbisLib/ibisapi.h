#ifndef IBISAPI_H
#define IBISAPI_H

#include <QObject>
#include <QList>
#include <QMap>

class Application;
class SceneManager;
class SceneObject;
class USAcquisitionObject;
class UsProbeObject;
class PointerObject;
class CameraObject;
class ImageObject;
class PointsObject;
class TrackedSceneObject;
class PolyDataObject;
class ToolPluginInterface;
class ObjectPluginInterface;
class View;
class USAcquisitionObject;
class GlobalEventHandler;
class vtkMatrix4x4;
#include "pointsobject.h"
class PointsObject;

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
    void RemoveObjectByID( int id );
    SceneObject * GetSceneRoot();
    PointerObject *GetNavigationPointerObject( );
    int  GetNumberOfUserObjects();
    void GetAllUserObjects(QList<SceneObject*> &);
    ImageObject * GetReferenceDataObject( );
    void GetAllImageObjects( QList<ImageObject*> & objects );
    void GetAllPolyDataObjects( QList<PolyDataObject*> & objects );
    const QList< SceneObject* > & GetAllObjects();
    void GetAllUSAcquisitionObjects( QList<USAcquisitionObject*> & all );
    void GetAllUsProbeObjects( QList<UsProbeObject*> & all );
    void GetAllCameraObjects( QList<CameraObject*> & all );

    void GetAllPointsObjects( QList<PointsObject*> & objects );
    void GetAllPointerObjects( QList<PointerObject*> & all );
    void GetAllTrackedObjects( QList<TrackedSceneObject*> & all );
    void GetAllListableNonTrackedObjects( QList<SceneObject*> & all );
    void GetAllObjectsOfType( const char * typeName, QList<SceneObject*> & all );

    View * GetViewByID( int id );
    View * GetMain3DView();
    View * GetMainCoronalView();
    View * GetMainSagittalView();
    View * GetMainTransverseView();
    QMap<View*, int> GetAllViews( );
    double * GetViewBackgroundColor();

    void ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex );

    void NewScene();
    void LoadScene( QString & filename, bool interactive = true );
    bool IsLoadingScene();
    const QString GetSceneDirectory();

    static QString FindUniqueName( QString wantedName, QStringList & otherNames );

    void GetCursorPosition( double pos[3] );
    void SetCursorPosition( double *pos );

    // from Application
    QProgressDialog * StartProgress( int max, const QString & caption = QString() );
    void StopProgress( QProgressDialog * progressDialog);
    void UpdateProgress( QProgressDialog*, int current );

    void Warning( const QString &title, const QString & text );

    bool IsViewerOnly();
    QString GetWorkingDirectory();
    QString GetConfigDirectory();
    QString GetFileNameOpen( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetFileNameSave( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetExistingDirectory( const QString & caption = QString(), const QString & dir = QString() );
    QString GetGitHashShort();
    ToolPluginInterface * GetToolPluginByName( QString name );
    ObjectPluginInterface *GetObjectPluginByName( QString className );
    SceneObject * GetGlobalObjectInstance( const QString & className );

    // Allow plugins to listen for events (e.g. key press) on any window of Ibis
    void AddGlobalEventHandler( GlobalEventHandler * h );
    void RemoveGlobalEventHandler( GlobalEventHandler * h );

    // Data loading utilities
    bool OpenTransformFile( QString filename, vtkMatrix4x4 * mat );

    // Control layout of Main Window
    void SetMainWindowFullscreen( bool f );
    void SetToolbarVisibility( bool v );
    void SetLeftPanelVisibility( bool v );
    void SetRightPanelVisibility( bool v );
    void AddBottomWidget( QWidget * w );
    void RemoveBottomWidget( QWidget * w );

    // Rotate cut planes
    void RotateSagittalCutPlane(double angle[3], double point[3]);
    void EnablePlaneRotation(bool);

public slots:
    void ObjectAddedSlot( int id );
    void ObjectRemovedSlot( int id );
    void ReferenceTransformChangedSlot();
    void CursorPositionChangedSlot();
    void IbisClockTickSlot();

signals:

    void ObjectAdded( int );
    void ObjectRemoved( int );
    void ReferenceTransformChanged();
    void CursorPositionChanged();
    void IbisClockTick();

private:
    Application  * m_application;
    SceneManager * m_sceneManager;

    friend class Application;
    void SetApplication( Application * app );
};

#endif // IBISAPI_H
