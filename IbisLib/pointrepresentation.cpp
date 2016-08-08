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
#include <vtkRenderer.h>
#include <vtkFollower.h>
#include <vtkVectorText.h>
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"

#include "pointrepresentation.h"
#include "pointsobject.h"
#include "scenemanager.h"
#include "view.h"
#include "vtkCircleWithCrossSource.h"

ObjectSerializationMacro( PointRepresentation );

PointRepresentation::PerViewElements::PerViewElements()
{
    pointRepresentationActor = 0;
    labelActor = 0;
}

PointRepresentation::PerViewElements::~PerViewElements()
{
    if( pointRepresentationActor )
        pointRepresentationActor->Delete();
    if( labelActor )
        labelActor->Delete();
}

PointRepresentation::PointRepresentation()
{
    this->AllowChildren = false;
    m_property = vtkProperty::New();
    m_property->SetColor(1.0,0.0,0.0);
    m_property->SetAmbient(1);
    m_property->SetLineWidth(1);
    m_sphere = vtkSphereSource::New();
    m_circle = vtkCircleWithCrossSource::New();
    m_pointIndex = -1;
    m_active = true;

    //label
    m_label = vtkVectorText::New();
    m_label->SetText(" ");
    m_labelScale = INITIAL_TAG_LABEL_SCALE;
    m_labelVisible = true;

    m_invWorldRotTransform = vtkTransform::New();

    m_posTransform = vtkTransform::New();

    m_point2DTransform = vtkTransform::New();
    m_point2DTransform->Concatenate( this->GetWorldTransform() );
    m_point2DTransform->Concatenate( m_posTransform );
    m_point2DTransform->Concatenate( m_invWorldRotTransform );

    m_point3DTransform = vtkTransform::New();
    m_point3DTransform->Concatenate( this->GetWorldTransform() );
    m_point3DTransform->Concatenate( m_posTransform );

    m_labelOffset = vtkTransform::New();

    m_labelTransform = vtkTransform::New();
    m_labelTransform->Concatenate( m_point2DTransform );
    m_labelTransform->Concatenate( m_labelOffset );
}

PointRepresentation::~PointRepresentation()
{
    m_property->Delete();
    m_sphere->Delete();
    m_circle->Delete();

    m_label->Delete();

    m_point2DTransform->Delete();
    m_point3DTransform->Delete();
    m_invWorldRotTransform->Delete();
    m_posTransform->Delete();
    m_labelOffset->Delete();
    m_labelTransform->Delete();
}

void PointRepresentation::Hide()
{
    PerViewContainer::iterator it = m_perViewContainer.begin();
    while( it != m_perViewContainer.end() )
    {
        PerViewElements * perView = (*it).second;
        perView->pointRepresentationActor->VisibilityOff();
        if( perView->labelActor )
            perView->labelActor->VisibilityOff();
         ++it;
    }
}

void PointRepresentation::Show()
{
    PerViewContainer::iterator it = m_perViewContainer.begin();
    while( it != m_perViewContainer.end() )
    {
        PerViewElements * perView = (*it).second;
        perView->pointRepresentationActor->VisibilityOn();
        if( perView->labelActor )
            perView->labelActor->SetVisibility( m_labelVisible ? 1 : 0 );
         ++it;
    }
}

bool PointRepresentation::Setup( View * view )
{
    PointsObject *parent = PointsObject::SafeDownCast( this->GetParent() );

    double size3D =  parent->Get3DRadius();
    double size2D = parent->Get2DRadius();
    bool visible = !parent->IsHidden();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkActor *pointActor = 0;
    vtkFollower * labelActor = 0;

    // 3D view (THREED_VIEW_TYPE)
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        pointActor = vtkActor::New();
        m_sphere->SetRadius( size3D );
        mapper->SetInputConnection( m_sphere->GetOutputPort() );
        pointActor->SetUserTransform( m_point3DTransform );
        view->GetRenderer()->AddActor( pointActor );

        labelActor = vtkFollower::New();
        vtkSmartPointer<vtkPolyDataMapper> labelMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        labelMapper->SetInputConnection( m_label->GetOutputPort() );
        labelActor->SetMapper( labelMapper );
        labelActor->SetScale( m_labelScale, m_labelScale, m_labelScale );
        labelActor->SetCamera( view->GetRenderer()->GetActiveCamera() );
        labelActor->SetUserTransform( m_labelTransform );
        labelActor->SetProperty(m_property);
        labelActor->SetVisibility( visible && m_labelVisible ? 1 : 0 );
        view->GetRenderer()->AddActor(labelActor);
     }
    else //2D views
    {
        vtkFollower * f = vtkFollower::New();
        pointActor = f;
        m_circle->SetRadius( size2D );
        m_circle->SetResolution( 3 );
        mapper->SetInputConnection( m_circle->GetOutputPort() );
        f->SetCamera( view->GetRenderer()->GetActiveCamera() );
        f->SetUserTransform( m_point2DTransform );
        view->GetOverlayRenderer()->AddActor(pointActor);
    }
    pointActor->SetProperty(m_property);
    pointActor->SetPickable( m_active ? 1 : 0 );
    pointActor->DragableOn();
    if( visible )
        pointActor->VisibilityOn();
    else
        pointActor->VisibilityOff();
    pointActor->SetMapper( mapper );
    PerViewElements * perView = new PerViewElements;
    perView->pointRepresentationActor = pointActor;
    perView->labelActor = labelActor;
    m_perViewContainer[view] = perView;
    connect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
    return true;
}

bool PointRepresentation::Release( View * view )
{
    disconnect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
    PerViewContainer::iterator it = m_perViewContainer.find( view );
    if( it != m_perViewContainer.end() )
    {
        PerViewElements * perView = (*it).second;
        if( view->GetType() == THREED_VIEW_TYPE )
        {
            view->GetRenderer()->RemoveActor( perView->pointRepresentationActor );
            view->GetRenderer()->RemoveActor( perView->labelActor );
        }
        else
            view->GetOverlayRenderer()->RemoveActor( perView->pointRepresentationActor );
        delete perView;
        this->m_perViewContainer.erase( it );
    }
    return true;
}

void PointRepresentation::UpdateVisibility()
{
    if( IsHidden() )
        return;

    // Compute point position in world space
    vtkTransform * wt = this->GetWorldTransform();
    PointsObject * parent = PointsObject::SafeDownCast( this->GetParent() );
    double local[3];
    parent->GetPointCoordinates( m_pointIndex, local );
    double world[3];
    wt->TransformPoint( local, world );

    // update its visibility in each of the 2d views
    PerViewContainer::iterator it = m_perViewContainer.begin( );
    while( it != m_perViewContainer.end() )
    {
        View * view = (*it).first;
        if( view->GetType() != THREED_VIEW_TYPE )
        {
            bool isInPlane = this->GetManager()->IsInPlane( (VIEWTYPES)view->GetType(), world );
            PerViewElements * perView = (*it).second;
            if( isInPlane )
                perView->pointRepresentationActor->VisibilityOn();
            else
                perView->pointRepresentationActor->VisibilityOff();
        }
        ++it;
    }
}

void PointRepresentation::SetPropertyColor(double color[3])
{
    m_property->SetColor(color[0], color[1], color[2]);
}

void PointRepresentation::SetOpacity(double opacity)
{
    m_property->SetOpacity(opacity);
}

void PointRepresentation::SetPosition(double p[3])
{
    this->SetPosition(p[0], p[1], p[2]);
}

void PointRepresentation::SetPosition(double x, double y, double z)
{
    m_posTransform->Identity();
    m_posTransform->Translate( x, y, z );

    // simtodo : why is this needed? it shouldn't since transform pipeline is self-updating
    UpdateAllTransforms();
}

void PointRepresentation::GetPosition(double p[3])
{
    vtkMatrix4x4 * m = m_posTransform->GetMatrix();
    p[0] = m->GetElement( 0, 3 );
    p[1] = m->GetElement( 1, 3 );
    p[2] = m->GetElement( 2, 3 );
}

void PointRepresentation::SetPointSizeIn3D(double r)
{
    m_sphere->SetRadius(r);
}

void PointRepresentation::SetPointSizeIn2D(double r)
{
    m_circle->SetRadius( r );
}

void PointRepresentation::SetActive(bool yes)
{
    m_active = yes;
    PerViewContainer::iterator it = m_perViewContainer.begin();
    while( it != m_perViewContainer.end() )
    {
        PerViewElements *perView = (*it).second;
        if( yes )
            perView->pointRepresentationActor->PickableOn( );
        else
            perView->pointRepresentationActor->PickableOff( );
        ++it;
    }
}

bool PointRepresentation::HasActor( vtkActor * actor )
{
    PerViewContainer::iterator it = m_perViewContainer.begin();
    while( it != m_perViewContainer.end() )
    {
        PerViewElements * perView = (*it).second;
        if( perView->pointRepresentationActor == actor )
            return true;
        ++it;
    }
    return false;
}

void PointRepresentation::SetLabelScale(double s)
{
    m_labelScale = s;
    if( m_labelScale <= 0 )
        m_labelScale = 1.0;
    PerViewContainer::iterator it = m_perViewContainer.begin();
    while( it != m_perViewContainer.end() )
    {
        PerViewElements *perView = (*it).second;
        if( perView->labelActor )
            perView->labelActor->SetScale( m_labelScale, m_labelScale, m_labelScale );
        ++it;
    }
}

void PointRepresentation::SetLabel( const std::string & text )
{
    m_label->SetText(text.c_str());
}

const char* PointRepresentation::GetLabel()
{
    return m_label->GetText();
}

void PointRepresentation::ShowLabel( bool show)
{
    m_labelVisible = show;
    bool showNow = m_labelVisible && !this->IsHidden();
    PerViewContainer::iterator it = m_perViewContainer.begin();
    while( it != m_perViewContainer.end() )
    {
        PerViewElements *perView = (*it).second;
        if( perView->labelActor )
            perView->labelActor->SetVisibility( showNow ? 1 : 0 );
        ++it;
    }
}

void PointRepresentation::InternalWorldTransformChanged()
{
    vtkMatrix4x4 * invRot = vtkMatrix4x4::New();
    invRot->DeepCopy( this->GetWorldTransform()->GetMatrix() );
    invRot->SetElement( 0, 3, 0.0 );
    invRot->SetElement( 1, 3, 0.0 );
    invRot->SetElement( 2, 3, 0.0 );
    invRot->Invert();
    m_invWorldRotTransform->SetMatrix( invRot );
    invRot->Delete();

    // simtodo : why is this needed? it shouldn't since transform pipeline is self-updating
    UpdateAllTransforms();
}

void PointRepresentation::UpdateAllTransforms()
{
    m_invWorldRotTransform->Update();
    m_posTransform->Update();
    m_labelOffset->Update();
    m_point2DTransform->Update();
    m_point3DTransform->Update();
    m_labelTransform->Update();
}
