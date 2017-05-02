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

#include "CameraAnimation.h"
#include <vtkCameraInterpolator.h>
#include <vtkCamera.h>

ObjectSerializationMacro( CameraAnimation );

CameraAnimation::CameraAnimation()
{
    m_interpolator = vtkSmartPointer<vtkCameraInterpolator>::New();
	m_interpolator->SetInterpolationTypeToSpline();
}

CameraAnimation::~CameraAnimation()
{
}

void CameraAnimation::AddKeyframe( int frame, vtkCamera * cam )
{
	CameraKey camKey;
	camKey.frame = frame;
	camKey.pos = cam->GetPosition();
	camKey.target = cam->GetFocalPoint();
	camKey.up = cam->GetViewUp();
	camKey.viewAngle = cam->GetViewAngle();
	AddKey( camKey );

    UpdateInterpolator();
}

void CameraAnimation::MoveKey( int oldFrame, int newFrame )
{
    Animation< CameraKey >::MoveKey( oldFrame, newFrame );
    UpdateInterpolator();
}

void CameraAnimation::RemoveKeyframe( int frame )
{
    this->RemoveKey( frame );
    UpdateInterpolator();
}

void CameraAnimation::UpdateInterpolator()
{
    m_interpolator->Initialize();
	vtkCamera * cam = vtkCamera::New();
	int numberOfKeys = m_keys.size();
	KeyContIt it = m_keys.begin();
	for( int i = 0; i < numberOfKeys; ++i, ++it )
	{
		CameraKey & key = *it;
		cam->SetPosition( key.pos.Ref() );
		cam->SetFocalPoint( key.target.Ref() );
		cam->SetViewUp( key.up.Ref() );
		cam->SetViewAngle( key.viewAngle );
		
		m_interpolator->AddCamera( (double)key.frame, cam );
	}
	cam->Delete();
}

void CameraAnimation::ComputeFrame( int frame, vtkCamera * cam )
{
	if( m_keys.size() >= 3 )
		m_interpolator->InterpolateCamera( (double)frame, cam );
	else if ( m_keys.size() >= 1 )
	{
		CameraKey camKey;
		Animation<CameraKey>::ComputeFrame( frame, camKey );
		cam->SetPosition( camKey.pos.Ref() );
		cam->SetFocalPoint( camKey.target.Ref() );
		cam->SetViewUp( camKey.up.Ref() );
		cam->SetViewAngle( camKey.viewAngle );
	}
}

void CameraAnimation::Serialize( Serializer * ser )
{
    ::Serialize( ser, "Keys", m_keys );
    if( ser->IsReader() )
        UpdateInterpolator();
}
