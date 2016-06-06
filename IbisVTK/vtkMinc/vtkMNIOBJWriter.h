/*=========================================================================

 Program:   Visualization Toolkit
 Module:    $RCSfile: vtkMNIOBJWriter.h,v $

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMNIOBJWriter - read MNI .obj files
// .SECTION Description
// vtkMNIOBJWriter is a source object that writes MNI .obj
// files. The input of this source object is polygonal data
// and the output may or may not be junk.
// .SECTION See Also


#ifndef __vtkMNIOBJWriter_h
#define __vtkMNIOBJWriter_h

#include "vtkPolyDataSource.h"
#include <ostream>
#include <fstream>

class vtkProperty;
class vtkCellArray;
class vtkPolyData;

class vtkMNIOBJWriter : public vtkPolyDataSource
{
public:
 static vtkMNIOBJWriter* New();
 vtkMNIOBJWriter(char* fname, bool aLine);
 vtkTypeRevisionMacro(vtkMNIOBJWriter,vtkPolyDataSource);
 void PrintSelf(ostream& os, vtkIndent indent);
 void Write(vtkPolyData *output);
 void Write();//vtkPolyData *output);
 
 // Description:
 // Specify file name of MNI .obj file.
 vtkSetStringMacro(FileName);
 vtkGetStringMacro(FileName);

 void SetIsLine(bool itIs){isALine=itIs;}
 bool GetIsLine(){return isALine;}

 // Description:
 // Return properties of MNI .obj file
 void SetProperty(vtkProperty* prop){Property=prop;};

protected:
 void WriteLines(std::ostream& out);
 void WritePolygons(std::ostream& out);
 void WritePoints(std::ostream& out);
 void WriteColors(std::ostream& out);
 void WriteItems( std::ostream& out);
 
 vtkMNIOBJWriter();
 ~vtkMNIOBJWriter();
 vtkProperty* Property;

 char* FileName; 
 bool isALine; //is not a Polygon
 int NbPoints;
 int NbItems;

private:

 vtkMNIOBJWriter(const vtkMNIOBJWriter&);  // Not implemented.
 void operator=(const vtkMNIOBJWriter&);  // Not implemented.
};

#endif

