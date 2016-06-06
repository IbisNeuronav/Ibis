/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __SimplePropCreator_h_
#define __SimplePropCreator_h_

#include "SVL.h"
#include <vector>

class vtkProp3D;

class SimplePropCreator
{
public:

    static vtkProp3D * CreateLine( double start[3], double end[3], double color[4] );
    static vtkProp3D * CreatePath( std::vector< Vec3 > & points, double color[4] );
    static vtkProp3D * CreateSphere( double center[3], double radius, double color[4] );

private:
    // This class in not just a collection of static functions
    SimplePropCreator();
    ~SimplePropCreator();
};

#endif
