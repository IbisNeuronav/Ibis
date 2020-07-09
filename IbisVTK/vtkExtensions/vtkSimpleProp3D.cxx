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

#include "vtkSimpleProp3D.h"
#include "vtkSimpleMapper3D.h"
#include <vtkLinearTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

#include <math.h>

vtkStandardNewMacro(vtkSimpleProp3D);

// Creates a Volume with the following defaults: origin(0,0,0) 
// position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
// orientation=(0,0,0).
vtkSimpleProp3D::vtkSimpleProp3D()
{
    this->Mapper = NULL;
}

// Destruct a volume
vtkSimpleProp3D::~vtkSimpleProp3D()
{
    this->SetMapper(NULL);
}

// Shallow copy
void vtkSimpleProp3D::ShallowCopy(vtkProp *prop)
{
    vtkSimpleProp3D *v = vtkSimpleProp3D::SafeDownCast(prop);
    if ( v != NULL )
    {
        this->SetMapper(v->GetMapper());
    }

    // Now do superclass
    this->vtkProp3D::ShallowCopy(prop);
}

void vtkSimpleProp3D::SetMapper(vtkSimpleMapper3D *mapper)
{
    if( this->Mapper == mapper )
        return;
    if (this->Mapper != NULL)
        this->Mapper->UnRegister(this);
    this->Mapper = mapper;
    if (this->Mapper != NULL)
        this->Mapper->Register(this);
    this->Modified();
}

// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkSimpleProp3D::GetBounds()
{
    // get the bounds of the Mapper if we have one
    if (!this->Mapper)
    {
        return this->Bounds;
    }

    double * bounds = this->Mapper->GetBounds();
    // Check for the special case when the mapper's bounds are unknown
    if (!bounds)
    {
        return bounds;
    }

    // fill out vertices of a bounding box
    double bbox[24];
    bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
    bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
    bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
    bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
    bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
    bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
    bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
    bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];

    // make sure matrix (transform) is up-to-date
    this->ComputeMatrix();

    // and transform into actors coordinates
    double * fptr = bbox;
    for ( int n = 0; n < 8; n++)
    {
        double homogeneousPt[4] = {fptr[0], fptr[1], fptr[2], 1.0};
        this->Matrix->MultiplyPoint(homogeneousPt, homogeneousPt);
        fptr[0] = homogeneousPt[0] / homogeneousPt[3];
        fptr[1] = homogeneousPt[1] / homogeneousPt[3];
        fptr[2] = homogeneousPt[2] / homogeneousPt[3];
        fptr += 3;
    }

    // now calc the new bounds
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
    for ( int i = 0; i < 8; i++)
    {
        for ( int n = 0; n < 3; n++)
        {
            if (bbox[i*3+n] < this->Bounds[n*2])
            {
                this->Bounds[n*2] = bbox[i*3+n];
            }
            if (bbox[i*3+n] > this->Bounds[n*2+1])
            {
                this->Bounds[n*2+1] = bbox[i*3+n];
            }
        }
    }

    return this->Bounds;
}

int vtkSimpleProp3D::RenderOpaqueGeometry( vtkViewport * vp )
{
    this->Update();
    if ( !this->Mapper )
    {
        vtkErrorMacro( << "You must specify a mapper!\n" );
        return 0;
    }
    int res = this->Mapper->RenderOpaqueGeometry( static_cast<vtkRenderer *>(vp), this );
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    return res;
}

int vtkSimpleProp3D::HasTranslucentPolygonalGeometry()
{
    this->Update();
    if ( !this->Mapper )
    {
        vtkErrorMacro( << "You must specify a mapper!\n" );
        return 0;
    }
    return this->Mapper->HasTranslucentPolygonalGeometry();
}

int vtkSimpleProp3D::RenderTranslucentPolygonalGeometry( vtkViewport * vp )
{
    this->Update();
    if ( !this->Mapper )
    {
        vtkErrorMacro( << "You must specify a mapper!\n" );
        return 0;
    }
    int res = this->Mapper->RenderTranslucentPolygonalGeometry( static_cast<vtkRenderer *>(vp), this );
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    return res;
}

int vtkSimpleProp3D::RenderVolumetricGeometry( vtkViewport * vp )
{
    this->Update();
    if ( !this->Mapper )
    {
        vtkErrorMacro( << "You must specify a mapper!\n" );
        return 0;
    }
    int res = this->Mapper->RenderVolumetricGeometry( static_cast<vtkRenderer *>(vp), this );
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    return res;
}

int vtkSimpleProp3D::RenderOverlay( vtkViewport * vp )
{
    this->Update();
    if ( !this->Mapper )
    {
        vtkErrorMacro( << "You must specify a mapper!\n" );
        return 0;
    }
    int res = this->Mapper->RenderOverlay( static_cast<vtkRenderer *>(vp), this );
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    return res;
}

void vtkSimpleProp3D::ReleaseGraphicsResources(vtkWindow *win)
{
    // pass this information onto the mapper
    if (this->Mapper)
    {
        this->Mapper->ReleaseGraphicsResources(win);
    }
}

void vtkSimpleProp3D::Update()
{
    if ( this->Mapper )
    {
        this->Mapper->Update();
    }
}

vtkMTimeType vtkSimpleProp3D::GetRedrawMTime()
{
    unsigned long mTime=this->GetMTime();
    unsigned long time;

    if ( this->Mapper != NULL )
    {
        time = this->Mapper->GetRedrawMTime();
        mTime = ( time > mTime ? time : mTime );
    }

    return mTime;
}

void vtkSimpleProp3D::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);

    if( this->Mapper )
    {
        os << indent << "Mapper:\n";
        this->Mapper->PrintSelf(os,indent.GetNextIndent());
    }
    else
    {
        os << indent << "Mapper: (not defined)\n";
    }

    // make sure our bounds are up to date
    if ( this->Mapper )
    {
        this->GetBounds();
        os << indent << "Bounds: (" << this->Bounds[0] << ", "
           << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
           << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
           << this->Bounds[5] << ")\n";
    }
    else
    {
        os << indent << "Bounds: (not defined)\n";
    }
}

