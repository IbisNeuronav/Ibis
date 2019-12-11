/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkXFMReader.h"
#include "vtkMatrix4x4.h"
#include <string>
#include <vector>
#include <sstream>
#include "stringtools.h"

vtkXFMReader * vtkXFMReader::New()
{
    return new vtkXFMReader();
}

#define MAX_LINE_LENGTH 1024

void vtkXFMReader::Update()
{
    if( !FileName )
    {
        vtkErrorMacro(<< "No tag file name was provided" << endl);
        return;
    }
    
    if( !Matrix )
    {
        vtkErrorMacro( << "No valid matrix." << endl );
        return;
    }

    ifstream f(FileName );
    if( !f.is_open() )
    {
        vtkErrorMacro( << "Couldn't open file " << this->FileName << endl );
        return;
    }
    
    Matrix->Identity();
    // read the first line of the file to make sure it is a tag file
    char tempLine[MAX_LINE_LENGTH];
    int found = 0;
    while( !found && !f.eof() )
    {
        f.getline( tempLine, MAX_LINE_LENGTH );
        std::string line( tempLine );
        std::vector<std::string> tokens;
        Tokenize( line, tokens );
        if( tokens.size() == 2 && tokens[0] == "Linear_Transform" )
        {
            found = 1;
        }
    }

    if( !found )
    {
        vtkErrorMacro( << "Couldn't find matrix in the file." << endl );
        return;
    }
    
    int lastLine = 0;
    int i = 0, j;
    int tokenIndex;
    double elem[4];
    while( !lastLine && !f.eof() )
    {
        f.getline( tempLine, MAX_LINE_LENGTH );
        std::string line( tempLine );
        std::istringstream stream( line );
        std::vector<std::string> tokens;
        Tokenize( line, tokens );
        
        if( line.rfind( ';' ) != std::string::npos )
        {
            lastLine = 1;
        }
        if( tokens.size() == 4)
        {
            tokenIndex = 0;
            for( j = 0; j < 4; j++ )
            {
                std::istringstream stream( tokens[ tokenIndex ] );
                stream >> elem[j];
                tokenIndex++;
            }
            for (j = 0; j < 4; j++)
            {
                Matrix->SetElement(i, j, elem[j]);
            }
            i++;
        }
    }
    if( i != 3 )
    {
        vtkErrorMacro( << "Matrix not complete." << endl );
    }    
    f.close();
}

vtkXFMReader::vtkXFMReader()
{
    FileName = 0;
    Matrix = 0;
}

vtkXFMReader::~vtkXFMReader()
{
    if( this->FileName )
        delete [] this->FileName;
    if (Matrix)
    {
        Matrix->UnRegister(this);
    }
}


void vtkXFMReader::SetMatrix(vtkMatrix4x4 *mat)
{
    if (Matrix == mat || !mat)
        return;
    Matrix = mat;
    Matrix->Register(this);
}

void vtkXFMReader::PrintSelf(ostream &os, vtkIndent indent)
{
    os << indent << "vtkXFMReader begin: "<< endl;
    if (FileName)
        os << indent << FileName << endl;
    else
        os << indent << "FileName not set." << endl;
    if (Matrix)
        Matrix->PrintSelf(os, indent);
    else
        os << indent << "Matrix not set." << endl;
    os << indent << "vtkXFMReader end: "<< endl;
}

int vtkXFMReader::CanReadFile( const char * fname )
{
    ifstream f(fname );
    if( !f.is_open() )
    {
        vtkErrorMacro( << "Couldn't open file " << fname << endl );
        return 0;
    }
    
    std::string fileType;
    std::getline( f, fileType);
    f.close();
    if( fileType.find( "MNI Transform File" ) == std::string::npos )
    {
        vtkErrorMacro( << "This is not MNI XFM type file." << endl );
        return 0;
    }
    return 1;
}
