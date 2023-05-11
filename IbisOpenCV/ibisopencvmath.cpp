/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "ibisopencvmath.h"

#include <vtkMatrix4x4.h>

#include "vtkMatrix4x4Operators.h"

cv::Point3f IbisMath::AverageCvPoint3f( std::vector<cv::Point3f> & all )
{
    cv::Point3f res( 0.0, 0.0, 0.0 );
    for( int i = 0; i < all.size(); ++i )
    {
        res += all[ i ];
    }
    res *= 1.0 / all.size();
    return res;
}

cv::Point2f IbisMath::AverageCvPoint2f( std::vector<cv::Point2f> & all )
{
    cv::Point2f res( 0.0, 0.0 );
    for( int i = 0; i < all.size(); ++i )
    {
        res += all[ i ];
    }
    res *= 1.0 / all.size();
    return res;
}

cv::Point3f IbisMath::MultMat4CvPoint3f( vtkMatrix4x4 * mat, cv::Point3f & in )
{
    double in4[ 4 ];
    in4[ 0 ]         = in.x;
    in4[ 1 ]         = in.y;
    in4[ 2 ]         = in.z;
    in4[ 3 ]         = 1.0;
    double out4[ 4 ] = { 0.0, 0.0, 0.0, 1.0 };
    mat->MultiplyPoint( in4, out4 );
    return cv::Point3f( out4[ 0 ], out4[ 1 ], out4[ 2 ] );
}

cv::Point3f IbisMath::Vec3ToCVPoint3f( Vec3 & v ) { return cv::Point3f( v[ 0 ], v[ 1 ], v[ 2 ] ); }

Vec2 IbisMath::CVPoint2fToVec2( cv::Point2f & p ) { return Vec2( p.x, p.y ); }
