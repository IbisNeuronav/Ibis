/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "generictrackedobject.h"
#include "generictrackedobjectsettingswidget.h"

#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkTransform.h"
#include "vtkTrackerTool.h"
#include "vtkAxes.h"
#include "view.h"
#include "scenemanager.h"
#include "application.h"

GenericTrackedObject::PerViewElements::PerViewElements()
{
    genericObjectActor = 0;
}

GenericTrackedObject::PerViewElements::~PerViewElements()
{
    if( genericObjectActor )
        genericObjectActor->Delete();
}

GenericTrackedObject::GenericTrackedObject()
{
    m_trackerToolIndex = -1;
    this->ObjectManagedByTracker = true;
    this->AllowChangeParent = false;
}

GenericTrackedObject::~GenericTrackedObject()
{
}

void GenericTrackedObject::ObjectAddedToScene()
{
    connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT( ChangeInView() ) );
}

void GenericTrackedObject::ObjectRemovedFromScene()
{
    disconnect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT( ChangeInView() ) );
}

TrackerToolState GenericTrackedObject::GetToolState()
{
    return Application::GetHardwareModule()->GetTrackerToolState( m_trackerToolIndex );
}

bool GenericTrackedObject::Setup( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        // Get the position of the tracked point in the tip's coordinate system
        double direction[3] = { 0, 1, 0 };

        //----------------------------------------------------------------
        // Build the origin
        //----------------------------------------------------------------
        vtkAxes * markerSource = vtkAxes::New();
        markerSource->SetScaleFactor( 10 );
        markerSource->SymmetricOn();
        markerSource->ComputeNormalsOff();

        vtkPolyDataMapper * markerMapper = vtkPolyDataMapper::New();
        markerMapper->SetInputData( markerSource->GetOutput() );

        vtkActor * markerActor = vtkActor::New();
        markerActor->SetMapper( markerMapper );

        markerSource->Delete();
        markerMapper->Delete();

        view->GetRenderer()->AddActor( markerActor );

        markerActor->SetUserTransform( this->GetWorldTransform() );

        // remember what we put in that view
        PerViewElements * perView = new PerViewElements;
        perView->genericObjectActor = markerActor;
        m_genericObjectInstances[ view ] = perView;
        connect( this, SIGNAL( Modified() ), this, SLOT(ChangeInView()) );
    }
    return true;
}

bool GenericTrackedObject::Release( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        this->disconnect( this, SLOT(ChangeInView()) );
        GenericTrackedObjectViewAssociation::iterator itAssociations = m_genericObjectInstances.find( view );
        if( itAssociations != m_genericObjectInstances.end() )
        {
            PerViewElements * perView = (*itAssociations).second;
            view->GetRenderer()->RemoveViewProp( perView->genericObjectActor );
            delete perView;
            m_genericObjectInstances.erase( itAssociations );
        }
    }
    return true;
}

void GenericTrackedObject::Hide()
{
    GenericTrackedObjectViewAssociation::iterator it = m_genericObjectInstances.begin();
    while( it != m_genericObjectInstances.end() )
    {
        PerViewElements *perView = (*it).second;
        perView->genericObjectActor->VisibilityOff();
        ++it;
    }
    emit Modified();
}

void GenericTrackedObject::Show()
{
    GenericTrackedObjectViewAssociation::iterator it = m_genericObjectInstances.begin();
    while( it != m_genericObjectInstances.end() )
    {
        PerViewElements *perView = (*it).second;
        perView->genericObjectActor->VisibilityOn();
        ++it;
    }
    emit Modified();
}

void GenericTrackedObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets)
{
    GenericTrackedObjectSettingsWidget *w = new GenericTrackedObjectSettingsWidget;
    w->setAttribute( Qt::WA_DeleteOnClose );
    w->setObjectName( "Properties" );
    w->SetGenericTrackedObject( this) ;
    widgets->append( w );
}

void GenericTrackedObject::ChangeInView()
{
    emit StatusChanged( );
}
