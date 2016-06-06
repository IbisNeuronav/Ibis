/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "octants.h"
#include "vtkTransform.h"

Octants::Octants()
{
    m_octantsOfInterest = vtkOctant::New();
}

Octants::~Octants()
{
    m_octantsOfInterest->Delete();
}

void Octants::SetOctantNumber(int octantNo)
{
    m_octantsOfInterest->SetOctantNumber(octantNo);
}

int Octants::GetOctantNumber()
{
    return m_octantsOfInterest->GetOctantNumber();
}

void Octants::SetBounds(double bounds[6])
{
    m_octantsOfInterest->SetGlobalBounds(bounds);
}

void Octants::GetBounds(double bounds[6])
{
    m_octantsOfInterest->GetGlobalBounds(bounds);
}

void Octants::SetOrigin(double origin[3])
{
    m_octantsOfInterest->SetOrigin(origin);
}
void Octants::GetOrigin(double origin[3])
{
    m_octantsOfInterest->GetOrigin(origin);
}

void Octants::SetTransform(vtkTransform *t)
{
    if (t)
        m_octantsOfInterest->SetTransform(t->GetInverse());
}

void Octants::UpdateOctantOfInterest(double val, int axis)
{
    m_octantsOfInterest->ComputeBounds(val, axis);
}
