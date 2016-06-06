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

#ifndef fractionalAnisotropy_H_
#define fractionalAnisotropy_H_

#include <string>
using std::string;
#include <vector>
#include<iostream>

class fractionalAnisotropy
{
public:
    // Constructor/Destructor
    //fractionalAnisotropy();
    fractionalAnisotropy( const std::string &filename );
    fractionalAnisotropy(){}
    ~fractionalAnisotropy();

    std::vector< float >* getMainDirData()
    { 
        return &m_value;
    }

    // From DatasetInfo
    bool load();
    //void saveNifti( std_string fileName );
    std::vector< float > m_value;
    float getFractionalAnisotropy(double* position);
    float getVoxelSizeX() { return m_voxelSizeX; }
    float getVoxelSizeY() { return m_voxelSizeY; }
    float getVoxelSizeZ() { return m_voxelSizeZ; }
    int GetNbColumns() { return m_columns; }
    int GetNbRows() { return m_rows; }
    int GetNbFrames() { return m_frames; }


private:    
    std::vector< float > m_fileFloatData;
    string m_filename;

    int m_columns;
    int m_rows;
    int m_frames;
    int m_bands;

    int m_dataType;

    float m_voxelSizeX;
    float m_voxelSizeY;
    float m_voxelSizeZ;
};

#endif /* fractionalAnisotropy_H_ */
