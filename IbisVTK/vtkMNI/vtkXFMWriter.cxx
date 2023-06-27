/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkXFMWriter.h"

#include <vtkMatrix4x4.h>

#include <fstream>

using namespace std;

vtkXFMWriter * vtkXFMWriter::New() { return new vtkXFMWriter(); }

vtkXFMWriter::vtkXFMWriter()
{
    FileName = 0;
    Matrix   = 0;
}

vtkXFMWriter::~vtkXFMWriter()
{
    if( FileName ) delete[] FileName;
    if( Matrix ) Matrix->Delete();
}

void vtkXFMWriter::Write()
{
    // try to open the file for writing
    if( !FileName )
    {
        vtkErrorMacro( << "No valid filename specified." << endl );
        return;
    }

    if( !Matrix )
    {
        vtkErrorMacro( << "No valid matrix." << endl );
        return;
    }

    ofstream f( FileName );
    if( !f.is_open() )
    {
        vtkErrorMacro( << "Could not open file " << FileName << endl );
        return;
    }

    // Write the header
    f << "MNI Transform File" << endl;
    f << "% Single linear transformation.\n" << endl;
    f << "Transform_Type = Linear;" << endl;
    f << "Linear_Transform =" << endl;
    f << Matrix->GetElement( 0, 0 ) << " " << Matrix->GetElement( 0, 1 ) << " " << Matrix->GetElement( 0, 2 ) << " "
      << Matrix->GetElement( 0, 3 ) << endl;
    f << Matrix->GetElement( 1, 0 ) << " " << Matrix->GetElement( 1, 1 ) << " " << Matrix->GetElement( 1, 2 ) << " "
      << Matrix->GetElement( 1, 3 ) << endl;
    f << Matrix->GetElement( 2, 0 ) << " " << Matrix->GetElement( 2, 1 ) << " " << Matrix->GetElement( 2, 2 ) << " "
      << Matrix->GetElement( 2, 3 ) << ";" << endl;
    f.close();
}

void vtkXFMWriter::SetMatrix( vtkMatrix4x4 * mat )
{
    if( !Matrix )
    {
        Matrix = vtkMatrix4x4::New();
    }
    Matrix->DeepCopy( mat );
}

void vtkXFMWriter::PrintSelf( ostream & os, vtkIndent indent )
{
    os << indent << "vtkXFMWriter begin: " << endl;
    if( FileName )
        os << indent << FileName << endl;
    else
        os << indent << "FileName not set." << endl;
    if( Matrix )
        Matrix->PrintSelf( os, indent );
    else
        os << indent << "Matrix not set." << endl;
    os << indent << "vtkXFMWriter end: " << endl;
}
