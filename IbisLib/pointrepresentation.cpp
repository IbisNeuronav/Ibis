/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkActor.h>
#include <vtkTransform.h>
#include <vtkLinearTransform.h>
#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkRenderer.h>
#include <vtkFollower.h>
#include <vtkVectorText.h>
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"

#include "pointrepresentation.h"
#include "scenemanager.h"
#include "triplecutplaneobject.h"
#include "view.h"
#include "vtkCylinderWithPlanesSource.h"
#include "vtkMultiImagePlaneWidget.h"

ObjectSerializationMacro( PointRepresentation );

PointRepresentation::PointRepresentation()
{
    m_sizeIn3D = 2.0;
    m_sizeIn2D = 20.0;
    m_property = vtkProperty::New();
    m_property->SetColor(1.0,0.0,0.0);
    m_property->SetAmbient(1);
    m_property->SetLineWidth(1);
    m_worldTransform = 0;
    m_manager = 0;
    m_sphere = vtkSphereSource::New();
    int i;
    for (i = 0; i < 3; i++)
    {
        m_position[i] = 0.0;
        m_cylinder[i] = vtkCylinderWithPlanesSource::New();
        m_cutter[i] = vtkCutter::New();
        m_cuttingPlane[i] = vtkPlane::New();
    }
    for (i = 0; i < 4; i++)
        m_pointRepresentationActor[i] = vtkActor::New();
    m_pointIndex = -1;
    //label
    m_label = vtkVectorText::New();
    m_label->SetText(" ");
    m_labelActor = vtkFollower::New();
    m_labelMapper = vtkPolyDataMapper::New();
    m_labelMapper->SetInputConnection(m_label->GetOutputPort());
    m_labelActor->SetMapper(m_labelMapper);
    m_labelActor->SetScale( INITIAL_TAG_LABEL_SCALE, INITIAL_TAG_LABEL_SCALE, INITIAL_TAG_LABEL_SCALE );
    m_labelActor->VisibilityOff();
    m_representedInAllViews = false;
    m_active = true;
    m_pickable = true;
}

PointRepresentation::~PointRepresentation()
{
    int i;
    m_property->Delete();
    m_sphere->Delete();
    for (i = 0; i < 3; i++)
    {
        m_cylinder[i]->Delete();
        m_cutter[i]->Delete();
        m_cuttingPlane[i]->Delete();
    }
    for (i = 0; i < 3; i++)
    {
        if (m_manager)
            m_manager->GetView(i)->GetOverlayRenderer()->RemoveActor(m_pointRepresentationActor[i]);
        m_pointRepresentationActor[i]->Delete();
    }
    m_pointRepresentationActor[THREED_VIEW_TYPE]->Delete();
    if (m_manager)
    {
        m_manager->GetView(THREED_VIEW_TYPE)->GetRenderer()->RemoveActor(m_pointRepresentationActor[THREED_VIEW_TYPE]);
        m_manager->GetView(THREED_VIEW_TYPE)->GetRenderer()->RemoveActor(m_labelActor);
    }
    if(m_worldTransform)
    {
        m_worldTransform->UnRegister(this);
    }
    m_label->Delete();
    m_labelActor->Delete();
    m_labelMapper->Delete();
}

void PointRepresentation::Serialize( Serializer * ser )
{
    QString label;
    int active;
    if(!ser->IsReader())
    {
        label = m_label->GetText();
        active = m_active? 1 : 0;
    }
    ::Serialize( ser, "PointName", label );
    ::Serialize( ser, "PointCoordinates", m_position, 3 );
    ::Serialize( ser, "Active", active );
    if (ser->IsReader())
    {
        this->SetLabel(label.toStdString());
        this->SetPointActive(active == 1);
        this->SetPosition(m_position);
    }
}

void PointRepresentation::SetSceneManager( SceneManager *manager )
{
    m_manager = manager;
    if( m_manager && m_manager->GetReferenceDataObject() )
        m_representedInAllViews = true;
}

void PointRepresentation::SetPropertyColor(double color[3])
{
    m_property->SetColor(color[0], color[1], color[2]);
}

void PointRepresentation::SetOpacity(double opacity)
{
    m_property->SetOpacity(opacity);
}

void PointRepresentation::CreatePointRepresentation(double size2D, double size3D)
{
    int i;
    m_sizeIn3D = size3D;
    m_sizeIn2D = size2D;
    vtkPolyDataMapper *mapper[4];
    Q_ASSERT_X(m_manager,"PointRepresentation::CreatePointRepresentation()","SceneManager not set.");
    Q_ASSERT_X(m_worldTransform,"PointRepresentation::CreatePointRepresentation()","Missing world transform.");

    for (i =0; i < 4; i++)
    {
        mapper[i] = vtkPolyDataMapper::New();
        m_pointRepresentationActor[i]->SetProperty(m_property);
        m_pointRepresentationActor[i]->PickableOff();
        m_pointRepresentationActor[i]->SetOrientation(0,0,0);
        m_pointRepresentationActor[i]->SetOrigin(0,0,0);
        m_pointRepresentationActor[i]->SetPosition(0,0,0);
        m_pointRepresentationActor[i]->DragableOn();
        m_pointRepresentationActor[i]->VisibilityOn();
    }
    // 3D view (THREED_VIEW_TYPE)
    m_sphere->SetCenter(0,0,0);
    m_sphere->SetRadius(m_sizeIn3D);
    mapper[THREED_VIEW_TYPE]->SetInputConnection(m_sphere->GetOutputPort());

    //2D views
    // World transform may have multiple concatenations
    // For some reason when we try to GetInverse() of the WorldTransform with 2 or more concatenations,
    // we get wrong invewrse e.g world is concatenation of: identity, t1, t2 - we get inverse of t1
    // while if we go for the inverse matrix, it is correct
    vtkSmartPointer<vtkTransform> inversedWorldTransform = vtkSmartPointer<vtkTransform>::New();
    vtkMatrix4x4 *inverseMat = vtkMatrix4x4::New();
    m_worldTransform->GetInverse(inverseMat);
    inversedWorldTransform->SetMatrix(inverseMat);
    inverseMat->Delete();
    for (i = 0; i < 3; i++)
    {
        m_cylinder[i]->SetRadius(m_sizeIn2D);
        m_cylinder[i]->SetHeight(2.0);
        connect(m_manager->GetMainImagePlanes(), SIGNAL(PlaneMoved(int)), this, SLOT(UpdateCuttingPlane(int)));
#ifdef USE_NEW_IMAGE_PLANES
        m_manager->GetMainImagePlanes()->GetPlane( i )->GetPlaneParams( m_cuttingPlane[i] );
#else
        TripleCutPlaneObject * mainPlanes = m_manager->GetMainImagePlanes();
        vtkMultiImagePlaneWidget *plane = mainPlanes->GetPlane(i);
        Q_ASSERT_X(plane,"PointRepresentation::CreatePointRepresentation()","Missing cut plane.");
        m_cuttingPlane[i]->SetNormal(plane->GetNormal());
        m_cuttingPlane[i]->SetOrigin(plane->GetCenter());
#endif
        m_cuttingPlane[i]->SetTransform(inversedWorldTransform);
        m_cutter[i]->SetCutFunction(m_cuttingPlane[i]);
        m_cutter[i]->SetInputData(m_cylinder[i]->GetOutput());
        m_cutter[i]->Update();
        mapper[i]->SetInputConnection(m_cutter[i]->GetOutputPort());
        if (!m_representedInAllViews)
            m_pointRepresentationActor[i]->VisibilityOff();
    }
    m_cylinder[SAGITTAL_VIEW_TYPE]->SetOrientation(2);
    m_cylinder[TRANSVERSE_VIEW_TYPE]->SetOrientation(1);

    for (i = 0; i < 4; i++)
    {
        m_pointRepresentationActor[i]->SetMapper(mapper[i]);
        mapper[i]->Delete();
        if (m_manager)
        {
            View *view = m_manager->GetView(i);
            if (view->GetType() == THREED_VIEW_TYPE)
            {
                view->GetRenderer()->AddActor(m_pointRepresentationActor[i]);
                m_labelActor->SetCamera( view->GetRenderer()->GetActiveCamera() );
                view->GetRenderer()->AddActor(m_labelActor);
                this->ShowLabel(true);
            }
            else
                view->GetOverlayRenderer()->AddActor(m_pointRepresentationActor[i]);
        }
    }
    m_pointRepresentationActor[THREED_VIEW_TYPE]->SetUserTransform(m_worldTransform);
}

void PointRepresentation::SetWorldTransform(vtkTransform *tr)
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
            if (m_pointRepresentationActor[THREED_VIEW_TYPE])
                m_pointRepresentationActor[THREED_VIEW_TYPE]->SetUserTransform(m_worldTransform);
            for (int i = 0; i < 3; i++)
            {
                if (m_cylinder[i])
                    m_cylinder[i]->SetWorldTransform(m_worldTransform);
            }
        }
    }
}


void PointRepresentation::UpdateCuttingPlane(int index)
{
    Q_ASSERT( m_manager );

    vtkMatrix4x4 * inverseMat = vtkMatrix4x4::New();
    m_worldTransform->GetInverse(inverseMat);
    vtkSmartPointer<vtkTransform> inversedWorldTransform = vtkSmartPointer<vtkTransform>::New();
    inversedWorldTransform->SetMatrix(inverseMat);
    inverseMat->Delete();
#ifdef USE_NEW_IMAGE_PLANES
    m_manager->GetMainImagePlanes()->GetPlane( index )->GetPlaneParams( m_cuttingPlane[index] );
#else
    TripleCutPlaneObject * mainPlanes = m_manager->GetMainImagePlanes();
    vtkMultiImagePlaneWidget *plane = mainPlanes->GetPlane(index);
    m_cuttingPlane[index]->SetNormal(plane->GetNormal());
    m_cuttingPlane[index]->SetOrigin(plane->GetCenter());
#endif
    m_cuttingPlane[index]->SetTransform(inversedWorldTransform);
    m_cutter[index]->Update();
}

void PointRepresentation::SetPosition(double p[3])
{
    this->SetPosition(p[0], p[1], p[2]);
}

void PointRepresentation::SetPosition(double x, double y, double z)
{
    m_sphere->SetCenter(x, y, z);
    for (int i = 0; i < 3; i++)
    {
        m_cylinder[i]->SetPosition(x, y, z);
    }
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
    this->UpdateLabelPosition();
}

void PointRepresentation::SetPointSizeIn3D(double r)
{
    m_sphere->SetRadius(r);
    m_sizeIn3D = r;
}

void PointRepresentation::SetPointSizeIn2D(double r)
{
    for (int i = 0; i < 3; i++)
        m_cylinder[i]->SetRadius(r);
    m_sizeIn2D = r;
}

void PointRepresentation::SetPickable(bool yes)
{
    for (int i = 0; i < 4; i++)
    {
        if (yes)
            m_pointRepresentationActor[i]->PickableOn();
        else
            m_pointRepresentationActor[i]->PickableOff();
    }
    m_pickable = yes;
}

vtkActor *PointRepresentation::GetPointActor(int index)
{
    if (index >= 0 && index < 4)
        return m_pointRepresentationActor[index];
    return 0;
}

void PointRepresentation::SetLabelScale(double s)
{
    if (s <= 0)
        s = 1.0;
    m_labelActor->SetScale( s, s, s );
}

double *PointRepresentation::GetLabelScale()
{
    return m_labelActor->GetScale();
}

void PointRepresentation::UpdateLabelPosition()
{
    double pos[3];
    m_sphere->GetCenter(pos);
    double newPos[3];
    for (int i = 0; i < 3; i++)
        newPos[i] = pos[i];
    if (m_worldTransform)
        m_worldTransform->TransformPoint(pos, newPos);
    m_labelActor->SetPosition(newPos[0]+m_sizeIn3D+2, newPos[1]+m_sizeIn3D+2, newPos[2]+m_sizeIn3D+2);
}

void PointRepresentation::SetLabel( const std::string & text )
{
    m_label->SetText(text.c_str());
    m_labelMapper->SetInputConnection( m_label->GetOutputPort() );
    m_labelActor->SetProperty(m_property);
    this->UpdateLabelPosition();
}

const char* PointRepresentation::GetLabel()
{
    return m_label->GetText();
}

void PointRepresentation::SetNumericLabel(int n)
{
    char num[8];
    sprintf(num, "%i", n);
    this->SetLabel(num);
}

void PointRepresentation::ShowLabel( bool show)
{
    if (show)
    {
        m_labelActor->VisibilityOn();
    }
    else
    {
        this->m_labelActor->VisibilityOff();
    }
}

void PointRepresentation::Enable(bool show)
{
    if (m_manager)
    {
        this->Update();
        for (int i = 0; i < 3; i++)
        {
            if (m_pointRepresentationActor[i])
            {
                if (show)
                    m_manager->GetView(i)->GetOverlayRenderer()->AddActor(m_pointRepresentationActor[i]);
                else
                    m_manager->GetView(i)->GetOverlayRenderer()->RemoveActor(m_pointRepresentationActor[i]);
            }
        }
        if (show)
        {
            m_manager->GetView(THREED_VIEW_TYPE)->GetRenderer()->AddActor(m_pointRepresentationActor[THREED_VIEW_TYPE]);
            m_manager->GetView(THREED_VIEW_TYPE)->GetRenderer()->AddActor(m_labelActor);
        }
        else
        {
            m_manager->GetView(THREED_VIEW_TYPE)->GetRenderer()->RemoveActor(m_pointRepresentationActor[THREED_VIEW_TYPE]);
            m_manager->GetView(THREED_VIEW_TYPE)->GetRenderer()->RemoveActor(m_labelActor);
        }
    }
}

void PointRepresentation::SetCylindersTransform(vtkMatrix4x4 *mat)
{
    for (int i = 0; i < 3; i++)
        m_cylinder[i]->SetRotation(mat);
}

void PointRepresentation::SetCuttingPlaneTransform(vtkTransform *tr)
{
    for (int i = 0; i < 3; i++)
        m_cuttingPlane[i]->SetTransform(tr->GetInverse());
}

void PointRepresentation::ShallowCopy(PointRepresentation *source)
{
    if (this == source)
        return;
    source->GetPosition(m_position);
    m_pointIndex = source->GetPointIndex();
    m_active = source->GetPointActive();
    m_pickable = source->GetPickable();
    this->SetLabel(source->GetLabel());
    double *scale = source->GetLabelScale();
    this->SetLabelScale(*scale);
}

void PointRepresentation::Update()
{
    m_sphere->Update();
    m_label->Update();
    for (int i = 0; i < 3; i++)
        m_cylinder[i]->Update();
}
