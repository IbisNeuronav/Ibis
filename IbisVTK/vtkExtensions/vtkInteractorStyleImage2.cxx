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

#include "vtkInteractorStyleImage2.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

vtkStandardNewMacro(vtkInteractorStyleImage2);

vtkInteractorStyleImage2::vtkInteractorStyleImage2()
{
}

vtkInteractorStyleImage2::~vtkInteractorStyleImage2()
{

}

void vtkInteractorStyleImage2::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf( os, indent );
}

// Reimplemented to prevent rotation
void vtkInteractorStyleImage2::OnLeftButtonDown()
{
    if( !this->Interactor->GetControlKey())
    {
        this->Superclass::OnLeftButtonDown();
    }
}
