/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "view.h"
#include "viewinteractor.h"
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyle.h>
#include <vtkInteractorStyleTerrain.h>
#include <vtkInteractorStyleImage2.h>
#include <vtkCellPicker.h>
#include <vtkTransform.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkRendererCollection.h>
#include <vtkObjectCallback.h>
#include "vtkMatrix4x4Operators.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "SVL.h"

vtkCxxSetObjectMacro(View,Picker,vtkCellPicker);

ObjectSerializationMacro( View );

const char DefaultViewNames[4][20] = { "Sagittal\0", "Coronal\0", "Transverse\0", "ThreeD\0" };

View::View()
{
    this->Name = "";
    this->SetName( DefaultViewNames[THREED_VIEW_TYPE] );
    this->RenderWidget = 0;
    this->m_renderingEnabled = true;
    this->InteractorStyle = vtkSmartPointer<vtkInteractorStyleTerrain>::New();
    this->Picker = vtkCellPicker::New();
    this->Picker->SetTolerance(0.005); //need some fluff
    this->Picker->PickFromListOn();
    this->Type = THREED_VIEW_TYPE;
    this->Renderer = vtkSmartPointer<vtkRenderer>::New();
    this->Renderer->SetLayer( 0 );
    this->Renderer->GetActiveCamera()->SetPosition( 1, 0, 0 );
    this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
    this->Renderer->GetActiveCamera()->SetViewUp( 0, 0, 1 );
    this->OverlayRenderer = vtkSmartPointer<vtkRenderer>::New();
    this->OverlayRenderer->SetLayer( 1 );
    this->OverlayRenderer->SetActiveCamera( this->Renderer->GetActiveCamera() ); // use same camera as main renderer
    this->OverlayRenderer2 = vtkSmartPointer<vtkRenderer>::New();
    this->OverlayRenderer2->SetLayer( 2 );
    this->OverlayRenderer2->SetActiveCamera( this->Renderer->GetActiveCamera() ); // use same camera as main renderer
    this->Manager = 0;
    this->PrevViewingTransform = vtkSmartPointer<vtkMatrix4x4>::New();
    this->PrevViewingTransform->Identity();
    this->EventObserver = vtkSmartPointer<vtkEventQtSlotConnect>::New();

    // Callback to observe interaction events
    this->InteractionCallback = vtkSmartPointer< vtkObjectCallback<View> >::New();
    this->InteractionCallback->SetCallback( this, &View::ProcessInteractionEvents );
    this->Priority = 2.0;
    m_leftButtonDown = false;
    m_middleButtonDown = false;
    m_rightButtonDown = false;
    m_backupWindowParent = 0;
    CurrentController = 0;
}


View::~View()
{
    if( this->Picker )
        this->Picker->Delete();
}

void View::Serialize( Serializer * ser )
{
    // camera settings: position, focal point, zoom factor
    vtkCamera *camera = this->Renderer->GetActiveCamera();
    int parallel = camera->GetParallelProjection();
    double fp[3], pos[3], scale;
    double viewUp[3], viewAngle;
    // get current camera parameters to save then in scene or to set them as defzult.
    camera->GetPosition( pos );
    camera->GetFocalPoint( fp );
    scale = camera->GetParallelScale();
    camera->GetViewUp( viewUp );
    viewAngle = camera->GetViewAngle();

    ::Serialize( ser, "Position", pos, 3 );
    ::Serialize( ser, "FocalPoint", fp, 3 );
    ::Serialize( ser, "Scale", scale );
    ::Serialize( ser, "ViewUp", viewUp, 3 );
    ::Serialize( ser, "ViewAngle", viewAngle );

    if (ser->IsReader())
    {
        this->ResetCamera();
        camera->SetPosition( pos );
        camera->SetFocalPoint( fp );
        camera->SetViewUp( viewUp );
        if( parallel )
        {
            camera->SetParallelScale(scale);
        }
        else
        {
            camera->SetViewAngle( viewAngle );
        }

        this->NotifyNeedRender();
    }
}

void View::SetType( int type )
{
    if( this->Type == type )
    {
        return;
    }
    
    this->Type = type;
    switch( type )
    {
        case SAGITTAL_VIEW_TYPE:
            this->Renderer->GetActiveCamera()->ParallelProjectionOn();
            this->Renderer->GetActiveCamera()->SetPosition( -1, 0, 0 ); // nose to the left
            this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
            this->Renderer->GetActiveCamera()->SetViewUp( 0, 0, 1 ); 
            if (this->InteractorStyle)
            {
                this->InteractorStyle = vtkSmartPointer<vtkInteractorStyleImage2>::New();
            }
            this->SetName( DefaultViewNames[SAGITTAL_VIEW_TYPE] );
            break;
        case CORONAL_VIEW_TYPE:
            this->Renderer->GetActiveCamera()->ParallelProjectionOn();
            this->Renderer->GetActiveCamera()->SetPosition( 0, -1, 0 ); // left on right
            this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
            this->Renderer->GetActiveCamera()->SetViewUp( 0, 0, 1 ); 
            if (this->InteractorStyle)
            {
                this->InteractorStyle = vtkSmartPointer<vtkInteractorStyleImage2>::New();
            }
            this->SetName( DefaultViewNames[CORONAL_VIEW_TYPE] );
            break;
        case TRANSVERSE_VIEW_TYPE: // axial
            this->Renderer->GetActiveCamera()->ParallelProjectionOn();
            this->Renderer->GetActiveCamera()->SetPosition( 0, 0, 1 ); //left on right
            this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
            this->Renderer->GetActiveCamera()->SetViewUp( 0, 1, 0 ); //nose up
            if (this->InteractorStyle)
            {
                this->InteractorStyle = vtkSmartPointer<vtkInteractorStyleImage2>::New();
            }
            this->SetName( DefaultViewNames[TRANSVERSE_VIEW_TYPE] );
            break;
        case THREED_VIEW_TYPE:
            this->Renderer->GetActiveCamera()->SetPosition( 1, 0, 0 );
            this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
            this->Renderer->GetActiveCamera()->SetViewUp( 0, 0, 1 );
            break;
    }
}

void View::SetQtRenderWidget( QVTKRenderWidget * w )
{
    if( w == this->RenderWidget )
        return;
    this->RenderWidget = w;
}

void View::SetRenderingEnabled( bool b )
{
    if( m_renderingEnabled == b )
        return;
    m_renderingEnabled = b;
    if( m_renderingEnabled )
        NotifyNeedRender();
}

vtkRenderWindowInteractor * View::GetInteractor()
{
    return this->Interactor;
}

void View::SetInteractor( vtkRenderWindowInteractor * interactor )
{
    if( interactor == this->Interactor )
    {
        return;
    }
    
    if( this->Interactor )
    {
        this->ReleaseAllObjects();
        this->EventObserver->Disconnect( this->Interactor );
        this->EventObserver->Disconnect( this->Interactor->GetRenderWindow() );
    }
    
    this->Interactor = interactor;
    this->SetupAllObjects();
    this->Interactor->SetInteractorStyle( this->InteractorStyle );
    this->Interactor->GetRenderWindow()->SetNumberOfLayers( 3 );
    this->Interactor->GetRenderWindow()->AddRenderer( this->Renderer );
    this->Interactor->GetRenderWindow()->AddRenderer( this->OverlayRenderer );
    this->Interactor->GetRenderWindow()->AddRenderer( this->OverlayRenderer2 );

    // Add observer to know when the window starts rendering ( to reset clipping range )
    this->EventObserver->Connect( this->Interactor->GetRenderWindow(), vtkCommand::StartEvent, this, SLOT(WindowStartsRendering()) );

    this->Interactor->AddObserver( vtkCommand::KeyPressEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::LeftButtonPressEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::LeftButtonReleaseEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::MiddleButtonPressEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::MiddleButtonReleaseEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::RightButtonPressEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::RightButtonReleaseEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::MouseWheelForwardEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::MouseWheelBackwardEvent, this->InteractionCallback, this->Priority );
    this->Interactor->AddObserver( vtkCommand::MouseMoveEvent, this->InteractionCallback, this->Priority );
}

vtkRenderer * View::GetRenderer( int level )
{
    Q_ASSERT( level <= 2 );

    if( level == 0 )
        return GetRenderer();
    if( level == 1 )
        return GetOverlayRenderer();
    if( level == 2 )
        return GetOverlayRenderer2();

    // Just in case
    return GetRenderer();
}

vtkRenderer * View::GetRenderer()
{
    return this->Renderer;
}

vtkRenderer * View::GetOverlayRenderer()
{
    return this->OverlayRenderer;
}

vtkRenderer * View::GetOverlayRenderer2()
{
    return this->OverlayRenderer2;
}

void View::SetManager( SceneManager * manager )
{
    if( this->Manager )
        disconnect( this->Manager, SIGNAL(ReferenceTransformChanged()), this, SLOT(ReferenceTransformChanged()) );

    this->Manager = manager;

    if( this->Manager )
    {
        connect( this->Manager, SIGNAL(ReferenceTransformChanged()), this, SLOT(ReferenceTransformChanged()) );
    }
}

void View::SetInteractorStyle( vtkInteractorStyle * style )
{
    this->InteractorStyle = style;

    if( this->InteractorStyle )
    {
        if( this->Interactor )
            this->Interactor->SetInteractorStyle( this->InteractorStyle );
    }
}

vtkInteractorStyle * View::GetInteractorStyle()
{
    return this->InteractorStyle;
}

void View::ReleaseView()
{
    if( CurrentController )
        CurrentController->ReleaseControl( this );
    this->ReleaseAllObjects();
    this->Renderer->SetRenderWindow( 0 );
    this->OverlayRenderer->SetRenderWindow( 0 );
    this->OverlayRenderer2->SetRenderWindow( 0 );
    this->Interactor->SetRenderWindow( 0 );
}

void View::TakeControl( ViewController * c )
{
    Q_ASSERT( c );

    // Tell previous controller to release
    if( CurrentController )
    {
        CurrentController->ReleaseControl( this );
    }

    CurrentController = c;
    this->Interactor->SetInteractorStyle( nullptr );
}

void View::ReleaseControl( ViewController * c )
{
    Q_ASSERT( CurrentController == c );

    CurrentController = nullptr;
    this->Interactor->SetInteractorStyle( this->InteractorStyle );
}

void View::Fullscreen()
{
    Q_ASSERT( this->RenderWidget );
    Q_ASSERT( !this->RenderWidget->isFullScreen() );

    m_backupWindowParent = this->RenderWidget->parent();
    this->RenderWidget->setParent( 0 );
    this->RenderWidget->showFullScreen();
}

void View::AddInteractionObject( ViewInteractor * obj, double priority )
{
    m_interactionObjects.insert( std::pair<double,ViewInteractor*>( priority, obj ) );
}

void View::RemoveInteractionObject( ViewInteractor * obj )
{
    InteractionObjectContainer::iterator it = m_interactionObjects.begin();
    while( it != m_interactionObjects.end() )
    {
        if( (*it).second == obj )
        {
            m_interactionObjects.erase( it );
            break;
        }
        ++it;
    }
}

void View::ProcessInteractionEvents( vtkObject * /*caller*/, unsigned long event, void * /*calldata*/ )
{
    // if in fullscreen mode, get back if ESC key is pressed
    if( event == vtkCommand::KeyPressEvent && this->Interactor->GetKeyCode() == 27 )
    {
        if( this->RenderWidget->isFullScreen() )
        {
            this->RenderWidget->setParent( qobject_cast<QWidget*>(m_backupWindowParent) );
            this->RenderWidget->showNormal();
        }
    }

    // Get mouse position and keyboard modifiers
    int * pos = this->Interactor->GetEventPosition();
    unsigned modifier = 0;
    if( this->Interactor->GetControlKey() )
        modifier |= CtrlModifier;
    if( this->Interactor->GetShiftKey() )
        modifier |= ShiftModifier;
    if( this->Interactor->GetAltKey() )
        modifier |= AltModifier;

    // Set Button State variables
    if( event == vtkCommand::LeftButtonPressEvent )
        m_leftButtonDown = true;
    else if( event == vtkCommand::LeftButtonReleaseEvent )
        m_leftButtonDown = false;
    else if( event == vtkCommand::MiddleButtonPressEvent )
        m_middleButtonDown = true;
    else if( event == vtkCommand::MiddleButtonReleaseEvent )
        m_middleButtonDown = false;
    else if( event == vtkCommand::RightButtonPressEvent )
        m_rightButtonDown = true;
    else if( event == vtkCommand::RightButtonReleaseEvent )
        m_rightButtonDown = false;

    // If a button is pressed, we have to ignore the mouse wheel events
    /*if( event == vtkCommand::MouseWheelForwardEvent ||  event == vtkCommand::MouseWheelBackwardEvent )
    {
        if( m_leftButtonDown || m_middleButtonDown || m_rightButtonDown )
        {
            this->InteractionCallback->AbortFlagOn();
            return;
        }
    }*/

    InteractionObjectContainer::reverse_iterator it = m_interactionObjects.rbegin();
    while( it != m_interactionObjects.rend() )
    {
        bool swallow = false;
        ViewInteractor * obj = (*it).second;
        switch ( event )
        {
        case vtkCommand::LeftButtonPressEvent:
            m_leftButtonDown = true;
            swallow = obj->OnLeftButtonPressed( this, pos[0], pos[1], modifier );
            break;
        case vtkCommand::LeftButtonReleaseEvent:
            m_leftButtonDown = false;
            swallow = obj->OnLeftButtonReleased( this, pos[0], pos[1], modifier );
            break;
        case vtkCommand::MiddleButtonPressEvent:
            m_middleButtonDown = true;
            swallow = obj->OnMiddleButtonPressed( this, pos[0], pos[1], modifier );
            break;
        case vtkCommand::MiddleButtonReleaseEvent:
            m_middleButtonDown = false;
            swallow = obj->OnMiddleButtonReleased( this, pos[0], pos[1], modifier );
            break;
        case vtkCommand::RightButtonPressEvent:
            m_rightButtonDown = true;
            swallow = obj->OnRightButtonPressed( this, pos[0], pos[1], modifier );
            break;
        case vtkCommand::RightButtonReleaseEvent:
            m_rightButtonDown = false;
            swallow = obj->OnRightButtonReleased( this, pos[0], pos[1], modifier );
            break;
        case vtkCommand::MouseMoveEvent:
            swallow = obj->OnMouseMoved( this, pos[0], pos[1], modifier );
            break;
        }

        if( swallow )
        {
            this->InteractionCallback->AbortFlagOn();
            break;
        }

        ++it;
    }
}

void View::WorldToWindow( double world[3], double & xWin, double & yWin )
{
    double w4[4];
    w4[0] = world[0];
    w4[1] = world[1];
    w4[2] = world[2];
    w4[3] = 1.0;
    this->Renderer->SetWorldPoint( w4 );
    this->Renderer->WorldToDisplay();
    xWin = this->Renderer->GetDisplayPoint()[0];
    yWin = this->Renderer->GetDisplayPoint()[1];
}

void View::SetBackgroundColor( double * color )
{
    this->Renderer->SetBackground( color );
    this->NotifyNeedRender();
}

int * View::GetWindowSize( )
{
    return this->RenderWidget->GetRenderWindow()->GetSize();
}

void View::ResetCamera()
{
    if( this->GetType() != THREED_VIEW_TYPE  )
        this->Reset2DView();
    double prevViewAngle = this->Renderer->GetActiveCamera()->GetViewAngle();
    this->Renderer->ResetCamera();
    AdjustCameraDistance( prevViewAngle );
}

void View::ResetCamera( double bounds[6] )
{
    double prevViewAngle = this->Renderer->GetActiveCamera()->GetViewAngle();
    this->Renderer->ResetCamera( bounds );
    AdjustCameraDistance( prevViewAngle );
}

void View::ZoomCamera(double factor)
{
    this->Renderer->GetActiveCamera()->Zoom(factor);
}

double View::GetViewAngle()
{
    return this->Renderer->GetActiveCamera()->GetViewAngle();
}

void View::SetViewAngle( double angle )
{
    this->Renderer->GetActiveCamera()->SetViewAngle( angle );
    this->NotifyNeedRender();
}

void View::NotifyNeedRender()
{
    if( m_renderingEnabled )
    {
        this->DoVTKRender();
    }
}

void View::ReferenceTransformChanged()
{
    // We update transform only if it is a 2D view or if Manager says we follow 3D views as well
    if( this->Manager && ( this->GetType() != THREED_VIEW_TYPE || this->Manager->Is3DViewFollowingReferenceVolume() ) )
    {
        ImageObject * obj = this->Manager->GetReferenceDataObject();
        vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
        vtkCamera * cam = this->Renderer->GetActiveCamera();
        if( obj && this->Renderer )
        {
            vtkTransform * refTransform = obj->GetWorldTransform();
            t->SetInput( refTransform );
            t->Concatenate( this->PrevViewingTransform );

            cam->ApplyTransform( t );

            // backup inverted current transform
            this->PrevViewingTransform->DeepCopy( refTransform->GetMatrix() );
            this->PrevViewingTransform->Invert();

            NotifyNeedRender();
        }
        else
        {
            t->Identity();
            t->Concatenate( this->PrevViewingTransform );
            cam->ApplyTransform( t );
            // backup inverted current transform
            this->PrevViewingTransform->Identity();
        }
    }
}

void View::WindowStartsRendering()
{
    this->Renderer->ResetCameraClippingRange();
}

// Really call the vtk rendering code
void View::DoVTKRender()
{
    if( this->Interactor )
    {
        this->Interactor->Render();
    }
}

void View::SetupAllObjects()
{
    if( !this->Interactor )
    {
        vtkErrorMacro(<< "Can't setup objects before setting its interactor." );
        return;
    }
    
    this->Manager->SetupAllObjects( this );
}

void View::ReleaseAllObjects()
{
    this->Manager->ReleaseAllObjects( this );
}

#include "vtkMath.h"

// This function is used to conveniently readjust the view angle
// of the camera after vtkRenderer::ResetCamera is called. The
// vtkRenderer::ResetCamera function sets the view angle back
// to 30 degrees, which is not what we want in Ibis. This function
// readjusts the camera position to cover the same area as the
// 30 degrees camera at focal point and sets the camera view
// angle to the angle passed in parameter.
void View::AdjustCameraDistance( double viewAngle )
{
    vtkCamera * cam = this->Renderer->GetActiveCamera();
    Vec3 pos( cam->GetPosition() );
    Vec3 target( cam->GetFocalPoint() );
    Vec3 dir = target - pos;
    double dr = len( dir );
    Vec3 ndir = dir / dr;
    double vtkHardcodedAngle = vtkMath::RadiansFromDegrees( 30.0 * 0.5 );
    double wantedAngle = vtkMath::RadiansFromDegrees( viewAngle * 0.5 );
    double du = dr * tan( vtkHardcodedAngle ) / tan( wantedAngle );
    Vec3 newPos = pos + ( dr - du ) * ndir;
    cam->SetPosition( newPos.Ref() );
    cam->SetViewAngle( viewAngle );
}

void View::Reset2DView()
{
    if( this->Manager )
    {
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        ImageObject * obj = this->Manager->GetReferenceDataObject();
        if( obj )
        {
            transform = this->Manager->GetReferenceTransform();
        }

        vtkCamera * cam = this->Renderer->GetActiveCamera();
        Vec3 pos( cam->GetPosition() );
        Vec3 fp( cam->GetFocalPoint() );
        Vec3 vup( cam->GetViewUp() );
        double dist = cam->GetDistance();
        Vec3 normal( 0, 0, 0 );
        Vec3 up( 0, 0, 0 );

        switch( this->Type )
        {
            case SAGITTAL_VIEW_TYPE:
                normal[0] = -1;
                up[2] = 1;
                break;
            case CORONAL_VIEW_TYPE:
                normal[1] = -1;
                up[2] = 1;
                break;
            case TRANSVERSE_VIEW_TYPE:
                normal[2] = 1;
                up[1] = 1;
               break;
            case THREED_VIEW_TYPE:
                return;
        }
        Vec3 distVec = normal * dist;
        Vec3 newNormal;
        vtkMatrix4x4Operators::MultiplyVector( transform->GetMatrix(), distVec.Ref(), newNormal.Ref() );
        Vec3 newPos = fp + newNormal;
        Vec3 newVup;
        vtkMatrix4x4Operators::MultiplyVector( transform->GetMatrix(), up.Ref(), newVup.Ref() );
        cam->SetPosition( newPos.Ref() );
        cam->SetViewUp( newVup.Ref() );
    }
}
