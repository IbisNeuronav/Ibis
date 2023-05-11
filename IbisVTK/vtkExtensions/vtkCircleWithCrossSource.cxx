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

#include "vtkCircleWithCrossSource.h"

#include <math.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

vtkStandardNewMacro( vtkCircleWithCrossSource );

vtkCircleWithCrossSource::vtkCircleWithCrossSource()
{
    this->Radius      = 1.0;
    this->Resolution  = 0;
    this->Center[ 0 ] = 0.0;
    this->Center[ 1 ] = 0.0;
    this->Center[ 2 ] = 0.0;
    this->SetNumberOfInputPorts( 0 );
}

int vtkCircleWithCrossSource::RequestData( vtkInformation * vtkNotUsed( request ),
                                           vtkInformationVector ** vtkNotUsed( inputVector ),
                                           vtkInformationVector * outputVector )
{
    // get the info object
    vtkInformation * outInfo = outputVector->GetInformationObject( 0 );
    // get the ouptut
    vtkPolyData * output = vtkPolyData::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

    // Generate points
    int numLines          = (int)( pow( 2, 2 + this->Resolution ) );
    int numPts            = numLines;
    int increment         = (int)( pow( 2, this->Resolution ) );
    vtkPoints * newPoints = vtkPoints::New();
    newPoints->SetNumberOfPoints( numPts );
    if( this->Resolution > 0 )
    {
        double delta = ( 360.0 / numPts ) * ( vtkMath::Pi() / 180.0 );
        double newPoint[ 3 ];
        for( int i = 0; i < numPts; i++ )
        {
            newPoint[ 0 ] = Center[ 0 ] + this->Radius * sin( delta * i );
            newPoint[ 1 ] = Center[ 1 ] + this->Radius * cos( delta * i );
            newPoint[ 2 ] = Center[ 2 ] + 0.0;
            newPoints->SetPoint( i, newPoint );
        }
    }

    //  Generate circle lines
    vtkCellArray * newLines;
    newLines = vtkCellArray::New();
    int size = newLines->EstimateSize( 1, numLines + 1 ) + newLines->EstimateSize( 2, 2 );
    newLines->Allocate( size );
    newLines->InsertNextCell( numPts + 1 );
    int i;
    for( i = 0; i < numPts; i++ )
    {
        newLines->InsertCellPoint( i );
    }
    newLines->InsertCellPoint( 0 );

    // Generate cross lines
    newLines->InsertNextCell( 2 );
    newLines->InsertCellPoint( 0 );
    newLines->InsertCellPoint( 2 * increment );
    newLines->InsertNextCell( 2 );
    newLines->InsertCellPoint( 3 * increment );
    newLines->InsertCellPoint( increment );

    // Update ourselves and release memory
    this->GetOutput()->SetPoints( newPoints );
    newPoints->Delete();

    output->SetLines( newLines );
    newLines->Delete();
    return 1;
}

void vtkCircleWithCrossSource::PrintSelf( ostream & os, vtkIndent indent )
{
    this->Superclass::PrintSelf( os, indent );

    os << indent << "Resolution: " << this->Resolution << "\n";

    os << indent << "Radius: " << this->Radius << "\n";
}
