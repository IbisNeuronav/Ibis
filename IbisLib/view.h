/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __View_h_
#define __View_h_

#include <list>
#include <map>
#include "vtkObject.h"
#include <QObject>
#include "serializer.h"
#include <vtkSmartPointer.h>
#include <QVTKRenderWidget.h>

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
template< class T > class vtkObjectCallback;

#define CtrlModifier (unsigned(1))
#define ShiftModifier (unsigned(2))
#define AltModifier (unsigned(4))

extern const char DefaultViewNames[4][20];

// ViewController is an interface that client class can implement to take control
// of the view, eg. control virtual camera and lighting of a 3D view

class View;

class ViewController
{
public:
    ViewController() {}
    virtual ~ViewController() {}
    virtual void ReleaseControl( View * v ) = 0;
};


class View : public QObject, public vtkObject
{

Q_OBJECT

public:

    static View * New() { return new View; }

    vtkTypeMacro(View,vtkObject);


    View();
    virtual ~View() override;

    virtual void Serialize( Serializer * ser );

    void SetName( QString name ) { this->Name = name; }
    QString GetName() { return this->Name; }

    vtkGetMacro(Type,int);
    void SetType( int type );

    void SetQtRenderWidget( QVTKRenderWidget * w );

    // Control rendering of the view
    void SetRenderingEnabled( bool b );

    vtkRenderWindowInteractor * GetInteractor();
    void SetInteractor( vtkRenderWindowInteractor * interactor );

    vtkRenderer * GetRenderer( int level );
    vtkRenderer * GetRenderer();
    vtkRenderer * GetOverlayRenderer();
    vtkRenderer * GetOverlayRenderer2();

    vtkGetObjectMacro(Picker,vtkCellPicker);
    void SetPicker( vtkCellPicker * picker );

    vtkInteractorStyle * GetInteractorStyle();
    void SetInteractorStyle( vtkInteractorStyle * style );

    vtkGetObjectMacro(Manager,SceneManager);
    void SetManager( SceneManager * manager );

    void SetBackgroundColor( double * color );
    int * GetWindowSize();

    void ReleaseView();
    void TakeControl( ViewController * c );
    void ReleaseControl( ViewController * c );
    void Fullscreen();

    void AddInteractionObject( ViewInteractor * obj, double priority );
    void RemoveInteractionObject( ViewInteractor * obj );
    void ProcessInteractionEvents( vtkObject * caller, unsigned long event, void * calldata );

    void WorldToWindow( double world[3], double & xWin, double & yWin );

public slots:

    // Description:
    // Notify the view that something it contains needs render. The view is then
    // going to emit a Modified event.
    void NotifyNeedRender();
    void ReferenceTransformChanged();

    // Description:
    // Reset the active camera in the renderer of this view
    void ResetCamera();
    void ResetCamera( double bounds[6] );
    void ZoomCamera( double factor );
    double GetViewAngle();
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
    vtkSmartPointer< vtkObjectCallback<View> > InteractionCallback;
    double Priority;
    typedef std::multimap<double,ViewInteractor*> InteractionObjectContainer;
    InteractionObjectContainer m_interactionObjects;
    bool m_leftButtonDown;
    bool m_middleButtonDown;
    bool m_rightButtonDown;

    QObject * m_backupWindowParent;  // used when we go fullscreen
};

ObjectSerializationHeaderMacro( View );

#endif
