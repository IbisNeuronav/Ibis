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
 * @brief   Management of display
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

    /** call instead of Delete(),  SceneManager has a lot of cleanup to do before it can be deleted */
    void Destroy();

    void OnStartMainLoop();

    virtual void Serialize( Serializer * ser );
    void PostSceneRead( int n );


    /** @name Basic layout
     * @brief  Create different view windows and/or window layouts
     */
    ///@{
    QWidget * CreateQuadViewWindow( QWidget * parent );
    QWidget * CreateObjectTreeWidget( QWidget * parent );
    QWidget * CreateTrackedToolsStatusWidget( QWidget * parent );
    ///@}

    /** @name Getting views
     * @brief  Create and access different views
     *
     * There are 4 main views: Saggital, Coronal, 3D and Transverese.
     * View types and IDs are defined in ibistypes.h.
     *
     * The next functions are used to get a pointer to one of the views of the scene.
     * GetViewByID(int) will return a view with the id passed in parameter
     * or 0 if no such view exists.
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
     * @brief  Get and se view attributes
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

    /** @name World and reference object space
     * @brief  transform between reference object space and world space coordinate systems
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

    void EmitShowGenericLabel( bool );
    void EmitShowGenericLabelText();
    void SetGenericLabelText( const QString &text ) { GenericText = text; }
    const QString GetGenericLabelText( ) { return GenericText; }

    bool ImportUsAcquisition();

public slots:

    void ResetCursorPosition();
    void SetCursorPosition( double * );
    void SetCursorWorldPosition( double * );
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

    // Manage Objects
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

    // Manage cursor visibility and color - wrapper for TrippleCutPlane
    bool GetCursorVisible();
    void SetCursorVisibility( bool v );
    void SetCursorColor( const QColor & c );
    QColor GetCursorColor();
    // Manage planes visibility - wrapper for TrippleCutPlane
    void ViewPlane( int index, bool show );
    bool IsPlaneVisible( int index );
    void ViewAllPlanes( bool show );
    // Manage data interpolation for reslice and display - wrapper for TrippleCutPlane
    void SetResliceInterpolationType( int type );
    int GetResliceInterpolationType();
    void SetDisplayInterpolationType( int type );
    int GetDisplayInterpolationType();

    // Description:
    // Get object by ID
    SceneObject * GetObjectByID( int id );

    // Current object
    SceneObject * GetCurrentObject( ) { return m_currentObject; }
    void SetCurrentObject( SceneObject * cur  );

    // Reference object
    ImageObject * GetReferenceDataObject( );
    void SetReferenceDataObject( SceneObject * );
    bool CanBeReference( SceneObject * );
    void GetReferenceBounds( double bounds[6] );
    vtkTransform * GetReferenceTransform() { return m_referenceTransform; }
    vtkTransform * GetInverseReferenceTransform() { return m_invReferenceTransform; }

    // Special root objects
    SceneObject * GetSceneRoot();
    void SetAxesObject( vtkSmartPointer<PolyDataObject> obj );
    vtkSmartPointer<PolyDataObject> GetAxesObject();

    // Description:
    // Function that is called by a window to using this manager. The
    // function is called just before displaying to turn every object on.
    // In particular, objects use this call to enable 3D widgets.
    void PreDisplaySetup();

    // Description:
    // Utility function to reset the cameras in all views.
    void ResetAllCameras();
    void ResetAllCameras( double bounds[6] );
    void ZoomAllCameras(double factor);

    // Description:
    // Setup all objects in the view passed as a parameter. This is typically used to
    // initialize new Views.
    void SetupAllObjects( View * v );
    void ReleaseAllObjects( View * v );

    // Description:
    // Utility function to setup an object in all views
    void SetupInAllViews( SceneObject * object );

    // Description:
    // Utility function to detach all views from the render window.
    void ReleaseAllViews();

    //Description
    // set one of the standard views (front, back, left, right, top, bottom) in 3D window
    void SetStandardView(STANDARDVIEW type);

    //Description
    // Set SceneSave version found in scene file in order to control loaded variables
    const QString GetSupportedSceneSaveVersion() { return SupportedSceneSaveVersion; }

    //Description
    // Set working directory
    void SetSceneDirectory( const QString &directory ) { SceneDirectory = directory; }
    const QString GetSceneDirectory() { return SceneDirectory; }
    void SetSceneFile( const QString &fileName ) { SceneFile = fileName; }
    const QString GetSceneFile() { return SceneFile; }

    //Description
    //Remove all children added to system objects: world, preop and intraop, also all tools
    void RemoveAllSceneObjects();
    void RemoveAllChildrenObjects(SceneObject *);
    void ClearScene();
    void LoadScene( QString & fileName, bool interactive = true );
    void SaveScene( QString & fileName );
    void NewScene();
    void ObjectReader( Serializer * ser, bool interactive );
    void ObjectWriter( Serializer * ser );

    // Description
    const ObjectList & GetAllObjects() { return AllObjects; }

    // Description
    // returns list of all the user created/added objects with their parents
    int  GetNumberOfUserObjects();
    void GetAllUserObjects(QList<SceneObject*> &);

    // Description
    // returns list of all the listable, non-tracked objects with their parents
    void GetAllListableNonTrackedObjects(QList<SceneObject*> &);
    void GetChildrenListableNonTrackedObjects( SceneObject * obj, QList<SceneObject*> & );

    // Description
    // Manages interaction style of 3D views
    void Set3DInteractorStyle( InteractorStyle style );
    InteractorStyle Get3DInteractorStyle() { return InteractorStyle3D; }

    // Description
    // Utility functions
    static QString FindUniqueName( QString wantedName, QStringList & otherNames );

    // navigation
    void EnablePointerNavigation( bool on);
    void SetNavigationPointerID( int id );
    int GetNavigationPointerObjectId() { return NavigationPointerID; }
    bool GetNavigationState() {return this->IsNavigating; }
    PointerObject *GetNavigationPointerObject( );
    bool IsLoadingScene() { return LoadingScene; }
public slots:

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

    int m_nextObjectID;
    int m_nextSystemObjectID;
    SceneObject * m_currentObject;
    ImageObject * m_referenceDataObject;
    vtkTransform * m_referenceTransform;
    vtkTransform * m_invReferenceTransform;
    WorldObject * m_sceneRoot;
    InteractorStyle InteractorStyle3D;

    // Description:
    // Allow settin object id when adding object
    void AddObjectUsingID( SceneObject * object, SceneObject * attachTo = 0, int objID = SceneManager::InvalidId);

    // Description:
    // Recursive function used to setup all objects bellow obj
    // in view v.
    void SetupOneObject( View * v, SceneObject * obj );

    // Description:
    // Creates a new interactor style object of the right type and assign it to the view passed as param.
    void AssignInteractorStyleToView( InteractorStyle style, View * v );

    // Views
    bool m_viewFollowsReferenceObject;
    typedef QMap<View*, int> ViewMap;
    ViewMap Views;

    // Objects
    ObjectList AllObjects;

    double ViewBackgroundColor[3];
    double View3DBackgroundColor[3];
    double CameraViewAngle3D;

    // scene basic settings
    QString SupportedSceneSaveVersion;

    // scene loading/saving progress
    QProgressDialog *m_sceneLoadSaveProgressDialog;
    bool UpdateProgress(int value);

    // navigation
    int NavigationPointerID;
    bool IsNavigating;

    bool LoadingScene;

    QString GenericText;

    int Main3DViewID;
    int MainCoronalViewID;
    int MainSagittalViewID;
    int MainTransverseViewID;
private:

    vtkSmartPointer<TripleCutPlaneObject> MainCutPlanes;

    friend class QuadViewWindow;
    vtkSetMacro(Main3DViewID,int);
    vtkSetMacro(MainCoronalViewID,int);
    vtkSetMacro(MainSagittalViewID,int);
    vtkSetMacro(MainTransverseViewID,int);
    QString SceneDirectory;
    QString SceneFile;
    // We declare these to make sure no one is registering or unregistering SceneManager
    // since SceneManager should have only one instance and be deleted at the end
    // by the unique instance of Application.
    virtual void Register(vtkObjectBase* o) override;
    virtual void UnRegister(vtkObjectBase* o) override;
};

ObjectSerializationHeaderMacro( SceneManager );

#endif //TAG_SCENEMANAGER_H
