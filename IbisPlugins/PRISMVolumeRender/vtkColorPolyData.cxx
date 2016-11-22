/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkColorPolyData.h"

#include "vtkDataSet.h"
#include "vtkUnsignedCharArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkColorPolyData);

vtkColorPolyData::vtkColorPolyData()
{
    this->Bounds[0] = 0.0;
    this->Bounds[1] = 1.0;
    this->Bounds[2] = 0.0;
    this->Bounds[3] = 1.0;
    this->Bounds[4] = 0.0;
    this->Bounds[5] = 1.0;
}

void vtkColorPolyData::SetBounds(const double bounds[6])
{
    int i = 0;
    for (int i=0; i<6; i++)
    {
        if ( this->Bounds[i] != bounds[i] )
        {
            break;
        }
    }
    if ( i >= 6 )
    {
        return; //same as before don't modify
    }

    for( int i = 0; i < 6; ++i )
        this->Bounds[i] = bounds[i];

    // okay, need to allocate stuff
    this->Modified();
}

void vtkColorPolyData::SetBounds(double xmin, double xmax, double ymin, double ymax,
                          double zmin, double zmax)
{
    double bounds[6];
    bounds[0] = xmin;
    bounds[1] = xmax;
    bounds[2] = ymin;
    bounds[3] = ymax;
    bounds[4] = zmin;
    bounds[5] = zmax;

    this->SetBounds(bounds);
}

int vtkColorPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  int numPts = input->GetNumberOfPoints();

  //  Allocate color data
  //
  // Setup the colors array
  vtkUnsignedCharArray * colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  colors->SetName("Colors");
  colors->SetNumberOfTuples(numPts);

  int progressInterval = numPts/20 + 1;

  double p[3];
  double col[3];
  double colRange[2] = { 0.0, 1.0 };
  int abort = 0;
  for (int i=0; i<numPts && !abort; i++)
  {
      if ( !(i % progressInterval) )
      {
          this->UpdateProgress((double)i/numPts);
          abort = this->GetAbortExecute();
      }

      output->GetPoint(i, p);
      for (int j=0; j<3; j++)
      {
          double val = ( p[j] - this->Bounds[2*j] ) / ( this->Bounds[2*j+1] - this->Bounds[2*j] );
          vtkMath::ClampValue( &val, colRange );
          col[j] = val * 255.0;
      }

      colors->SetTuple(i,col);
  }

  output->GetPointData()->SetScalars( colors );
  colors->Delete();

  return 1;
}

void vtkColorPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: (" << this->Bounds[0] << ", "
     << this->Bounds[1] << ", " << this->Bounds[2] << " )\n";
}
