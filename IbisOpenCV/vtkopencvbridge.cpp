/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkopencvbridge.h"
#include <vtkMatrix4x4.h>

void VtkOpenCvBridge::RvecTvecToMatrix4x4( const cv::Mat & tVec, const cv::Mat & rVec, vtkMatrix4x4 * mat )
{
    // Rotation
    cv::Mat camRotation;
    cv::Rodrigues( rVec, camRotation );

    // Convert OpenCV rotation mat to vtk rotation mat, transposing
    // at the same time to get the inverse (calibration gives world to camera
    // space transform and we want camera to world space transform)
    mat->Identity();
    for( int row = 0; row < 3; ++row )
        for( int col = 0; col < 3; ++col )
            mat->SetElement( row, col, camRotation.at<double>( col, row ) );

    // Translation inverse
    double translation[4];
    for( int elem = 0; elem < 3; ++elem )
        translation[elem] = tVec.at<double>( elem, 0 );
    translation[3] = 1.0;
    double poseTranslation[4];
    mat->MultiplyPoint( translation, poseTranslation );

    for( int elem = 0; elem < 3; ++elem )
        mat->SetElement( elem, 3, -1.0 * poseTranslation[ elem ] );

    // Rotate calibration by 180 deg around x. Seems to be a difference of formulation between us and opencv
    vtkMatrix4x4 * rotMatrix = vtkMatrix4x4::New();
    rotMatrix->Identity();
    rotMatrix->SetElement( 1, 1, -1.0 );
    rotMatrix->SetElement( 2, 2, -1.0 );
    vtkMatrix4x4::Multiply4x4( mat, rotMatrix, mat );
}
