/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkColorPolyData - Colorize points of the polydata according to coordinate
// .SECTION Description

#ifndef vtkColorPolyData_h
#define vtkColorPolyData_h

#include "vtkDataSetAlgorithm.h"

class vtkColorPolyData : public vtkDataSetAlgorithm
{

public:

  vtkTypeMacro(vtkColorPolyData,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with s,t range=(0,1) and automatic plane generation turned on.
  static vtkColorPolyData *New();

  // Description:
  // Specify the bounding box inside of which RGB values vary
  void SetBounds(const double bounds[6]);
  void SetBounds(double xmin, double xmax, double ymin, double ymax,
                 double zmin, double zmax);

protected:
  vtkColorPolyData();
  ~vtkColorPolyData() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Bounds[6];

private:
  vtkColorPolyData(const vtkColorPolyData&);  // Not implemented.
  void operator=(const vtkColorPolyData&);  // Not implemented.
};

#endif
