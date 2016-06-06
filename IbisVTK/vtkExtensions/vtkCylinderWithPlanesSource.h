/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// .NAME vtkCylinderWithPlanesSource - create a 2D circle with a cross in it
// .SECTION Description
// vtkCylinderWithPlanesSource is a source object that creates a polyline that
// forms a circle with a cross in the middle. The circle is centered at
// ( 0, 0 ). The parameters are the radius of the circle and the 
// number of lines used to draw it.

#ifndef __vtkCylinderWithPlanesSource_h
#define __vtkCylinderWithPlanesSource_h

#include <vtkObject.h>
#include <vtkTransform.h>
class vtkAppendPolyData;
class vtkPolyData;
class vtkTransformPolyDataFilter;
class vtkCylinderSource;
class vtkPlaneSource;
class vtkMatrix4x4;

class vtkCylinderWithPlanesSource : public vtkObject
{

public:

    static vtkCylinderWithPlanesSource *New();
    vtkTypeMacro(vtkCylinderWithPlanesSource,vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description
    // Get final output
    vtkPolyData *GetOutput();

    // Description:
    // The cylinder Resolution
    void SetResolution(int res);
    vtkGetMacro(Resolution,int);

    // Description:
    // The radius of the cylinder.
    void SetRadius(double radius);
    vtkGetMacro(Radius,double);

    // Description:
    // The center of the cylinder.
    double * GetCenter() {return Center;}
    void GetCenter(double c[3]) {for (int i = 0; i < 3; i++) c[i] = Center[i];}
    void SetCenter(double c[3]);
    void SetCenter(double x, double y, double z);

    // Description:
    // The position of the cylinder - coordinates of the center of the "bottom".
    double * GetPosition() {return this->Transform->GetPosition();}
    void GetPosition(double pos[3]) {for (int i = 0; i < 3; i++) pos[i] = this->Transform->GetPosition()[i];}
    void SetPosition(double pos[3]);
    void SetPosition(double x, double y, double z);

    // Description
    // Set cylinder orientation
    void SetOrientation(int);

    // Description
    // Set cylinder rotation
    void SetRotation(vtkMatrix4x4 *mat);

    // Description:
    // The Height of the cylinder.
    void SetHeight(double height);
    vtkGetMacro(Height,double);

    // Description:
    // Set World Transform.
    void SetWorldTransform(vtkTransform *tr);

    // Description:
    // update cylinder and planes
    void Update();

protected:

    vtkCylinderWithPlanesSource( );
    ~vtkCylinderWithPlanesSource();

    void BuildRepresentation();

    vtkPlaneSource *Plane1;
    vtkPlaneSource *Plane2;
    vtkCylinderSource *Cylinder;
    vtkAppendPolyData *CylinderAndPlanes;
    vtkTransformPolyDataFilter *ModifiedCylinderAndPlanes;
    vtkTransform *Transform;
    vtkTransform *AppliedTransform;
    vtkTransform *WorldTransform;
    double Height;
    double Center[3];
    double Radius;
    int Resolution;
    int Orientation; //0 along y, 1 along z, 2 along x

private:

    vtkCylinderWithPlanesSource(const vtkCylinderWithPlanesSource&);  // Not implemented.
    void operator=(const vtkCylinderWithPlanesSource&);            // Not implemented.
};


#endif


