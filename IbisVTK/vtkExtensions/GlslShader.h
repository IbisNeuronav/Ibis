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

#ifndef __GlslShader_h_
#define __GlslShader_h_

#include <string>
#include <vector>
#include "vtkObject.h"

class vtkMatrix4x4;

class GlslShader : public vtkObject
{

public:

    GlslShader();
    ~GlslShader();

	void AddShaderMemSource( const char * src );
	void AddVertexShaderMemSource( const char * src );
    void Reset();

	bool Init();
    std::string GetErrorMessage() { return m_errorMessage; }
	bool UseProgram( bool use );
	bool SetVariable( const char * name, int value );
    bool SetVariable( const char * name, int count, int * values );
	bool SetVariable( const char * name, float value );
    bool SetVariable( const char * name, double value );
    bool SetVariable( const char * name, int val1, int val2 );  // set a ivec2
    bool SetVariable( const char * name, float val1, float val2 );  // set a vec2
    bool SetVariable( const char * name, float val1, float val2, float val3 );  // set a vec3
    bool SetVariable( const char * name, vtkMatrix4x4 * mat );    // set a mat4

protected:

    bool CreateAndCompileShader( unsigned shaderType, unsigned & shaderId, std::vector< std::string > & memSources );
	void Clear();

	std::vector< std::string > m_memSources;
	std::vector< std::string > m_vertexMemSources;
	unsigned m_glslShader;
	unsigned m_glslVertexShader;
	unsigned m_glslProg;
	bool m_init;
    std::string m_errorMessage;
};

#endif
