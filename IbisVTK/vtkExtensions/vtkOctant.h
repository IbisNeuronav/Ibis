/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef VTKOCTANT_H
#define VTKOCTANT_H
#include <vtkBox.h>

class vtkOctant : public vtkBox
{
public:
    vtkTypeMacro(vtkOctant, vtkBox);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description
    // Construct box with center at (0,0,0) and each side of length 1.0.
    static vtkOctant *New();

    //Description
    // Set octant number will also initialize octant box using GlobalBounds
    void SetOctantNumber(int);
    //Description
    //get octant number
    vtkGetMacro(OctantNumber,int);

    // Description:
    // Set / get the global bounding box including all 8 octants
    // the octants intersect at origin
    void SetGlobalBounds(double bounds[6]);
    void GetGlobalBounds(double bounds[6]);
    double *GetGlobalBounds() {return GlobalBounds;}
    void GetOrigin(double origin[3]);
    double *GetOrigin() {return Origin;}
    void SetOrigin(double origin[3]);
    // Description:
    // Compute octant bounding box depending on new division point
    void ComputeBounds(double newValue, int axis);

protected:
    // Description:
    // Compute octant bounding box
    void ComputeInitialBounds();

    double GlobalBounds[6];
    double Origin[3];
    int OctantNumber;
    // octants: x   y   z
    //      0   +   +   +
    //      1   -   +   +
    //      2   -   -   +
    //      3   +   -   +
    //      4   +   +   -
    //      5   -   +   -
    //      6   -   -   -
    //      7   +   -   -

    vtkOctant();
    ~vtkOctant();


private:
  vtkOctant(const vtkOctant&);  // Not implemented.
  void operator=(const vtkOctant&);  // Not implemented.
};

#endif // VTKOCTANT_H
