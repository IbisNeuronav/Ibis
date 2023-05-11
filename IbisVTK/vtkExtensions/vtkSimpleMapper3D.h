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

// .NAME vtkSimpleMapper3D - Simple mapper to derive from for custom 3D rendering

// .SECTION Description
// vtkSimpleMapper3D is the base class for any mapper that implements custom 3D rendering
// for IBIS. It is meant to be attached to a vtkSimpleProp3D.

// .SECTION see also
// vtkSimpleProp3D

#ifndef __vtkSimpleMapper3D_h
#define __vtkSimpleMapper3D_h

#include <vtkAbstractMapper3D.h>

class vtkRenderer;
class vtkWindow;
class vtkSimpleProp3D;

class vtkSimpleMapper3D : public vtkAbstractMapper3D
{
public:
    vtkTypeMacro( vtkSimpleMapper3D, vtkAbstractMapper3D );

    // Description:
    // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
    // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
    // Implement any of these in subclasses to do the rendering
    virtual int RenderOpaqueGeometry( vtkRenderer * ren, vtkSimpleProp3D * prop ) { return 0; }
    virtual int HasTranslucentPolygonalGeometry() { return 0; }
    virtual int RenderTranslucentPolygonalGeometry( vtkRenderer * ren, vtkSimpleProp3D * prop ) { return 0; }
    virtual int RenderVolumetricGeometry( vtkRenderer * ren, vtkSimpleProp3D * prop ) { return 0; }
    virtual int RenderOverlay( vtkRenderer * ren, vtkSimpleProp3D * prop ) { return 0; }

    // Description:
    // Reimplement this function in subclasses to take into account
    // MTime of all dependent objects (data, filters, etc.)
    virtual unsigned long int GetRedrawMTime() { return this->GetMTime(); }

protected:
    vtkSimpleMapper3D();
    ~vtkSimpleMapper3D();

private:
    vtkSimpleMapper3D( const vtkSimpleMapper3D & );  // Not implemented.
    void operator=( const vtkSimpleMapper3D & );     // Not implemented.
};

#endif
