/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMNIOBJReader.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMNIOBJReader - read MNI .obj files
// .SECTION Description
// vtkMNIOBJReader is a source object that reads MNI .obj
// files. The output of this source object is polygonal data.
// .SECTION See Also


#ifndef __vtkMNIOBJReader_h
#define __vtkMNIOBJReader_h

#include "vtkPolyDataSource.h"

class vtkProperty;
class vtkCellArray;

class vtkMNIOBJReader : public vtkPolyDataSource
{
public:
  static vtkMNIOBJReader *New();
  vtkTypeRevisionMacro(vtkMNIOBJReader,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  int CanReadFile( const char * fname );

  // Description:
  // Specify file name of MNI .obj file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Return properties of MNI .obj file
  vtkGetObjectMacro(Property,vtkProperty);

	vtkSetMacro( UseAlpha, bool );
	
protected:

	void ReadLines( FILE * in );
	void ReadPolygons( FILE * in );
	void ReadPoints( FILE * in );
	void ReadColors( FILE * in );
	void ReadItems( FILE * in, vtkCellArray * indexCells );

  vtkMNIOBJReader();
  ~vtkMNIOBJReader();
  vtkProperty * Property;

  void Execute();

  char *FileName;

  int NbPoints;
  int NbItems;
	bool UseAlpha;

private:
  
	vtkMNIOBJReader(const vtkMNIOBJReader&);  // Not implemented.
  void operator=(const vtkMNIOBJReader&);  // Not implemented.
};

#endif


