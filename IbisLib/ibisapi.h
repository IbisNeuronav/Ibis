/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef IBISAPI_H
#define IBISAPI_H

#include <QList>
#include <QMap>
#include <QDockWidget>
#include <QObject>

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
class IbisPreferences;

class vtkMatrix4x4;

class QProgressDialog;
class QString;

class OpenFileParams
{

public:

    struct SingleFileParam
    {
        SingleFileParam() : isReference(false), isLabel(false), loadedObject(0), secondaryObject(0), parent(0) {}
        QString fileName;
        QString objectName;
        bool isReference;
        bool isLabel;  // load image as label image instead of floats.
        SceneObject * loadedObject;
        SceneObject * secondaryObject;  // This is a hack to attach the second point object that can be found in PointsObjects
        SceneObject * parent;
    };
    OpenFileParams() : defaultParent(0) {}
    ~OpenFileParams() {}
    void AddInputFile( QString filename, QString objectName = QString() )
    {
        SingleFileParam p;
        p.fileName = filename;
        p.objectName = objectName;
        filesParams.push_back( p );
    }
    void SetAllFileNames( const QStringList & inFiles )
    {
        for( int i = 0; i < inFiles.size(); ++i )
        {
            AddInputFile( inFiles[i] );
        }
    }
    QList<SingleFileParam> filesParams;
    QString lastVisitedDir;
    SceneObject * defaultParent;
};

/**
 * @class   IbisAPI
 * @brief   interface to Application and SceneManager, to be used in ibis plugins
 *
 * IbisAPI provides 2 groups of functions. Functions from the first group,
 * interfacing with SceneManager, are used to manipulate objects in the scene
 * and some scene parameters, cursor color for example.
 * SceneManager also controls views.
 * Ibis has 4 predefined views: 3D, Sagittal, Coronal and Transverse accessible either by type
 * or by id. User created views are not yet implemented.
 *
 * The second group functions, interfacing with Application, are used to manipulate
 * GUI and application preferences.
 * @sa
 * Application SceneManager View SceneObject ImageObject PolydataObject
 */
class IbisAPI : public QObject
{

    Q_OBJECT

public:
    IbisAPI( Application *app);
    ~IbisAPI();

    /**
     * @brief InvalidId is a constant used to define an invalid scene object, -1 is used
     *
     * Every object in scene has its id, we use negative integers < -1 for system objects
     * and 0 or positive number for any other object. WorldObject is an example of system object.
     *
     */
    static const int InvalidId;

    // from SceneManager:
    /**
     * Add an object to the scene.  The object must be derived
     * from SceneObject class.  If the second parameter, parenObject, is not null,
     * the object will be added as a child of parenObject.
     */
    void AddObject( SceneObject * object, SceneObject * parenObject = nullptr );
    /**
     * Remove the object from the scene together with all its children.
     */
    void RemoveObject( SceneObject * object );
    /**
     * Remove all object's children, leaving the object on the scene.
     */
    void RemoveAllChildrenObjects( SceneObject *obj );
    /**
     * Set object as current and show the object's properties
     * in the left side panel.
     */
    void SetCurrentObject( SceneObject * cur  );
    /**
     * Return currently selected object
     */
    SceneObject * GetCurrentObject( );
    /**
     * Return object with a given Id
     */
    SceneObject * GetObjectByID( int id );
    /**
     * Remove object with a given Id
     */
    void RemoveObjectByID( int id );
    /**
     * WorldObject, the parent of all the objects in the scene is the root scene object
     */
    SceneObject * GetSceneRoot();
    /**
     * Return the object used as current pointer to navigate the scene
     */
    PointerObject *GetNavigationPointerObject( );
    /**
     * Return number of objects created by the user
     */
    int  GetNumberOfUserObjects();
    /**
     * Return a list of objects created by the user
     */
    void GetAllUserObjects(QList<SceneObject*> &);
    /**
     * In the scene there is one main object called Reference object. It is used
     * for registration and it defines a bounding box in which other objects are displayed.
     */
    ImageObject * GetReferenceDataObject( );
    /**
     * Return a list of objects of type ImageObject.
     * ImageObject is derived from SceneOBject. It is used to represent volumes.
     */
    void GetAllImageObjects( QList<ImageObject*> & objects );
    /**
     * Return a list of objects of type PolyDataObject.
     * PolyDataObject is derived from SceneOBject. It is mostly used to represent surfaces.
     */
    void GetAllPolyDataObjects( QList<PolyDataObject*> & objects );
    /**
     * Return a list of all objects in the scene.
     */
    const QList< SceneObject* > & GetAllObjects();
    /**
     * Return a list of objects of type USAcquisitionObject.
     * USAcquisitionObject is derived from SceneOBject. It is used to store UltraSound acquisition.
     */
    void GetAllUSAcquisitionObjects( QList<USAcquisitionObject*> & all );
    /**
     * Return a list of objects of type UsProbeObject.
     * UsProbeObject is derived from SceneOBject. It represents an UltraSound probe.
     */
    void GetAllUsProbeObjects( QList<UsProbeObject*> & all );
    /**
     * Return a list of objects of type CameraObject.
     * CameraObject is derived from SceneOBject. It represents a camera.
     */
    void GetAllCameraObjects( QList<CameraObject*> & all );
    /**
     * Return a list of objects of type PointsObject.
     * PointsObject is derived from SceneOBject. It is used to represent multiple points in the scene.
     */
    void GetAllPointsObjects( QList<PointsObject*> & objects );
    /**
     * Return a list of objects of type PointerObject.
     * PointerObject is derived from SceneOBject. It represents a pointer.
     */
    void GetAllPointerObjects( QList<PointerObject*> & all );
    /**
     * Return a list of all objects that are tracked using a tracking system.
     */
    void GetAllTrackedObjects( QList<TrackedSceneObject*> & all );
    /**
     * Return a list of all objects that are listed in the left side panel, except tracked objects.
     */
    void GetAllListableNonTrackedObjects( QList<SceneObject*> & all );
    /**
     * Return a list of all objects of a given type.
     */
    void GetAllObjectsOfType( const char * typeName, QList<SceneObject*> & all );

    /**
     * Get a View using its id.
     */
    View * GetViewByID( int id );
    /**
     * @{
     * Get one of the main application views.
     */
    View * GetMain3DView();
    View * GetMainCoronalView();
    View * GetMainSagittalView();
    View * GetMainTransverseView();
    /** @}*/
    /**
     * Get all views in QMap form.
     */
    QMap<View*, int> GetAllViews( );
    /**
     * Enable/Disable in all views of the main window
     */
    void SetRenderingEnabled( bool enable );
    /**
     * Get the background color of main 2D Views, they all have the same color.
     */
    double * GetViewBackgroundColor();
    /**
     * Get the background color of 3D View, it may be different from 2D.
     */
    double * GetView3DBackgroundColor();
    /**
     * Set the background color of all main views.
     */
    void SetViewBackgroundColor( double * color );
    /**
     * Set the background color of the main 3D view.
     */
    void SetView3DBackgroundColor( double * color );
    /**
     * Assign object to a different parent object.
     */
    void ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex );
    /**
     * Clear all the objects from the scene to prepare for a new scene.
     */
    void NewScene();
    /**
     * Load a saved scene.
     */
    void LoadScene( QString & filename, bool interactive = true );
    /**
     * Check if the scene is currently beeing loaded
     */
    bool IsLoadingScene();
    /**
     * Return path to the scene directory
     */
    const QString GetSceneDirectory();

    static QString FindUniqueName( QString wantedName, QStringList & otherNames );
    /**
     * @{
     * Get/set cursor position.
     */
    void GetCursorPosition( double pos[3] );
    void SetCursorPosition( double *pos );
    void GetCursorWorldPosition( double pos[3] );
    void SetCursorWorldPosition( double * );
    /** @}*/

    // from Application

    /**
     * @{
     * Manipulate progress.
     */
    QProgressDialog * StartProgress( int max, const QString & caption = QString() );
    void StopProgress( QProgressDialog * progressDialog);
    void UpdateProgress( QProgressDialog*, int current );
    /** @}*/

    /**
     * Display warning message
     */
    void Warning( const QString &title, const QString & text );
    /**
     *  Check ibis execution mode,
     *  ibis can be run without tracking, in a viewer mode.
     */
    bool IsViewerOnly();
    /**
     * @{
     * Manage directories and files
     */
    QString GetWorkingDirectory();
    QString GetConfigDirectory();
    QString GetFileNameOpen( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetFileNameSave( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetExistingDirectory( const QString & caption = QString(), const QString & dir = QString() );
    /** @}*/
    /**
     * Get ibis git revision
     */
    QString GetGitHashShort();
    /**
     * @{
     * Get plugins and global objects using their names
     * Basic types of plugins in ibis are: tool, object and global object.
     */
    ToolPluginInterface * GetToolPluginByName( QString name );
    ObjectPluginInterface *GetObjectPluginByName( QString className );
    SceneObject * GetGlobalObjectInstance( const QString & className );
    /** @}*/
    /**
     * @{
     * Handle events in plugins
     * Allow plugins to listen for events (e.g. key press) on any window of ibis
     */
    void AddGlobalEventHandler( GlobalEventHandler * h );
    void RemoveGlobalEventHandler( GlobalEventHandler * h );
    /** @}*/
    /**
     * @{
     * Data loading utilities
     */
    bool OpenTransformFile( const QString & filename, vtkMatrix4x4 * mat );
    bool OpenTransformFile( const QString & filename, SceneObject * obj = 0 );
    void OpenFiles( OpenFileParams * params, bool addToScene = true );
    /** @}*/
    /**
     * @{
     * Control layout of Main Window
     */
    void SetMainWindowFullscreen( bool f );
    void SetToolbarVisibility( bool v );
    void SetLeftPanelVisibility( bool v );
    void SetRightPanelVisibility( bool v );
    void AddBottomWidget( QWidget * w );
    void RemoveBottomWidget( QWidget * w );
    void ShowFloatingDock( QWidget * w, QFlags<QDockWidget::DockWidgetFeature> features=QDockWidget::AllDockWidgetFeatures );
    /** @}*/
    /**
     * @{
     * Manage ibis preferences, e.g. path to MINC tools
     */
    IbisPreferences *GetIbisPreferences();
    void RegisterCustomPath( const QString & pathName, const QString & directoryPath );
    void UnRegisterCustomPath( const QString & pathName );
    const QString GetCustomPath( const QString & pathName );
    bool IsCustomPathRegistered( const QString &pathName );
    /** @}*/

public slots:
    void ObjectAddedSlot( int id );
    void ObjectRemovedSlot( int id );
    void ReferenceTransformChangedSlot();
    void ReferenceObjectChangedSlot();
    void CursorPositionChangedSlot();
    void NavigationPointerChangedSlot();
    void IbisClockTickSlot();

signals:

    void ObjectAdded( int );
    void ObjectRemoved( int );
    void ReferenceTransformChanged();
    void ReferenceObjectChanged();
    void CursorPositionChanged();
    void NavigationPointerChanged();
    void IbisClockTick();

private:
    Application  * m_application;
    SceneManager * m_sceneManager;

    /**
     * Assure access to the core functionality of ibis
     */
    void SetApplication( Application * app );
};

#endif // IBISAPI_H
