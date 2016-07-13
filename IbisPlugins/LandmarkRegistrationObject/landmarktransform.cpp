/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "landmarktransform.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkLandmarkTransform.h"
#include "vtkMatrix4x4.h"

LandmarkTransform::LandmarkTransform()
{
    this->Init();
}

LandmarkTransform::~LandmarkTransform()
{
    this->Clear();
}

void LandmarkTransform::SetSourcePoints(vtkPoints *pts)
{
    if (this->SourcePoints == pts)
        return;
    if (this->SourcePoints)
        this->SourcePoints->UnRegister(this);
    this->SourcePoints = pts;
    if (this->SourcePoints)
    {
        this->SourcePoints->Register(this);
    }
}

void LandmarkTransform::SetTargetPoints(vtkPoints *pts)
{
    if (this->TargetPoints == pts)
        return;
    if (this->TargetPoints)
        this->TargetPoints->UnRegister(this);
    this->TargetPoints = pts;
    if (this->TargetPoints)
    {
        this->TargetPoints->Register(this);
    }
}

double LandmarkTransform::GetFRE(int index)
{
    if( this->SourcePoints->GetNumberOfPoints() > 2 &&
        this->FiducialRegistrationErrorMagnitude &&
        index < this->FiducialRegistrationErrorMagnitude->GetNumberOfTuples() && index >= 0 )
    {
        return this->FiducialRegistrationErrorMagnitude->GetValue(index);
    }
    return INVALID_NUMBER;
}

double LandmarkTransform::GetRMS(int index)
{
    if( this->FiducialRegistrationErrorRMS &&
        index < this->FiducialRegistrationErrorRMS->GetNumberOfTuples() && index >= 0 )
    {
        return this->FiducialRegistrationErrorRMS->GetValue(index);
    }
    return INVALID_NUMBER;
}

bool LandmarkTransform::IsScalingAllowed()
{
    return this->RegistrationTransform->GetMode() != VTK_LANDMARK_RIGIDBODY;
}

void LandmarkTransform::SetScalingAllowed( bool allow )
{
    if( allow )
    {
        this->RegistrationTransform->SetModeToAffine();
        this->InternalTransform->SetModeToAffine();
    }
    else
    {
        this->RegistrationTransform->SetModeToRigidBody();
        this->InternalTransform->SetModeToRigidBody();
    }
    UpdateRegistrationTransform();
}

void LandmarkTransform::Init()
{
    this->FiducialRegistrationError = vtkPoints::New();
    this->FiducialRegistrationErrorMagnitude = vtkDoubleArray::New();
    this->FiducialRegistrationErrorRMS = vtkDoubleArray::New();
    this->FinalRMS = 0.0;
    this->InternalTransform = vtkLandmarkTransform::New();
    this->InternalTransform->SetModeToRigidBody();
    this->RegistrationTransform = vtkLandmarkTransform::New();
    this->RegistrationTransform->SetModeToRigidBody();
    this->SourcePoints = 0;
    this->TargetPoints = 0;
}

void LandmarkTransform::Clear()
{
    this->SourcePoints->UnRegister(this);
    this->SourcePoints = 0;
    this->TargetPoints->UnRegister(this);
    this->TargetPoints = 0;
    this->FiducialRegistrationError->Delete();
    this->FiducialRegistrationErrorMagnitude->Delete();
    this->FiducialRegistrationErrorRMS->Delete();
    this->InternalTransform->Delete();
    this->RegistrationTransform->Delete();
    this->FinalRMS = 0.0;
}

bool LandmarkTransform::UpdateRegistrationTransform()
{
    if (this->InternalUpdate())
    {
        this->RegistrationTransform->SetSourceLandmarks( this->InternalTransform->GetSourceLandmarks() );
        this->RegistrationTransform->SetTargetLandmarks( this->InternalTransform->GetTargetLandmarks() );
        this->RegistrationTransform->Update();
        return true;
    }
    return false;
}

bool LandmarkTransform::InternalUpdate()
{
    if (this->SourcePoints && this->SourcePoints->GetNumberOfPoints() > 2 ) // && this->TargetPoints && this->TargetPoints->GetNumberOfPoints() == this->SourcePoints->GetNumberOfPoints())
    {
        this->InternalTransform->SetSourceLandmarks( SourcePoints );
        this->InternalTransform->SetTargetLandmarks( TargetPoints );
        this->InternalTransform->Update( );
        this->UpdateFRE( );
        return true;
    }
    else
    {
        FinalRMS = 0;
        return false;
    }
}

void LandmarkTransform::UpdateFRE()
{
    double in[3] = {0, 0, 0};
    double out[3] = {0, 0, 0};
    double target[3] = {0, 0, 0};
    double error[3] = {0, 0, 0};
    double error_mag_square = 0;
    double error_summed_square = 0;
    int n = this->SourcePoints->GetNumberOfPoints( );

    this->FiducialRegistrationError->SetNumberOfPoints( n );
    this->FiducialRegistrationError->SetNumberOfPoints( n );
    this->FiducialRegistrationErrorMagnitude->SetNumberOfValues (n );
    this->FiducialRegistrationErrorRMS->SetNumberOfValues( n );
    this->FinalRMS = 0;

    for ( int i = 0, numOfPointsSummed = 1; i < n; i++ )
    {
        this->SourcePoints->GetPoint( i, in );
        this->InternalTransform->TransformPoint( in, out );
        this->TargetPoints->GetPoint( i, target );
        error[0] = out[0] - target[0];
        error[1] = out[1] - target[1];
        error[2] = out[2] - target[2];
        this->FiducialRegistrationError->SetPoint( i, error );
        error_mag_square = error[0]*error[0] + error[1]*error[1] + error[2]*error[2];
        error_summed_square += error_mag_square;
        this->FiducialRegistrationErrorMagnitude->SetValue( i, sqrt( error_mag_square ) );
        this->FinalRMS = sqrt( error_summed_square/numOfPointsSummed++ );
        this->FiducialRegistrationErrorRMS->SetValue( i, FinalRMS );
    }
}

void LandmarkTransform::Reset()
{
    this->Clear( );
    this->Init( );
}
