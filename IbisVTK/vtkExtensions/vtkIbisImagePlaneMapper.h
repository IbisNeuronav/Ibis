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

// .NAME vtkIbisImagePlaneMapper - Simple mapper to display an input image using a custom shader

// .SECTION Description

// .SECTION see also
// vtkSimpleMapper3D vtkSimpleProp3D

#ifndef __vtkIbisImagePlaneMapper_h
#define __vtkIbisImagePlaneMapper_h

#include <vtkSimpleMapper3D.h>
#include <vtkTimeStamp.h>

class vtkRenderer;
class vtkWindow;
class vtkRenderWindow;
class vtkImageData;
class GlslShader;

class vtkIbisImagePlaneMapper : public vtkSimpleMapper3D
{
public:
    vtkTypeMacro( vtkIbisImagePlaneMapper, vtkSimpleMapper3D );
    static vtkIbisImagePlaneMapper * New();

    // Description:
    // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
    // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
    // Do the actual rendering
    virtual int HasTranslucentPolygonalGeometry() override { return 1; }
    virtual int RenderTranslucentPolygonalGeometry( vtkRenderer * ren, vtkSimpleProp3D * prop ) override;
    void ReleaseGraphicsResources( vtkWindow * ) override;
    virtual unsigned long int GetRedrawMTime() override;

    // Description:
    // Return bounding box (array of six doubles) of data expressed as
    // (xmin,xmax, ymin,ymax, zmin,zmax).
    virtual double * GetBounds() override;
    virtual void GetBounds( double bounds[6] ) override { this->vtkAbstractMapper3D::GetBounds( bounds ); };

    void SetGlobalOpacity( double opacity );
    void SetImageCenter( double x, double y );
    void SetLensDistortion( double dist );
    void SetUseTransparency( bool use );
    void SetUseGradient( bool use );
    void SetShowMask( bool show );
    void SetTransparencyPosition( double x, double y );
    void SetTransparencyRadius( double min, double max );
    void SetSaturation( double s );
    void SetBrightness( double b );

protected:
    vtkIbisImagePlaneMapper();
    ~vtkIbisImagePlaneMapper();

    virtual int FillInputPortInformation( int, vtkInformation * ) override;
    void UpdateTexture();
    bool UpdateShader();
    vtkImageData * GetInput();

    vtkImageData * LastInput;
    vtkTimeStamp LastInputTimeStamp;
    unsigned TextureId;
    GlslShader * Shader;
    double GlobalOpacity;
    double ImageCenter[2];
    double LensDistortion;
    bool UseTransparency;
    bool UseGradient;
    bool ShowMask;
    double TransparencyPosition[2];
    double TransparencyRadius[2];  // min - max
    double Saturation;
    double Brightness;

private:
    vtkIbisImagePlaneMapper( const vtkIbisImagePlaneMapper & );  // Not implemented.
    void operator=( const vtkIbisImagePlaneMapper & );           // Not implemented.
};

#endif
