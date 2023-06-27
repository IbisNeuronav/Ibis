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

// .NAME vtkSimpleProp3D - simple prop 3d that can be used with custom mappers
//
//
// .SECTION Description
// ( vtkSimpleMapper3D and its subclasses).
//
// .SECTION see also
// vtkSimpleMapper3D vtkProp3D

#ifndef __vtkSimpleProp3D_h
#define __vtkSimpleProp3D_h

#include <vtkProp3D.h>

class vtkWindow;
class vtkSimpleMapper3D;

// class VTK_RENDERING_EXPORT vtkSimpleProp3D : public vtkProp3D
class vtkSimpleProp3D : public vtkProp3D
{
public:
    vtkTypeMacro( vtkSimpleProp3D, vtkProp3D );
    void PrintSelf( ostream & os, vtkIndent indent ) override;

    // Description:
    // Creates a Volume with the following defaults: origin(0,0,0)
    // position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
    // orientation=(0,0,0).
    static vtkSimpleProp3D * New();

    // Description:
    // Set/Get the volume mapper.
    void SetMapper( vtkSimpleMapper3D * mapper );
    vtkGetObjectMacro( Mapper, vtkSimpleMapper3D );

    // Description:
    // Update the volume rendering pipeline by updating the volume mapper
    void Update();

    // Description:
    // Get the bounds - either all six at once
    // (xmin, xmax, ymin, ymax, zmin, zmax) or one at a time.
    double * GetBounds() override;
    void GetBounds( double bounds[6] ) { this->vtkProp3D::GetBounds( bounds ); };
    double GetMinXBound();
    double GetMaxXBound();
    double GetMinYBound();
    double GetMaxYBound();
    double GetMinZBound();
    double GetMaxZBound();

    // Description:
    // Return the mtime of anything that would cause the rendered image to
    // appear differently. Usually this involves checking the mtime of the
    // prop plus anything else it depends on such as properties, mappers,
    // etc.
    vtkMTimeType GetRedrawMTime() override;

    // Description:
    // Shallow copy of this vtkSimpleProp3D. Overloads the virtual vtkProp method.
    void ShallowCopy( vtkProp * prop ) override;

    // Description:
    // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
    // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
    // Pass all rendering stages on to the mapper.
    // There are four key render methods in vtk and they correspond
    // to four different points in the rendering cycle. Any given
    // prop may implement one or more of these methods.
    // The first method is intended for rendering all opaque geometry. The
    // second method is intended for rendering all translucent polygonal
    // geometry. The third one is intended for rendering all translucent
    // volumetric geometry. Most of the volume rendering mappers draw their
    // results during this third method.
    // The last method is to render any 2D annotation or overlays.
    // Each of these methods return an integer value indicating
    // whether or not this render method was applied to this data.
    virtual int RenderOpaqueGeometry( vtkViewport * ) override;
    virtual int HasTranslucentPolygonalGeometry() override;
    virtual int RenderTranslucentPolygonalGeometry( vtkViewport * ) override;
    virtual int RenderVolumetricGeometry( vtkViewport * ) override;
    virtual int RenderOverlay( vtkViewport * ) override;

    // Description:
    // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
    // Release any graphics resources that are being consumed by this volume.
    // The parameter window could be used to determine which graphic
    // resources to release.
    void ReleaseGraphicsResources( vtkWindow * ) override;

protected:
    vtkSimpleProp3D();
    ~vtkSimpleProp3D();

    vtkSimpleMapper3D * Mapper;

private:
    vtkSimpleProp3D( const vtkSimpleProp3D & );  // Not implemented.
    void operator=( const vtkSimpleProp3D & );   // Not implemented.
};

#endif
