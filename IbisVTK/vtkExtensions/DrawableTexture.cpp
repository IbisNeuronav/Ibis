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
#include "GlslShader.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLError.h"
#include "vtkObjectFactory.h"  // for vtkStandardNewMacro

static const GLenum pixelFormat = GL_RGBA;
static const GLenum pixelType = GL_FLOAT;
static const int pixelInternalFormat = GL_RGBA16F;
static const GLenum pixelTypeByte = GL_UNSIGNED_BYTE;
static const int pixelInternalFormatByte = GL_RGBA8;

vtkStandardNewMacro(DrawableTexture);

DrawableTexture::DrawableTexture()
    : m_isFloatTexture(true)
    , m_texId(0)
	, m_fbId(0)
	, m_width(1)
	, m_height(1)
    , m_backupFramebuffer(0)
    , m_pasteShader(nullptr)
    , m_clearShader(nullptr)
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

bool DrawableTexture::Init( int width, int height, vtkOpenGLState * s )
{
	m_width = width;
	m_height = height;

	// init texture
	glGenTextures( 1, &m_texId );
    glBindTexture( GL_TEXTURE_RECTANGLE, m_texId );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
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
    s->vtkglClearColor( 0.0, 0.0, 0.0, 0.0 );
    s->vtkglClear( GL_COLOR_BUFFER_BIT );

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
    if( m_pasteShader )
    {
        delete m_pasteShader;
        m_pasteShader = nullptr;
    }
    if( m_clearShader )
    {
        delete m_clearShader;
        m_clearShader = nullptr;
    }
}

void DrawableTexture::DrawToTexture( bool drawTo )
{
	if( drawTo && m_fbId )
        BindFramebuffer();
	else
        UnBindFramebuffer();
}

const char * vertexShaderCode = "#version 150 \
in vec2 in_vertex; \
out vec2 tcoord; \
void main(void) \
{ \
    gl_Position = vec4(in_vertex,0.0,1.0); \
    tcoord = in_vertex; \
}";

const char * fragmentShaderCode = "#version 150 \
in vec2 tcoord; \
in Sampler2D tex; \
out vec4 fragColor;\
void main() \
{ \
    fragColor = texture( tex, tcoord ); \
}";


// Paste the content of a sub-rectangle of
// the texture on the screen, assuming the screen is the
// same size as the texture. If it is not the case, scaling
// will happen.
void DrawableTexture::PasteToScreen( int px, int py, int pwidth, int pheight )
{
    if( !m_pasteShader )
    {
        m_pasteShader = new GlslShader;
        m_pasteShader->AddVertexShaderMemSource( vertexShaderCode );
        m_pasteShader->AddShaderMemSource( fragmentShaderCode );
        m_pasteShader->Init();
    }

    // convert input param (in pixel coord) to normalized screen coords
    GLfloat x = static_cast<GLfloat>(px) / m_width;
    GLfloat y = static_cast<GLfloat>(py) / m_height;
    GLfloat width = static_cast<GLfloat>(pwidth) / m_width;
    GLfloat height = static_cast<GLfloat>(pheight) / m_height;

    static GLfloat vertices[4][2];
    vertices[0][0] = x;          vertices[0][1] = y;
    vertices[1][0] = x + width;  vertices[1][1] = y;
    vertices[2][0] = x + width;  vertices[2][1] = y + height;
    vertices[3][0] = x;          vertices[3][1] = y + height;

    static GLuint indices[4] = { 0, 1, 2, 3 };

    // Enable the shader
    m_pasteShader->UseProgram( true );

    // Bind the texture
    glBindTexture( GL_TEXTURE_RECTANGLE, m_texId );

    // Vertices
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, vertices );

    // Draw Triangles
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, indices );

    // Restore previous state
    glDisableVertexAttribArray( 0 );

    m_pasteShader->UseProgram( false );

}

const char * clearVertexShaderCode = "#version 150 \
in vec2 in_vertex; \
void main(void) \
{ \
    gl_Position = vec4(in_vertex,0.0,1.0); \
}";

const char * clearFragmentShaderCode = "#version 150 \
out vec4 fragColor;\
void main() \
{ \
    fragColor = vec4(0.0,0.0,0.0,0.0); \
}";

void DrawableTexture::Clear( int px, int py, int pwidth, int pheight )
{
    if( !m_clearShader )
    {
        m_clearShader = new GlslShader;
        m_clearShader->AddVertexShaderMemSource( clearVertexShaderCode );
        m_clearShader->AddShaderMemSource( clearFragmentShaderCode );
        m_clearShader->Init();
    }

    // convert input param (in pixel coord) to normalized screen coords
    GLfloat x = static_cast<GLfloat>(px) / m_width;
    GLfloat y = static_cast<GLfloat>(py) / m_height;
    GLfloat width = static_cast<GLfloat>(pwidth) / m_width;
    GLfloat height = static_cast<GLfloat>(pheight) / m_height;

    static GLfloat vertices[4][2];
    vertices[0][0] = x;          vertices[0][1] = y;
    vertices[1][0] = x + width;  vertices[1][1] = y;
    vertices[2][0] = x + width;  vertices[2][1] = y + height;
    vertices[3][0] = x;          vertices[3][1] = y + height;

    static GLuint indices[4] = { 0, 1, 2, 3 };

    // Enable the shader
    m_clearShader->UseProgram( true );

    // Bind the texture
    glBindTexture( GL_TEXTURE_RECTANGLE, m_texId );

    // Vertices
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, vertices );

    // Draw Triangles
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, indices );

    // Restore previous state
    glDisableVertexAttribArray( 0 );

    m_clearShader->UseProgram( false );
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
