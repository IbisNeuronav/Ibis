/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColoredCube.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkColoredCube.h"

#include "vtk_glew.h"
#include "vtkTessellatedBoxSource.h"
#include "vtkClipConvexPolyData.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkColorPolyData.h"
#include "vtkTriangleFilter.h"
#include "vtkUnsignedIntArray.h"
#include "vtkPointData.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkMatrix4x4.h"

vtkStandardNewMacro(vtkColoredCube);

//----------------------------------------------------------------------------
vtkColoredCube::vtkColoredCube()
{
    this->Vao = 0;
    this->VertexBuffer = 0;
    this->ColorBuffer = 0;
    this->IndicesBuffer = 0;

    this->Cropping = 0;

    for ( int i = 0; i < 3; i++ )
    {
        this->Bounds[2*i]   = 0.0;
        this->Bounds[2*i+1] = 1.0;
        this->CroppingRegionPlanes[2*i]   = 0.0;
        this->CroppingRegionPlanes[2*i+1] = 1.0;
    }

    BoxSource = vtkTessellatedBoxSource::New();

    BoxClip = vtkClipConvexPolyData::New();
    BoxClip->SetInputConnection( BoxSource->GetOutputPort() );

    // build all clipping planes
    AllPlanes = vtkPlaneCollection::New();
    NearFarPlanes = vtkPlaneCollection::New();

    vtkPlane * pxMin = vtkPlane::New();
    pxMin->SetNormal( 1.0, 0.0, 0.0 );
    AllPlanes->AddItem( pxMin );
    pxMin->Delete();

    vtkPlane * pxMax = vtkPlane::New();
    pxMax->SetNormal( -1.0, 0.0, 0.0 );
    AllPlanes->AddItem( pxMax );
    pxMax->Delete();

    vtkPlane * pyMin = vtkPlane::New();
    pyMin->SetNormal( 0.0, 1.0, 0.0 );
    AllPlanes->AddItem( pyMin );
    pyMin->Delete();

    vtkPlane * pyMax = vtkPlane::New();
    pyMax->SetNormal( 0.0, -1.0, 0.0 );
    AllPlanes->AddItem( pyMax );
    pyMax->Delete();

    vtkPlane * pzMin = vtkPlane::New();
    pzMin->SetNormal( 0.0, 0.0, 1.0 );
    AllPlanes->AddItem( pzMin );
    pzMin->Delete();

    vtkPlane * pzMax = vtkPlane::New();
    pzMax->SetNormal( 0.0, 0.0, -1.0 );
    AllPlanes->AddItem( pzMax );
    pzMax->Delete();

    vtkPlane * pNear = vtkPlane::New();
    pNear->SetNormal( 1.0, 0.0, 0.0 );
    AllPlanes->AddItem( pNear );
    NearFarPlanes->AddItem( pNear );
    pNear->Delete();

    vtkPlane * pFar = vtkPlane::New();
    pFar->SetNormal( 1.0, 0.0, 0.0 );
    AllPlanes->AddItem( pFar );
    NearFarPlanes->AddItem( pFar );
    pFar->Delete();

    BoxClip->SetPlanes( NearFarPlanes ); // Default: only clip near far

    BoxColoring = vtkColorPolyData::New();
    BoxColoring->SetInputConnection( BoxClip->GetOutputPort() );

    BoxTriangles = vtkTriangleFilter::New();
    BoxTriangles->SetInputConnection( BoxColoring->GetOutputPort() );

    BoxIndices = vtkUnsignedIntArray::New();
    BoxIndices->SetNumberOfComponents(3);
}

//----------------------------------------------------------------------------
vtkColoredCube::~vtkColoredCube()
{
    BoxSource->Delete();
    BoxClip->Delete();
    BoxColoring->Delete();
    BoxTriangles->Delete();
    BoxIndices->Delete();
    AllPlanes->Delete();
    NearFarPlanes->Delete();
}

GLenum vtkDataTypeToGlEnum( int vtkDataType )
{
    GLenum res = GL_FLOAT;
    switch( vtkDataType )
    {
    case VTK_CHAR:
        res = GL_BYTE;
        break;
    case VTK_SIGNED_CHAR:
        res = GL_BYTE;
        break;
    case VTK_UNSIGNED_CHAR:
        res = GL_UNSIGNED_BYTE;
        break;
    case VTK_SHORT:
        res = GL_SHORT;
        break;
    case VTK_UNSIGNED_SHORT:
        res = GL_UNSIGNED_SHORT;
        break;
    case VTK_INT:
        res = GL_INT;
        break;
    case VTK_UNSIGNED_INT:
        res = GL_UNSIGNED_INT;
        break;
    case VTK_LONG:
        res = GL_INT;
        break;
    case VTK_UNSIGNED_LONG:
        res = GL_UNSIGNED_INT;
        break;
    case VTK_FLOAT:
        res = GL_FLOAT;
        break;
    case VTK_DOUBLE:
        res = GL_DOUBLE;
        break;
    }
    return res;
}

#include "vtkOpenGLError.h"

//----------------------------------------------------------------------------
void vtkColoredCube::Render()
{
  vtkPolyData * cubePoly = vtkPolyData::SafeDownCast( BoxTriangles->GetOutput() );

  // Create vertex attribute and triangle buffers if they don't already exist
  if( VertexBuffer == 0 )
  {
      glGenBuffers(1, &VertexBuffer);
      glGenBuffers(1, &ColorBuffer);
      glGenBuffers(1, &IndicesBuffer);
  }

  // Put the data into the buffers (if different data every frame)
  glBindBuffer( GL_ARRAY_BUFFER, VertexBuffer);
  vtkDataArray * points = cubePoly->GetPoints()->GetData();
  GLsizeiptr pointSize = points->GetDataSize() * points->GetDataTypeSize();
  glBufferData( GL_ARRAY_BUFFER, pointSize, points->GetVoidPointer(0), GL_STREAM_DRAW );

  glBindBuffer( GL_ARRAY_BUFFER, ColorBuffer);
  vtkDataArray * colors = cubePoly->GetPointData()->GetScalars();
  GLsizeiptr colorSize = points->GetDataSize() * points->GetDataTypeSize();
  glBufferData( GL_ARRAY_BUFFER, colorSize, colors->GetVoidPointer(0), GL_STREAM_DRAW );

  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, IndicesBuffer);
  GLsizeiptr indicesSize = BoxIndices->GetDataSize() * BoxIndices->GetDataTypeSize();
  glBufferData( GL_ELEMENT_ARRAY_BUFFER, indicesSize, BoxIndices->GetVoidPointer(0), GL_STREAM_DRAW );

  // Create vertex array object if it doesn't exist already
  if( this->Vao == 0 )
  {
      glGenVertexArrays( 1, &(this->Vao) );
      glBindVertexArray( this->Vao );

      // Vertices
      glEnableVertexAttribArray( 0 );  // by convention, shaders in Ibis bind index 0 attribute array in_vertex
      GLenum vertexType = vtkDataTypeToGlEnum( points->GetDataType() );
      glVertexAttribPointer( 0, 3, vertexType, GL_FALSE, 0, static_cast<void*>(nullptr) );

      // Colors
      glEnableVertexAttribArray( 1 );  // by convention, shaders in Ibis bind index 1 attribute array in_color
      glVertexAttribPointer( 1, 3, GL_UNSIGNED_BYTE, GL_FALSE, 0, static_cast<void*>(nullptr) );

  }
  else
  {
      glBindVertexArray( this->Vao );
  }

  // can this be done in the vao?
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndicesBuffer);

  // Draw Triangles
  glDrawElements( GL_TRIANGLES, BoxIndices->GetDataSize(), GL_UNSIGNED_INT, static_cast<void*>(nullptr) );

  // unbind vertex array
  glBindVertexArray( 0 );
}

void vtkColoredCube::ReleaseGraphicsResources(vtkWindow *)
{
    if( this->Vao )
    {
        glDeleteVertexArrays( 1, &(this->Vao) );
        this->Vao = 0;
    }
    if( this->VertexBuffer )
    {
        glDeleteBuffers( 1, &this->VertexBuffer );
        this->VertexBuffer = 0;
    }
    if( this->ColorBuffer )
    {
        glDeleteBuffers( 1, &this->ColorBuffer );
        this->ColorBuffer = 0;
    }
    if( this->IndicesBuffer )
    {
        glDeleteBuffers( 1, &this->IndicesBuffer );
        this->IndicesBuffer = 0;
    }
}

//----------------------------------------------------------------------------
void vtkColoredCube::UpdateGeometry( vtkRenderer * ren, vtkMatrix4x4  *mat )
{
    BoxSource->SetBounds(this->Bounds);
    BoxColoring->SetBounds(this->Bounds);

    // Update bounds planes position
    AllPlanes->GetItem( 0 )->SetOrigin( CroppingRegionPlanes[0], 0.0, 0.0 );
    AllPlanes->GetItem( 1 )->SetOrigin( CroppingRegionPlanes[1], 0.0, 0.0 );
    AllPlanes->GetItem( 2 )->SetOrigin( 0.0, CroppingRegionPlanes[2], 0.0 );
    AllPlanes->GetItem( 3 )->SetOrigin( 0.0, CroppingRegionPlanes[3], 0.0 );
    AllPlanes->GetItem( 4 )->SetOrigin( 0.0, 0.0, CroppingRegionPlanes[4] );
    AllPlanes->GetItem( 5 )->SetOrigin( 0.0, 0.0, CroppingRegionPlanes[5] );

    // Update near/far planes normal and position
    vtkCamera * cam = ren->GetActiveCamera();
    double camNear = cam->GetClippingRange()[0];
    double camFar = cam->GetClippingRange()[1];

    // Get the inverse of the volume matrix
    vtkSmartPointer<vtkMatrix4x4> invVolMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    invVolMatrix->DeepCopy( mat );
    invVolMatrix->Invert();

    // Transform camera position and target to volume space
    double pos[4] = { 0.0, 0.0, 0.0, 1.0 };
    cam->GetPosition( pos );
    invVolMatrix->MultiplyPoint( pos, pos );

    double target[4] = { 0.0, 0.0, 0.0, 1.0 };
    cam->GetFocalPoint( target );
    invVolMatrix->MultiplyPoint( target, target );

    // Compute direction of projection
    double dir[3] = { 1.0, 0.0, 0.0 };
    vtkMath::Subtract( target, pos, dir );
    vtkMath::Normalize( dir );

    // Compute an offset for the near and far planes to avoid being clipped
    // due to floating-point precision
    // offset calculation stolen from vtkOpenGLVolumeRaycastMapper : choose arbitrary
    // offset. if the offset is larger than the distance between near and far point,
    // it will not work, in this case we pick a fraction of the near-far distance.
    double distNearFar = camFar - camNear;
    double offset = 0.001; // some arbitrary small value.
    if( offset >= distNearFar)
      offset = distNearFar / 1000.0;
    camNear += offset;
    camFar -= offset;

    // Compute near plane
    double nearOrigin[3] = { 0.0, 0.0, 0.0 };
    double nearNormal[3] = { 1.0, 0.0, 0.0 };
    for( int i = 0; i < 3; ++i )
    {
        nearOrigin[i] = pos[i] + dir[i] * camNear;
        nearNormal[i] = dir[i];
    }
    AllPlanes->GetItem(6)->SetOrigin( nearOrigin );
    AllPlanes->GetItem(6)->SetNormal( nearNormal );

    // Compute far plane
    double farOrigin[3] = { 0.0, 0.0, 0.0 };
    double farNormal[3] = { 1.0, 0.0, 0.0 };
    for( int i = 0; i < 3; ++i )
    {
        farOrigin[i] = pos[i] + dir[i] * camFar;
        farNormal[i] = -dir[i];
    }
    AllPlanes->GetItem(7)->SetOrigin( farOrigin );
    AllPlanes->GetItem(7)->SetNormal( farNormal );

    BoxClip->Modified();

    // Make sure polydata is up to date
    BoxTriangles->Update();

    // Convert triangle indices to unsigned int
    vtkPolyData * cubePoly = vtkPolyData::SafeDownCast( BoxTriangles->GetOutput() );
    vtkCellArray * cells = cubePoly->GetPolys();
    BoxIndices->SetNumberOfTuples( 0 );
    vtkIdType npts;
    const vtkIdType *pts;
    cells->InitTraversal();
    while(cells->GetNextCell(npts, pts))
    {
        BoxIndices->InsertNextTuple3(pts[0], pts[1], pts[2]);
    }
}

void vtkColoredCube::SetCropping( int c )
{
    int clamped = c > 0 ? 1 : 0;
    if( this->Cropping == clamped )
        return;

    this->Cropping = clamped;
    if( this->Cropping == 0 )
        this->BoxClip->SetPlanes( this->NearFarPlanes );
    else
        this->BoxClip->SetPlanes( this->AllPlanes );

    this->Modified();
}

//----------------------------------------------------------------------------
void vtkColoredCube::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: ( ";
  for( int i = 0; i < 5; ++i )
      os << this->Bounds[i] << ", ";
  os << this->Bounds[5] << " )" << endl;
}
