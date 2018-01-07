/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkXFMReader - Read XFM files into vtkMatrix4x4
// .SECTION Description
// This is a preliminary version that reads only Single linear transformation.

#ifndef TAG_VTKXFMREADER_H
#define TAG_VTKXFMREADER_H

#include "vtkObject.h"


class vtkMatrix4x4;

class vtkXFMReader : public vtkObject
{

public:

    static vtkXFMReader * New();
    vtkTypeMacro(vtkXFMReader,vtkObject);

    vtkSetStringMacro(FileName);
    virtual void Update();
    virtual void SetMatrix(vtkMatrix4x4 *mat);
    virtual int CanReadFile( const char * fname );
    
    void PrintSelf(ostream &os, vtkIndent indent) override;

//BTX
protected:

    char * FileName;
    vtkMatrix4x4 *Matrix;

    vtkXFMReader();
    ~vtkXFMReader();

private:
    vtkXFMReader(const vtkXFMReader&);      // Not implemented.
    void operator=(const vtkXFMReader&);  // Not implemented.
//ETX
};
#endif //TAG_VTKXFMREADER_H




