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

#include "vtkMultiTextureMapToPlane.h"

#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>

vtkStandardNewMacro( vtkMultiTextureMapToPlane );

// Construct with s,t range=(0,1) and automatic plane generation turned on.
vtkMultiTextureMapToPlane::vtkMultiTextureMapToPlane()
{
    // all zero - indicates that using normal is preferred and automatic is off
    this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
    this->Point1[0] = this->Point1[1] = this->Point1[2] = 0.0;
    this->Point2[0] = this->Point2[1] = this->Point2[2] = 0.0;
}

void vtkMultiTextureMapToPlane::AddTCoordSet( const char * name, double xOffset, double yOffset )
{
    RangeInfo info;
    info.name    = name;
    info.xOffset = xOffset;
    info.yOffset = yOffset;
    Ranges.push_back( info );
}

void vtkMultiTextureMapToPlane::ClearTCoordSets() { Ranges.clear(); }

int vtkMultiTextureMapToPlane::RequestData( vtkInformation * vtkNotUsed( request ), vtkInformationVector ** inputVector,
                                            vtkInformationVector * outputVector )
{
    // get the info objects
    vtkInformation * inInfo  = inputVector[0]->GetInformationObject( 0 );
    vtkInformation * outInfo = outputVector->GetInformationObject( 0 );

    // get the input and output
    vtkDataSet * input  = vtkDataSet::SafeDownCast( inInfo->Get( vtkDataObject::DATA_OBJECT() ) );
    vtkDataSet * output = vtkDataSet::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

    vtkDebugMacro( << "Generating texture coordinates!" );

    // First, copy the input to the output, except for Texture coords
    output->CopyStructure( input );
    output->GetPointData()->CopyTCoordsOff();
    output->GetPointData()->PassData( input->GetPointData() );
    output->GetCellData()->PassData( input->GetCellData() );

    int nbVertices = input->GetNumberOfPoints();

    // compute axes orientation vectors
    double sAxis[3];
    double tAxis[3];
    for( int i = 0; i < 3; i++ )
    {
        sAxis[i] = this->Point1[i] - this->Origin[i];
        tAxis[i] = this->Point2[i] - this->Origin[i];
    }

    // compute len^2 of axes orientation vectors
    double sDenom = vtkMath::Dot( sAxis, sAxis );
    double tDenom = vtkMath::Dot( tAxis, tAxis );

    if( sDenom == 0.0 || tDenom == 0.0 )
    {
        vtkErrorMacro( "Bad plane definition" );
        sDenom = tDenom = 1.0;
    }

    int nbSteps          = nbVertices * (int)Ranges.size();
    int progressInterval = nbSteps / 20 + 1;

    // For each set of tex coords to generate
    for( int texIndex = 0; texIndex < Ranges.size(); ++texIndex )
    {
        //  Allocate texture coord data
        vtkFloatArray * newTCoords = vtkFloatArray::New();
        newTCoords->SetName( Ranges[texIndex].name.c_str() );
        newTCoords->SetNumberOfComponents( 2 );
        newTCoords->SetNumberOfTuples( nbVertices );

        double xRange = 1 - 2 * Ranges[texIndex].xOffset;
        double xMin   = Ranges[texIndex].xOffset;

        double yRange = 1 - 2 * Ranges[texIndex].yOffset;
        double yMin   = Ranges[texIndex].yOffset;

        // compute s-t coordinates for each vertex
        int abort = 0;
        for( int vertIndex = 0; vertIndex < nbVertices && !abort; ++vertIndex )
        {
            // Update progress if needed
            int step = texIndex * nbVertices + vertIndex;
            if( step % progressInterval == 0 )
            {
                double progress = (double)step / nbSteps;
                this->UpdateProgress( progress );
                abort = this->GetAbortExecute();
            }

            // axis = current_vertex - origin
            double p[3];
            output->GetPoint( vertIndex, p );
            double axis[3];
            for( int i = 0; i < 3; ++i )
            {
                axis[i] = p[i] - this->Origin[i];
            }

            double tcoords[2] = { 0.0, 0.0 };

            // s-coordinate = dot( sAxis, axis ) / len(saxis)^2 = |axis| / |sAxis| * cos(angle(axis,sAxis))
            double num = sAxis[0] * axis[0] + sAxis[1] * axis[1] + sAxis[2] * axis[2];
            tcoords[0] = num / sDenom;
            tcoords[0] = tcoords[0] * xRange + xMin;

            // t-coordinate = dot( tAxis, axis ) / len(tAxis)^2 = |axis| / |tAxis| * cos(angle(axis,tAxis))
            num        = tAxis[0] * axis[0] + tAxis[1] * axis[1] + tAxis[2] * axis[2];
            tcoords[1] = num / tDenom;
            tcoords[1] = tcoords[1] * yRange + yMin;

            newTCoords->SetTuple( vertIndex, tcoords );
        }

        output->GetPointData()->AddArray( newTCoords );
        newTCoords->Delete();
    }

    return 1;
}

void vtkMultiTextureMapToPlane::PrintSelf( ostream & os, vtkIndent indent )
{
    this->Superclass::PrintSelf( os, indent );

    os << indent << "Origin: (" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2] << " )\n";

    os << indent << "Axis Point 1: (" << this->Point1[0] << ", " << this->Point1[1] << ", " << this->Point1[2]
       << " )\n";

    os << indent << "Axis Point 2: (" << this->Point2[0] << ", " << this->Point2[1] << ", " << this->Point2[2]
       << " )\n";

    os << indent << "Ranges: \n";
    vtkIndent rangesIndent = indent.GetNextIndent();
    for( int i = 0; i < Ranges.size(); ++i )
    {
        os << rangesIndent << "name: " << Ranges[i].name << "\n";
        os << rangesIndent << "Offsets: "
           << "( " << Ranges[i].xOffset << ", " << Ranges[i].yOffset << " )\n";
    }
}
