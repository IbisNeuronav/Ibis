/*=========================================================================

  Program:   Visualization Toolkit Bic Extension
  Module:    $RCSfile: vtkMNIOBJWriter.cxx,v $
  Language:  C++
  Date:      $Date: 2010-05-10 19:47:01 $
  Version:   $Revision: 1.1 $

  Copyright (c) 2007-2010  IPL, BIC, MNI, McGill, Sean Jy-Shyang Chen
  All rights reserved.

=========================================================================*/

#include "vtkMNIOBJWriter.h"

#include <fstream>
#include <ostream>

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro( vtkMNIOBJWriter, "$Revision: 1.1 $" );
vtkStandardNewMacro( vtkMNIOBJWriter );

// Description:
// Instantiate object with NULL filename.
vtkMNIOBJWriter::vtkMNIOBJWriter()
{
    this->FileName = NULL;
    this->Property = vtkProperty::New();
    this->NbPoints = 0;
}

vtkMNIOBJWriter::vtkMNIOBJWriter( char * fname, bool aLine )
{
    this->FileName = fname;
    this->Property = vtkProperty::New();
    this->NbPoints = 0;
}

vtkMNIOBJWriter::~vtkMNIOBJWriter()
{
    if( this->FileName )
    {
        delete[] this->FileName;
        this->FileName = NULL;
        this->Property->Delete();
    }
}

void vtkMNIOBJWriter::Write( vtkPolyData * output )
{
    this->SetOutput( output );
    Write();
}

void vtkMNIOBJWriter::Write()
{
    if( this->GetOutput()->GetNumberOfCells() == 0 || this->GetOutput()->GetNumberOfPoints() == 0 )
    {
        vtkErrorMacro( << "No data. Empty Data Object." );
        return;
    }

    if( !this->FileName )
    {
        vtkErrorMacro( << "A FileName must be specified." );
        return;
    }

    std::string fileName = this->FileName;

    std::ofstream out;
    out.open( fileName.data(), ios::out | ios::trunc );
    if( out.is_open() )
    {
        if( isALine )
        {
            std::cout << "!!!!! MNI line object writing has not been tested. Use at your own peril. !!!!!" << std::endl;
            out << "L ";
            WriteLines( out );
        }
        else
        {
            out << "P ";
            WritePolygons( out );
        }

        out.close();
    }
    else
    {
        cout << "Unable to open file";
    }
}

void vtkMNIOBJWriter::WriteLines( std::ostream & out )
{
    float thickness = 1;
    out << thickness << std::endl;

    WritePoints( out );

    out << NbItems << std::endl;

    WriteColors( out );

    out << std::endl;

    WriteItems( out );
}

void vtkMNIOBJWriter::WritePolygons( std::ostream & out )
{
    out << this->Property->GetAmbient() << " ";
    out << this->Property->GetDiffuse() << " ";
    out << this->Property->GetSpecular() << " ";
    out << this->Property->GetSpecularPower() << " ";
    out << this->Property->GetOpacity() << " ";

    WritePoints( out );

    out << std::endl;

    vtkDoubleArray * normals = vtkDoubleArray::SafeDownCast( this->GetOutput()->GetPointData()->GetNormals() );
    double norm[ 3 ];
    for( int j = 0; j < NbPoints; j++ )
    {
        normals->GetTuple( j, norm );
        out << norm[ 0 ] << " " << norm[ 1 ] << " " << norm[ 2 ] << std::endl;
    }

    out << std::endl;

    out << this->GetOutput()->GetPolys()->GetNumberOfCells() << std::endl;

    WriteColors( out );

    out << std::endl;

    WriteItems( out );

    out << std::endl;
}

void vtkMNIOBJWriter::WritePoints( std::ostream & out )
{
    // fill point coordinates in points
    double thePoint[ 3 ];
    vtkPoints * points = this->GetOutput()->GetPoints();
    NbPoints           = points->GetNumberOfPoints();

    out << NbPoints << std::endl;

    for( int i = 0; i < NbPoints; i++ )
    {
        points->GetPoint( i, thePoint );
        out << thePoint[ 0 ] << " " << thePoint[ 1 ] << " " << thePoint[ 2 ] << std::endl;
    }
}

void vtkMNIOBJWriter::WriteColors( std::ostream & out )
{
    vtkUnsignedCharArray * charColors =
        vtkUnsignedCharArray::SafeDownCast( this->GetOutput()->GetPointData()->GetScalars() );

    if( charColors == 0 )  // if there are no colours than just get whatever is in property
    {
        out << "0 ";

        double rgba[ 4 ];
        this->Property->GetColor( rgba );
        out << rgba[ 0 ] << " " << rgba[ 1 ] << " " << rgba[ 2 ] << " 1" << std::endl;
    }
    else if( charColors->GetNumberOfComponents() == 4 )  // if there are colors, then write them out
    {
        int nTuples = charColors->GetNumberOfTuples();

        if( nTuples != NbPoints ) return;
        out << "2 ";

        double * rgba;
        for( int k = 0; k < NbPoints; k++ )
        {
            rgba = charColors->GetTuple4( k );
            out << rgba[ 0 ] / 255 << " " << rgba[ 1 ] / 255 << " " << rgba[ 2 ] / 255 << " " << rgba[ 3 ] / 255
                << std::endl;
        }
    }
}

void vtkMNIOBJWriter::WriteItems( std::ostream & out )
{
    vtkCellArray * indexCells = this->GetOutput()->GetPolys();
    std::cout << "Cells = " << indexCells->GetNumberOfCells() << std::endl;

    vtkIdType numPoints     = 0;
    vtkIdType nextNumPoints = 0;
    vtkIdType * pointList;

    // Write out end-index of the point list for polygons
    indexCells->InitTraversal();
    for( int s = 0; s < indexCells->GetNumberOfCells(); s++ )
    {
        indexCells->GetNextCell( nextNumPoints, pointList );
        numPoints += nextNumPoints;

        out << numPoints << " ";

        if( ( s + 1 ) % 8 == 0 )
        {
            out << std::endl;
        }
    }

    out << std::endl << std::endl;

    // Write out the list of points that makes up the polygons
    int eightCount = 0;
    indexCells->InitTraversal();
    for( int m = 0; m < indexCells->GetNumberOfCells(); m++ )
    {
        indexCells->GetNextCell( numPoints, pointList );

        for( int p = 0; p < numPoints; p++ )
        {
            out << pointList[ p ] << " ";
            ++eightCount;

            if( eightCount % 8 == 0 )
            {
                out << std::endl;
            }
        }
    }
}

void vtkMNIOBJWriter::PrintSelf( std::ostream & os, vtkIndent indent )
{
    this->Superclass::PrintSelf( os, indent );

    os << indent << "File Name: " << ( this->FileName ? this->FileName : "(none)" ) << "\n";
}
