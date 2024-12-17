/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef VIEW_H
#define VIEW_H

#include <QVTKRenderWidget.h>
#include <vtkSmartPointer.h>

#include <QObject>
#include <list>
#include <map>

#include "serializer.h"
#include "vtkObject.h"

class ViewInteractor;
class vtkInteractorStyle;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkCellPicker;
class vtkTransform;
class vtkMatrix4x4;
class vtkEventQtSlotConnect;
class SceneManager;
class SceneObject;
template <class T>
class vtkObjectCallback;

#define CtrlModifier ( unsigned( 1 ) )
#define ShiftModifier ( unsigned( 2 ) )
#define AltModifier ( unsigned( 4 ) )

extern const char DefaultViewNames[4][20];

class View;

/**
 * @class   ViewController
 *
 *  ViewController is an interface that client class can implement to take control
 *  of the view, eg. control virtual camera and lighting of a 3D view
 */
class ViewController
{
public:
    ViewController() {}
    virtual ~ViewController() {}
    virtual void ReleaseControl( View * v ) = 0;
};

/**
 * @class   View
 * @brief   View shows objects in the scene
 *
 *  There are 2 types of views - 2D and 3D.\n
 *  As standard in IBIS we have three 2D views and one 3D view.\n
 *  2D views will show cross sections of ImageObjects.\n
 *  As the primary application of IBIS is to show brain and its cross sections, Views are named as follows:\n
 *  Sagittal (bottom right), Coronal (bottom left), Transverse (top left) and ThreeD (top right).\n
 *  In order to manage better the displayed objects every view has 3 renderers, main and two overlays.\n
 *  By default all renderers use the same camera.\n
 *  The default InteractorStyle is vtkInteractorStyleImage2 for 2D views and vtkInteractorStyleTrackballCamera for 3D.
 *
 *  @sa vtkInteractorStyleImage2 SceneManager SceneObject ViewController
 */
class View : public QObject, public vtkObject
{
    Q_OBJECT

public:
    static View * New() { return new View; }

    vtkTypeMacro( View, vtkObject );

    View();
    virtual ~View() override;

    /** Save/restore view parameters. */
    virtual void Serialize( Serializer * ser );

    /** Set view name */
    void SetName( QString name ) { this->Name = name; }
    /** Get view name */
    QString GetName() { return this->Name; }

    /** Get view type */
    vtkGetMacro( Type, int );
    /** Set view type */
    void SetType( int type );

    /** Set QVTKRenderWidget - a generic widget for displaying VTK rendering
     *  results in a Qt application. */
    void SetQtRenderWidget( QVTKRenderWidget * w );

    /** Control rendering of the view */
    void SetRenderingEnabled( bool b );

    /** Get view interactor */
    vtkRenderWindowInteractor * GetInteractor();
    /** Set view interactor */
    void SetInteractor( vtkRenderWindowInteractor * interactor );

    /** @name Renderers
     *  @brief  Getting renderers
     */
    ///@{
    /** Get renderer
     *  @param level 0 - main Renderer, 1 OverlayRenderer, 2 OverlayRenderer2
     */
    vtkRenderer * GetRenderer( int level );
    /** Get main renderer */
    vtkRenderer * GetRenderer();
    /** Get OverlayRenderer */
    vtkRenderer * GetOverlayRenderer();
    /** Get OverlayRenderer2 */
    vtkRenderer * GetOverlayRenderer2();
    ///@}

    /** Get Picker */
    vtkGetObjectMacro( Picker, vtkCellPicker );
    /** Set Picker */
    void SetPicker( vtkCellPicker * picker );

    /** Get InteractorStyle */
    vtkInteractorStyle * GetInteractorStyle();
    /** Set InteractorStyle */
    void SetInteractorStyle( vtkInteractorStyle * style );

    /** Get SceneManager */
    vtkGetObjectMacro( Manager, SceneManager );
    /** Set SceneManager */
    void SetManager( SceneManager * manager );

    /** Set view bacground color. */
    void SetBackgroundColor( double * color );
    /** Get view window size. */
    int * GetWindowSize();

    /** ReleaseView() starts cleaning and deleting the view. The function is called on the application exit.*/
    void ReleaseView();

    /** Transfer view control to the caller. */
    void TakeControl( ViewController * c );
    /** Return control to the view. */
    void ReleaseControl( ViewController * c );
    /** Expand view to full screen. */
    void Fullscreen();
    /** Add an object that will react to the interactions events of the view.
     *  Such objects are kept in a Multimap associative container m_interactionObjects.*/
    void AddInteractionObject( ViewInteractor * obj, double priority );
    /** Remove an object from m_interactionObjects. */
    void RemoveInteractionObject( ViewInteractor * obj );
    /** Send the interaction events to the objects from m_interactionObjects. */
    void ProcessInteractionEvents( vtkObject * caller, unsigned long event, void * calldata );

    /** Get window coordinates (xWin, yWin) of a world point (world[3]). */
    void WorldToWindow( double world[3], double & xWin, double & yWin );

public slots:

    /** Notify the view that something it contains needs render. The view is then
     *  going to emit a Modified event. */
    void NotifyNeedRender();
    /** Set enable render to true to refresh rendering */
    void EnableRendering();
    /** Update camera transform when the reference transform is modified. */
    void ReferenceTransformChanged();

    /** Reset the active camera in the renderer of this view. */
    void ResetCamera();
    /** Reset the active camera in the renderer of this view using predefined bounds. */
    void ResetCamera( double bounds[6] );
    /** Zoom the active camera in the renderer of this view. */
    void ZoomCamera( double factor );
    /** Get active camera view angle. */
    double GetViewAngle();
    /** Set active camera view angle. */
    void SetViewAngle( double angle );

private slots:

    void WindowStartsRendering();

protected:
    void DoVTKRender();
    void SetupAllObjects();
    void ReleaseAllObjects();
    void GetPositionAndModifier( int & x, int & y, unsigned & modifier );
    void AdjustCameraDistance( double viewAngle );
    void Reset2DView();
    void SetRotationCenter3D();

    QString Name;
    QVTKRenderWidget * RenderWidget;
    bool m_renderingEnabled;
    vtkSmartPointer<vtkRenderWindowInteractor> Interactor;
    vtkSmartPointer<vtkRenderer> Renderer;
    vtkSmartPointer<vtkRenderer> OverlayRenderer;
    vtkSmartPointer<vtkRenderer> OverlayRenderer2;
    vtkCellPicker * Picker;
    vtkSmartPointer<vtkInteractorStyle> InteractorStyle;
    vtkSmartPointer<vtkEventQtSlotConnect> EventObserver;
    SceneManager * Manager;
    vtkSmartPointer<vtkMatrix4x4> PrevViewingTransform;
    int Type;
    ViewController * CurrentController;  // this should be 0 if view is not controlled

    // Manage mouse and keyboard interaction with sceneobject
    vtkSmartPointer<vtkObjectCallback<View> > InteractionCallback;
    double Priority;
    typedef std::multimap<double, ViewInteractor *> InteractionObjectContainer;
    InteractionObjectContainer m_interactionObjects;
    bool m_leftButtonDown;
    bool m_middleButtonDown;
    bool m_rightButtonDown;

    QObject * m_backupWindowParent;  // used when we go fullscreen
};

ObjectSerializationHeaderMacro( View );

#endif
