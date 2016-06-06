/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "polydataclipper.h"
#include <vtkDataSetMapper.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkClipPolyData.h>
#include <vtkImplicitFunction.h>
#include <vtkTransform.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>

#include "view.h"
#include "scenemanager.h"

PolyDataClipper::PolyDataClipper()
{
    m_clipper = vtkClipPolyData::New();
    m_clipper->SetValue(1);
    m_clipper->SetInsideOut(0);
    m_clipperActor = vtkActor::New();
    m_clipperMapper = vtkDataSetMapper::New();
    m_clipperActor->SetMapper(m_clipperMapper);
    m_clipFunction = 0;
    m_manager = 0;
    m_worldTransform = 0;
    m_polyDataToClip = 0;
}

PolyDataClipper::~PolyDataClipper()
{
    m_clipper->Delete();
    m_clipperActor->Delete();
    m_clipperMapper->Delete();
    if(m_worldTransform)
        m_worldTransform->UnRegister(this);
    if (m_polyDataToClip)
        m_polyDataToClip->UnRegister(this);
}

void PolyDataClipper::SetManager( SceneManager *manager )
{
    m_manager = manager;
}

void PolyDataClipper::SetInput(vtkPolyData *obj)
{
    if (!obj || !m_manager || m_polyDataToClip == obj)
        return;
    if (m_polyDataToClip)
        m_polyDataToClip->UnRegister(this);
    m_polyDataToClip = obj;
    m_polyDataToClip->Register(this);
    m_clipper->SetInputData(m_polyDataToClip);
    m_clipper->Update();
}

vtkPolyData * PolyDataClipper::GetClipperOutput()
{
    return m_clipper->GetClippedOutput();
}

void PolyDataClipper::SetActorProperty(vtkProperty *prop)
{
    m_clipperActor->SetProperty(prop);
}

void PolyDataClipper::SetWorldTransform(vtkTransform *tr)
{
    if (m_worldTransform != tr)
    {
        if(m_worldTransform)
        {
            m_worldTransform->UnRegister(this);
        }
        m_worldTransform = tr;
        if (m_worldTransform)
        {
            m_worldTransform->Register(this);
            m_clipperActor->SetUserTransform(m_worldTransform);
        }
    }
}

void PolyDataClipper::SetClipFunction(vtkImplicitFunction *func)
{
    m_clipper->SetClipFunction(func);
    m_clipper->Update();
    m_clipperMapper->SetInputConnection(m_clipper->GetOutputPort());
    View *v = m_manager->GetView(THREED_VIEW_TYPE);
    v->GetRenderer()->AddActor(m_clipperActor);
}

void PolyDataClipper::RemoveClipActor()
{
    View *v = m_manager->GetView(THREED_VIEW_TYPE);
    v->GetRenderer()->RemoveActor(m_clipperActor);
}

void PolyDataClipper::HideClipActor()
{
    m_clipperActor->VisibilityOff();
}

void PolyDataClipper::ShowClipActor()
{
    m_clipperActor->VisibilityOn();
}

void PolyDataClipper::SetScalarsVisible( int use )
{
    m_clipperMapper->SetScalarVisibility(use);
}
