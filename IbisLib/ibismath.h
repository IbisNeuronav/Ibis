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

#include "SVL.h"
#include <vector>
#include "opencv2/opencv.hpp"

class vtkMatrix4x4;

class IbisMath
{

public:

    // Scalar operations
    static double Average( std::vector<double> & samples );
    static double StdDev( std::vector<double> & samples, double average );
    static double StdDev( std::vector<double> & samples );

    // SVL vec operations
    static Vec3 AverageVec3( std::vector<Vec3> & allVecs );
    static Vec3 StdDevVec3( std::vector<Vec3> & allVecs );

    // cv::point operations
    static cv::Point3f AverageCvPoint3f( std::vector<cv::Point3f> & all );
    static cv::Point2f AverageCvPoint2f( std::vector<cv::Point2f> & all );

    // VTK mat 4 and SVL Vec3
    static Vec3 Translation( vtkMatrix4x4 * mat );
    static void MultMat4Vec3( vtkMatrix4x4 * mat, const Vec3 & in, Vec3 & out );
    static Vec3 MultMat4Point3( vtkMatrix4x4 * mat, const Vec3 & in );

    // VTK mat 4 and Cv::Point3f
    static cv::Point3f MultMat4CvPoint3f( vtkMatrix4x4 * mat, cv::Point3f & in );

    // SVL vec and cv vec
    static cv::Point3f Vec3ToCVPoint3f( Vec3 & v );
    static Vec2 CVPoint2fToVec2( cv::Point2f & p );

    // VTK mat4 utilities
    static vtkMatrix4x4 * DuplicateMat4( vtkMatrix4x4 * in );
    static void ClearMat4Array( std::vector< vtkMatrix4x4 * > & allMats );

};

#endif
