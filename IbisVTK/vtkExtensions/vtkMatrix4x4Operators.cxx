/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "vtkMatrix4x4Operators.h"
#include <vtkMatrix4x4.h>
#include <vtkMath.h>

void vtkMatrix4x4Operators::AddMatrix( vtkMatrix4x4 * dst, vtkMatrix4x4 * src )
{
    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            double tmp = dst->GetElement( i, j ) + src->GetElement( i, j );
            dst->SetElement( i, j, tmp );
        }
    }
}

void vtkMatrix4x4Operators::MatrixMultScalar( vtkMatrix4x4 * mat, double scalar )
{
    MatrixMultScalar( mat, mat, scalar );
}

void vtkMatrix4x4Operators::MatrixMultScalar( vtkMatrix4x4 * src, vtkMatrix4x4 * dst, double scalar )
{
    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            dst->SetElement( i, j, src->GetElement( i, j ) * scalar );
        }
    }
}

void vtkMatrix4x4Operators::MultiplyVector( vtkMatrix4x4 * mat, double in[3], double out[3] )
{
    double x = mat->GetElement( 0, 0 )*in[0] + mat->GetElement( 0, 1 )*in[1] + mat->GetElement( 0, 2 )*in[2];
    double y = mat->GetElement( 1, 0 )*in[0] + mat->GetElement( 1, 1 )*in[1] + mat->GetElement( 1, 2 )*in[2];
    double z = mat->GetElement( 2, 0 )*in[0] + mat->GetElement( 2, 1 )*in[1] + mat->GetElement( 2, 2 )*in[2];

    out[0] = x;
    out[1] = y;
    out[2] = z;
}

void vtkMatrix4x4Operators::MeanMatrix( std::vector< vtkMatrix4x4 * > & allMatrices, vtkMatrix4x4 * dst )
{
    dst->Zero();
    for( int i = 0; i < allMatrices.size(); ++i )
        AddMatrix( dst, allMatrices[i] );
    MatrixMultScalar( dst, 1.0 / double( allMatrices.size()) );
}

void vtkMatrix4x4Operators::MatrixToTransRot( vtkMatrix4x4 * mat, double t[3], double r[3] )
{
    double m21 = mat->GetElement( 2, 1 );
    double m22 = mat->GetElement( 2, 2 );
    double m20 = mat->GetElement( 2, 0 );
    double m00 = mat->GetElement( 0, 0 );
    double m10 = mat->GetElement( 1, 0 );
    r[0] = vtkMath::DegreesFromRadians( atan2( m21, m22 ) );
    r[1] = vtkMath::DegreesFromRadians( atan2( -m20, sqrt( m21 * m21 + m22 * m22 ) ) );
    r[2] = vtkMath::DegreesFromRadians( atan2( m10, m00 ) );

    // Get the inverse of the rotation
    vtkMatrix4x4 * invRot = vtkMatrix4x4::New();
    invRot->DeepCopy( mat );
    invRot->SetElement( 0, 3, 0.0 );
    invRot->SetElement( 1, 3, 0.0 );
    invRot->SetElement( 2, 3, 0.0 );
    invRot->Transpose();

    // Get translation
    vtkMatrix4x4 * trans = vtkMatrix4x4::New();
    trans->Identity();
    trans->SetElement( 0, 3, mat->GetElement( 0, 3 ) );
    trans->SetElement( 1, 3, mat->GetElement( 1, 3 ) );
    trans->SetElement( 2, 3, mat->GetElement( 2, 3 ) );
    vtkMatrix4x4::Multiply4x4( invRot, trans, trans );
    t[0] = trans->GetElement( 0, 3 );
    t[1] = trans->GetElement( 1, 3 );
    t[2] = trans->GetElement( 2, 3 );

    invRot->Delete();
    trans->Delete();
}

// M = Rz * Ry * Rx * T
void vtkMatrix4x4Operators::TransRotToMatrix( double t[3], double r[3], vtkMatrix4x4 * mat )
{
    // Apply rotations
    mat->Identity();
    double sinX = sin( vtkMath::RadiansFromDegrees( r[0] ) );
    double cosX = cos( vtkMath::RadiansFromDegrees( r[0] ) );
    double sinY = sin( vtkMath::RadiansFromDegrees( r[1] ) );
    double cosY = cos( vtkMath::RadiansFromDegrees( r[1] ) );
    double sinZ = sin( vtkMath::RadiansFromDegrees( r[2] ) );
    double cosZ = cos( vtkMath::RadiansFromDegrees( r[2] ) );
    mat->SetElement( 0, 0, cosZ * cosY );
    mat->SetElement( 0, 1, cosZ * sinY * sinX - sinZ * cosX );
    mat->SetElement( 0, 2, cosZ * sinY * cosX + sinZ * sinX );
    mat->SetElement( 1, 0, sinZ * cosY );
    mat->SetElement( 1, 1, sinZ * sinY * sinX + cosZ * cosX );
    mat->SetElement( 1, 2, sinZ * sinY * cosX - cosZ * sinX );
    mat->SetElement( 2, 0, -sinY );
    mat->SetElement( 2, 1, cosY * sinX );
    mat->SetElement( 2, 2, cosY * cosX );

    // Apply translation
    vtkMatrix4x4 * trans = vtkMatrix4x4::New();
    trans->Identity();
    trans->SetElement( 0, 3, t[0] );
    trans->SetElement( 1, 3, t[1] );
    trans->SetElement( 2, 3, t[2] );
    vtkMatrix4x4::Multiply4x4( mat, trans, mat );
    trans->Delete();
}
