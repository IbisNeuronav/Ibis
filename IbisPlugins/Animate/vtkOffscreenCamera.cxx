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

#include "vtkOffscreenCamera.h"

#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOutputWindow.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGL.h>
#include "DomeRenderer.h"

#include <math.h>

vtkStandardNewMacro(vtkOffscreenCamera);


// Implement base class method.
void vtkOffscreenCamera::Render(vtkRenderer *ren)
{
  double aspect[2];
  int  lowerLeft[2] = { 0, 0 };
  int usize, vsize;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  vtkOpenGLRenderWindow *win=vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();

  // Get render size
  DomeRenderer * domeRen = (DomeRenderer*)ren->GetDelegate();
  if( domeRen )
  {
      usize = domeRen->GetCubeTextureSize();
      vsize = usize;
  }
  else
  {
      usize = RenderSize[0];
      vsize = RenderSize[1];
  }

  // This is how it was done in vtkCamera
  // ren->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);

  // if were on a stereo renderer draw to special parts of screen
  if (this->Stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
        if (this->LeftEye)
          {
          if(ren->GetRenderWindow()->GetDoubleBuffer())
            {
            glDrawBuffer(static_cast<GLenum>(win->GetBackLeftBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetBackLeftBuffer()));
            }
          else
            {
            glDrawBuffer(static_cast<GLenum>(win->GetFrontLeftBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetFrontLeftBuffer()));
            }
          }
        else
          {
           if(ren->GetRenderWindow()->GetDoubleBuffer())
            {
            glDrawBuffer(static_cast<GLenum>(win->GetBackRightBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetBackRightBuffer()));
            }
          else
            {
            glDrawBuffer(static_cast<GLenum>(win->GetFrontRightBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetFrontRightBuffer()));
            }
          }
        break;
      case VTK_STEREO_LEFT:
        this->LeftEye = 1;
        break;
      case VTK_STEREO_RIGHT:
        this->LeftEye = 0;
        break;
      default:
        break;
      }
    }
  else
    {
    if (ren->GetRenderWindow()->GetDoubleBuffer())
      {
      glDrawBuffer(static_cast<GLenum>(win->GetBackBuffer()));

      // Reading back buffer means back left. see OpenGL spec.
      // because one can write to two buffers at a time but can only read from
      // one buffer at a time.
      glReadBuffer(static_cast<GLenum>(win->GetBackBuffer()));
      }
    else
      {
      glDrawBuffer(static_cast<GLenum>(win->GetFrontBuffer()));

      // Reading front buffer means front left. see OpenGL spec.
      // because one can write to two buffers at a time but can only read from
      // one buffer at a time.
      glReadBuffer(static_cast<GLenum>(win->GetFrontBuffer()));
      }
    }

  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);

  glMatrixMode( GL_PROJECTION);
  if(usize && vsize)
  {
      double ratio = ((double)usize) / vsize;
      matrix->DeepCopy(this->GetProjectionTransformMatrix( ratio, -1,1 ) );
      matrix->Transpose();
  }

  // insert camera view transformation
  glLoadMatrixd(matrix->Element[0]);
  
  // push the model view matrix onto the stack, make sure we
  // adjust the mode first
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  matrix->DeepCopy(this->GetViewTransformMatrix());
  matrix->Transpose();
  
  // insert camera view transformation
  glMultMatrixd(matrix->Element[0]);

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase()
          && !ren->GetIsPicking())
  {
      ren->Clear();
  }
  
  matrix->Delete();
}

void vtkOffscreenCamera::UpdateViewport(vtkRenderer *ren)
{
  int  lowerLeft[2] = { 0, 0 };
  int usize, vsize;
  //ren->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);

  DomeRenderer * domeRen = (DomeRenderer*)ren->GetDelegate();
  if( domeRen )
  {
      usize = domeRen->GetCubeTextureSize();
      vsize = usize;
  }
  else
  {
      usize = RenderSize[0];
      vsize = RenderSize[1];
  }

  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);
}

//----------------------------------------------------------------------------
void vtkOffscreenCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
