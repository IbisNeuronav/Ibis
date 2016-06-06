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

#include "vtkObject.h"
#include "vtkPiecewiseFunctionLookupTable.h"
#include "ibistypes.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QProgressDialog>

#include <vector>
#include <algorithm>

#include "sceneobject.h"
#include "serializer.h"

class View;
class QWidget;
class QStringList;
class TripleCutPlaneObject;
class VolumeRenderingObject;
class WorldObject;
class ImageObject;
class PolyDataObject;
class CameraObject;
class vtkRenderer;
class PointsObject;
class USAcquisitionObject;
class UsProbeObject;
class PointerObject;
class Octants;
class SceneInfo;
class vtkInteractor;

//#define USE_NEW_IMAGE_PLANES

// Scene file format version
#define IGNS_SCENE_SAVE_VERSION "6.0"

#define PREOP_ROOT_OBJECT_NAME "Pre-operative"
#define INTRAOP_ROOT_OBJECT_NAME "Intra-operative"
// Description:
// This class is the main interface for all 3D display. It manages the hierarchy
// of 3D objects that compose the scene that can be displayed. It also manages
// the list of different views of the 3D scene that can be displayed in a window
// Furthermore, it is responsible for creating different Qt windows and/or window layouts.

enum STANDARDVIEW {SV_NONE, SV_FRONT, SV_BACK, SV_LEFT, SV_RIGHT, SV_TOP, SV_BOTTOM };

class SceneManager : public QObject, public vtkObject
{

    Q_OBJECT

public:

    typedef QList< SceneObject* > ObjectList;

    static SceneManager * New() { return new SceneManager; }

    SceneManager();
    ~SceneManager();

    void Destroy(); // call instead of Delete(),  SceneManager has a lot of cleanup to do before it can be deleted

    void OnStartMainLoop();

    virtual void Serialize( Serializer * ser );
    void PostSceneRead( int n );

    // Description:
    // Create different view windows and/or window layouts
    QWidget * CreateQuadViewWindow( QWidget * parent );
    QWidget * CreateObjectTreeWidget( QWidget * parent );

    // Description:
    // The three next functions are used to get a pointer to
    // one of the views of the scene. GetView(const char*) will
    // return a view by the name passed in parameter or 0 if
    // no such view exists. GetView(ViewType) will return the
    // first view it finds that is of 'type' and 0 if there
    // is no view of this type. GetOrCreateView() will return
    // a view of type 'type' if there is one, otherwise, it
    // will create one.
    View * CreateView( int type, QString name = QString::null );
    View * GetView( int type );
    View * GetMain3DView();
    View * GetView( const QString & name );
    View * GetViewFromInteractor( vtkRenderWindowInteractor * interactor );
    vtkSetMacro( CurrentView, int );
    vtkGetMacro( CurrentView, int );
    vtkSetMacro( ExpandedView, int );
    vtkGetMacro( ExpandedView, int );
    void Set3DViewFollowingReferenceVolume( bool follow ) { m_viewFollowsReferenceObject = follow; }
    bool Is3DViewFollowingReferenceVolume() { return m_viewFollowsReferenceObject; }
    void SetViewBackgroundColor( double * color );
    void UpdateBackgroundColor();
    vtkGetVector3Macro( ViewBackgroundColor, double );
    vtkRenderer *GetViewRenderer(int viewType);
    void SetRenderingEnabled( bool r );
    double Get3DCameraViewAngle();
    void Set3DCameraViewAngle( double angle );

    // Description:
    TripleCutPlaneObject * GetMainImagePlanes() { return this->MainCutPlanes; }

    // Description:
    // Utility functions to transform between reference object space and
    // world space coordinate systems.
    void WorldToReference( double worldPoint[3], double referencePoint[3] );
    void ReferenceToWorld( double referencePoint[3], double worldPoint[3] );

    // Description:
    // Manipulate the global cursor. The cursor is a general concept that
    // can be used by any module as a 3D reference point. Amongst other things,
    // its position is used to place the Triple Cut planes.
    void GetCursorPosition( double pos[3] );

    void EmitShowGenericLabel( bool );
    void EmitShowGenericLabelText();
    void SetGenericLabelText( const QString &text ) { GenericText = text; }
    const QString GetGenericLabelText( ) { return GenericText; }


public slots:
    void ResetCursorPosition();
    void SetCursorPosition( double * );
    void SetCursorWorldPosition( double * );
    void ClockTick();

private slots:
    void OnCutPlanesPositionChanged();
signals:
    void CursorPositionChanged();
    void ShowGenericLabel( bool );
    void ShowGenericLabelText( );

public:

    // Description:
    // Main volume renderer (there is only one object managing all volume redering)
    vtkGetObjectMacro( MainVolumeRenderer, VolumeRenderingObject );

    // Manage Objects
    void AddObject( SceneObject * object, SceneObject * attachTo = 0 );
    void RemoveObjectById( int objectId );
    void RemoveObject( SceneObject * object , bool viewChange = true);
    void ChangeParent( SceneObject * object, SceneObject * newParent, int newChildIndex );
    void GetAllImageObjects( QList<ImageObject*> & objects );
    void GetChildrenImageObjects( SceneObject * obj, QList<ImageObject*> & all );
    void GetAllPolydataObjects( QList<PolyDataObject*> & objects );
    void GetChildrenPolydataObjects( SceneObject * obj, QList<PolyDataObject*> & all );
    void GetAllPointsObjects( QList<PointsObject*> & objects );
    void GetChildrenPointsObjects( SceneObject * obj, QList<PointsObject*> & all );
    void GetAllCameraObjects( QList<CameraObject*> & all );
    void GetAllUSAcquisitionObjects( QList<USAcquisitionObject*> & all );
    void GetAllUsProbeObjects( QList<UsProbeObject*> & all );
    void GetAllObjectsOfType( const char * typeName, QList<SceneObject*> & all );

    // Description:
    // Get object by ID
    SceneObject * GetObjectByID( int id );

    // Find if the object exists
    bool ObjectExists(SceneObject *);

    // Current object
    SceneObject * GetCurrentObject( ) { return CurrentObject; }
    void SetCurrentObject( SceneObject * cur  );

    // Reference object
    ImageObject * GetReferenceDataObject( );
    void SetReferenceDataObject( SceneObject * );
    bool CanBeReference( SceneObject * );

    // Special root objects
    SceneObject * GetSceneRoot();
    void SetAxesObject( PolyDataObject * obj );
    PolyDataObject* GetAxesObject();

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
    void SetSceneDirectory(const QString &wDir);
    const QString GetSceneDirectory();

    //Description
    //Remove all children added to system objects: world, preop and intraop, also all tools
    void RemoveAllSceneObjects();
    void RemoveAllChildrenObjects(SceneObject *);
    void ClearScene();
    void LoadScene(QString & fileName, bool interactive = true );
    void SaveScene(QString & fileName);
    void ObjectReader( Serializer * ser, bool interactive );
    void ObjectWriter( Serializer * ser );

    // Description
    const ObjectList & GetAllObjects() { return AllObjects; }

    // Description
    // returns list of all the user created/added objects with their parents
    int  GetNumberOfUserObjects();
    void GetAllUserObjects(QList<SceneObject*> &);
    void GetChildrenUserObjects( SceneObject * obj, QList<SceneObject*> & );

    // Description
    // returns list of all the listable, non-tracked objects with their parents
    void GetAllListableNonTrackedObjects(QList<SceneObject*> &);
    void GetChildrenListableNonTrackedObjects( SceneObject * obj, QList<SceneObject*> & );

    //Description
    // Get octants of interest
    Octants *GetOctantsOfInterest() {return OctantsOfInterest;}

    //Description
    // Get session info
    SceneInfo *GetSceneInfo() {return CurrentSceneInfo;}

    // Description
    // Manages interaction style of 3D views
    void Set3DInteractorStyle( InteractorStyle style );
    InteractorStyle Get3DInteractorStyle() { return InteractorStyle3D; }

    // Description
    // Utility functions
    static QString FindUniqueName( QString wantedName, QStringList & otherNames );
    // for debugging only
    int GetRefCount();

    // navigation
    void EnablePointerNavigation( bool on);
    void SetNavigationPointerID( int id );
    bool GetNavigationState() {return this->IsNavigating; }
    PointerObject *GetNavigationPointerObject( );

public slots:

    void EmitSignalObjectRenamed(QString, QString);

private slots:

    void ReferenceTransformChangedSlot();
    void CancelProgress();

signals:

    void StartAddingObject( SceneObject*, int );
    void FinishAddingObject();
    void StartRemovingObject( SceneObject*, int );
    void FinishRemovingObject();

    void ObjectAdded( int );
    void ObjectRemoved( int );
    void ObjectNameChanged(QString, QString);
    void CurrentObjectChanged();
    void ReferenceTransformChanged();

    void ExpandView();

protected:

    void InternalClearScene();
    void Init();
    void Clear();
    void NotifyPluginsSceneAboutToLoad();
    void NotifyPluginsSceneFinishedLoading();
    void NotifyPluginsSceneAboutToSave();
    void NotifyPluginsSceneFinishedSaving();

    int NextObjectID;
    int NextSystemObjectID;
    SceneObject * CurrentObject;
    ImageObject *ReferenceDataObject;
    WorldObject * SceneRoot;
    Octants      *OctantsOfInterest;
    SceneInfo  *CurrentSceneInfo;
    InteractorStyle InteractorStyle3D;

    // Description:
    // Allow settin object id when adding object
    void AddObjectUsingID( SceneObject * object, SceneObject * attachTo = 0, int objID = SceneObject::InvalidObjectId);

    // Description:
    // Utility function to setup an object in all views
    void SetupInAllViews( SceneObject * object );

    // Description:
    // Recursive function used to setup all objects bellow obj
    // in view v.
    void SetupOneObject( View * v, SceneObject * obj );

    // Description:
    // Creates a new interactor style object of the right type and assign it to the view passed as param.
    void AssignInteractorStyleToView( InteractorStyle style, View * v );

    // Views
    bool m_viewFollowsReferenceObject;
    typedef std::vector<View*> ViewList;
    ViewList Views;
    int CurrentView;
    int ExpandedView;

    TripleCutPlaneObject * MainCutPlanes;

    // Special global object that manages all volume rendering
    VolumeRenderingObject * MainVolumeRenderer;

    // Objects
    ObjectList AllObjects;

    double ViewBackgroundColor[3];
    double CameraViewAngle3D;

    // scene basic settings
    QString SupportedSceneSaveVersion;

    // scene loading/saving progress
    QProgressDialog *m_sceneLoadSaveProgressDialog;
    bool UpdateProgress(int value);

    QStringList TemporaryFiles;

    // navigation
    int NavigationPointerID;
    int IsNavigating;

    QString GenericText;
private:

    // We declare these to make sure no one is registering or unregistering SceneManager
    // since SceneManager should have only one instance and be deleted at the end
    // by the unique instance of Application.
    virtual void Register(vtkObjectBase* o);
    virtual void UnRegister(vtkObjectBase* o);
};

ObjectSerializationHeaderMacro( SceneManager );

#endif //TAG_SCENEMANAGER_H
