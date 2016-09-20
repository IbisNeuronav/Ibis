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
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkInteractorStyleTerrain.h"
#include "vtkInteractorStyleImage2.h"
#include "vtkCellPicker.h"
#include "vtkTransform.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkRendererCollection.h"
#include "vtkObjectCallback.h"
#include "vtkqtrenderwindow.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"

vtkCxxSetObjectMacro(View,Picker,vtkCellPicker);

ObjectSerializationMacro( View );

const char DefaultViewNames[4][20] = { "Sagittal\0", "Coronal\0", "Transverse\0", "ThreeD" };


View::View() 
{
    this->Name = "";
    this->SetName( DefaultViewNames[THREED_VIEW_TYPE] );
    this->RenderWindow = 0;
    this->m_renderingEnabled = true;
    this->Interactor = 0;
    this->InteractorStyle = vtkInteractorStyleTerrain::New();
    this->Picker = vtkCellPicker::New();
    this->Picker->SetTolerance(0.005); //need some fluff
    this->Picker->PickFromListOn();
    this->Type = THREED_VIEW_TYPE;
    this->Renderer = vtkRenderer::New();
    this->Renderer->SetLayer( 0 );
    this->Renderer->GetActiveCamera()->SetPosition( 1, 0, 0 );
    this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
    this->Renderer->GetActiveCamera()->SetViewUp( 0, 0, 1 );
    this->OverlayRenderer = vtkRenderer::New();
    this->OverlayRenderer->SetLayer( 1 );
    this->OverlayRenderer->SetActiveCamera( this->Renderer->GetActiveCamera() ); // use same camera as main renderer
    this->OverlayRenderer2 = vtkRenderer::New();
    this->OverlayRenderer2->SetLayer( 2 );
    this->OverlayRenderer2->SetActiveCamera( this->Renderer->GetActiveCamera() ); // use same camera as main renderer
    this->Manager = 0;
    this->PrevViewingTransform = vtkMatrix4x4::New();
    this->PrevViewingTransform->Identity();
    this->EventObserver = vtkEventQtSlotConnect::New();

    // Callback to observe interaction events
    this->InteractionCallback = vtkObjectCallback<View>::New();
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
    this->Renderer->Delete();
    this->OverlayRenderer->Delete();
    this->OverlayRenderer2->Delete();
    if( this->Picker )
        this->Picker->Delete();
    this->InteractorStyle->Delete();
    this->PrevViewingTransform->Delete();
    this->EventObserver->Delete();
}

void View::Serialize( Serializer * ser )
{
    // camera settings: position, focal point, zoom factor
    vtkCamera *camera = this->Renderer->GetActiveCamera();
    int parallel = camera->GetParallelProjection();
    double fp[3] = {0.0,0.0,0.0}, pos[3]={0.0,0.0,1.0}, scale=150.0;
    double viewUp[3]= {0.0,0.0,1.0}, viewAngle = 30.0;
    if(!ser->IsReader())
    {
        camera->GetPosition( pos );
        camera->GetFocalPoint( fp );
        scale = camera->GetParallelScale();
        camera->GetViewUp( viewUp );
        viewAngle = camera->GetViewAngle();
    }

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
        if( parallel )
        {
            camera->SetParallelScale(scale);
        }
        else
        {
            camera->SetViewUp( viewUp );
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
                this->InteractorStyle->Delete();
                this->InteractorStyle = vtkInteractorStyleImage2::New();
            }
            this->SetName( DefaultViewNames[SAGITTAL_VIEW_TYPE] );
            break;
        case CORONAL_VIEW_TYPE:
            this->Renderer->GetActiveCamera()->ParallelProjectionOn();
            this->Renderer->GetActiveCamera()->SetPosition( 0, 1, 0 ); // left on right
            this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
            this->Renderer->GetActiveCamera()->SetViewUp( 0, 0, 1 ); 
            if (this->InteractorStyle)
            {
                this->InteractorStyle->Delete();
                this->InteractorStyle = vtkInteractorStyleImage2::New();
            }
            this->SetName( DefaultViewNames[CORONAL_VIEW_TYPE] );
            break;
        case TRANSVERSE_VIEW_TYPE: // axial
            this->Renderer->GetActiveCamera()->ParallelProjectionOn();
            this->Renderer->GetActiveCamera()->SetPosition( 0, 0, -1 ); //left on right
            this->Renderer->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
            this->Renderer->GetActiveCamera()->SetViewUp( 0, 1, 0 ); //nose up
            if (this->InteractorStyle)
            {
                this->InteractorStyle->Delete();
                this->InteractorStyle = vtkInteractorStyleImage2::New();
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

vtkQtRenderWindow * View::GetQtRenderWindow()
{
    return this->RenderWindow;
}


void View::SetQtRenderWindow( vtkQtRenderWindow * w )
{
    if( w == this->RenderWindow )
        return;
    this->RenderWindow = w;
}

void View::SetRenderingEnabled( bool b )
{
    if( m_renderingEnabled == b )
        return;
    m_renderingEnabled = b;
    if( this->RenderWindow )
        this->RenderWindow->SetRenderingEnabled( m_renderingEnabled );
    if( m_renderingEnabled )
        NotifyNeedRender();
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
        this->Interactor->UnRegister( this );
    }
    
    interactor->Register( this );
    this->Interactor = interactor;
    this->SetupAllObjects();
    this->Interactor->SetInteractorStyle( this->InteractorStyle );
    this->Interactor->GetRenderWindow()->SetNumberOfLayers( 3 );
    this->Interactor->GetRenderWindow()->AddRenderer( this->Renderer );
    this->Interactor->GetRenderWindow()->AddRenderer( this->OverlayRenderer );
    this->Interactor->GetRenderWindow()->AddRenderer( this->OverlayRenderer2 );

    // Add observer to know when the window starts rendering ( to reset clipping range )
    this->EventObserver->Connect( this->Interactor->GetRenderWindow(), vtkCommand::StartEvent, this, SLOT(WindowStartsRendering()) );

    // Make sure widget and other interactor observers don't call render directly, but instead, request a
    // render to the view. This will allow Qt to concatenate render requests.
    this->Interactor->EnableRenderOff();
    this->EventObserver->Connect( this->Interactor, vtkCommand::RenderEvent, this, SLOT(NotifyNeedRender()) );

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
    if( this->InteractorStyle )
        this->InteractorStyle->UnRegister( this );

    this->InteractorStyle = style;

    if( this->InteractorStyle )
    {
        this->InteractorStyle->Register( this );
        if( this->Interactor )
            this->Interactor->SetInteractorStyle( this->InteractorStyle );
    }
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
    this->Interactor->SetInteractorStyle( 0 );
}

void View::ReleaseControl( ViewController * c )
{
    Q_ASSERT( CurrentController == c );

    CurrentController = 0;
    this->Interactor->SetInteractorStyle( this->InteractorStyle );
}

void View::Fullscreen()
{
    Q_ASSERT( this->RenderWindow );
    Q_ASSERT( !this->RenderWindow->isFullScreen() );

    m_backupWindowParent = this->RenderWindow->parent();
    this->RenderWindow->setParent( 0 );
    this->RenderWindow->showFullScreen();
}

void View::AddInteractionObject( SceneObject * obj, double priority )
{
    if( obj )
        obj->Register( this );
    m_interactionObjects.insert( std::pair<double,SceneObject*>( priority, obj ) );
}

void View::RemoveInteractionObject( SceneObject * obj )
{
    InteractionObjectContainer::iterator it = m_interactionObjects.begin();
    while( it != m_interactionObjects.end() )
    {
        if( (*it).second == obj )
        {
            obj->UnRegister( this );
            m_interactionObjects.erase( it );
            break;
        }
        ++it;
    }
}

void View::ProcessInteractionEvents( vtkObject * caller, unsigned long event, void * calldata )
{
    // if in fullscreen mode, get back if ESC key is pressed
    if( event == vtkCommand::KeyPressEvent && this->Interactor->GetKeyCode() == 27 )
    {
        if( this->RenderWindow->isFullScreen() )
        {
            this->RenderWindow->setParent( qobject_cast<QWidget*>(m_backupWindowParent) );
            this->RenderWindow->showNormal();
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

    InteractionObjectContainer::iterator it = m_interactionObjects.begin();
    while( it != m_interactionObjects.end() )
    {
        bool swallow = false;
        SceneObject * obj = (*it).second;
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


void View::ResetCamera()
{
    if( this->Renderer )
    {
        this->Renderer->ResetCamera();
    }    
}

void View::ResetCamera( double bounds[6] )
{
    if( this->Renderer )
    {
        this->Renderer->ResetCamera( bounds );
    }
}

void View::ZoomCamera(double factor)
{
    if( this->Renderer )
    {
        this->Renderer->GetActiveCamera()->Zoom(factor);
    }    
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
        emit Modified();
}

void View::ReferenceTransformChanged()
{
    // We update transform only if it is a 2D view or if Manager says we follow 3D views as well
    if( this->Manager && ( this->GetType() != THREED_VIEW_TYPE || this->Manager->Is3DViewFollowingReferenceVolume() ) )
    {
        ImageObject * obj = this->Manager->GetReferenceDataObject();
        if( obj && this->Renderer )
        {
            vtkTransform * t = vtkTransform::New();

            vtkTransform * refTransform = obj->GetWorldTransform();
            t->SetInput( refTransform );
            t->Concatenate( this->PrevViewingTransform );

            vtkCamera * cam = this->Renderer->GetActiveCamera();
            cam->ApplyTransform( t );
            t->Delete();

            // backup inverted current transform
            this->PrevViewingTransform->DeepCopy( refTransform->GetMatrix() );
            this->PrevViewingTransform->Invert();

            NotifyNeedRender();
        }
    }
}

void View::WindowStartsRendering()
{
    this->Renderer->ResetCameraClippingRange();
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
