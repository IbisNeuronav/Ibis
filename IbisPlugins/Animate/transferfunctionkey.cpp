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

#include "transferfunctionkey.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "SVL.h"
#include "serializerhelper.h"

ObjectSerializationMacro( TransferFunctionKey );

TransferFunctionKey::TransferFunctionKey()
{
    frame = 0;
    m_colorFunc = vtkColorTransferFunction::New();
    m_opacityFunc = vtkPiecewiseFunction::New();
}

TransferFunctionKey::TransferFunctionKey( const TransferFunctionKey & other )
{
    frame = other.frame;
    m_colorFunc = vtkColorTransferFunction::New();
    m_colorFunc->DeepCopy( other.m_colorFunc );
    m_opacityFunc = vtkPiecewiseFunction::New();
    m_opacityFunc->DeepCopy( other.m_opacityFunc );
}

TransferFunctionKey::~TransferFunctionKey()
{
    m_colorFunc->Delete();
    m_opacityFunc->Delete();
}

void TransferFunctionKey::operator=( const TransferFunctionKey& other )
{
    frame = other.frame;
    m_colorFunc = vtkColorTransferFunction::New();
    m_colorFunc->DeepCopy( other.m_colorFunc );
    m_opacityFunc = vtkPiecewiseFunction::New();
    m_opacityFunc->DeepCopy( other.m_opacityFunc );
}

void TransferFunctionKey::Serialize( Serializer * ser )
{
    ::Serialize( ser, "Frame", frame );
    ::Serialize( ser, "ColorFunc", m_colorFunc );
    ::Serialize( ser, "OpacityFunc", m_opacityFunc );
}

void TransferFunctionKey::SetFunctions( vtkColorTransferFunction * color, vtkPiecewiseFunction * opacity )
{
    m_colorFunc->DeepCopy( color );
    m_opacityFunc->DeepCopy( opacity );
}

void TransferFunctionKey::Interpolate( TransferFunctionKey & keyA, TransferFunctionKey & keyB, double ratio )
{
    // Interpolate color function
    m_colorFunc->RemoveAllPoints();
    int keyASize = keyA.m_colorFunc->GetSize();
    int keyBSize = keyB.m_colorFunc->GetSize();
    vtkColorTransferFunction * colA = keyA.m_colorFunc;
    vtkColorTransferFunction * colB = keyB.m_colorFunc;
    for( int i = 0; i < keyASize || i < keyBSize; ++i )
    {
        if( i < keyASize && i < keyBSize )
        {
            double nodeA[6];
            colA->GetNodeValue( i, nodeA );
            double nodeB[6];
            colB->GetNodeValue( i, nodeB );
            double interpolated[6];
            for( int j = 0; j < 6; ++j )
            {
                interpolated[j] = mix( nodeA[j], nodeB[j], ratio );
            }
            m_colorFunc->AddRGBPoint( interpolated[0], interpolated[1], interpolated[2], interpolated[3], interpolated[4], interpolated[5] );
        }
        else if( i < keyASize )
        {
            double nodeA[6];
            colA->GetNodeValue( i, nodeA );
            m_colorFunc->AddRGBPoint( nodeA[0], nodeA[1], nodeA[2], nodeA[3], nodeA[4], nodeA[5] );
        }
        else
        {
            double nodeB[6];
            colB->GetNodeValue( i, nodeB );
            m_colorFunc->AddRGBPoint( nodeB[0], nodeB[1], nodeB[2], nodeB[3], nodeB[4], nodeB[5] );
        }
    }

    // Interpolate opacity function
    m_opacityFunc->RemoveAllPoints();
    keyASize = keyA.m_opacityFunc->GetSize();
    keyBSize = keyB.m_opacityFunc->GetSize();
    vtkPiecewiseFunction * opaA = keyA.m_opacityFunc;
    vtkPiecewiseFunction * opaB = keyB.m_opacityFunc;
    for( int i = 0; i < keyASize || i < keyBSize; ++i )
    {
        if( i < keyASize && i < keyBSize )
        {
            double nodeA[4];
            opaA->GetNodeValue( i, nodeA );
            double nodeB[4];
            opaB->GetNodeValue( i, nodeB );
            double interp[4];
            for( int j = 0; j < 4; ++j )
                interp[j] = mix( nodeA[j], nodeB[j], ratio );
            m_opacityFunc->AddPoint( interp[0], interp[1], interp[2], interp[3] );
        }
        else if( i < keyASize )
        {
            double nodeA[4];
            opaA->GetNodeValue( i, nodeA );
            m_opacityFunc->AddPoint( nodeA[0], nodeA[1], nodeA[2], nodeA[3] );
        }
        else
        {
            double nodeB[4];
            opaB->GetNodeValue( i, nodeB );
            m_opacityFunc->AddPoint( nodeB[0], nodeB[1], nodeB[2], nodeB[3] );
        }
    }
}

ObjectSerializationMacro( TFAnimation );

void TFAnimation::Serialize( Serializer * ser )
{
    ::Serialize( ser, "Keys", m_keys );
}
