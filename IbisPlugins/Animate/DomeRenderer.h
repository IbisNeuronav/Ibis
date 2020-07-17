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

#ifndef __DomeRenderer_h_
#define __DomeRenderer_h_

#include <vtkRendererDelegate.h>
#include <vtkSmartPointer.h>

class vtkCamera;
class GlslShader;

class DomeRenderer : public vtkRendererDelegate
{

public:

    vtkTypeMacro( DomeRenderer, vtkRendererDelegate );
    static DomeRenderer * New() { return new DomeRenderer; }
    DomeRenderer();
    virtual void Render( vtkRenderer * r );
    int GetCubeTextureSize() { return m_cubeTextureSize; }
    void SetCubeTextureSize( int size ) { m_cubeTextureSize = size; }
    double GetDomeViewAngle() { return m_domeViewAngle; }
    void SetDomeViewAngle( double angle ) { m_domeViewAngle = angle; }
    void SetOverideWindowSize( bool o );
    void SetOverideSize( int w, int z );

protected:

    void DrawFishEye( int w, int h );
    void DrawCubeMap( int w, int h );
    bool LoadGLExtensions( vtkRenderer * r );
    bool SetupFrameBuffer();
    bool ResizeCubeTexture( );

    vtkSmartPointer<vtkCamera> m_renderCam;
    GlslShader * m_domeShader;

    bool m_isInit;
    int m_cubeTextureSize;
    int m_prevCubeTextureSize;
    double m_domeViewAngle;  // angle in degrees
    unsigned m_cubeTextureId;
    unsigned m_fbId;
    unsigned m_depthRenderBufferId;
    bool m_overrideWindowSize;
    int m_overrideSize[2];
};

#endif
