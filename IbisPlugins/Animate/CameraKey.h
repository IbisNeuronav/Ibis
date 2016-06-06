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

#ifndef __CameraKey_h_
#define __CameraKey_h_

#include "SVL.h"
#include "serializer.h"

class CameraKey
{
	
public:
	
	CameraKey();
	~CameraKey();
	
	void Interpolate( CameraKey & keyA, CameraKey & keyB, double ratio );

    void Serialize( Serializer * ser );
	
	int frame;
	Vec3 pos;
	Vec3 target;
	Vec3 up;
	double viewAngle;
	
};

ObjectSerializationHeaderMacro( CameraKey );

#endif
