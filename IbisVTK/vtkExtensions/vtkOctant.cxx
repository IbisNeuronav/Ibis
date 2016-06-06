/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkObjectFactory.h"
#include "vtkOctant.h"

vtkStandardNewMacro(vtkOctant);

vtkOctant::vtkOctant() : vtkBox()
{
  this->OctantNumber = -1;
  for (int i = 0; i < 3; i++)
      this->Origin[i] = 0.0;
  double bounds[6] = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};
  this->SetGlobalBounds(bounds);
}

vtkOctant::~vtkOctant()
{
}

void vtkOctant::SetOctantNumber(int num)
{
    this->OctantNumber = -1;
    if (num >= 0 && num < 8)
        this->OctantNumber = num;
    this->ComputeInitialBounds();
}

void vtkOctant::SetGlobalBounds(double bounds[6])
{
    for (int i = 0; i < 6; i++)
        this->GlobalBounds[i] = bounds[i];
    this->ComputeInitialBounds();
}

void vtkOctant::GetGlobalBounds(double bounds[6])
{
    for (int i = 0; i < 6; i++)
        bounds[i] = this->GlobalBounds[i];
}

void vtkOctant::SetOrigin(double origin[3])
{
    if (origin[0] >= this->GlobalBounds[0] && origin[0] <= this->GlobalBounds[1])
        this->Origin[0] = origin[0];
    else
        this->Origin[0] = (this->GlobalBounds[0] + this->GlobalBounds[1])/2;
    if (origin[1] >= this->GlobalBounds[2] && origin[1] <= this->GlobalBounds[3])
        this->Origin[1] = origin[1];
    else
        this->Origin[1] = (this->GlobalBounds[2] + this->GlobalBounds[3])/2;
    if (origin[2] >= this->GlobalBounds[4] && origin[2] <= this->GlobalBounds[5])
        this->Origin[2] = origin[2];
    else
        this->Origin[2] = (this->GlobalBounds[4] + this->GlobalBounds[4])/2;
    this->ComputeInitialBounds();
}

void vtkOctant::GetOrigin(double origin[3])
{
    for (int i = 0; i < 3; i++)
        origin[i] = this->Origin[i];
}

void vtkOctant::ComputeInitialBounds()
{
    double boxBounds[6];
    switch(this->OctantNumber)
    {
    case 0:
        boxBounds[0] = this->Origin[0];
        boxBounds[1] = this->GlobalBounds[1];
        boxBounds[2] = this->Origin[1];
        boxBounds[3] = this->GlobalBounds[3];
        boxBounds[4] = this->Origin[2];
        boxBounds[5] = this->GlobalBounds[5];
        break;
    case 1:
        boxBounds[0] = this->GlobalBounds[0];
        boxBounds[1] = this->Origin[0];
        boxBounds[2] = this->Origin[1];
        boxBounds[3] = this->GlobalBounds[3];
        boxBounds[4] = this->Origin[2];
        boxBounds[5] = this->GlobalBounds[5];
        break;
    case 2:
        boxBounds[0] = this->GlobalBounds[0];
        boxBounds[1] = this->Origin[0];
        boxBounds[2] = this->GlobalBounds[2];
        boxBounds[3] = this->Origin[1];
        boxBounds[4] = this->Origin[2];
        boxBounds[5] = this->GlobalBounds[5];
        break;
    case 3:
        boxBounds[0] = this->Origin[0];
        boxBounds[1] = this->GlobalBounds[1];
        boxBounds[2] = this->GlobalBounds[2];
        boxBounds[3] = this->Origin[1];
        boxBounds[4] = this->Origin[2];
        boxBounds[5] = this->GlobalBounds[5];
        break;
    case 4:
        boxBounds[0] = this->Origin[0];
        boxBounds[1] = this->GlobalBounds[1];
        boxBounds[2] = this->Origin[1];
        boxBounds[3] = this->GlobalBounds[3];
        boxBounds[4] = this->GlobalBounds[4];
        boxBounds[5] = this->Origin[2];
        break;
    case 5:
        boxBounds[0] = this->GlobalBounds[0];
        boxBounds[1] = this->Origin[0];
        boxBounds[2] = this->Origin[1];
        boxBounds[3] = this->GlobalBounds[3];
        boxBounds[4] = this->GlobalBounds[4];
        boxBounds[5] = this->Origin[2];
        break;
    case 6:
        boxBounds[0] = this->GlobalBounds[0];
        boxBounds[1] = this->Origin[0];
        boxBounds[2] = this->GlobalBounds[2];
        boxBounds[3] = this->Origin[1];
        boxBounds[4] = this->GlobalBounds[4];
        boxBounds[5] = this->Origin[2];
        break;
    case 7:
        boxBounds[0] = this->Origin[0];
        boxBounds[1] = this->GlobalBounds[1];
        boxBounds[2] = this->GlobalBounds[2];
        boxBounds[3] = this->Origin[1];
        boxBounds[4] = this->GlobalBounds[4];
        boxBounds[5] = this->Origin[2];
        break;
    default: // -1
        return;
    }
    this->SetBounds(boxBounds);
}

void vtkOctant::ComputeBounds(double newValue, int index)
{
    double pmin[3], pmax[3];
    this->GetXMin(pmin);
    this->GetXMax(pmax);
    switch(this->OctantNumber)
    {
    case 0:
    default:
        if (newValue < pmax[index])
            pmin[index] = newValue;
        else
        {
            pmin[index] = pmax[index];
            pmax[index] = newValue;
        }
        break;
    case 1:
        if (index == 0)
        {
             pmax[index] = newValue;
        }
        else
        {
            if (newValue < pmax[index])
                pmin[index] = newValue;
            else
            {
                pmin[index] = pmax[index];
                pmax[index] = newValue;
            }
         }
        break;
    case 2:
        if (index < 2)
        {
             pmax[index] = newValue;
        }
        else
        {
            if (newValue < pmax[index])
                pmin[index] = newValue;
            else
            {
                pmin[index] = pmax[index];
                pmax[index] = newValue;
            }
         }
        break;
    case 3:
        if (index == 1)
        {
             pmax[index] =  newValue;
        }
        else
        {
            if (newValue < pmax[index])
                pmin[index] =  newValue;
            else
            {
                pmin[index] = pmax[index];
                pmax[index] =  newValue;
            }
        }
        break;
    case 4:
        if (index == 2)
        {
             pmax[index] =  newValue;
        }
        else
        {
            if (newValue < pmax[index])
                pmin[index] =  newValue;
            else
            {
                pmin[index] = pmax[index];
                pmax[index] =  newValue;
            }
        }
        break;
    case 5:
        if (index != 1)
        {
             pmax[index] =  newValue;
        }
        else
        {
            if (newValue < pmax[index])
                pmin[index] = newValue;
            else
            {
                pmin[index] = pmax[index];
                pmax[index] = newValue;
            }
         }
        break;
    case 6:
        pmax[index] =  newValue;
        break;
    case 7:
        if (index != 0)
        {
             pmax[index] = newValue;
        }
        else
        {
            if (newValue < pmax[index])
                pmin[index] = newValue;
            else
            {
                pmin[index] = pmax[index];
                pmax[index] =  newValue;
            }
        }
        break;
    }
    this->SetXMin(pmin);
    this->SetXMax(pmax);
}

void vtkOctant::PrintSelf(ostream& os, vtkIndent indent)
{

    os << indent << "Octant Number: " << this->OctantNumber << "\n";

    os << indent << "Origin: " << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2] << "\n";
    this->Superclass::PrintSelf(os,indent);
}
