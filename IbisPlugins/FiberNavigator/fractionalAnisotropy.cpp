/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Francois Rheau for writing this class

#include "fractionalAnisotropy.h"

#include "nifti/nifti1_io.h"

#include <algorithm>
#include <string>
using std::string;
#include <vector>
using std::vector;

//#ifndef isnan
//inline bool isnan(double x) {
//    return x != x;
//}
//#endif

///////////////////////////////////////////
fractionalAnisotropy::fractionalAnisotropy( const string &filename )
: m_dataType( 16 ),
  m_filename( filename )
{

}

//////////////////////////////////////////////////////////////////////////
fractionalAnisotropy::~fractionalAnisotropy()
{
}

//////////////////////////////////////////////////////////////////////////
bool fractionalAnisotropy::load()
{
    // Create nifti header and data
    nifti_image *pHeader = nifti_image_read( m_filename.c_str(), 0 );
    nifti_image *pBody   = nifti_image_read( m_filename.c_str(), 1 );

    m_columns  = pHeader->dim[1]; //XSlice
    m_rows     = pHeader->dim[2]; //YSlice
    m_frames   = pHeader->dim[3]; //ZSlice
    m_bands    = pHeader->dim[4]; // 1
    m_dataType = pHeader->datatype;//16

    m_voxelSizeX = pHeader->dx;
    m_voxelSizeY = pHeader->dy;
    m_voxelSizeZ = pHeader->dz;

    int datasetSize = pHeader->dim[1] * pHeader->dim[2] * pHeader->dim[3];
    
    m_fileFloatData.assign( datasetSize * m_bands, 0.0f);
    
    float *pData = (float*)pBody->data;
    for( int i( 0 ); i < datasetSize; ++i )
    {
        for( int j( 0 ); j < m_bands; ++j )
        {
            if(!std::isnan(pData[j * datasetSize + i]))
            {
                m_fileFloatData[i * m_bands + j] = pData[j * datasetSize + i];
            }

        }
    }
    m_value = m_fileFloatData;

    nifti_image_free( pHeader );
    nifti_image_free( pBody );

    return true;
}

float fractionalAnisotropy::getFractionalAnisotropy(double* position)
{
    int pos = (position[2]-1)*m_columns*m_rows + (position[1]-1)*m_columns + position[0];
    if(pos < 0)
        return 0;
    else
        return m_value[pos];
}
