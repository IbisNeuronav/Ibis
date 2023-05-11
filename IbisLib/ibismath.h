/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef IBISMATH_H
#define IBISMATH_H

#include <vector>

#include "SVL.h"

class vtkMatrix4x4;

class IbisMath
{
public:
    static constexpr const double pi  = 3.14159265358979323846;
    static constexpr const double pi2 = 0.5 * IbisMath::pi;
    static constexpr const double pi4 = 0.25 * IbisMath::pi;

    // Scalar operations
    static double Average( std::vector<double> & samples );
    static double StdDev( std::vector<double> & samples, double average );
    static double StdDev( std::vector<double> & samples );

    // SVL vec operations
    static Vec3 AverageVec3( std::vector<Vec3> & allVecs );
    static Vec3 StdDevVec3( std::vector<Vec3> & allVecs );

    // VTK mat 4 and SVL Vec3
    static Vec3 Translation( vtkMatrix4x4 * mat );
    static void MultMat4Vec3( vtkMatrix4x4 * mat, const Vec3 & in, Vec3 & out );
    static Vec3 MultMat4Point3( vtkMatrix4x4 * mat, const Vec3 & in );

    // VTK mat4 utilities
    static vtkMatrix4x4 * DuplicateMat4( vtkMatrix4x4 * in );
    static void ClearMat4Array( std::vector<vtkMatrix4x4 *> & allMats );
};

#endif
