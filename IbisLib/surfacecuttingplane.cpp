/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "surfacecuttingplane.h"

#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkAppendPolyData.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkTransform.h>
#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkRenderer.h>

#include <QWidget>

#include "view.h"
#include "scenemanager.h"
#include "triplecutplaneobject.h"
#include "polydataobject.h"
#include "vtkMultiImagePlaneWidget.h"

SurfaceCuttingPlane::SurfaceCuttingPlane()
    :m_manager(0)
    ,m_worldTransform(0)
{
    m_surfaceToCut = vtkAppendPolyData::New();
    m_property = vtkProperty::New();
    m_property->SetColor(1.0,0.0,0.0);
    m_property->SetAmbient(1);
    m_property->SetLineWidth(1);
    for (int i = 0; i < 3; i++)
    {
        m_cutter[i] = vtkCutter::New();
        m_cuttingPlane[i] = vtkPlane::New();
        m_cutterActor[i] = vtkActor::New();
        m_cutterActor[i]->SetProperty(m_property);
        m_cutterActor[i]->PickableOff();
        m_cutterActor[i]->SetOrientation(0,0,0);
        m_cutterActor[i]->SetOrigin(0,0,0);
        m_cutterActor[i]->SetPosition(0,0,0);
        m_cutterActor[i]->DragableOff();
        m_cutterActor[i]->VisibilityOn();
    }
}

SurfaceCuttingPlane::~SurfaceCuttingPlane()
{
    if(m_worldTransform)
    {
        m_worldTransform->UnRegister(this);
    }
    if (m_manager != NULL)
    {
        disconnect( m_manager->GetMainImagePlanes(), SIGNAL(PlaneMoved(int)), this, SLOT(UpdateCuttingPlane(int)) );
    }
    for (int i = 0; i < 3; i++)
    {
        if (m_manager != NULL)
        {
            View *view = m_manager->GetView(i);
            view->GetOverlayRenderer()->RemoveActor(m_cutterActor[i]);
        }
        m_cutterActor[i]->Delete();
        m_cutter[i]->Delete();
        m_cuttingPlane[i]->Delete();
    }
    m_surfaceToCut->Delete();
    m_property->Delete();
}

void SurfaceCuttingPlane::SetManager( SceneManager *manager )
{
    m_manager = manager;
}

void SurfaceCuttingPlane::SetWorldTransform(vtkTransform *tr)
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
            for (int i = 0; i < 3; i++)
            {
                m_cutterActor[i]->SetUserTransform(m_worldTransform);
            }
        }
    }
}

void SurfaceCuttingPlane::SetInput(vtkPolyData *obj)
{
    Q_ASSERT( m_manager );
    if( !obj || !m_manager->GetReferenceDataObject() )
        return;

    m_surfaceToCut->RemoveAllInputs();
    m_surfaceToCut->AddInputData(obj);
    vtkPolyDataMapper * mapper[3];
    for (int i = 0; i < 3; i++)
    {
        connect( m_manager->GetMainImagePlanes(), SIGNAL(PlaneMoved(int)), this, SLOT(UpdateCuttingPlane(int)) );

#ifdef USE_NEW_IMAGE_PLANES
        m_manager->GetMainImagePlanes()->GetPlane( i )->GetPlaneParams( m_cuttingPlane[i] );
#else
        vtkMultiImagePlaneWidget *plane =  m_manager->GetMainImagePlanes()->GetPlane(i);
        m_cuttingPlane[i]->SetNormal(plane->GetNormal());
        m_cuttingPlane[i]->SetOrigin(plane->GetOrigin());
#endif

        if (m_worldTransform)
            m_cuttingPlane[i]->SetTransform(m_worldTransform->GetInverse());
        m_cutter[i]->SetCutFunction(m_cuttingPlane[i]);
        m_cutter[i]->SetInputConnection(m_surfaceToCut->GetOutputPort());
        m_cutter[i]->Update();

        vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
        mapper->SetInputConnection(m_cutter[i]->GetOutputPort());
        m_cutterActor[i]->SetMapper(mapper);
        View *view = m_manager->GetView(i);
        view->GetOverlayRenderer()->AddActor(m_cutterActor[i]);
        mapper->Delete();
    }
}

vtkPolyData * SurfaceCuttingPlane::GetCutterOutput(int index)
{
    if (index >= 0 && index < 3)
        return m_cutter[index]->GetOutput();
    return 0;
}

void SurfaceCuttingPlane::AddInput(vtkPolyData *obj)
{
    if (obj)
    {
        m_surfaceToCut->AddInputData(obj);
        for (int i = 0; i < 3; i++)
        {
            m_cutter[i]->Update();
        }
    }
}


void SurfaceCuttingPlane::RemoveInput(vtkPolyData *obj)
{
}

void SurfaceCuttingPlane::SetPropertyColor(double color[3])
{
    m_property->SetColor(color[0], color[1], color[2]);
}

void SurfaceCuttingPlane::SetPropertyLineWidth(int n)
{
    m_property->SetLineWidth(n);
}


void SurfaceCuttingPlane::SetVisible(bool show)
{
    for (int i = 0; i < 3; i++)
        m_cutterActor[i]->SetVisibility(show? 1 : 0);
}

void SurfaceCuttingPlane::UpdateCuttingPlane(int index)
{
    Q_ASSERT( m_manager );
    if( !m_manager->GetReferenceDataObject() )
        return;

#ifdef USE_NEW_IMAGE_PLANES
    m_manager->GetMainImagePlanes()->GetPlane( index )->GetPlaneParams( m_cuttingPlane[index] );
#else
    TripleCutPlaneObject *mainPlanes = m_manager->GetMainImagePlanes();
    vtkMultiImagePlaneWidget *plane = mainPlanes->GetPlane(index);
    m_cuttingPlane[index]->SetOrigin(plane->GetCenter());
#endif
    m_cutter[index]->Update();
}

