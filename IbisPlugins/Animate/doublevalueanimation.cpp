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

#include "doublevalueanimation.h"
#include "SVL.h"

ObjectSerializationMacro( DoubleValueKey );

void DoubleValueKey::Interpolate( DoubleValueKey & keyA, DoubleValueKey & keyB, double r )
{
    double ratio = r*r*r*( r*(r*6 - 15) + 10 );
    m_value = mix( keyA.m_value, keyB.m_value, ratio );
}

void DoubleValueKey::Serialize( Serializer * ser )
{
    ::Serialize( ser, "Frame", frame );
    ::Serialize( ser, "Value", m_value );
}

ObjectSerializationMacro( DoubleValueAnimation );

DoubleValueAnimation::DoubleValueAnimation()
{
}

void DoubleValueAnimation::Serialize( Serializer * ser )
{
    ::Serialize( ser, "Keys", m_keys );
}
