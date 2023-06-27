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

#ifndef __vtkMatrix4x4Operators_h_
#define __vtkMatrix4x4Operators_h_

#include <vector>

class vtkMatrix4x4;

class vtkMatrix4x4Operators
{
public:
    // Add src to dst
    static void AddMatrix( vtkMatrix4x4 * dst, vtkMatrix4x4 * src );

    // Multiply all elements of mat by scalar
    static void MatrixMultScalar( vtkMatrix4x4 * mat, double scalar );
    static void MatrixMultScalar( vtkMatrix4x4 * src, vtkMatrix4x4 * dst, double scalar );

    // Multiply a vector (as opposed to a point). Basically, doesn't translate
    static void MultiplyVector( vtkMatrix4x4 * mat, double in[3], double out[3] );

    // Compute the mean of a vector of matrices
    static void MeanMatrix( std::vector<vtkMatrix4x4 *> & allMatrices, vtkMatrix4x4 * dst );

    // Convert back and forth between matrix and (translation,rotation) pair (assuming matrix doesn't contain scale)
    static void MatrixToTransRot( vtkMatrix4x4 * mat, double t[3], double r[3] );
    static void TransRotToMatrix( double t[3], double r[3], vtkMatrix4x4 * mat );
};

#endif
