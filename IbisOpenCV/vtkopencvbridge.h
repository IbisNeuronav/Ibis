/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef VTKOPENCVBRIDGE_H
#define VTKOPENCVBRIDGE_H

#include <opencv2/opencv.hpp>

class vtkMatrix4x4;

class VtkOpenCvBridge
{
public:
    static void RvecTvecToMatrix4x4( const cv::Mat & tVec, const cv::Mat & rVec, vtkMatrix4x4 * mat );
};

#endif
