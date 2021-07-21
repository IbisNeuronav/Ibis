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

#ifndef __OffscreenRenderer_h_
#define __OffscreenRenderer_h_

#include <QString>

class vtkImageImport;
class vtkPNGWriter;
class vtkOffscreenCamera;
class AnimatePluginInterface;
class vtkOpenGLRenderWindow;
class vtkCamera;

class OffscreenRenderer
{

public:

    OffscreenRenderer( AnimatePluginInterface * anim );
    void SetRenderSize( int w, int h ) { m_renderSize[0] = w; m_renderSize[1] = h; }
    void RenderCurrent();
    void RenderAnimation();

protected:

    void Setup();
    void RenderOneFrame( QString filename );
    void Cleanup();

    bool m_glInit;
    AnimatePluginInterface * m_animate;
    vtkCamera * m_backupCam;
    vtkOffscreenCamera * m_cam;
    vtkImageImport * m_importer;
    vtkPNGWriter * m_writer;
    unsigned char * m_openglRawImage;
    int m_renderSize[2];
    unsigned m_fbId;
    unsigned m_textureId;
    unsigned m_depthId;

};

#endif
