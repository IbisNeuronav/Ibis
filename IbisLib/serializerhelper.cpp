/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "serializerhelper.h"
#include "serializer.h"
#include "vtkGenericParam.h"
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkMatrix4x4.h>
#include "SVL.h"

bool Serialize( Serializer * serial, const char * attrName, vtkGenericParamValues & value )
{
    if( serial->BeginSection( attrName ) )
    {
        std::string paramName( value.GetParamName() );
        Serialize( serial, "ParamName", paramName );
        if( serial->IsReader() )
            value.SetParamName( paramName.c_str() );
        Serialize( serial, "Items", value.GetAllItems() );
        serial->EndSection();
        return true;
    }
    return false;
}

bool Serialize( Serializer * serial, const char * attrName, vtkColorTransferFunction * colorFunc )
{
    if( serial->BeginSection( attrName ) )
    {
        if( serial->IsReader() )
            colorFunc->RemoveAllPoints();

        int nbColorPoints = colorFunc->GetSize();
        ::Serialize( serial, "NbColorPoints", nbColorPoints );

        for( int i = 0; i < nbColorPoints; ++i )
        {
            double nodeValue[6];
            if( !serial->IsReader() )
                colorFunc->GetNodeValue( i, nodeValue );
            QString elemName = QString( "ColorPoint_%1" ).arg(i);
            Serialize( serial, elemName.toUtf8().data(), nodeValue, 6 );
            if( serial->IsReader() )
                colorFunc->AddRGBPoint( nodeValue[0], nodeValue[1], nodeValue[2], nodeValue[3], nodeValue[4], nodeValue[5] );
        }
        serial->EndSection();
        return true;
    }
    return false;
}


bool Serialize( Serializer * serial, const char * attrName, vtkPiecewiseFunction * opacityFunc )
{
    if( serial->BeginSection( attrName ) )
    {
        if( serial->IsReader() )
            opacityFunc->RemoveAllPoints();

        int nbPoints = opacityFunc->GetSize();
        ::Serialize( serial, "NbPoints", nbPoints );

        for( int i = 0; i < nbPoints; ++i )
        {
            double nodeValue[4];
            if( !serial->IsReader() )
                opacityFunc->GetNodeValue( i, nodeValue );
            QString elemName = QString( "Point_%1" ).arg(i);
            Serialize( serial, elemName.toUtf8().data(), nodeValue, 4 );
            if( serial->IsReader() )
                opacityFunc->AddPoint( nodeValue[0], nodeValue[1], nodeValue[2], nodeValue[3] );
        }
        serial->EndSection();
        return true;
    }
    return false;
}

bool Serialize( Serializer * serial, const char * attrName, Vec3 & v )
{
    return serial->Serialize( attrName, v.Ref(), 3 );
}

bool Serialize( Serializer * serial, const char * attrName, vtkMatrix4x4 * mat )
{
    double * elem = (double*)mat->Element;
    return Serialize( serial, attrName, elem, 16 );
}
