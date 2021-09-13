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

#ifndef __DoubleValueKey_h_
#define __DoubleValueKey_h_

#include "Animation.h"
#include "serializer.h"

class DoubleValueKey
{

public:

    DoubleValueKey() { frame = 0; m_value = 0.0; }
    DoubleValueKey( const DoubleValueKey & other ) { frame = other.frame; m_value = other.m_value; }
    ~DoubleValueKey() {}

    void operator=(const DoubleValueKey & other ) { frame = other.frame; m_value = other.m_value; }

    void Interpolate( DoubleValueKey & keyA, DoubleValueKey & keyB, double ratio );
    void Serialize( Serializer * ser );

    void SetValue( double val ) { m_value = val; }
    double GetValue() { return m_value; }
    int frame;

protected:

    double m_value;
};

ObjectSerializationHeaderMacro( DoubleValueKey );


class DoubleValueAnimation : public Animation<DoubleValueKey>
{

public:

    DoubleValueAnimation();
    virtual ~DoubleValueAnimation() {}

    void Serialize( Serializer * ser );
};

ObjectSerializationHeaderMacro( DoubleValueAnimation );

#endif
