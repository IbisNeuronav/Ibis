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

#include "vtkMultiInteractorObserver.h"

#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include "vtkObjectCallback.h"

vtkMultiInteractorObserver::vtkMultiInteractorObserver()
{
    this->Enabled = 0;

    // Listen to the key press to activate/deactivate the widgets
    this->KeyPressCallbackCommand = vtkObjectCallback<vtkMultiInteractorObserver>::New();
    this->KeyPressCallbackCommand->SetCallback( this, &vtkMultiInteractorObserver::OnChar );

    this->Priority = 0.0;

    this->KeyPressActivation      = 1;
    this->KeyPressActivationValue = 'i';

    this->CurrentInteractorIndex = -1;
}

vtkMultiInteractorObserver::~vtkMultiInteractorObserver()
{
    this->SetEnabled( 0 );
    this->KeyPressCallbackCommand->Delete();

    // remove reference from interactors
    InteractorVec::iterator itInt = this->Interactors.begin();
    while( itInt != this->Interactors.end() )
    {
        if( ( *itInt ) )
        {
            ( *itInt )->UnRegister( this );
        }
        ++itInt;
    }
}

void vtkMultiInteractorObserver::AddInteractor( vtkRenderWindowInteractor * interactor )
{
    if( interactor )
    {
        interactor->Register( this );
        interactor->AddObserver( vtkCommand::CharEvent, this->KeyPressCallbackCommand, this->Priority );
        this->Interactors.push_back( interactor );
        this->InternalAddInteractor();
        this->Modified();
    }
}

void vtkMultiInteractorObserver::RemoveInteractor( vtkRenderWindowInteractor * interactor )
{
    if( interactor )
    {
        InteractorVec::iterator it = this->Interactors.begin();
        int index                  = 0;
        for( ; it != this->Interactors.end(); ++it, ++index )
        {
            if( ( *it ) == interactor )
            {
                break;
            }
        }

        if( it != this->Interactors.end() )
        {
            ( *it )->RemoveObserver( this->KeyPressCallbackCommand );
            this->InternalRemoveInteractor( index );
            ( *it )->UnRegister( this );
            this->Interactors.erase( it );
            this->Modified();
        }
    }
}

size_t vtkMultiInteractorObserver::GetNumberOfInteractors() { return this->Interactors.size(); }

vtkRenderWindowInteractor * vtkMultiInteractorObserver::GetInteractor( unsigned int index )
{
    if( this->Interactors.size() > index )
    {
        return Interactors[index];
    }
    return 0;
}

//----------------------------------------------------------------------------
void vtkMultiInteractorObserver::StartInteraction()
{
    InteractorVec::iterator it = this->Interactors.begin();
    while( it != this->Interactors.end() )
    {
        ( *it )->GetRenderWindow()->SetDesiredUpdateRate( ( *it )->GetDesiredUpdateRate() );
        ++it;
    }
}

//----------------------------------------------------------------------------
void vtkMultiInteractorObserver::EndInteraction()
{
    InteractorVec::iterator it = this->Interactors.begin();
    while( it != this->Interactors.end() )
    {
        ( *it )->GetRenderWindow()->SetDesiredUpdateRate( ( *it )->GetStillUpdateRate() );
        ++it;
    }
}

void vtkMultiInteractorObserver::OnChar( vtkObject * caller, unsigned long eventId, void * callData )
{
    vtkRenderWindowInteractor * interactor = vtkRenderWindowInteractor::SafeDownCast( caller );

    // catch additional keycodes otherwise
    if( this->KeyPressActivation && interactor )
    {
        if( interactor->GetKeyCode() == this->KeyPressActivationValue )
        {
            if( !this->Enabled )
            {
                this->On();
            }
            else
            {
                this->Off();
            }
            this->KeyPressCallbackCommand->SetAbortFlag( 1 );
        }
    }
}

void vtkMultiInteractorObserver::PrintSelf( ostream & os, vtkIndent indent )
{
    this->Superclass::PrintSelf( os, indent );

    os << indent << "Enabled: " << this->Enabled << "\n";
    os << indent << "Priority: " << this->Priority << "\n";
    os << indent << "Key Press Activation: " << ( this->KeyPressActivation ? "On" : "Off" ) << "\n";
    os << indent << "Key Press Activation Value: " << this->KeyPressActivationValue << "\n";
}
