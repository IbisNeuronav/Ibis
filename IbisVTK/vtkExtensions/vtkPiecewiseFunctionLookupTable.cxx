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

#include "vtkPiecewiseFunctionLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"

vtkStandardNewMacro(vtkPiecewiseFunctionLookupTable);


// Construct with range=(0,1); and hsv ranges set up for rainbow color table
// (from red to blue).
vtkPiecewiseFunctionLookupTable::vtkPiecewiseFunctionLookupTable()
{
    this->ColorFunction = vtkColorTransferFunction::New();
    this->AlphaFunction = vtkPiecewiseFunction::New();
    this->IntensityFactor = 1.0;
}


vtkPiecewiseFunctionLookupTable::~vtkPiecewiseFunctionLookupTable()
{
    this->ColorFunction->Delete();
    this->AlphaFunction->Delete();
}


void vtkPiecewiseFunctionLookupTable::SetIntensityFactor( double f )
{
    if( IntensityFactor != f )
    {
        IntensityFactor = f;
        Modified();
    }
}

// Force the lookup table to rebuild
void vtkPiecewiseFunctionLookupTable::ForceBuild()
{
    int number = this->NumberOfColors;
    float step = (float)1.0 / (number - 1);
    float x = 0.0;
    float r, g, b, a;
    for( int i = 0; i < number; i++ )
    {
        r = IntensityFactor * this->ColorFunction->GetRedValue( x );
        g = IntensityFactor * this->ColorFunction->GetGreenValue( x );
        b = IntensityFactor * this->ColorFunction->GetBlueValue( x );
        a = IntensityFactor * this->AlphaFunction->GetValue( x );
        this->SetTableValue( i, r, g, b, a );
        x += step;
    }

    this->BuildTime.Modified();
}


void vtkPiecewiseFunctionLookupTable::AddColorPoint( float value, float r, float g, float b )
{
    this->ColorFunction->AddRGBPoint( value, r, g, b );
    Modified();
}


void vtkPiecewiseFunctionLookupTable::AddAlphaPoint( float value, float alpha )
{
    this->AlphaFunction->AddPoint( value, alpha );
    Modified();
}

void vtkPiecewiseFunctionLookupTable::RemoveAllPoints()
{
    this->ColorFunction->RemoveAllPoints();
    this->AlphaFunction->RemoveAllPoints();
    Modified();
}

void vtkPiecewiseFunctionLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);

    this->ColorFunction->PrintSelf( os, indent );
    this->AlphaFunction->PrintSelf( os, indent );
}


