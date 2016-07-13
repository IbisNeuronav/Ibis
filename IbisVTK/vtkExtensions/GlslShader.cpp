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
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include "vtkgl.h"
#include "vtkMatrix4x4.h"

using namespace std;

GlslShader::GlslShader()
	: m_glslShader(0)
	, m_glslVertexShader(0)
	, m_glslProg(0)
	, m_init( false )
{
}

GlslShader::~GlslShader()
{
	Clear();
}

void GlslShader::AddShaderMemSource( const char * src )
{
	m_memSources.push_back( std::string( src ) );
}

void GlslShader::AddVertexShaderMemSource( const char * src )
{
	m_vertexMemSources.push_back( std::string( src ) );
}

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
        if( !CreateAndCompileShader( vtkgl::VERTEX_SHADER, m_glslVertexShader, m_vertexMemSources ) )
			return false;
	
	// Load and try compiling pixel shader
    if( m_memSources.size() != 0 )
        if( !CreateAndCompileShader( vtkgl::FRAGMENT_SHADER, m_glslShader, m_memSources ) )
			return false;
	
    // Check that at least one of the shaders has been compiled
	if( m_glslVertexShader == 0 && m_glslShader == 0 )
		return false;

	// Create program object and attach shader
    m_glslProg = vtkgl::CreateProgram();
	if( m_glslVertexShader )
        vtkgl::AttachShader( m_glslProg, m_glslVertexShader );
	if( m_glslShader )
        vtkgl::AttachShader( m_glslProg, m_glslShader );

	// Create program and link shaders
    vtkgl::LinkProgram( m_glslProg );
	GLint success = 0;
    vtkgl::GetProgramiv( m_glslProg, vtkgl::LINK_STATUS, &success );
    if (!success)
    {
		GLint logLength = 0;
        vtkgl::GetProgramiv( m_glslProg, vtkgl::INFO_LOG_LENGTH, &logLength );
        vtkgl::GLchar * infoLog = new vtkgl::GLchar[ logLength + 1 ];
        vtkgl::GetProgramInfoLog( m_glslProg, logLength, NULL, infoLog );
        vtkErrorMacro( << "Error in glsl program linking:" << infoLog );
		delete [] infoLog;
		return false;
    }

	m_init = true;
	return true;
}

bool GlslShader::CreateAndCompileShader( unsigned shaderType, unsigned & shaderId, std::vector< std::string > & memSources )
{
	// put all the sources in an array of const GLchar*
    const vtkgl::GLchar ** shaderStringPtr = new const vtkgl::GLchar*[ memSources.size() ];
	for( unsigned i = 0; i < memSources.size(); ++i )
	{
		shaderStringPtr[i] = memSources[i].c_str();
	}
	
	// Create the shader and set its source
    shaderId = vtkgl::CreateShader( shaderType );
    vtkgl::ShaderSource( shaderId, memSources.size(), shaderStringPtr, NULL);
	
	delete [] shaderStringPtr;
	
	// Compile the shader
	GLint success = 0;
    vtkgl::CompileShader( shaderId );
    vtkgl::GetShaderiv( shaderId, vtkgl::COMPILE_STATUS, &success );
    if (!success)
    {
		GLint logLength = 0;
        vtkgl::GetShaderiv( shaderId, vtkgl::INFO_LOG_LENGTH, &logLength );
        vtkgl::GLchar * infoLog = new vtkgl::GLchar[logLength+1];
        vtkgl::GetShaderInfoLog( shaderId, logLength, NULL, infoLog);
        vtkErrorMacro( << "Error in shader complilation." << infoLog );
		delete [] infoLog;
        return false;
    }
	return true;
}

bool GlslShader::UseProgram( bool use )
{
	bool res = true;
	if( use && m_init )
	{
        vtkgl::UseProgram( m_glslProg );
	}
	else
	{
		res = true;
        vtkgl::UseProgram( 0 );
	}
	return res;
}

bool GlslShader::SetVariable( const char * name, int value )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
	if( location != -1 )
	{
        vtkgl::Uniform1i( location, value );
		return true;
	}
	return false;
}

bool GlslShader::SetVariable( const char * name, int count, int * values )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        vtkgl::Uniform1iv( location, count, values );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, float value )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
	if( location != -1 )
	{
        vtkgl::Uniform1f( location, value );
		return true;
	}
	return false;
}

bool GlslShader::SetVariable( const char * name, double value )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        vtkgl::Uniform1d( location, value );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, int val1, int val2 )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        vtkgl::Uniform2i( location, val1, val2 );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, float val1, float val2 )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        vtkgl::Uniform2f( location, val1, val2 );
        return true;
    }
    return false;
}

bool GlslShader::SetVariable( const char * name, float val1, float val2, float val3 )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        vtkgl::Uniform3f( location, val1, val2, val3 );
        return true;
    }
    return false;
}

#include "vtkMatrix4x4.h"

bool GlslShader::SetVariable( const char * name, vtkMatrix4x4 * mat )
{
    int location = vtkgl::GetUniformLocation( m_glslProg, name );
    if( location != -1 )
    {
        float local[16];
        for( int i = 0; i < 4; ++i )
            for( int j = 0; j < 4; ++j )
                local[ i * 4 + j ] = mat->Element[i][j];
        vtkgl::UniformMatrix4fv( location, 1, GL_TRUE, local );
        return true;
    }
    return false;
}

void GlslShader::Clear()
{
	if( m_glslVertexShader != 0 )
	{
        vtkgl::DeleteShader( m_glslVertexShader );
		m_glslVertexShader = 0;
	}
	if( m_glslShader != 0 )
	{
        vtkgl::DeleteShader( m_glslShader );
		m_glslShader = 0;
	}
	if( m_glslProg != 0 )
	{
        vtkgl::DeleteProgram( m_glslProg );
		m_glslProg = 0;
	}
	m_init = false;
}

