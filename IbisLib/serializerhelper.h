/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __SerializerHelper_h_
#define __SerializerHelper_h_

class vtkGenericParamValues;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class Serializer;
class Vec3;
class vtkMatrix4x4;

/** @name Serializer Helper
*   @brief This file contains a collection of functions used to serialize vtk classes or
* classes from third party libs that are often used in Ibis
*
*/

bool Serialize( Serializer * serial, const char * attrName, vtkGenericParamValues & value );
bool Serialize( Serializer * serial, const char * attrName, vtkColorTransferFunction * colorFunc );
bool Serialize( Serializer * serial, const char * attrName, vtkPiecewiseFunction * opacityFunc );
bool Serialize( Serializer * serial, const char * attrName, Vec3 & v );
bool Serialize( Serializer * serial, const char * attrName, vtkMatrix4x4 * mat );

#endif
