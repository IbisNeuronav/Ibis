/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkXFMWriter - Write matrix in an XFM format
// .SECTION Description
// This is a preliminary version that writes only Single linear transformation.

#ifndef TAG_VTKXFMWRITER_H
#define TAG_VTKXFMWRITER_H

#include <vtkObject.h>

class vtkMatrix4x4;

class vtkXFMWriter : public vtkObject
{

public:

    static vtkXFMWriter * New();
    vtkTypeMacro(vtkXFMWriter,vtkObject);

    vtkSetStringMacro(FileName);
    
    virtual void Write();   
    virtual void SetMatrix(vtkMatrix4x4 *mat);
    
    void PrintSelf(ostream &os, vtkIndent indent) override;

protected:

    char * FileName;
    vtkMatrix4x4 *Matrix;

    vtkXFMWriter();
    virtual ~vtkXFMWriter();

private:
    
    vtkXFMWriter(const vtkXFMWriter&);    // Not implemented.
    void operator=(const vtkXFMWriter&);  // Not implemented.

};

#endif //TAG_VTKXFMWRITER_H




