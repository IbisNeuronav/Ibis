/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImagePlaneActor.cxx,v $
  Language:  C++
  Date:      $Date: 2007-03-05 21:50:09 $
  Version:   $Revision: 1.3 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImagePlaneActor.h"

#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkGraphicsFactory.h"
#include "vtkImageConstantPad.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkTexture.h"
#include "vtkTextureMapToPlane.h"

vtkImagePlaneActor * vtkImagePlaneActor::New()
{
    if( vtkGraphicsFactory::GetUseMesaClasses() )
    {
        // vtkErrorMacro(<< "This class is only available under OpenGL.");  vtkError macro needs a non-static member
        // function because it references 'this'.
        return 0;
    }
    else
    {
        return new vtkImagePlaneActor;
    }
}

void vtkImagePlaneActor::SetImageExtent( int ext0, int ext1, int ext2, int ext3, int ext4, int ext5 )
{
    Plane->SetOrigin( (float)ext0, (float)ext2, (float)0 );
    Plane->SetPoint1( (float)ext1, (float)ext2, (float)0 );
    Plane->SetPoint2( (float)ext0, (float)ext3, (float)0 );
}

void vtkImagePlaneActor::SetWholeExtent( int ext0, int ext1, int ext2, int ext3, int ext4, int ext5 )
{
    PlaneWithTexCoord->SetOrigin( (float)ext0 - .5, (float)ext2 - .5, (float)0 );
    PlaneWithTexCoord->SetPoint1( (float)ext1 + .5, (float)ext2 - .5, (float)0 );
    PlaneWithTexCoord->SetPoint2( (float)ext0 - .5, (float)ext3 + .5, (float)0 );
}

void vtkImagePlaneActor::SetUseLookupTable( int useIt ) { Texture->SetMapColorScalarsThroughLookupTable( useIt ); }

void vtkImagePlaneActor::SetLookupTable( vtkScalarsToColors * table ) { Texture->SetLookupTable( table ); }

void vtkImagePlaneActor::SetInput( vtkImageData * image ) { this->ImagePad->SetInputData( image ); }

vtkImageData * vtkImagePlaneActor::GetInput() { return vtkImageData::SafeDownCast( this->ImagePad->GetInput() ); }

int vtkImagePlaneActor::RenderOpaqueGeometry( vtkViewport * viewport )
{
    this->PreRenderSetup();
    return vtkActor::RenderOpaqueGeometry( viewport );
}

int vtkImagePlaneActor::RenderTranslucentGeometry( vtkViewport * viewport )
{
    this->PreRenderSetup();
    return vtkActor::RenderTranslucentPolygonalGeometry( viewport );
}

void vtkImagePlaneActor::PreRenderSetup()
{
    // Get the input
    vtkImageData * input = this->GetInput();
    if( !input )
    {
        vtkErrorMacro( << "vtkImagePlaneActor::PreRenderSetup: input image is not valid." );
        return;
    }

    // make sure extent are up-to-date
    // VTK6    input->UpdateInformation();

    // if the display extent has not been set, then compute one
    //    int * wExtent = input->GetWholeExtent();
    int * wExtent = input->GetExtent();  // VTK6
    int xSize     = wExtent[1] - wExtent[0] + 1;
    int ySize     = wExtent[3] - wExtent[2] + 1;
    int zSize     = wExtent[5] - wExtent[4] + 1;
    if( zSize != 1 )
    {
        vtkErrorMacro( << "vtkImagePlaneActor::PreRenderSetup: input image has z size > 1. Only 2D images are valid" );
        return;
    }

    int newXSize = FindPowerOfTwo( xSize );
    int newYSize = FindPowerOfTwo( ySize );

    this->ImagePad->SetOutputWholeExtent( 0, newXSize - 1, 0, newYSize - 1, 0, 0 );

    if( this->DisplayExtent[0] == -1 )
    {
        this->DisplayExtent[0] = wExtent[0];
        this->DisplayExtent[1] = wExtent[1];
        this->DisplayExtent[2] = wExtent[2];
        this->DisplayExtent[3] = wExtent[3];
        this->DisplayExtent[4] = wExtent[4];
        this->DisplayExtent[5] = wExtent[4];
    }

    SetImageExtent( this->DisplayExtent );
    SetWholeExtent( 0, newXSize - 1, 0, newYSize - 1, 0, 0 );

    // VTK6    input->SetUpdateExtent(this->DisplayExtent);
    // VTK6    input->PropagateUpdateExtent();
    // VTK6    input->UpdateData();
}

int vtkImagePlaneActor::FindPowerOfTwo( int input )
{
    int result = 1;
    while( result < input ) result = result << 1;
    return result;
}

vtkImagePlaneActor::vtkImagePlaneActor()
{
    this->DisplayExtent[0] = -1;
    this->DisplayExtent[1] = -1;
    this->DisplayExtent[2] = -1;
    this->DisplayExtent[3] = -1;
    this->DisplayExtent[4] = -1;
    this->DisplayExtent[5] = -1;

    //----------------------------------------------------------------
    // create plane on which video texture will be mapped
    //----------------------------------------------------------------
    this->Plane = vtkPlaneSource::New();
    this->Plane->SetXResolution( 1 );
    this->Plane->SetYResolution( 1 );
    this->Plane->SetOrigin( -0.5, -0.5, 0 );   // Specify a point defining the origin of the plane.
    this->Plane->SetPoint1( 639.5, -0.5, 0 );  // Specify a point defining the first axis of the plane.
    this->Plane->SetPoint2( -0.5, 479.5, 0 );  // Specify a point defining the second axis of the plane.

    //----------------------------------------------------------------
    // generate texture coordinates by mapping points to plane
    //----------------------------------------------------------------
    this->PlaneWithTexCoord = vtkTextureMapToPlane::New();
    this->PlaneWithTexCoord->SetOrigin( -0.5, -0.5, 0 );
    this->PlaneWithTexCoord->SetPoint1( 639.5, -0.5, 0 );
    this->PlaneWithTexCoord->SetPoint2( -0.5, 479.5, 0 );
    this->PlaneWithTexCoord->SetInputData( this->Plane->GetOutput() );
    this->PlaneWithTexCoord->AutomaticPlaneGenerationOff();

    //----------------------------------------------------------------
    // map vtkDataSet and derived classes to graphics primitives.
    //----------------------------------------------------------------
    this->Mapper = vtkDataSetMapper::New();
    this->Mapper->SetInputData( this->PlaneWithTexCoord->GetOutput() );

    //----------------------------------------------------------------
    // handles properties associated with a texture map.
    //----------------------------------------------------------------
    this->ImagePad = vtkImageConstantPad::New();
    this->ImagePad->SetConstant( 0.0 );
    this->ImagePad->SetOutputWholeExtent( 0, 127, 0, 127, 0, 0 );

    //----------------------------------------------------------------
    // handles properties associated with a texture map.
    //----------------------------------------------------------------
    this->Texture = vtkTexture::New();
    this->Texture->SetQualityTo32Bit();
    this->Texture->InterpolateOff();
    this->Texture->RepeatOff();
    this->Texture->SetInputData( this->ImagePad->GetOutput() );

    this->SetMapper( Mapper );
    this->SetTexture( Texture );
}

vtkImagePlaneActor::~vtkImagePlaneActor()
{
    this->Plane->Delete();
    this->PlaneWithTexCoord->Delete();
    this->Mapper->Delete();
    this->Texture->Delete();
    this->ImagePad->Delete();
}
