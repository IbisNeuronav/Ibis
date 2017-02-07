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

// .NAME vtkVideoSource2Factory -
// .SECTION Description

#ifndef __vtkVideoSource2Factory_h
#define __vtkVideoSource2Factory_h

#include "vtkObject.h"
#include <vector>
#include <string>

class vtkVideoSource2;

#define GenericVideoDeviceTypeName "Generic Device"

class vtkVideoSource2Factory : public vtkObject
{

public:

  vtkTypeMacro(vtkVideoSource2Factory,vtkObject);
  static vtkVideoSource2Factory *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create and return an instance of the named vtk video source2 class.
  vtkVideoSource2 * CreateInstance( const char * typeName );

  // Description:
  // The next functions can be used to determine which types of source types are available
  // On the current platform
  int GetNumberOfTypes();
  const char * GetTypeName( int index );

protected:

  vtkVideoSource2Factory();

  std::vector< std::string > m_typeNames;

private:
    
  vtkVideoSource2Factory(const vtkVideoSource2Factory&);  // Not implemented.
  void operator=(const vtkVideoSource2Factory&);  // Not implemented.
};

#endif
