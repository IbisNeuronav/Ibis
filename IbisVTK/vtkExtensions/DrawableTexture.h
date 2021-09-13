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

#ifndef __DrawableTexture_h_
#define __DrawableTexture_h_

#include "vtkObject.h"

class GlslShader;
class vtkOpenGLState;

class DrawableTexture : public vtkObject
{

public:

    static DrawableTexture *New();
    vtkTypeMacro(DrawableTexture,vtkObject);
	
    void UseByteTexture();
    bool Init( int width, int height, vtkOpenGLState * s );
	void Resize( int width, int height );
	void Release();
	
	void DrawToTexture( bool drawTo );
    void PasteToScreen( int x, int y, int width, int height );
    void PasteToScreen();
    void Clear( int x, int y, int width, int height );

    unsigned GetTexId() { return m_texId; }

    static void PrintGLTextureState();

protected:

    DrawableTexture();
    ~DrawableTexture();

    void RenderPlane( int x, int y, int width, int height );
    void BindFramebuffer();
    void UnBindFramebuffer();
	
    bool m_isFloatTexture;
	unsigned m_texId;
	unsigned m_fbId;
	int m_width;
	int m_height;
    unsigned m_backupFramebuffer;

    GlslShader * m_pasteShader;
    GlslShader * m_clearShader;

private:

    DrawableTexture(const DrawableTexture&); // Not implemented.
    void operator=(const DrawableTexture&);  // Not implemented.
};

#endif
