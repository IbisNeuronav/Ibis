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

// .NAME vtkMNIOBJReader - read MNI .obj files
// .SECTION Description
// vtkMNIOBJReader is a source object that reads MNI .obj
// files. The output of this source object is polygonal data.
// .SECTION See Also

#ifndef VTKMNIOBJREADER_H
#define VTKMNIOBJREADER_H

#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

class vtkProperty;
class vtkCellArray;

class vtkMNIOBJReader : public vtkPolyDataAlgorithm
{
public:
    static vtkMNIOBJReader * New();
    vtkTypeMacro( vtkMNIOBJReader, vtkPolyDataAlgorithm );
    virtual void PrintSelf( ostream & os, vtkIndent indent ) override;

    virtual int CanReadFile( const char * fname );

    // Description:
    // Specify file name of MNI .obj file.
    vtkSetStringMacro( FileName );
    vtkGetStringMacro( FileName );

    // Description:
    // Return properties of MNI .obj file
    vtkProperty * GetProperty();

    vtkSetMacro( UseAlpha, bool );

protected:
    virtual int ReadFile( vtkPolyData * output );
    void ReadLines( FILE * in, vtkPolyData * output );
    void ReadPolygons( FILE * in, vtkPolyData * output );
    void ReadPoints( FILE * in, vtkPolyData * output );
    void ReadColors( FILE * in, vtkPolyData * output );
    void ReadItems( FILE * in, vtkCellArray * indexCells );

    vtkMNIOBJReader();
    ~vtkMNIOBJReader();
    vtkSmartPointer<vtkProperty> Property;

    virtual int RequestData( vtkInformation * request, vtkInformationVector ** inputVector,
                             vtkInformationVector * outputVector ) override;

    char * FileName;

    int NbPoints;
    int NbItems;
    bool UseAlpha;

private:
    vtkMNIOBJReader( const vtkMNIOBJReader & );  // Not implemented.
    void operator=( const vtkMNIOBJReader & );   // Not implemented.
};

#endif
