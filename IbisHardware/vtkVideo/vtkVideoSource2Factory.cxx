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

#include "vtkVideoSource2Factory.h"
#include "vtkVideoConfigure.h"
#include "vtkVideoSource2.h"
#include "vtkObjectFactory.h"
#include "assert.h"

#ifdef DEFINE_USE_V4L2
#include "vtkV4L2VideoSource2.h"
#endif
#ifdef DEFINE_USE_POINT_GREY_CAMERA
#include "vtkPointGreyVideoSource2.h"
#endif

vtkStandardNewMacro(vtkVideoSource2Factory);

#define v4l2_type_name "V4L2 Video Capture"
#define PointGrey_type_name "Point Grey Camera"

vtkVideoSource2Factory::vtkVideoSource2Factory()
{
#ifdef DEFINE_USE_V4L2
    m_typeNames.push_back( std::string( v4l2_type_name ) );
#endif
#ifdef DEFINE_USE_POINT_GREY_CAMERA
    m_typeNames.push_back( std::string( PointGrey_type_name ) );
#endif
}

vtkVideoSource2 * vtkVideoSource2Factory::CreateInstance( const char * typeName )
{
    vtkVideoSource2 * ret = 0;

#ifdef DEFINE_USE_V4L2
    if( std::string( v4l2_type_name ) == std::string( typeName ) )
    {
        vtkV4L2VideoSource2 * retV4l = vtkV4L2VideoSource2::New();
        ret = retV4l;
    }
#endif
#ifdef DEFINE_USE_POINT_GREY_CAMERA
    if( std::string( PointGrey_type_name ) == std::string( typeName ) )
    {
        vtkPointGreyVideoSource2 * retPointGrey = vtkPointGreyVideoSource2::New();
        ret = retPointGrey;
    }
#endif

    // by default, create a generic video source that produces noise
    if( !ret )
        ret = vtkVideoSource2::New();

    return ret;
}

int vtkVideoSource2Factory::GetNumberOfTypes()
{
    return m_typeNames.size();
}

const char * vtkVideoSource2Factory::GetTypeName( int index )
{
    assert( "Index out of range" && index < m_typeNames.size() );
    return m_typeNames[ index ].c_str();
}

//----------------------------------------------------------------------------
void vtkVideoSource2Factory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
