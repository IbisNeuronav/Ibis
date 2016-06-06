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

#include "vtkImageResliceMapper2.h"
#include "vtkObjectFactory.h"
#include "vtkImageResliceToColors.h"

vtkStandardNewMacro( vtkImageResliceMapper2 );

//----------------------------------------------------------------------------
void vtkImageResliceMapper2::SetImageReslice( vtkImageResliceToColors * reslice )
{
    if( this->ImageReslice == reslice )
    {
        return;
    }
    if( this->ImageReslice )
    {
        this->ImageReslice->Delete();
    }
    this->ImageReslice = reslice;
    if( this->ImageReslice )
    {
        this->ImageReslice->Register(this);
    }
    this->Modified();
}

void vtkImageResliceMapper2::PrintSelf( ostream & os, vtkIndent indent )
{
    this->Superclass::PrintSelf( os, indent );
}

vtkImageResliceMapper2::vtkImageResliceMapper2()
{
}
