/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_SCENEMANAGER_H
#define TAG_SCENEMANAGER_H

#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include "ibistypes.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QProgressDialog>
#include <QObject>

#include <vector>
#include <algorithm>

#include "sceneobject.h"
#include "serializer.h"
#include "view.h"
#include <QColor>

class QWidget;
class QStringList;
class TripleCutPlaneObject;
class TrackedSceneObject;
class WorldObject;
class ImageObject;
class PolyDataObject;
class CameraObject;
class vtkRenderer;
class PointsObject;
class USAcquisitionObject;
class UsProbeObject;
class PointerObject;
class vtkInteractor;

/** Scene file format version */
#define IBIS_SCENE_SAVE_VERSION "6.0"

/**
 * @class   SceneManager
 * @brief   Management of the display
 *
 * This class is the main interface for all 3D/2D display. It manages the hierarchy
 * of 3D objects that compose the scene that can be displayed. It also manages
 * the list of different views of the 3D scene that can be displayed in a window
 * Furthermore, it is responsible for creating different Qt windows and/or window layouts.
 * */

enum STANDARDVIEW {SV_NONE, SV_FRONT, SV_BACK, SV_LEFT, SV_RIGHT, SV_TOP, SV_BOTTOM };

class SceneManager : public QObject, public vtkObject
{

    Q_OBJECT

public:

    typedef QList< SceneObject* > ObjectList;
    static const int InvalidId;

    static SceneManager * New() { return new SceneManager; }

    vtkTypeMacro(SceneManager,vtkObject);

    SceneManager();
    virtual ~SceneManager();

    /** Call instead of Delete(),  SceneManager has a lot of cleanup to do before it can be deleted */
    void Destroy();

    /** Enable reaction to changes in the hardware module */
    void OnStartMainLoop();

    /** Save/Read scene data */
    virtual void Serialize( Serializer * ser );

    /** Update information and some variables after scene was loaded.
     *  @param n progress stage
    */
    void PostSceneRead( int n );

    /** @name Basic layout
     *  @brief  Create different view windows and/or window layouts
     */
    ///@{
    QWidget * CreateQuadViewWindow( QWidget * parent );
    QWidget * CreateObjectTreeWidget( QWidget * parent );
    QWidget * CreateTrackedToolsStatusWidget( QWidget * parent );
    ///@}

    /** @name Getting views
     *  @brief  Create and access different views
     *
     * There are 4 main views: Saggital, Coronal, 3D and Transverse.
     * View types and IDs are defined in ibistypes.h.
     *
     * The next functions are used to get a pointer to one of the views of the scene.
     * GetViewByID(int) will return a view with the id passed in parameter
     * or null pointer if no such view exists.
     */
    ///@{
    QMap<View*, int> GetAllViews( ) {return this->Views;}
    int GetNumberOfViews() { return this->Views.size(); }
    vtkGetMacro(Main3DViewID,int);
    vtkGetMacro(MainCoronalViewID,int);
    vtkGetMacro(MainSagittalViewID,int);
    vtkGetMacro(MainTransverseViewID,int);
    View * GetViewByID( int id );
    View * CreateView(int type, QString name = QString::null, int id = InvalidId );
    View * GetMain3DView();
    View * GetMainCoronalView();
    View * GetMainSagittalView();
    View * GetMainTransverseView();
    View * GetViewFromInteractor( vtkRenderWindowInteractor * interactor );
    ///@}
    /** @name View attributes
     *  @brief  Get and se view attributes
     */
    ///@{
    /** Decide if main 3D view is following the reference object. */
    void Set3DViewFollowingReferenceVolume( bool follow ) { m_viewFollowsReferenceObject = follow; }
    /** Find out if main 3D view is following the reference object. */
    bool Is3DViewFollowingReferenceVolume() { return m_viewFollowsReferenceObject; }
    /** SetViewBackgroundColor will set the backgroud color of all views, 3D included. */
    void SetViewBackgroundColor( double * color );
    /** SetView3DBackgroundColor will only set the backgroud color of main 3D view. */
    void SetView3DBackgroundColor( double * color );
    /** UpdateBackgroundColor is called after updating application settings. */
    void UpdateBackgroundColor();
    /** Get the color of all views, 3D may have different color. */
    vtkGetVector3Macro( ViewBackgroundColor, double );
    /** Get the color of 3D view. */
    vtkGetVector3Macro( View3DBackgroundColor, double );
    /** Get renderer of the view by view ID. */
    vtkRenderer *GetViewRenderer(int viewID);
    /** Eneble/disable rendering in all views. */
    void SetRenderingEnabled( bool r );
    /** Get camera view angle in main 3D view. */
    double Get3DCameraViewAngle();
    /** Set camera view angle in main 3D view. */
    void Set3DCameraViewAngle( double angle );
    ///@}

    /** @name  World and reference object space
     *  @brief Transform between reference object space and world space coordinate systems
     * */
    ///@{
    void WorldToReference( double worldPoint[3], double referencePoint[3] );
    void ReferenceToWorld( double referencePoint[3], double worldPoint[3] );
    void GetReferenceOrientation( vtkMatrix4x4 * mat );
    ///@}

    /** Manipulate the global cursor. The cursor is a general concept that
     * can be used by any module as a 3D reference point. Amongst other things,
     * its position is used to place the Triple Cut planes. */
    void GetCursorPosition( double pos[3] );


    /** Determine whether the pos is in one of the 3 planes identified by planeType. Point is
     * in the plane if it is closer than .5 * voxel size of the reference volume. */
    bool IsInPlane( VIEWTYPES planeType, double pos[3] );

    /** @name  Generic Label
     *  @brief The label is used to display some additional information on the toolbar
     *
     *  Mostly it is used to display the distance from the pointer tip to some selected point.
     * */
    ///@{
    void EmitShowGenericLabel( bool );
    void EmitShowGenericLabelText();
    void SetGenericLabelText( const QString &text ) { GenericText = text; }
    const QString GetGenericLabelText( ) { return GenericText; }
    ///@}

    /** Load a previously acquired US frames sequence into the scene. */
    bool ImportUsAcquisition();

public slots:

    /** Causes the cutting planes to move to cross in the center, cursor is at the crossing point of the planes. */
    void ResetCursorPosition();
    /** Causes the cutting planes to move to the point given the local coordinates. */
    void SetCursorPosition( double * );
    /** Causes the cutting planes to move to the point given the world coordinates. */
    void SetCursorWorldPosition( double * );
    /** ClockTick is used to move the cutting planes to the pointer position when ibis is in navigation mode. */
    void ClockTick();

private slots:

    void OnStartCutPlaneInteraction();
    void OnCutPlanesPositionChanged();
    void OnEndCutPlaneInteraction();

signals:

    void CursorPositionChanged();
    void StartCursorInteraction();
    void EndCursorInteraction();
    void ShowGenericLabel( bool );
    void ShowGenericLabelText( );

public:

    /** @name  Manage objects
     *  @brief Add, remove, get SceneObject.
     * */
    ///@{
    void AddObject( SceneObject * object, SceneObject * attachTo = 0 );
    void RemoveObjectById( int objectId );
    void RemoveObject(SceneObject * object );
    void ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex );
    int  GetNumberOfImageObjects();
    void GetAllImageObjects( QList<ImageObject*> & objects );
    void GetAllPolydataObjects( QList<PolyDataObject*> & objects );
    void GetAllPointsObjects( QList<PointsObject*> & objects );
    void GetAllCameraObjects( QList<CameraObject*> & all );
    void GetAllUSAcquisitionObjects( QList<USAcquisitionObject*> & all );
    void GetAllUsProbeObjects( QList<UsProbeObject*> & all );
    void GetAllPointerObjects( QList<PointerObject*> & all );
    void GetAllTrackedObjects( QList<TrackedSceneObject*> & all );
    void GetAllObjectsOfType( const char * typeName, QList<SceneObject*> & all );
    SceneObject * GetObjectByID( int id );
    SceneObject * GetCurrentObject( ) { return m_currentObject; }
    void SetCurrentObject( SceneObject * cur  );
    int  GetNumberOfUserObjects();
    void GetAllUserObjects(QList<SceneObject*> &);
    void GetAllListableNonTrackedObjects(QList<SceneObject*> &);
    void GetChildrenListableNonTrackedObjects( SceneObject * obj, QList<SceneObject*> & );
    ///@}

    /** @name  Reference object
     *  @brief ImageObject used as a reference for all other objects
     * */
    ///@{
    ImageObject * GetReferenceDataObject( );
    void SetReferenceDataObject( SceneObject * );
    bool CanBeReference( SceneObject * );
    void GetReferenceBounds( double bounds[6] );
    vtkTransform * GetReferenceTransform() { return m_referenceTransform; }
    vtkTransform * GetInverseReferenceTransform() { return m_invReferenceTransform; }
    ///@}

    /** @name  Special root objects
     *  @brief Scene Root and Axes
     * */
    ///@{
    SceneObject * GetSceneRoot();
    void SetAxesObject( vtkSmartPointer<PolyDataObject> obj );
    vtkSmartPointer<PolyDataObject> GetAxesObject();
    ///@}


    /** @name  Manage cursor
     *  @brief Set visibility and color - wrapper for TrippleCutPlane
     * */
    ///@{
    bool GetCursorVisible();
    void SetCursorVisibility( bool v );
    void SetCursorColor( const QColor & c );
    QColor GetCursorColor();
    ///@}

    /** @name Manage planes
     *  @brief set visibility - wrapper for TrippleCutPlane
     * */
    ///@{
    /** Show/Hide plane, index 0 = Sagittal, 1 = Coronal, 2 = Transverse */
    void ViewPlane( int index, bool show );
    /** Check visibility */
    bool IsPlaneVisible( int index );
    /** Show/Hide all cutting planes. */
    void ViewAllPlanes( bool show );
    ///@}

    //
    /** @name  Data Interpolation
     *  @brief Manage data interpolation for reslice and display - wrapper for TrippleCutPlane
     */
    ///@{
    /** Set/Get interpolation type. */
    void SetResliceInterpolationType( int type );
    int  GetResliceInterpolationType();
    void SetDisplayInterpolationType( int type );
    int  GetDisplayInterpolationType();
    ///@}


    /** The function is called just before displaying the object.
     * In particular, objects use this call to enable 3D widgets. */
    void PreDisplaySetup();

    /** @name  Camera
     *  @brief Utility functions to reset the cameras in all views. */
    ///@{
    void ResetAllCameras();
    void ResetAllCameras( double bounds[6] );
    void ZoomAllCameras(double factor);
    ///@}


    /** @name Objects and Views
     *  @brief Setup objects in Views
     * */
    ///@{
    /** Setup/Release all objects in the view passed as a parameter. This is typically used to
     * initialize new Views. */
    void SetupAllObjects( View * v );
    void ReleaseAllObjects( View * v );

    /** Utility function to setup an object in all views. */
    void SetupInAllViews( SceneObject * object );

    /** Utility function to detach all views from the render window. */
    void ReleaseAllViews();

    /** Set one of the standard views (front, back, left, right, top, bottom) in 3D window. */
    void SetStandardView(STANDARDVIEW type);
    ///@}

    /** Get saved scene version found in scene file in order to control loaded variables. */
    const QString GetSupportedSceneSaveVersion() { return SupportedSceneSaveVersion; }

    /** Set working directory */
    void SetSceneDirectory( const QString &directory ) { SceneDirectory = directory; }
    /** Get working directory */
    const QString GetSceneDirectory() { return SceneDirectory; }
    /** Save the name of the scene xml file including full path */
    void SetSceneFile( const QString &fileName ) { SceneFile = fileName; }
    /** Get the name of the scene xml file including full path */
    const QString GetSceneFile() { return SceneFile; }

    /** @name  Global objects operations
     *  @brief Add, remove, clean
     * */
    ///@{
    /** Remove all children added to system objects: world and all tools. */
    void RemoveAllSceneObjects();
    void RemoveAllChildrenObjects(SceneObject *);
    /** Remove all objects including system objects: world and all tools. */
    void ClearScene();
    /** Get a list of all the objects in the scene. */
    const ObjectList & GetAllObjects() { return AllObjects; }
    ///@}

    /** @name  Scene operations
     *  @brief Load, Save, Read/Write scene files
     * */
    ///@{
    void LoadScene( QString & fileName, bool interactive = true );
    void SaveScene( QString & fileName );
    void NewScene();
    void ObjectReader( Serializer * ser, bool interactive );
    void ObjectWriter( Serializer * ser );
    ///@}

    /** @name  Interactor style
     *  @brief Manage interactor style in 3D views
     * */
    ///@{
    /** Get/Set interaction style of 3D views. */
    void Set3DInteractorStyle( InteractorStyle style );
    InteractorStyle Get3DInteractorStyle() { return InteractorStyle3D; }
    ///@}

    /** Create a unique name for your object. */
    static QString FindUniqueName( QString wantedName, QStringList & otherNames );

    // navigation
    /** @name  Navigations
     *  @brief Start, end navigation, access the pointer object.
     * */
    ///@{
    /** Enable/disable navigation. */
    void EnablePointerNavigation( bool on);
    /** Choose navigation pointer by ID. */
    void SetNavigationPointerID( int id );
    /** Find navigation pointer ID. */
    int GetNavigationPointerObjectId() { return NavigationPointerID; }
    /** Check if ibis is in navigation mode. */
    bool GetNavigationState() {return this->IsNavigating; }
    /** Get navigation pointer object. */
    PointerObject *GetNavigationPointerObject( );
    ///@}

    /** Check if ibis is currently loading a scene. */
    bool IsLoadingScene() { return LoadingScene; }

public slots:

    /** Inform the application that an object was renamed. */
    void EmitSignalObjectRenamed(QString, QString);

private slots:

    void ReferenceTransformChangedSlot();
    void CancelProgress();
    void EmitSignalObjectAttributesChanged( SceneObject* obj );

signals:

    void StartAddingObject( SceneObject*, int );
    void FinishAddingObject();
    void StartRemovingObject( SceneObject*, int );
    void FinishRemovingObject();

    void ObjectAdded( int );
    void ObjectRemoved( int );
    void ObjectNameChanged(QString, QString);
    void NavigationPointerChanged();
    void CurrentObjectChanged();
    void ReferenceTransformChanged();
    void ReferenceObjectChanged();
    void ObjectAttributesChanged(  SceneObject* );

protected:

    void ValidatePointerObject();

    void InternalClearScene();
    void Init();
    void Clear();
    void NotifyPluginsSceneAboutToLoad();
    void NotifyPluginsSceneFinishedLoading();
    void NotifyPluginsSceneAboutToSave();
    void NotifyPluginsSceneFinishedSaving();

    /** Next object id is set when adding an object. */
    int m_nextObjectID;
    /** Next system object id is set when adding a system controlled object. */
    int m_nextSystemObjectID;
    /** Currently selected object. */
    SceneObject * m_currentObject;
    /** Reference object - all other objects are relating to it. */
    ImageObject * m_referenceDataObject;
    /** World transform of the reference object. */
    vtkTransform * m_referenceTransform;
    /** Inversed transform of the world transform of reference object. */
    vtkTransform * m_invReferenceTransform;
    /** The parent object of all the objects in the scene, by defaulrt it is World. */
    WorldObject * m_sceneRoot;
    /** Interactor style used in 3D view. */
    InteractorStyle InteractorStyle3D;

    /** Allow setting object id when adding object.
     *  @param object object added
     *  @param attachTo parent object, 0 means add to scene root
     *  @param objID proposed id
    */
    void AddObjectUsingID( SceneObject * object, SceneObject * attachTo = 0, int objID = SceneManager::InvalidId);

    /** Recursive function used to setup all objects bellow obj in view v. */
    void SetupOneObject( View * v, SceneObject * obj );


    /** Creates a new interactor style object of the right type and assign it to the view passed as param. */
    void AssignInteractorStyleToView( InteractorStyle style, View * v );

    /** @name  Views
     *  @brief Variables used to describe views.
     * */
    ///@{
    typedef QMap<View*, int> ViewMap;
    ViewMap Views;
    int Main3DViewID;
    int MainCoronalViewID;
    int MainSagittalViewID;
    int MainTransverseViewID;
    bool m_viewFollowsReferenceObject;
    double ViewBackgroundColor[3];
    double View3DBackgroundColor[3];
    double CameraViewAngle3D;
    ///@}

    /** List of all the objects in the scene. */
    ObjectList AllObjects;

    /** Version number saved in the scene xml file, used to verify if the scene is still supported,
     * some very old scenes cannot be loaded. */
    QString SupportedSceneSaveVersion;

    /** Scene loading/saving progress. */
    QProgressDialog *m_sceneLoadSaveProgressDialog;
    /** Update  scene loading/saving progress. */
    bool UpdateProgress(int value);

    /** Navigation pointer id. */
    int NavigationPointerID;
    /** Navigation mode flag. */
    bool IsNavigating;

    /** Loading scene mode flag. */
    bool LoadingScene;

    /** Text to display as a generic label on the tool bar. */
    QString GenericText;

private:

    /** A set of 3 cutting planes used to show sagittal, coronal and transversal cross section of objects. */
    vtkSmartPointer<TripleCutPlaneObject> MainCutPlanes;

    friend class QuadViewWindow;

    /** @name  Views ID
     *  @brief Set specific view id.
     * */
    ///@{
    vtkSetMacro(Main3DViewID,int);
    vtkSetMacro(MainCoronalViewID,int);
    vtkSetMacro(MainSagittalViewID,int);
    vtkSetMacro(MainTransverseViewID,int);
    ///@}

    /** Directory containing files used in the current scene. */
    QString SceneDirectory;
    /** Name of the xml file describing the current scene. */
    QString SceneFile;

    ///@{
    /** We declare these to make sure no one is registering or unregistering SceneManager
     * since SceneManager should have only one instance and be deleted at the end
     * by the unique instance of Application. */
    virtual void Register(vtkObjectBase* o) override;
    virtual void UnRegister(vtkObjectBase* o) override;
    ///@}
};

ObjectSerializationHeaderMacro( SceneManager );

#endif //TAG_SCENEMANAGER_H
