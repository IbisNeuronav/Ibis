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

#ifndef VTKINTERACTORSTYLEIMAGE2_H
#define VTKINTERACTORSTYLEIMAGE2_H

#include "vtkInteractorStyleImage.h"

class vtkInteractorStyleImage2 : public vtkInteractorStyleImage
{

public:

    static vtkInteractorStyleImage2 *New();

    vtkInteractorStyleImage2();
    ~vtkInteractorStyleImage2();

    vtkTypeMacro(vtkInteractorStyleImage2,vtkInteractorStyleImage);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Reimplemented to prevent rotation
    virtual void OnLeftButtonDown();

private:

    vtkInteractorStyleImage2(const vtkInteractorStyleImage2&);  //Not implemented
    void operator=(const vtkInteractorStyleImage2&);  //Not implemented

};

#endif
