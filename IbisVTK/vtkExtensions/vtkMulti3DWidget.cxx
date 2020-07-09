/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include <vtkMulti3DWidget.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkDataSet.h>
#include <vtkAssembly.h>
#include <vtkProp3DCollection.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include "vtkObjectCallback.h"

vtkMulti3DWidget::vtkMulti3DWidget()
{
    this->Callback = vtkSmartPointer< vtkObjectCallback<vtkMulti3DWidget> >::New();
    this->Callback->SetCallback( this, &vtkMulti3DWidget::ProcessEvents );

    this->Interaction = 1;
	this->NoModifierDisables = 0;
    this->ControlDisables = 0;
    this->ShiftDisables = 0;

    this->Placed = 0;
    this->PlaceFactor = 0.5;

    this->Priority = 0.5;

    this->HandleSize = 0.01;
    this->ValidPick = 0;

}


vtkMulti3DWidget::~vtkMulti3DWidget()
{
    // unref renderers
    RendererVec::iterator it = this->Renderers.begin();
    AssemblyVec::iterator itProp = this->Assemblies.begin();
    while( it != this->Renderers.end() )
    {
        if( *it )
        {
            (*it)->UnRegister( this );
        }
        if( *itProp )
        {
            (*itProp)->UnRegister( this );
        }
        ++it;
        ++itProp;
    }
}

void vtkMulti3DWidget::PlaceWidget(double xmin, double xmax,
                              double ymin, double ymax,
                              double zmin, double zmax)
{
    double bounds[6];

    bounds[0] = xmin;
    bounds[1] = xmax;
    bounds[2] = ymin;
    bounds[3] = ymax;
    bounds[4] = zmin;
    bounds[5] = zmax;

    this->PlaceWidget(bounds);
}


void vtkMulti3DWidget::SetEnabled( int enabling )
{
    // Can't enable/disable if there are no interactors
    if ( ! this->GetNumberOfInteractors() )
        return;

    this->CurrentInteractorIndex = 0;

    //--------------- Enabling -------------------------------------------
    if ( enabling )
    {
        if ( this->Enabled ) //already enabled, just return
        {
            return;
        }

        this->Enabled = 1;

        // we have to honour this ivar: it could be that this->Interaction was
        // set to off when we were disabled
        if( this->Interaction )
        {
            this->AddObservers();
        }

        // Let subclasses add their geometry
        this->InternalEnable();

        this->InvokeEvent(vtkCommand::EnableEvent,0);

        this->RenderAll();
    }

    //--------------- disabling -------------------------------------------
    else
    {
        if ( ! this->Enabled ) //already disabled, just return
        {
            return;
        }

        this->Enabled = 0;

        this->RemoveObservers();

        // Let subclasses remove their geometry
        this->InternalDisable();

        this->InvokeEvent(vtkCommand::DisableEvent,0);

        this->RenderAll();
    }
}


int vtkMulti3DWidget::AddRenderer( vtkRenderer * ren, vtkAssembly * assembly )
{
    if( ren )
    {
        ren->Register( this );
        this->Renderers.push_back( ren );
        if( assembly )
            assembly->Register( this );
        this->Assemblies.push_back( assembly );
        this->InternalAddRenderer( ren, assembly );
        this->Modified();
		return this->Renderers.size() - 1;
    }
	return -1;
}


int vtkMulti3DWidget::GetNumberOfRenderers()
{
    return this->Renderers.size();
}


vtkRenderer * vtkMulti3DWidget::GetRenderer( int index )
{
    if( index < this->Renderers.size() && index >= 0 )
    {
        return this->Renderers[ index ];
    }
    return 0;
}


vtkAssembly * vtkMulti3DWidget::GetAssembly( int index )
{
    if( index < this->Renderers.size() && index >= 0 )
    {
        return this->Assemblies[ index ];
    }
	return 0;
}

void vtkMulti3DWidget::RemoveRenderer( vtkRenderer * ren )
{
    if( ren )
    {
        int rendererIndex = this->GetRendererIndex( ren );
        if( rendererIndex != -1 )
        {
            this->InternalRemoveRenderer( rendererIndex );
            
            RendererVec::iterator itRen = this->Renderers.begin() + rendererIndex;
            (*itRen)->UnRegister( this );
            this->Renderers.erase( itRen );
            
            AssemblyVec::iterator itAssembly = this->Assemblies.begin() + rendererIndex;
            if( (*itAssembly) )
            {
                (*itAssembly)->UnRegister( this );
            }
            this->Assemblies.erase( itAssembly );
        }
        
        this->Modified();
    }
}

vtkAssembly * vtkMulti3DWidget::GetAssembly( vtkRenderer * ren )
{
    int index = this->GetRendererIndex( ren );
    if( index != -1 )
    {
        return this->Assemblies[ index ];
    }
    return 0;
}


void vtkMulti3DWidget::SetInteraction( int interact )
{
    if( this->Interactors.size() && this->Enabled )
    {
        if (this->Interaction == interact)
        {
            return;
        }
        if( interact == 0 )
        {
            this->RemoveObservers();
        }
        else
        {
            this->AddObservers();
        }
        this->Interaction = interact;
    }
    else
    {
        vtkGenericWarningMacro(<<"set interactor and Enabled before changing interaction...");
    }
}

void vtkMulti3DWidget::InternalRemoveInteractor( int index )
{
    this->Interactors[ index ]->RemoveObserver( this->Callback );
}

void vtkMulti3DWidget::AddObservers()
{
    for( InteractorVec::iterator itInt = this->Interactors.begin(); itInt != this->Interactors.end(); ++itInt )
    {
        for( EventIdVec::iterator itEvent = this->EventsObserved.begin(); itEvent != this->EventsObserved.end(); ++itEvent )
        {
            (*itInt)->AddObserver( (*itEvent), this->Callback, this->Priority );
        }
    }
}


void vtkMulti3DWidget::RemoveObservers()
{
    for( InteractorVec::iterator itInt = this->Interactors.begin(); itInt != this->Interactors.end(); ++itInt )
    {
        (*itInt)->RemoveObserver( this->Callback );
    }
}


void vtkMulti3DWidget::RenderAll()
{
    for( InteractorVec::iterator it = this->Interactors.begin(); it != this->Interactors.end(); ++it )
    {
        (*it)->Render();
    }
}

int RecursiveFindChild( vtkAssembly * assembly, vtkProp3D * propToFind )
{
    vtkProp3DCollection * props = assembly->GetParts();
    vtkProp3D * prop;
    vtkAssembly * childAssembly = 0;
    for( props->InitTraversal(); (prop=props->GetNextProp3D()); )
    {
        if( prop )
        {
            if( prop == propToFind )
            {
                return 1;
            }
        }
        
        childAssembly = vtkAssembly::SafeDownCast( prop );
        if( childAssembly )
        {
            if( RecursiveFindChild( childAssembly, propToFind ) )
            {
                return 1;
            }
        }
    }
    return 0;
}

vtkAssembly * vtkMulti3DWidget::GetUppermostParent( vtkRenderer * ren, vtkProp3D * propToFind )
{
    vtkProp * prop = 0;
    vtkProp3D * current = 0;
    vtkAssembly * assembly = 0;
    vtkPropCollection * props = ren->GetViewProps();
    for( props->InitTraversal(); (prop=props->GetNextProp()); )
    {
        current = vtkProp3D::SafeDownCast( prop );
        if( current )
        {
            if( current == propToFind )
            {
                return 0;
            }
        }
        
        assembly = vtkAssembly::SafeDownCast( prop );
        if( assembly )
        {
            if( RecursiveFindChild( assembly, propToFind ) )
            {
                return assembly;
            }
        }
    }
    vtkErrorMacro( << "The prop to find is not attached to the renderer in any way." << endl );
    return 0;
}

// Find a renderer that is both in the current render window and displaying stuff for the current widget
int vtkMulti3DWidget::FindPokedRenderer( vtkRenderWindowInteractor * interactor, int x, int y )
{
    vtkRendererCollection * rc = interactor->GetRenderWindow()->GetRenderers();

    for( int winRen = 0; winRen < rc->GetNumberOfItems(); ++winRen )
    {
        vtkRenderer * winRenderer = vtkRenderer::SafeDownCast( rc->GetItemAsObject( winRen ) );
        for( int widgetRen = 0; widgetRen < this->Renderers.size(); ++widgetRen )
        {
            if( winRenderer == this->Renderers[ widgetRen ] &&
                    winRenderer->GetInteractive() &&
                    winRenderer->IsInViewport( x, y ) )
                return widgetRen;
        }
    }

    return -1;
}


void vtkMulti3DWidget::ProcessEvents( vtkObject * caller, unsigned long event, void * calldata )
{
    // find which interactor generated the event
    vtkRenderWindowInteractor * interactor = vtkRenderWindowInteractor::SafeDownCast( caller );
    if( interactor )
    {
        int i = 0;
        this->CurrentInteractorIndex = -1;
        for( InteractorVec::iterator it = Interactors.begin(); it != Interactors.end(); ++it, ++i )
        {
            if( *it == interactor )
            {
                this->CurrentInteractorIndex = i;
                break;
            }
        }

        if( this->CurrentInteractorIndex != -1 )
        {
            // See if we should treat the event
			bool ctrl = (bool)interactor->GetControlKey();
			bool shift = (bool)interactor->GetShiftKey();
			bool noModif = !ctrl && !shift;
			if( ( ctrl && this->ControlDisables ) || ( shift && this->ShiftDisables ) || ( noModif && this->NoModifierDisables ))
            {
                return;
            }

            // Try to find the renderer where the event happened
            int * pos = interactor->GetEventPosition();
            this->CurrentRendererIndex = FindPokedRenderer( interactor, pos[0], pos[1] );
            if( this->CurrentRendererIndex == -1 )
                return;

            // Dispatch events to subclasses.
            this->LastButtonPressed = vtkMulti3DWidget::NO_BUTTON;

            switch ( event )
            {
            case vtkCommand::LeftButtonPressEvent:
                this->LastButtonPressed = vtkMulti3DWidget::LEFT_BUTTON;
                this->OnLeftButtonDown( );
                break;
            case vtkCommand::LeftButtonReleaseEvent:
                this->LastButtonPressed = vtkMulti3DWidget::LEFT_BUTTON;
                this->OnLeftButtonUp( );
                break;
            case vtkCommand::MiddleButtonPressEvent:
                this->LastButtonPressed = vtkMulti3DWidget::MIDDLE_BUTTON;
                this->OnMiddleButtonDown( );
                break;
            case vtkCommand::MiddleButtonReleaseEvent:
                this->LastButtonPressed = vtkMulti3DWidget::MIDDLE_BUTTON;
                this->OnMiddleButtonUp( );
                break;
            case vtkCommand::RightButtonPressEvent:
                this->LastButtonPressed = vtkMulti3DWidget::RIGHT_BUTTON;
                this->OnRightButtonDown( );
                break;
            case vtkCommand::RightButtonReleaseEvent:
                this->LastButtonPressed = vtkMulti3DWidget::RIGHT_BUTTON;
                this->OnRightButtonUp( );
                break;
            case vtkCommand::MouseMoveEvent:
                this->OnMouseMove( );
                break;
            case vtkCommand::MouseWheelForwardEvent:
                this->OnMouseWheelForward();
                break;
            case vtkCommand::MouseWheelBackwardEvent:
                this->OnMouseWheelBackward();
                break;
            }
        }
    }
}


void vtkMulti3DWidget::AdjustBounds(double bounds[6], double newBounds[6], double center[3])
{
    center[0] = (bounds[0] + bounds[1])/2.0;
    center[1] = (bounds[2] + bounds[3])/2.0;
    center[2] = (bounds[4] + bounds[5])/2.0;

    newBounds[0] = center[0] + this->PlaceFactor*(bounds[0]-center[0]);
    newBounds[1] = center[0] + this->PlaceFactor*(bounds[1]-center[0]);
    newBounds[2] = center[1] + this->PlaceFactor*(bounds[2]-center[1]);
    newBounds[3] = center[1] + this->PlaceFactor*(bounds[3]-center[1]);
    newBounds[4] = center[2] + this->PlaceFactor*(bounds[4]-center[2]);
    newBounds[5] = center[2] + this->PlaceFactor*(bounds[5]-center[2]);
}


int vtkMulti3DWidget::GetRendererIndex( vtkRenderer * ren )
{
    RendererVec::iterator it = this->Renderers.begin();
    int index = 0;
    for( ; it != this->Renderers.end(); ++it, ++index )
    {
        if( *it == ren )
        {
            return index;
        }
    }
    return -1;
}

//----------------------------------------------------------------------------
// Description:
// Transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtkMulti3DWidget::ComputeDisplayToWorld( unsigned int index, double x, double y, double z, double worldPt[4] )
{
    if( index >= this->Renderers.size() )
    {
        vtkErrorMacro( << "index >= number of renderers." << endl );
        return;
    }

    if ( !this->Renderers[ index ] )
    {
        return;
    }

    this->Renderers[ index ]->SetDisplayPoint(x, y, z);
    this->Renderers[ index ]->DisplayToWorld();
    this->Renderers[ index ]->GetWorldPoint( worldPt );
    if (worldPt[3])
    {
        worldPt[0] /= worldPt[3];
        worldPt[1] /= worldPt[3];
        worldPt[2] /= worldPt[3];
        worldPt[3] = 1.0;
    }
}


// Description:
// Transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtkMulti3DWidget::ComputeWorldToDisplay( unsigned int index, double x, double y, double z, double displayPt[3] )
{
    if( index >= this->Renderers.size() )
    {
        vtkErrorMacro( << "index >= number of renderers." << endl );
        return;
    }

    if ( !this->Renderers[ index ] )
    {
        return;
    }

    this->Renderers[ index ]->SetWorldPoint(x, y, z, 1.0);
    this->Renderers[ index ]->WorldToDisplay();
    this->Renderers[ index ]->GetDisplayPoint(displayPt);
}


void vtkMulti3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);
}

