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

// .NAME vtkMultiTextureMapToPlane - generate multiple sets of texture coordinates by mapping points to plane
// .SECTION Description
// vtkMultiTextureMapToPlane is a filter that generates multiple sets of 2D texture coordinates
// by mapping input dataset points onto a plane. A different plane must be specified for
// each of the set of texture coordinate generated.
// .SECTION See Also
//  vtkMultiImagePlaneWidget

#ifndef __vtkMultiTextureMapToPlane_h
#define __vtkMultiTextureMapToPlane_h

#include "vtkDataSetAlgorithm.h"
#include <vector>

class vtkMultiTextureMapToPlane : public vtkDataSetAlgorithm
{

public:

  vtkTypeMacro(vtkMultiTextureMapToPlane,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct
  static vtkMultiTextureMapToPlane  *New();

  // Description:
  // Specify a point defining the origin of the plane. Used in conjunction with
  // the Point1 and Point2 ivars to specify a map plane.
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);

  // Description:
  // Manage different texture coord sets
  void AddTCoordSet( const char * name, double min, double max );
  void ClearTCoordSets();

protected:

  vtkMultiTextureMapToPlane();
  ~vtkMultiTextureMapToPlane() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Plane specification
  double Origin[3];
  double Point1[3];
  double Point2[3];

  struct RangeInfo
  {
      double xOffset;
      double yOffset;
      std::string name;
  };

  std::vector< RangeInfo > Ranges;


private:

  vtkMultiTextureMapToPlane(const vtkMultiTextureMapToPlane&);  // Not implemented.
  void operator=(const vtkMultiTextureMapToPlane&);  // Not implemented.
};

#endif
