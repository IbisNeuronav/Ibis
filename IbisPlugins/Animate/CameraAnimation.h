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

#ifndef __CameraAnimation_h_
#define __CameraAnimation_h_

#include "Animation.h"
#include "CameraKey.h"
#include "serializer.h"
#include <vtkSmartPointer.h>

class vtkCameraInterpolator;
class vtkCamera;

class CameraAnimation : public Animation< CameraKey >
{
	
public:
	
	CameraAnimation();
    virtual ~CameraAnimation();
	
	void AddKeyframe( int frame, vtkCamera * cam );
    virtual void MoveKey( int oldFrame, int newFrame );
    void RemoveKeyframe( int frame );
	void ComputeFrame( int frame, vtkCamera * cam );

    void Serialize( Serializer * ser );
	
protected:
	
    void UpdateInterpolator();
	
    vtkSmartPointer<vtkCameraInterpolator> m_interpolator;
	
};

ObjectSerializationHeaderMacro( CameraAnimation );

#endif
