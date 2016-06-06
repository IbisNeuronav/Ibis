/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef OCTANTS_H
#define OCTANTS_H

#include <QObject>
#include <vtkObject.h>

#include "vtkOctant.h"

class vtkTransform;

class Octants : public QObject, public vtkObject
{
    Q_OBJECT
public:
    static Octants * New() { return new Octants; }
    vtkTypeMacro(Octants,vtkObject);

    virtual ~Octants();

    // Description
    // set/get octant number
    void SetOctantNumber(int);
    int GetOctantNumber();

    //Description
    // Get the octant of interest
    vtkOctant * GetOctantOfInterest() {return m_octantsOfInterest;}

    //Description
    // Set transform
    void SetTransform (vtkTransform *t);

    // Description:
    // Set / get the global bounding box including all 8 octants
    // the octants intersect at origin
    void SetBounds(double bounds[6]);
    void GetBounds(double bounds[6]);
    void GetOrigin(double origin[3]);
    void SetOrigin(double origin[3]);

    // Description
    // Update octant bounds
    void UpdateOctantOfInterest(double val, int axis);

protected:
    vtkOctant * m_octantsOfInterest;

    Octants();
};

#endif // OCTANTS_H
