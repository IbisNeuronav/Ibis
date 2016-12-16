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

#include "DrawableTexture.h"
#include "vtk_glew.h"

static const GLenum pixelFormat = GL_RGBA;
static const GLenum pixelType = GL_FLOAT;
static const int pixelInternalFormat = GL_RGBA16F;
static const GLenum pixelTypeByte = GL_UNSIGNED_BYTE;
static const int pixelInternalFormatByte = GL_RGBA8;

DrawableTexture::DrawableTexture()
    : m_isFloatTexture(true)
    , m_texId(0)
	, m_fbId(0)
	, m_width(1)
	, m_height(1)
    , m_backupFramebuffer(0)
{
}

DrawableTexture::~DrawableTexture()
{
	Release();
}

void DrawableTexture::UseByteTexture()
{
    m_isFloatTexture = false;
}

bool DrawableTexture::Init( int width, int height )
{
	m_width = width;
	m_height = height;

	// init texture
	glGenTextures( 1, &m_texId );
    glBindTexture( GL_TEXTURE_RECTANGLE, m_texId );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    if( m_isFloatTexture )
        glTexImage2D( GL_TEXTURE_RECTANGLE, 0, pixelInternalFormat, width, height, 0, pixelFormat, pixelType, 0 );
    else
        glTexImage2D( GL_TEXTURE_RECTANGLE, 0, pixelInternalFormatByte, width, height, 0, pixelFormat, pixelTypeByte, 0 );
    glBindTexture( GL_TEXTURE_RECTANGLE, 0 );

	// Init framebuffer
	bool success = true;
    glGenFramebuffers( 1, &m_fbId );
    BindFramebuffer();
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, m_texId, 0 );

    GLenum ret = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    if( ret != GL_FRAMEBUFFER_COMPLETE )
		success = false;

	// clear the texture
	glClearColor( 0.0, 0.0, 0.0, 0.0 );
	glClear( GL_COLOR_BUFFER_BIT );

    UnBindFramebuffer();

	return success;
}

void DrawableTexture::Resize( int width, int height )
{
	if( m_texId && ( m_width != width || m_height != height ) )
	{
		m_width = width;
		m_height = height;
        glBindTexture( GL_TEXTURE_RECTANGLE, m_texId );
        if( m_isFloatTexture )
            glTexImage2D( GL_TEXTURE_RECTANGLE, 0, pixelInternalFormat, width, height, 0, pixelFormat, pixelType, 0 );
        else
            glTexImage2D( GL_TEXTURE_RECTANGLE, 0, pixelInternalFormatByte, width, height, 0, pixelFormat, pixelTypeByte, 0 );
        glBindTexture( GL_TEXTURE_RECTANGLE, 0 );

        BindFramebuffer();
		glClearColor( 0.0, 0.0, 0.0, 0.0 );
		glClear( GL_COLOR_BUFFER_BIT );
        UnBindFramebuffer();
	}
}

void DrawableTexture::Release()
{
	if( m_fbId )
        glDeleteFramebuffers( 1, &m_fbId );
	if( m_texId )
        glDeleteTextures( 1, &m_texId );
}

void DrawableTexture::DrawToTexture( bool drawTo )
{
	if( drawTo && m_fbId )
        BindFramebuffer();
	else
        UnBindFramebuffer();
}

// Paste the content of a sub-rectangle of
// the texture on the screen, assuming the screen is the
// same size as the texture. If it is not the case, scaling
// will happen.
void DrawableTexture::PasteToScreen( int x, int y, int width, int height )
{
    // Push projection
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
    glOrtho( 0, m_width, 0, m_height, -1, 1 );

    // Push modelview
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();

    glEnable( GL_TEXTURE_RECTANGLE );
    glBindTexture( GL_TEXTURE_RECTANGLE, m_texId );
	glBegin( GL_QUADS );
	{
		glTexCoord2i( x, y );					glVertex2d( x, y );
		glTexCoord2i( x + width, y );			glVertex2d( x + width, y );
		glTexCoord2i( x + width, y + height );	glVertex2d( x + width, y + height );
		glTexCoord2i( x, y + height );			glVertex2d( x, y + height );
	}
    glEnd();
    glBindTexture( GL_TEXTURE_RECTANGLE, 0 );
    glDisable( GL_TEXTURE_RECTANGLE );

    // Pop modelview
    glPopMatrix();

    // Pop projection
    glMatrixMode( GL_PROJECTION );
	glPopMatrix();

    // back to modelview
	glMatrixMode( GL_MODELVIEW );
}

void DrawableTexture::Clear( int x, int y, int width, int height )
{
    glDisable( GL_BLEND );
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( 0, m_width, 0, m_height, -1, 1 );

    glColor4d( 0.0, 0.0, 0.0, 0.0 );
    glBegin( GL_QUADS );
    {
        glVertex2d( x, y );
        glVertex2d( x + width, y );
        glVertex2d( x + width, y + height );
        glVertex2d( x, y + height );
    }
    glEnd();

    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glEnable( GL_BLEND );
}

void DrawableTexture::PasteToScreen()
{
    PasteToScreen( 0, 0, m_width, m_height );
}

#include <iostream>

void DrawableTexture::PrintGLTextureState()
{
    GLint maxNbUnits;
    glGetIntegerv( GL_MAX_TEXTURE_UNITS, &maxNbUnits );
    for( int i = 0; i < maxNbUnits; ++i )
    {
        glActiveTexture( GL_TEXTURE0 + static_cast<GLenum>( i ) );
        std::cout << "Unit " << i << ":" << std::endl;
        int binding = 0;
        glGetIntegerv( GL_TEXTURE_BINDING_1D, &binding );
        std::cout << "   TEXTURE_1D - enabled: " << ( glIsEnabled( GL_TEXTURE_1D ) == GL_TRUE ? 1 : 0 ) << " - Binding: " << binding << std::endl;
        glGetIntegerv( GL_TEXTURE_BINDING_2D, &binding );
        std::cout << "   TEXTURE_2D - enabled: " << ( glIsEnabled( GL_TEXTURE_2D ) == GL_TRUE ? 1 : 0 ) << " - Binding: " << binding << std::endl;
        glGetIntegerv( GL_TEXTURE_BINDING_3D, &binding );
        std::cout << "   TEXTURE_3D - enabled: " << ( glIsEnabled( GL_TEXTURE_3D ) == GL_TRUE ? 1 : 0 ) << " - Binding: " << binding << std::endl;
        glGetIntegerv( GL_TEXTURE_BINDING_RECTANGLE, &binding );
        std::cout << "   TEXTURE_RECTANGLE - enabled: " << ( glIsEnabled( GL_TEXTURE_RECTANGLE ) == GL_TRUE ? 1 : 0 ) << " - Binding: " << binding << std::endl;
    }

    glActiveTexture( GL_TEXTURE0 );
}

void DrawableTexture::BindFramebuffer()
{
    GLint prevFrameBuffer;
    glGetIntegerv( GL_FRAMEBUFFER_BINDING, &prevFrameBuffer );
    m_backupFramebuffer = (unsigned)prevFrameBuffer;
    glBindFramebuffer( GL_FRAMEBUFFER, m_fbId );
}

void DrawableTexture::UnBindFramebuffer()
{
    glBindFramebuffer( GL_FRAMEBUFFER, m_backupFramebuffer );
}
