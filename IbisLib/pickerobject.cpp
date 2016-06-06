/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <vector>

#include <QDateTime>
#include "vtkActor.h"
#include "vtkIndent.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCellPicker.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkPoints.h"
#include "vtkTransform.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkObject.h"
#include "vtkIgnsEvents.h"

#include "pickerobject.h"
#include "pointsobject.h"
#include "pointrepresentation.h"
#include "scenemanager.h"
#include "view.h"

PickerObject::PickerObject()
    : m_currentPointIndex(PointsObject::InvalidPointIndex)
    , m_lastPickedPointIndex(PointsObject::InvalidPointIndex)
    , m_lastPickedActor(0)
    , m_pickingPriority(1.0)
    , m_addPoint(false)
    , m_selectedPoints(0)
    , m_manager(0)
{
    m_picker = vtkCellPicker::New();
    m_mouseCallback = vtkEventQtSlotConnect::New();
    // Priority - m_pickingPriority - has to be above MultiImagePlaneWidget(0.5)
}

PickerObject::~PickerObject()
{
    m_picker->Delete();
    if (m_selectedPoints)
        m_selectedPoints->UnRegister(0);
    m_mouseCallback->Disconnect();
}

void  PickerObject::SetSelectedPoints(PointsObject *pts)
{
    if (m_selectedPoints != pts)
    {
        if (m_selectedPoints != NULL)
        {
            m_selectedPoints->UnRegister(0);
        }
        m_selectedPoints = pts;
        if (m_selectedPoints != NULL)
        {
            m_selectedPoints->Register(0);
        }
    }
}

void PickerObject::HighlightPoint(int index)
{
    if (index >= 0 && index < m_selectedPoints->GetNumberOfPoints() )
    {
        m_currentPointIndex = index;
        m_selectedPoints->SetSelectedPoint(index);
    }
}

void PickerObject::AddNewPoint(double *xyz)
{
    QDateTime timeStamp = QDateTime::currentDateTime();
    m_currentPointIndex = m_selectedPoints->GetNumberOfPoints();
    m_selectedPoints->AddPoint( QString::number(m_currentPointIndex+1).toUtf8().data(), xyz, true );
    m_selectedPoints->SetPointTimeStamp( m_currentPointIndex, timeStamp.toString(Qt::ISODate) );
    this->HighlightPoint(m_currentPointIndex);
}

#include "scenemanager.h"

bool PickerObject::DoPicking( int x, int y, vtkRenderer * ren, double pickedPoint[3] )
{
    int validPickedPoint = m_picker->Pick( x, y, 0, ren );
    double pickPosition[4];
    m_picker->GetPickPosition( pickPosition );
    pickPosition[3] = 1;

    // Transform the point in the space of its parent PointsObject
    // Apply inverse of points object world transform to worldPoint
    // World transform may have multiple concatenations
    // For some reason when we try to GetLinearInverse() of the WorldTransform with 2 or more concatenations,
    // we get wron invewrse e.g world is concatenation of: identity, t1, t2 - we get inverse of t1
    // while if we go for the inverse matrix, it is correct
    vtkMatrix4x4 *inverseMat = vtkMatrix4x4::New();
    m_selectedPoints->GetWorldTransform()->GetInverse(inverseMat);

    double transformedPoint[4];
    inverseMat->MultiplyPoint(pickPosition, transformedPoint);

    pickedPoint[0] = transformedPoint[0];
    pickedPoint[1] = transformedPoint[1];
    pickedPoint[2] = transformedPoint[2];

    m_lastPickedActor = m_picker->GetActor();
    return validPickedPoint != 0;
}

void PickerObject::InteractorMouseEvent( vtkObject * caller, unsigned long vtk_event, void */*client_data*/, void */*call_data*/, vtkCommand * command )
{
    // Get Interactor
    vtkRenderWindowInteractor * interactor = vtkRenderWindowInteractor::SafeDownCast( caller );
    if( !interactor )
        return;

    // Get Event position
    Q_ASSERT( m_manager );
    View * v = m_manager->GetViewFromInteractor( interactor );
    Q_ASSERT( v );
    vtkRenderer * renderer = v->GetRenderer();
    int * pickWindowPosition = interactor->GetEventPosition();

    if( vtk_event == vtkCommand::LeftButtonPressEvent && interactor->GetShiftKey() )
    {
        double realPosition[3];
        bool validPickedPoint = DoPicking( pickWindowPosition[0], pickWindowPosition[1], renderer, realPosition );
        if( validPickedPoint )
        {
            // Find point clicked
            m_lastPickedPointIndex = m_selectedPoints->FindPoint( &m_lastPickedActor, realPosition );

            // No point is close to where we clicked -> save coordinates
            if( m_lastPickedPointIndex == PointsObject::InvalidPointIndex )
            {
                for( int i = 0; i < 3; i++ )
                {
                    m_lastPickedPosition[i] = realPosition[i];
                }
                m_addPoint = true;
            }
            // Found a close point, select it
            else
            {
                this->HighlightPoint(m_lastPickedPointIndex);
            }
        }
        command->AbortFlagOn();
    }
    else if( vtk_event == vtkCommand::LeftButtonReleaseEvent )
    {
        double realPosition[3];
        bool validPickedPoint = DoPicking( pickWindowPosition[0], pickWindowPosition[1], renderer, realPosition );
        if( m_lastPickedPointIndex != PointsObject::InvalidPointIndex )
        {
            if( validPickedPoint )
                this->MovePoint( m_lastPickedPointIndex, &realPosition[0] );
            else
                this->MovePoint(m_lastPickedPointIndex, &m_lastPickedPosition[0] );
            command->AbortFlagOn();
        }
        else if( validPickedPoint && m_addPoint)
        {
            this->AddNewPoint( &realPosition[0] );
            this->HighlightPoint(m_lastPickedPointIndex);
            m_addPoint = false;
            command->AbortFlagOn();
        }
        m_lastPickedPointIndex = PointsObject::InvalidPointIndex;
        m_lastPickedActor = 0;
    }
    if( vtk_event == vtkCommand::RightButtonPressEvent && interactor->GetShiftKey() )
    {
        double realPosition[3];
        bool validPickedPoint = DoPicking( pickWindowPosition[0], pickWindowPosition[1], renderer, realPosition );
        if( validPickedPoint )
        {
            int pickedPointIndex = m_selectedPoints->FindPoint( &m_lastPickedActor, realPosition );
            if( pickedPointIndex > PointsObject::InvalidPointIndex )
                m_selectedPoints->RemovePoint( pickedPointIndex );
            command->AbortFlagOn();
        }
    }
    else if( vtk_event == vtkCommand::MouseMoveEvent )
    {
        if (m_lastPickedPointIndex > PointsObject::InvalidPointIndex)
        {
            double realPosition[3];
            bool validPickedPoint = DoPicking( pickWindowPosition[0], pickWindowPosition[1], renderer, realPosition );
            if( validPickedPoint )
            {
                this->MovePoint( m_lastPickedPointIndex, &realPosition[0] );
                for( int i = 0; i < 3; ++i )
                    m_lastPickedPosition[i] = realPosition[i];
            }
            else
                this->MovePoint( m_lastPickedPointIndex, &m_lastPickedPosition[0] );
            command->AbortFlagOn();
        }
    }
}

void PickerObject::Setup( )
{
    this->ConnectObservers(true);
}

void PickerObject::Release()
{
    this->ConnectObservers(false);
    if (m_selectedPoints != NULL)
    {
        m_selectedPoints->UnRegister(0);
    }
    m_selectedPoints = 0;
}

void PickerObject::ConnectObservers( bool connect )
{
    Q_ASSERT( m_manager );
    for (int i = 0; i < 4; i++)
    {
        View * view = m_manager->GetView(i);
        vtkRenderWindowInteractor * interactor = view->GetInteractor();
        if ( connect )
        {
            interactor->SetPicker(m_picker);
            m_mouseCallback->Connect( interactor, vtkCommand::LeftButtonPressEvent, this, SLOT(InteractorMouseEvent(vtkObject*,ulong,void*,void*,vtkCommand*)), /*void *client_data=*/NULL, m_pickingPriority);
            m_mouseCallback->Connect( interactor, vtkCommand::MouseMoveEvent, this, SLOT(InteractorMouseEvent(vtkObject*,ulong,void*,void*,vtkCommand*)), /*void *client_data=*/NULL, m_pickingPriority);
            m_mouseCallback->Connect( interactor, vtkCommand::LeftButtonReleaseEvent, this, SLOT(InteractorMouseEvent(vtkObject*,ulong,void*,void*,vtkCommand*)), /*void *client_data=*/NULL, m_pickingPriority);
            m_mouseCallback->Connect( interactor, vtkCommand::RightButtonPressEvent, this, SLOT(InteractorMouseEvent(vtkObject*,ulong,void*,void*,vtkCommand*)), /*void *client_data=*/NULL, m_pickingPriority);
            m_mouseCallback->Connect( interactor, vtkCommand::RightButtonReleaseEvent, this, SLOT(InteractorMouseEvent(vtkObject*,ulong,void*,void*,vtkCommand*)), /*void *client_data=*/NULL, m_pickingPriority);
        }
        else
        {
            m_mouseCallback->Disconnect( interactor );
            interactor->SetPicker(view->GetPicker());
        }
    }
 }

void PickerObject::MovePoint(int index, double *position)
{
    m_selectedPoints->SetPointCoordinates(index, position);
}

