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

#include "GlslShader.h"

#include <stdarg.h>
#include <stdio.h>

#include <iostream>

#include "vtkMatrix4x4.h"
#include "vtk_glew.h"

using namespace std;

GlslShader::GlslShader() : m_glslShader( 0 ), m_glslVertexShader( 0 ), m_glslProg( 0 ), m_init( false ) {}

GlslShader::~GlslShader() { Clear(); }

void GlslShader::AddShaderMemSource( const char * src ) { m_memSources.push_back( std::string( src ) ); }

void GlslShader::AddVertexShaderMemSource( const char * src ) { m_vertexMemSources.push_back( std::string( src ) ); }

void GlslShader::Reset()
{
    m_memSources.clear();
    m_vertexMemSources.clear();
}

bool GlslShader::Init()
{
    // Fresh start
    Clear();

    // Load and try compiling vertex shader
    if( m_vertexMemSources.size() != 0 )
        if( !CreateAndCompileShader( GL_VERTEX_SHADER, m_glslVertexShader, m_vertexMemSources ) ) return false;

    // Load and try compiling pixel shader
    if( m_memSources.size() != 0 )
        if( !CreateAndCompileShader( GL_FRAGMENT_SHADER, m_glslShader, m_memSources ) ) return false;

    // Check that at least one of the shaders has been compiled
    if( m_glslVertexShader == 0 && m_glslShader == 0 ) return false;

    // Create program object and attach shader
    m_glslProg = glCreateProgram();
    if( m_glslVertexShader ) glAttachShader( m_glslProg, m_glslVertexShader );
    if( m_glslShader ) glAttachShader( m_glslProg, m_glslShader );

    // Create program and link shaders
    glLinkProgram( m_glslProg );
    GLint success = 0;
    glGetProgramiv( m_glslProg, GL_LINK_STATUS, &success );
    if( !success )
    {
        GLint logLength = 0;
        glGetProgramiv( m_glslProg, GL_INFO_LOG_LENGTH, &logLength );
        GLchar * infoLog = new GLchar[logLength + 1];
        glGetProgramInfoLog( m_glslProg, logLength, nullptr, infoLog );
        vtkErrorMacro( << "Error in glsl program linking:" << infoLog );
        m_errorMessage = infoLog;
        delete[] infoLog;
        return false;
    }

    // Set vertex attribut location
    UseProgram( true );
    glBindAttribLocation( m_glslProg, 0, "in_vertex" );
    glBindAttribLocation( m_glslProg, 1, "in_color" );
    UseProgram( false );

    m_errorMessage = "";
    m_init         = true;
    return true;
}

bool GlslShader::CreateAndCompileShader( unsigned shaderType, unsigned & shaderId,
                                         std::vector<std::string> & memSources )
{
    // put all the sources in an array of const GLchar*
    const GLchar ** shaderStringPtr = new const GLchar *[memSources.size()];
    for( unsigned i = 0; i < memSources.size(); ++i )
    {
        shaderStringPtr[i] = memSources[i].c_str();
    }

    // Create the shader and set its source
    shaderId = glCreateShader( shaderType );
    glShaderSource( shaderId, static_cast<GLsizei>( memSources.size() ), shaderStringPtr, NULL );

    delete[] shaderStringPtr;

    // Compile the shader
    GLint success = 0;
    glCompileShader( shaderId );
    glGetShaderiv( shaderId, GL_COMPILE_STATUS, &success );
    if( !success )
    {
        GLint logLength = 0;
        glGetShaderiv( shaderId, GL_INFO_LOG_LENGTH, &logLength );
        GLchar * infoLog = new GLchar[logLength + 1];
        glGetShaderInfoLog( shaderId, logLength, NULL, infoLog );
        vtkErrorMacro( << "Error in shader complilation." << infoLog );
        m_errorMessage = infoLog;
        delete[] infoLog;
        return false;
    }
    m_errorMessage = "";

    return true;
}

bool GlslShader::UseProgram( bool use )
{
    bool res = true;
    if( use && m_init )
    {
        glUseProgram( m_glslProg );
    }
    else
    {
        res = true;
        glUseProgram( 0 );
    }
    return res;
}

bool GlslShader::SetVariable( const char * name, int value )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        glUniform1i( location, value );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, int count, int * values )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        glUniform1iv( location, count, values );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, float value )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        glUniform1f( location, value );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, double value )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        glUniform1d( location, value );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, int val1, int val2 )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        glUniform2i( location, val1, val2 );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, float val1, float val2 )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        glUniform2f( location, val1, val2 );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, float val1, float val2, float val3 )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        glUniform3f( location, val1, val2, val3 );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, vtkMatrix4x4 * mat )
{
    int location = glGetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        float local[16];
        for( int i = 0; i < 4; ++i )
            for( int j = 0; j < 4; ++j ) local[i * 4 + j] = mat->Element[i][j];
        glUniformMatrix4fv( location, 1, GL_TRUE, local );
        return true;
    }
    return false;
}

void GlslShader::Clear()
{
    if( m_glslVertexShader != 0 )
    {
        glDeleteShader( m_glslVertexShader );
        m_glslVertexShader = 0;
    }
    if( m_glslShader != 0 )
    {
        glDeleteShader( m_glslShader );
        m_glslShader = 0;
    }
    if( m_glslProg != 0 )
    {
        glDeleteProgram( m_glslProg );
        m_glslProg = 0;
    }
    m_init = false;
}
