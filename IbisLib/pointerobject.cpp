/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "pointerobject.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "view.h"
#include "vtkTransform.h"
#include "vtkAxes.h"
#include "vtkMath.h"
#include <vtkProperty.h>
#include "scenemanager.h"
#include "pointerobjectsettingsdialog.h"
#include "pointsobject.h"

#include "application.h"
#include "hardwaremodule.h"

PointerObject::PerViewElements::PerViewElements() 
{ 
    tipActor = 0;
}
        
PointerObject::PerViewElements::~PerViewElements() 
{ 
    if( tipActor ) tipActor->Delete();
}

PointerObject::PointerObject()
{
    m_lastTipCalibrationRMS = 0.0;
    m_backupCalibrationRMS = 0.0;
    m_backupCalibrationMatrix = vtkMatrix4x4::New();

    m_pointerAxis[0] = 0.0;
    m_pointerAxis[1] = 0.0;
    m_pointerAxis[2] = 1.0;
    m_pointerUpDir[0] = 0.0;
    m_pointerUpDir[0] = 1.0;
    m_pointerUpDir[0] = 0.0;

    m_tipLength = 5.0;

    this->CurrentPointerPickedPointsObject = 0;
    this->AllowChangeParent = false;
}

PointerObject::~PointerObject()
{
}

void PointerObject::Setup( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        //----------------------------------------------------------------
        // Build the tip
        //----------------------------------------------------------------
        vtkAxes * tipSource = vtkAxes::New();
        tipSource->SetScaleFactor( m_tipLength );
        tipSource->SymmetricOn();
        tipSource->ComputeNormalsOff();
    
        vtkPolyDataMapper * tipMapper = vtkPolyDataMapper::New();
        tipMapper->SetInputConnection( tipSource->GetOutputPort() );
        tipSource->Delete();

        vtkActor * tipActor = vtkActor::New();
        tipActor->SetMapper( tipMapper );
        tipMapper->Delete();
        
        view->GetRenderer()->AddViewProp( tipActor );

		tipActor->SetUserTransform( this->GetWorldTransform() );
        
        // remember what we put in that view
        PerViewElements * perView = new PerViewElements;
        perView->tipActor = tipActor;
        this->pointerObjectInstances[ view ] = perView;
        connect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
    }
}

void PointerObject::Release( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        disconnect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
        PointerObjectViewAssociation::iterator itAssociations = this->pointerObjectInstances.find( view );
        if( itAssociations != this->pointerObjectInstances.end() )
        {
            PerViewElements * perView = (*itAssociations).second;
			view->GetRenderer()->RemoveViewProp( perView->tipActor );
            delete perView;
            this->pointerObjectInstances.erase( itAssociations );
        }
    }
}

void PointerObject::ObjectAddedToScene()
{
    Q_ASSERT( this->GetManager() );
    connect(this->GetManager(), SIGNAL(ObjectRemoved(int)), this, SLOT(RemovePointerPickedPointsObject(int)));
    connect( &(Application::GetInstance()), SIGNAL(IbisClockTick()), this, SLOT(UpdateTip()) );
}

void PointerObject::ObjectRemovedFromScene()
{
    Q_ASSERT( this->GetManager() );
    disconnect(this->GetManager(), SIGNAL(ObjectRemoved(int)), this, SLOT(RemovePointerPickedPointsObject(int)));
    disconnect( &(Application::GetInstance()), SIGNAL(IbisClockTick()), this, SLOT(UpdateTip()) );

    for (int i = 0; i < PointerPickedPointsObjectList.count(); i++)
    {
        PointerPickedPointsObjectList.value(i)->Delete();
        this->GetManager()->RemoveObject(PointerPickedPointsObjectList.value(i));
    }
    PointerPickedPointsObjectList.clear();
    this->CurrentPointerPickedPointsObject = 0;
}

double * PointerObject::GetTipPosition()
{
    return this->GetWorldTransform()->GetPosition();
}

void PointerObject::GetMainAxisPosition( double pos[3] )
{
    double localPos[3] = { 0.0, 0.0, 140.0 };   // simtodo : this is hardcoded for fs613 : implement way to set this
    GetWorldTransform()->TransformPoint( localPos, pos );
}

void PointerObject::StartTipCalibration()
{
    m_backupCalibrationRMS = m_lastTipCalibrationRMS;
    m_backupCalibrationMatrix->DeepCopy( GetCalibrationMatrix() );
    GetHardwareModule()->StartTipCalibration( this );
    connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(UpdateTipCalibration()) );
}

void PointerObject::UpdateTipCalibration()
{
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    m_lastTipCalibrationRMS = GetHardwareModule()->DoTipCalibration( this, mat );
    SetCalibrationMatrix( mat );
    emit Modified();
}

bool PointerObject::IsCalibratingTip()
{
    return GetHardwareModule()->IsCalibratingTip( this );
}

double PointerObject::GetTipCalibrationRMSError()
{
    return m_lastTipCalibrationRMS;
}

void PointerObject::CancelTipCalibration()
{
    GetHardwareModule()->StopTipCalibration( this );
    m_lastTipCalibrationRMS = m_backupCalibrationRMS;
    SetCalibrationMatrix( m_backupCalibrationMatrix );
}

void PointerObject::StopTipCalibration()
{
    GetHardwareModule()->StopTipCalibration( this );
}

void PointerObject::CreatePointerPickedPointsObject()
{
    Q_ASSERT(this->GetManager());
    int n = PointerPickedPointsObjectList.count();
    QString name("PointerPoints");
    name.append(QString::number(n));
    this->CurrentPointerPickedPointsObject = PointsObject::New();
    this->CurrentPointerPickedPointsObject->SetName(name);
    this->CurrentPointerPickedPointsObject->SetCanAppendChildren(false);
    this->CurrentPointerPickedPointsObject->SetCanChangeParent(false);
    connect(this->CurrentPointerPickedPointsObject, SIGNAL(NameChanged()), this, SLOT(UpdateSettings()));
    PointerPickedPointsObjectList.append(this->CurrentPointerPickedPointsObject);
}

void PointerObject::ManagerAddPointerPickedPointsObject()
{
    Q_ASSERT(this->GetManager());
    this->GetManager()->AddObject( this->CurrentPointerPickedPointsObject );
    this->GetManager()->SetCurrentObject( this->CurrentPointerPickedPointsObject );
}

void PointerObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets )
{
    PointerObjectSettingsDialog *dlg = new PointerObjectSettingsDialog;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (this->CurrentPointerPickedPointsObject)
        dlg->SetPointerPickedPointsObject(this->CurrentPointerPickedPointsObject);
    dlg->SetPointer(this);
    dlg->setObjectName("Properties");
    connect (this, SIGNAL(SettingsChanged()), dlg, SLOT(UpdateSettings()));
    widgets->append(dlg);
}

void PointerObject::Hide()
{
    View * view = 0;
    PointerObjectViewAssociation::iterator it = this->pointerObjectInstances.begin();
    for( ; it != this->pointerObjectInstances.end(); ++it )
    {
        view = (*it).first;
        if (view->GetType() == THREED_VIEW_TYPE)
        {
            PerViewElements * perView = (*it).second;
            vtkActor * tipActor = perView->tipActor;
            tipActor->VisibilityOff();
            emit Modified();
        }
    }
}


void PointerObject::Show()
{
    View * view = 0;
    PointerObjectViewAssociation::iterator it = this->pointerObjectInstances.begin();
    for( ; it != this->pointerObjectInstances.end(); ++it )
    {
        view = (*it).first;
        if (view->GetType() == THREED_VIEW_TYPE)
        {
            PerViewElements * perView = (*it).second;
            vtkActor * tipActor = perView->tipActor;
            tipActor->VisibilityOn();
            emit Modified();
        }
    }
}

void PointerObject::RemovePointerPickedPointsObject( int objID )
{
    Q_ASSERT(this->GetManager());
    SceneObject* obj = 0;
    PointerPickedPointsObjects::iterator it = PointerPickedPointsObjectList.begin();
    for(; it != PointerPickedPointsObjectList.end(); ++it)
    {
        obj = (SceneObject*)(*it);
        PointsObject *objectRemoved = 0;
        if (obj->GetObjectID() == objID )
        {
            objectRemoved = PointsObject::SafeDownCast(obj);
            PointerPickedPointsObjectList.removeOne(objectRemoved);
            objectRemoved->Delete();
            emit SettingsChanged();
            return;
        }
    }
}

void PointerObject::UpdateSettings()
{
    emit SettingsChanged();
}

// simtodo : this should not be needed
void PointerObject::UpdateTip()
{
    if( this->ObjectHidden )
        return;
    View * view = this->GetManager()->GetView(THREED_VIEW_TYPE);
    if (view)
    {
        PointerObjectViewAssociation::iterator itAssociations = this->pointerObjectInstances.find( view );
        if( itAssociations != this->pointerObjectInstances.end() )
        {
            PerViewElements * perView = (*itAssociations).second;
            vtkActor * tipActor = perView->tipActor;
            tipActor->GetMapper()->Update();
            emit Modified();
        }
    }
}
