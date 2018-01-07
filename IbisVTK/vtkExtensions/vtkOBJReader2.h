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

// .NAME vtkOBJReader2 - read Wavefront .obj files
// .SECTION Description
// vtkOBJReader2 is a source object that reads Wavefront .obj
// files. The output of this source object is polygonal data.
// .SECTION See Also
// vtkOBJImporter

#ifndef __vtkOBJReader2_h
#define __vtkOBJReader2_h

#include "vtkPolyDataAlgorithm.h"

class vtkOBJReader2 : public vtkPolyDataAlgorithm
{
public:
  static vtkOBJReader2 *New();
  vtkTypeMacro(vtkOBJReader2,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Specify file name of Wavefront .obj file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkOBJReader2();
  ~vtkOBJReader2();
  
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  char *FileName;
private:
  vtkOBJReader2(const vtkOBJReader2&);  // Not implemented.
  void operator=(const vtkOBJReader2&);  // Not implemented.
};

#endif
