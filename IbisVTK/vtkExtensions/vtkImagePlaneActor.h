/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImagePlaneActor.h,v $
  Language:  C++
  Date:      $Date: 2004-05-11 21:17:28 $
  Version:   $Revision: 1.3 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImagePlaneActor - Plane texture-mapped with 2D image
// .SECTION Description
// Create an actor containing a 2D plane texture
// mapped with the input image. Input image should be 2D.

#ifndef __vtkImagePlaneActor_h
#define __vtkImagePlaneActor_h

#include "vtkOpenGLActor.h"

class vtkPlaneSource;
class vtkTextureMapToPlane;
class vtkDataSetMapper;
class vtkScalarsToColors;
class vtkTexture;
class vtkImageData;
class vtkImageConstantPad;


class vtkImagePlaneActor : public vtkOpenGLActor
{

public:

    static vtkImagePlaneActor * New();

    void SetImageExtent( int ext0, int ext1, int ext2, int ext3, int ext4, int ext5 );
    void SetImageExtent( int ext[6] ) { SetImageExtent( ext[0], ext[1], ext[2], ext[3], ext[4], ext[5] ); }
    void SetWholeExtent( int ext0, int ext1, int ext2, int ext3, int ext4, int ext5 );
    void SetWholeExtent( int ext[6] ) { SetWholeExtent( ext[0], ext[1], ext[2], ext[3], ext[4], ext[5] ); }

    void SetUseLookupTable( int useIt );
    void SetLookupTable( vtkScalarsToColors * table );

    void SetInput( vtkImageData * image );
    vtkImageData * GetInput();

    virtual int RenderOpaqueGeometry(vtkViewport *viewport);
    virtual int RenderTranslucentGeometry(vtkViewport *viewport);
    
    vtkSetVector6Macro(DisplayExtent,int);

protected:

    void PreRenderSetup();
    int FindPowerOfTwo( int input );

    vtkImagePlaneActor();
    ~vtkImagePlaneActor();

    vtkPlaneSource       * Plane;
    vtkTextureMapToPlane * PlaneWithTexCoord;
    vtkDataSetMapper     * Mapper;
    vtkTexture           * Texture;
    vtkImageConstantPad  * ImagePad;
    int                    DisplayExtent[6];

private:

    vtkImagePlaneActor(const vtkImagePlaneActor&);  // Not implemented.
    void operator=(const vtkImagePlaneActor&);      // Not implemented.
};


#endif
