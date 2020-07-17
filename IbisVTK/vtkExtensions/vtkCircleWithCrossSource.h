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

// .NAME vtkCircleWithCrossSource - create a 2D circle with a cross in it
// .SECTION Description
// vtkCircleWithCrossSource is a source object that creates a polyline that 
// forms a circle with a cross in the middle. The circle is centered at
// ( 0, 0 ). The parameters are the radius of the circle and the 
// number of lines used to draw it.

#ifndef __vtkCircleWithCrossSource_h
#define __vtkCircleWithCrossSource_h

#include <vtkPolyDataAlgorithm.h>

class vtkPoints;
class vtkInformationVector;

class vtkCircleWithCrossSource : public vtkPolyDataAlgorithm
{

public:

    static vtkCircleWithCrossSource *New();
    vtkTypeMacro(vtkCircleWithCrossSource,vtkPolyDataAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    // Description:
    // The circle will be approximated with Resolution
    // line segments, number of segments will be set to pow(2, 2+res)
    vtkSetClampMacro(Resolution,int,0,VTK_INT_MAX);
    vtkGetMacro(Resolution,int);

    // Description:
    // The radius of the circle.
    vtkSetClampMacro(Radius,double,0.000001,VTK_DOUBLE_MAX);
    vtkGetMacro(Radius,double);

    // Description:
    // Get/Set center of the circle
    vtkSetVector3Macro(Center,double);
    vtkGetVector3Macro(Center,double);

protected:

    vtkCircleWithCrossSource();
    ~vtkCircleWithCrossSource() {}

    virtual int RequestData(vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo);

    double Center[3];
    double Radius;
    int Resolution;

private:

    vtkCircleWithCrossSource(const vtkCircleWithCrossSource&);  // Not implemented.
    void operator=(const vtkCircleWithCrossSource&);            // Not implemented.
};


#endif


