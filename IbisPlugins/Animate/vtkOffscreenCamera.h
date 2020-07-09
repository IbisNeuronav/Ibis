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

// .NAME vtkOffscreenCamera - OpenGL camera for offscreen rendering
// .SECTION Description
// vtkOffscreenCamera is the same as vtkOpenGLCamera, except it doesn't rely
// on the render window to obtain the image size. It unfortunately drops support
// for tile rendering and picking in the process.

#ifndef __vtkOffscreenCamera_h
#define __vtkOffscreenCamera_h

#include <vtkCamera.h>

class vtkOffscreenCamera : public vtkCamera
{

public:

    static vtkOffscreenCamera *New();
    vtkTypeMacro(vtkOffscreenCamera,vtkCamera);
    virtual void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Implement base class method.
    void Render(vtkRenderer *ren);

    void UpdateViewport(vtkRenderer *ren);

    vtkSetVector2Macro( RenderSize, int );
    vtkGetVector2Macro( RenderSize, int );

protected:  

    int RenderSize[2];

    vtkOffscreenCamera() { RenderSize[0] = 1; RenderSize[1] = 1; };
    ~vtkOffscreenCamera() {};

private:

  vtkOffscreenCamera(const vtkOffscreenCamera&);  // Not implemented.
  void operator=(const vtkOffscreenCamera&);  // Not implemented.

};

#endif
