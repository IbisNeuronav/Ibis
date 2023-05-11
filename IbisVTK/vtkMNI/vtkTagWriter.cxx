/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkTagWriter.h"

#include <vtkPoints.h>

#include <fstream>

using namespace std;

vtkTagWriter::vtkTagWriter()
{
    FileName                     = 0;
    this->TransformToSave[ 0 ]   = 0;
    this->ReferenceDataFile[ 0 ] = 0;
}

vtkTagWriter::~vtkTagWriter()
{
    PointsVec::iterator it = this->Volumes.begin();
    for( ; it != this->Volumes.end(); ++it )
    {
        ( *it )->UnRegister( this );
    }
}

void vtkTagWriter::Write()
{
    // make sure there is not more than 2 volumes, it does not seem to be supported by the format
    if( this->Volumes.size() > 2 )
    {
        vtkErrorMacro( << "The tag file format does not support more than 2 volumes" << endl );
        return;
    }

    // try to open the file for writing
    if( !this->FileName )
    {
        vtkErrorMacro( << "No valid filename specified." << endl );
        return;
    }

    ofstream f( this->FileName );
    if( !f.is_open() )
    {
        vtkErrorMacro( << "Could not open file " << this->FileName << endl );
        return;
    }

    // Write the header
    f << "MNI Tag Point File" << endl;
    f << "Volumes = " << this->Volumes.size() << ";" << endl;
    unsigned int i;
    for( i = 0; i < this->Volumes.size(); ++i )
    {
        f << "%Volume: " << this->VolumeNames[ i ].c_str() << endl;
    }
    f << endl;

    if( this->ReferenceDataFile[ 0 ] ) f << "%ReferenceDataFile: " << this->ReferenceDataFile << endl;
    if( this->TransformToSave[ 0 ] ) f << "%Transform: " << this->TransformToSave << endl;
    f << endl;
    // Write the points
    f << "Points = ";
    for( i = 0; i < this->PointNames.size(); ++i )
    {
        f << endl;
        for( unsigned int j = 0; j < this->Volumes.size(); ++j )
        {
            if( this->Volumes[ j ]->GetNumberOfPoints() > (vtkIdType)i )
            {
                double * point = this->Volumes[ j ]->GetPoint( i );
                f << point[ 0 ] << " " << point[ 1 ] << " " << point[ 2 ] << " ";
            }
            else
            {
                f << 0.0 << " " << 0.0 << " " << 0.0 << " ";
            }
        }
        if( this->PointNames.size() > i )
        {
            f << "\"" << this->PointNames[ i ].c_str() << "\"";
        }
        if( this->TimeStamps.size() > i )
        {
            f << " %TimeStamp " << this->TimeStamps[ i ].c_str();
        }
    }
    f << ";" << endl;

    f.close();
}

void vtkTagWriter::AddVolume( vtkPoints * volume, const char * name )
{
    if( volume )
    {
        volume->Register( this );
        this->Volumes.push_back( volume );
        if( name )
            this->VolumeNames.push_back( name );
        else
            this->VolumeNames.push_back( "data" );  // just put something
    }
}

void vtkTagWriter::SetPointNames( std::vector<std::string> & names ) { this->PointNames = names; }

void vtkTagWriter::SetTimeStamps( std::vector<std::string> & ts ) { this->TimeStamps = ts; }

void vtkTagWriter::SetTransformToSave( const char * ts )
{
    if( ts && *ts )
        strcpy( this->TransformToSave, ts );
    else
        this->TransformToSave[ 0 ] = 0;
}

void vtkTagWriter::SetReferenceDataFile( const char * ts )
{
    if( ts && *ts )
        strcpy( this->ReferenceDataFile, ts );
    else
        this->ReferenceDataFile[ 0 ] = 0;
}

void vtkTagWriter::PrintSelf( ostream & os, vtkIndent indent ) {}
