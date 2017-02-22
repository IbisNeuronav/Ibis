/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "ibismath.h"
#include "vtkMatrix4x4.h"
#include "vtkMatrix4x4Operators.h"

double IbisMath::Average( std::vector<double> & samples )
{
    double sum = 0.0;
    for( int i = 0; i < samples.size(); ++i )
        sum += samples[i];
    if( samples.size() > 0 )
        sum /= samples.size();
    return sum;
}

double IbisMath::StdDev( std::vector<double> & samples, double average )
{
    double sum = 0.0;
    for( int i = 0; i < samples.size(); ++i )
    {
        double diff = samples[i] - average;
        sum += diff * diff;
    }
    if( samples.size() > 0 )
        sum = sqrt( sum / samples.size() );
    return sum;
}

double IbisMath::StdDev( std::vector<double> & samples )
{
    double average = Average( samples );
    return StdDev( samples, average );
}

Vec3 IbisMath::AverageVec3( std::vector<Vec3> & allVecs )
{
    Vec3 sum( 0.0, 0.0, 0.0 );
    for( int i = 0; i < allVecs.size(); ++i )
    {
        sum += allVecs[i];
    }
    sum /= allVecs.size();
    return sum;
}

Vec3 IbisMath::StdDevVec3( std::vector<Vec3> & allVecs )
{
    Vec3 avg = AverageVec3( allVecs );

    Vec3 sum = Vec3( 0.0, 0.0, 0.0 );
    for( int i = 0; i < allVecs.size(); ++i )
    {
        Vec3 diff = allVecs[i] - avg;
        sum += diff * diff;
    }
    sum /= allVecs.size();
    Vec3 stdDev = Vec3( sqrt(sum[0]), sqrt(sum[1]), sqrt(sum[2]) );
    return stdDev;
}

cv::Point3f IbisMath::AverageCvPoint3f( std::vector<cv::Point3f> & all )
{
    cv::Point3f res( 0.0, 0.0, 0.0 );
    for( int i = 0; i < all.size(); ++i )
    {
        res += all[i];
    }
    res *= 1.0 / all.size();
    return res;
}

cv::Point2f IbisMath::AverageCvPoint2f( std::vector<cv::Point2f> & all )
{
    cv::Point2f res( 0.0, 0.0 );
    for( int i = 0; i < all.size(); ++i )
    {
        res += all[i];
    }
    res *= 1.0 / all.size();
    return res;
}

Vec3 IbisMath::Translation( vtkMatrix4x4 * mat )
{
    return Vec3( mat->GetElement(0,3), mat->GetElement(1,3), mat->GetElement(2,3) );
}

void IbisMath::MultMat4Vec3( vtkMatrix4x4 * mat, const Vec3 & in, Vec3 & out )
{
    vtkMatrix4x4Operators::MultiplyVector( mat, in.Ref(), out.Ref() );
}

Vec3 IbisMath::MultMat4Point3( vtkMatrix4x4 * mat, const Vec3 & in )
{
    Vec4 in4( in, 1.0 );
    mat->MultiplyPoint( in4.Ref(), in4.Ref() );
    return Vec3( in4[0], in4[1], in4[2] );
}

cv::Point3f IbisMath::MultMat4CvPoint3f( vtkMatrix4x4 * mat, cv::Point3f & in )
{
    double in4[4];
    in4[0] = in.x; in4[1] = in.y; in4[2] = in.z; in4[3] = 1.0;
    double out4[4] = { 0.0, 0.0, 0.0, 1.0 };
    mat->MultiplyPoint( in4, out4 );
    return cv::Point3f( out4[0], out4[1], out4[2] );
}

cv::Point3f IbisMath::Vec3ToCVPoint3f( Vec3 & v )
{
    return cv::Point3f( v[0], v[1], v[2] );
}

Vec2 IbisMath::CVPoint2fToVec2( cv::Point2f & p )
{
    return Vec2( p.x, p.y );
}

vtkMatrix4x4 * IbisMath::DuplicateMat4( vtkMatrix4x4 * in )
{
    vtkMatrix4x4 * out = vtkMatrix4x4::New();
    out->DeepCopy( in );
    return out;
}

void IbisMath::ClearMat4Array( std::vector< vtkMatrix4x4 * > & allMats )
{
    for( int i = 0; i < allMats.size(); ++i )
        allMats[i]->Delete();
    allMats.clear();
}
