/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkCylinderWithPlanesSource.h"

#include <vtkObjectFactory.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyData.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCylinderSource.h>
#include <vtkPlaneSource.h>
#include <vtkMatrix4x4.h>

vtkStandardNewMacro(vtkCylinderWithPlanesSource);

vtkCylinderWithPlanesSource::vtkCylinderWithPlanesSource():
    Height(2.0)
    ,Radius(20.0)
    ,Resolution(30)
    ,Orientation(0)
{
    for (int i = 0; i < 3; i++)
        this->Center[i] = 0;
    this->Cylinder = vtkCylinderSource::New();
    this->Plane1 = vtkPlaneSource::New();
    this->Plane2 = vtkPlaneSource::New();
    this->CylinderAndPlanes = vtkAppendPolyData::New();
    this->Transform =  vtkTransform::New();
    this->Transform->Identity();
    this->AppliedTransform =  vtkTransform::New();
    this->AppliedTransform->Identity();
    this->AppliedTransform->Concatenate(this->Transform);
    this->AppliedTransform->PostMultiply();
    this->WorldTransform = 0;
    this->ModifiedCylinderAndPlanes = vtkTransformPolyDataFilter::New();
    this->ModifiedCylinderAndPlanes->SetInputConnection(this->CylinderAndPlanes->GetOutputPort());
    this->ModifiedCylinderAndPlanes->SetTransform(this->AppliedTransform);
    this->BuildRepresentation();
}

vtkCylinderWithPlanesSource::~vtkCylinderWithPlanesSource()
{
    this->Cylinder->Delete();
    this->Plane1->Delete();
    this->Plane2->Delete();
    this->CylinderAndPlanes->Delete();
    this->ModifiedCylinderAndPlanes->Delete();
    this->Transform->Delete();
    this->AppliedTransform->Delete();
    if (this->WorldTransform)
        this->WorldTransform->UnRegister(this);
}

vtkPolyData *vtkCylinderWithPlanesSource::GetOutput()
{
    if (this->ModifiedCylinderAndPlanes)
        return this->ModifiedCylinderAndPlanes->GetOutput();
    return 0;
}

void vtkCylinderWithPlanesSource::SetWorldTransform(vtkTransform *tr)
{
    if (this->WorldTransform != tr)
    {
        if(this->WorldTransform)
        {
            this->WorldTransform->UnRegister(this);
            this->AppliedTransform->Identity();
            this->AppliedTransform->Concatenate(this->Transform);
        }
        this->WorldTransform = tr;
        if (this->WorldTransform)
        {
            this->WorldTransform->Register(this);
            this->AppliedTransform->Concatenate(this->WorldTransform);
        }
    }
    this->Update();
}

void vtkCylinderWithPlanesSource::SetResolution(int res)
{
    if (this->Resolution != res)
    {
        this->Resolution = res;
        this->BuildRepresentation();
    }
}

void vtkCylinderWithPlanesSource::SetRadius(double radius)
{
    if (this->Radius != radius)
    {
        this->Radius = radius;
        this->BuildRepresentation();
    }
}

void vtkCylinderWithPlanesSource::SetHeight(double height)
{
    if (this->Height != height)
    {
        this->Height = height;
        this->BuildRepresentation();
    }
}

void vtkCylinderWithPlanesSource::SetCenter(double c[3])
{
    this->SetCenter(c[0], c[1], c[2]);
}

void vtkCylinderWithPlanesSource::SetCenter(double x, double y, double z)
{

    this->Center[0] = x;
    this->Center[1] = y;
    this->Center[2] = z;
    this->BuildRepresentation();
}

void vtkCylinderWithPlanesSource::SetPosition(double pos[3])
{
    this->SetPosition(pos[0], pos[1], pos[2]);
}

void vtkCylinderWithPlanesSource::SetPosition(double x, double y, double z)
{
    double rotation[3];
    this->Transform->GetOrientation(rotation);
    this->Transform->Identity();
    this->Transform->Translate(x, y, z);
    this->Transform->RotateZ(rotation[2]);
    this->Transform->RotateY(rotation[1]);
    this->Transform->RotateX(rotation[0]);
    this->Update();
}

void vtkCylinderWithPlanesSource::SetOrientation(int axNo)
{
    if (this->Orientation != axNo)
    {
        this->Orientation = axNo;
        double p[3];
        this->Transform->GetPosition(p);
        this->Transform->Identity();
        this->Transform->Translate(p);
        if (this->Orientation == 1)
            this->Transform->RotateX(90);
        else if (this->Orientation == 2)
            this->Transform->RotateZ(90);
    }
    this->Update();
}

void vtkCylinderWithPlanesSource::SetRotation(vtkMatrix4x4 *rotationMatrix)
{
    double p[3];
    this->Transform->GetPosition(p);
    this->Transform->Identity();
    vtkMatrix4x4 *mat = vtkMatrix4x4::New();
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
            mat->SetElement(i, j, rotationMatrix->GetElement(i, j));
        mat->SetElement(i, 3, p[i] );
    }
    this->Transform->SetMatrix(mat);
    mat->Delete();
    if (this->Orientation == 1)
    {
        this->Transform->RotateX(90);
    }
    else if (this->Orientation == 2)
    {
        this->Transform->RotateZ(90);
    }
    this->Update();
}

void vtkCylinderWithPlanesSource::BuildRepresentation()
{
    this->CylinderAndPlanes->RemoveAllInputs();
    vtkDebugMacro(<<"Creating Cylinder");

    this->Cylinder->SetHeight(this->Height);
    this->Cylinder->SetRadius(this->Radius);
    this->Cylinder->SetResolution(this->Resolution);
    this->Cylinder->SetCenter(this->Center);
    this->Cylinder->SetCapping(0);

    this->CylinderAndPlanes->AddInputConnection(this->Cylinder->GetOutputPort());

    vtkDebugMacro(<<"Creating Plane1 and Plane2");

    this->Plane1->SetOrigin(this->Center[0]-2*this->Radius, this->Center[1]-0.5*this->Height, this->Center[2]);
    this->Plane1->SetPoint1(this->Center[0]-2*this->Radius, this->Center[1]+0.5*this->Height, this->Center[2]);
    this->Plane1->SetPoint2(this->Center[0]+2*this->Radius, this->Center[1]-0.5*this->Height, this->Center[2]);

    this->CylinderAndPlanes->AddInputConnection(this->Plane1->GetOutputPort());

    this->Plane2->SetOrigin(this->Center[0], this->Center[1]-0.5*this->Height, this->Center[2]-2*this->Radius);
    this->Plane2->SetPoint1(this->Center[0], this->Center[1]-0.5*this->Height, this->Center[2]+2*this->Radius);
    this->Plane2->SetPoint2(this->Center[0], this->Center[1]+0.5*this->Height, this->Center[2]-2*this->Radius);

    this->CylinderAndPlanes->AddInputConnection(this->Plane2->GetOutputPort());
    this->Update();
}

void vtkCylinderWithPlanesSource::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Resolution: " << this->Resolution << "\n";
    os << indent << "Radius: " << this->Radius << "\n";
    os << indent << "Height: " << this->Height << "\n";
    os << indent << "Center: " << this->Center[0] << ", " << this->Center[1] << ", "
            << this->Center[2] << "\n";
}

void vtkCylinderWithPlanesSource::Update()
{
    this->ModifiedCylinderAndPlanes->Update();
}
