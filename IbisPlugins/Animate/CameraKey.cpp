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

#include "CameraKey.h"
#include "stdio.h"
#include "serializerhelper.h"

ObjectSerializationMacro( CameraKey );

CameraKey::CameraKey()
{
	frame = 0;
	pos.MakeUnit( 2, 0.0f );
	target.MakeZero();
	up.MakeUnit( 1, 0.0f );
	viewAngle = 30.0f;
}

CameraKey::~CameraKey() { }

void CameraKey::Interpolate( CameraKey & keyA, CameraKey & keyB, double ratio )
{
	pos.Interp( keyA.pos, keyB.pos, ratio );
	target.Interp( keyA.target, keyB.target, ratio );
	up.Interp( keyA.up, keyB.up, ratio );
	viewAngle = mix( keyA.viewAngle, keyB.viewAngle, ratio );
}

void CameraKey::Serialize( Serializer * ser )
{
    ::Serialize( ser, "Frame", frame );
    ::Serialize( ser, "Pos", pos );
    ::Serialize( ser, "Target", target );
    ::Serialize( ser, "Up", up );
    ::Serialize( ser, "ViewAngle", viewAngle );
}
