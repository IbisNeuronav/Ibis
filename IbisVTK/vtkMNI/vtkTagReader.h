/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkTagReader - Read MNI tag files into vtkPoints
// .SECTION Description

#ifndef __vtkTagReader_h
#define __vtkTagReader_h

#include <vtkAlgorithm.h>
#include <vtkMatrix4x4.h>

#include <string>
#include <vector>

class vtkPoints;

class vtkTagReader : public vtkAlgorithm
{
public:
    static vtkTagReader * New();
    vtkTypeMacro( vtkTagReader, vtkAlgorithm );

    vtkSetStringMacro( FileName );
    virtual int CanReadFile( const char * fname );
    virtual void Update() override;

    char * GetReferenceDataFileName();
    vtkMatrix4x4 * GetSavedTransform();

    // Get the output of the reader
    size_t GetNumberOfVolumes();
    vtkPoints * GetVolume( int volumeIndex );
    std::vector<std::string> & GetPointNames() { return PointNames; }
    std::vector<std::string> & GetVolumeNames() { return VolumeNames; }
    std::vector<std::string> & GetTimeStamps() { return TimeStamps; }

    virtual void PrintSelf( ostream & os, vtkIndent indent ) override;

    // BTX
protected:
    char * FileName;
    typedef std::vector<vtkPoints *> PointsVec;
    PointsVec Volumes;
    std::vector<std::string> PointNames;
    std::vector<std::string> VolumeNames;
    std::vector<std::string> TimeStamps;
    vtkMatrix4x4 * SavedTransform;
    char ReferenceDataFile[128];

    void ClearOutput();

    vtkTagReader();
    ~vtkTagReader();

private:
    vtkTagReader( const vtkTagReader & );    // Not implemented.
    void operator=( const vtkTagReader & );  // Not implemented.
    // ETX
};
#endif
