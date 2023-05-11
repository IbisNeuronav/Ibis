/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef IBISOPENCVMATH_H
#define IBISOPENCVMATH_H

#include <opencv2/opencv.hpp>
#include <vector>

#include "SVL.h"

class vtkMatrix4x4;

namespace IbisMath
{
// cv::point operations
static cv::Point3f AverageCvPoint3f( std::vector<cv::Point3f> & all );
static cv::Point2f AverageCvPoint2f( std::vector<cv::Point2f> & all );

// VTK mat 4 and Cv::Point3f
static cv::Point3f MultMat4CvPoint3f( vtkMatrix4x4 * mat, cv::Point3f & in );

// SVL vec and cv vec
static cv::Point3f Vec3ToCVPoint3f( Vec3 & v );
static Vec2 CVPoint2fToVec2( cv::Point2f & p );
};  // namespace IbisMath

#endif
