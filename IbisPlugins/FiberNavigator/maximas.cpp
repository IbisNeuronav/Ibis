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

#include "maximas.h"

#include "nifti/nifti1_io.h"

#include <algorithm>
#include <string>
using std::string;
#include <vector>
using std::vector;

#ifndef isnan
inline bool isnan(double x) {
    return x != x;
}
#endif

///////////////////////////////////////////
Maximas::Maximas( const string &filename )
: m_dataType( 16 ),
  m_filename( filename )
{

}

//////////////////////////////////////////////////////////////////////////
Maximas::~Maximas()
{
}

//////////////////////////////////////////////////////////////////////////
bool Maximas::load()
{
    // Create nifti header and data
    nifti_image *pHeader = nifti_image_read( m_filename.c_str(), 0 );
    nifti_image *pBody   = nifti_image_read( m_filename.c_str(), 1 );

    m_columns  = pHeader->dim[1]; //XSlice
    m_rows     = pHeader->dim[2]; //YSlice
    m_frames   = pHeader->dim[3]; //ZSlice
    m_bands    = pHeader->dim[4]; // 3 * Number of sticks.
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
            if(!isnan(pData[j * datasetSize + i]))
                m_fileFloatData[i * m_bands + j] = pData[j * datasetSize + i];
        }
    }
    
    createStructure( m_fileFloatData );

    nifti_image_free( pHeader );
    nifti_image_free( pBody );

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool Maximas::createStructure  ( std::vector< float > &fileFloatData )
{
    int nbGlyphs = m_columns * m_rows * m_frames;
    
    m_mainDirections.resize( nbGlyphs );

    vector< float >::iterator it;
    int i = 0;

    //Fetching the directions
    for( it = fileFloatData.begin(), i = 0; it != fileFloatData.end(); it += m_bands, ++i )
    { 
        m_mainDirections[i].insert( m_mainDirections[i].end(), it, it + m_bands ); 
    }

    //Eliminate peak if 'NULL' or 'INF'
    for(int i = 0; i < m_mainDirections.size(); ++i)
    {
        for(int j = 0; j < m_mainDirections[i].size(); j+=3)
        {
            if(m_mainDirections[i][j] > 100 ||
                    m_mainDirections[i][j] == 0 && m_mainDirections[i][j+1] == 0 && m_mainDirections[i][j+2] == 0)
            {
                m_mainDirections[i].resize(j);
            }
        }
    }
    return true;
}

std::vector< float >& Maximas::getMaximaOrientation(double* position)
{
    int pos = (position[2]-1)*m_columns*m_rows + (position[1]-1)*m_columns + position[0];

    if(pos < 0)
    {
        return m_mainDirections[0];
    }
    return m_mainDirections[pos];
}
