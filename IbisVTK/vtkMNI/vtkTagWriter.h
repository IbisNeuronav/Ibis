/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkTagWriter - Write vtkPoints into MNI tag files
// .SECTION Description

#ifndef __vtkTagWriter_h
#define __vtkTagWriter_h

#include <string>
#include <vector>
#include <vtkObject.h>

class vtkPoints;

class vtkTagWriter : public vtkObject
{

public:

    static vtkTagWriter * New() { return new vtkTagWriter; }

    vtkTypeMacro(vtkTagWriter,vtkObject);

    vtkSetStringMacro(FileName);
    
    void Write();
    
    void AddVolume( vtkPoints * volume, const char * name );

    void SetPointNames( std::vector<std::string> & names );

    void SetReferenceDataFile(const char * ts);
    void SetTimeStamps(std::vector<std::string> & ts);
    void SetTransformToSave(const char * ts);

    void PrintSelf(ostream &os, vtkIndent indent) override;

protected:

    char * FileName;
    typedef std::vector<vtkPoints*> PointsVec;
    PointsVec Volumes;
    char TransformToSave[256];
    char ReferenceDataFile[128];
    std::vector<std::string> VolumeNames;
    std::vector<std::string> PointNames;
    std::vector<std::string> TimeStamps;

    vtkTagWriter();
    ~vtkTagWriter();

private:
    
    vtkTagWriter(const vtkTagWriter&);    // Not implemented.
    void operator=(const vtkTagWriter&);  // Not implemented.

};

#endif




