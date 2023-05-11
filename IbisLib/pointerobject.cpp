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

#include <vtkActor.h>
#include <vtkAmoebaMinimizer.h>
#include <vtkAxes.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

#include "application.h"
#include "hardwaremodule.h"
#include "pointerobjectsettingsdialog.h"
#include "pointsobject.h"
#include "scenemanager.h"
#include "view.h"

PointerObject::PerViewElements::PerViewElements() { tipActor = 0; }

PointerObject::PerViewElements::~PerViewElements()
{
    if( tipActor ) tipActor->Delete();
}

PointerObject::PointerObject()
{
    m_lastTipCalibrationRMS   = 0.0;
    m_backupCalibrationRMS    = 0.0;
    m_backupCalibrationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();

    m_calibrating      = 0;
    m_minimizer        = vtkAmoebaMinimizer::New();
    m_calibrationArray = vtkDoubleArray::New();
    m_calibrationArray->SetNumberOfComponents( 16 );

    m_pointerAxis[ 0 ]  = 0.0;
    m_pointerAxis[ 1 ]  = 0.0;
    m_pointerAxis[ 2 ]  = 1.0;
    m_pointerUpDir[ 0 ] = 0.0;
    m_pointerUpDir[ 0 ] = 1.0;
    m_pointerUpDir[ 0 ] = 0.0;

    m_tipLength = 5.0;

    this->CurrentPointerPickedPointsObject = 0;
    this->AllowChangeParent                = false;
}

PointerObject::~PointerObject()
{
    m_calibrationArray->Delete();
    m_minimizer->Delete();
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
        tipActor->SetVisibility( this->IsHidden() ? 0 : 1 );
        tipMapper->Delete();

        view->GetRenderer()->AddViewProp( tipActor );

        tipActor->SetUserTransform( this->GetWorldTransform() );

        // remember what we put in that view
        PerViewElements * perView            = new PerViewElements;
        perView->tipActor                    = tipActor;
        this->pointerObjectInstances[ view ] = perView;
        connect( this, SIGNAL( ObjectModified() ), view, SLOT( NotifyNeedRender() ) );
    }
}

void PointerObject::Release( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        disconnect( this, SIGNAL( ObjectModified() ), view, SLOT( NotifyNeedRender() ) );
        PointerObjectViewAssociation::iterator itAssociations = this->pointerObjectInstances.find( view );
        if( itAssociations != this->pointerObjectInstances.end() )
        {
            PerViewElements * perView = ( *itAssociations ).second;
            view->GetRenderer()->RemoveViewProp( perView->tipActor );
            delete perView;
            this->pointerObjectInstances.erase( itAssociations );
        }
    }
}

void PointerObject::ObjectAddedToScene()
{
    Q_ASSERT( this->GetManager() );
    connect( this->GetManager(), SIGNAL( ObjectRemoved( int ) ), this, SLOT( RemovePointerPickedPointsObject( int ) ) );
    connect( &( Application::GetInstance() ), SIGNAL( IbisClockTick() ), this, SLOT( UpdateTip() ) );
}

void PointerObject::ObjectRemovedFromScene()
{
    Q_ASSERT( this->GetManager() );
    disconnect( this->GetManager(), SIGNAL( ObjectRemoved( int ) ), this,
                SLOT( RemovePointerPickedPointsObject( int ) ) );
    disconnect( &( Application::GetInstance() ), SIGNAL( IbisClockTick() ), this, SLOT( UpdateTip() ) );

    for( int i = 0; i < PointerPickedPointsObjectList.count(); i++ )
    {
        this->GetManager()->RemoveObject( PointerPickedPointsObjectList.value( i ) );
    }
    PointerPickedPointsObjectList.clear();
}

double * PointerObject::GetTipPosition() { return this->GetWorldTransform()->GetPosition(); }

void PointerObject::GetMainAxisPosition( double pos[ 3 ] )
{
    double localPos[ 3 ] = { 0.0, 0.0, 140.0 };  // simtodo : this is hardcoded for fs613 : implement way to set this
    GetWorldTransform()->TransformPoint( localPos, pos );
}

void PointerObject::StartTipCalibration()
{
    m_backupCalibrationRMS = m_lastTipCalibrationRMS;
    m_backupCalibrationMatrix->DeepCopy( GetCalibrationMatrix() );
    m_calibrationArray->SetNumberOfTuples( 0 );
    m_calibrating = 1;
    connect( &Application::GetInstance(), SIGNAL( IbisClockTick() ), this, SLOT( UpdateTipCalibration() ) );
}

//----------------------------------------------------------------------------
void vtkTrackerToolCalibrationFunction( void * userData )
{
    PointerObject * self = (PointerObject *)userData;

    int i;
    int n = self->m_calibrationArray->GetNumberOfTuples();

    double x = self->m_minimizer->GetParameterValue( "x" );
    double y = self->m_minimizer->GetParameterValue( "y" );
    double z = self->m_minimizer->GetParameterValue( "z" );
    double nx, ny, nz, sx, sy, sz, sxx, syy, szz;

    double matrix[ 4 ][ 4 ];

    sx = sy = sz = 0.0;
    sxx = syy = szz = 0.0;

    for( i = 0; i < n; i++ )
    {
        self->m_calibrationArray->GetTuple( i, *matrix );

        nx = matrix[ 0 ][ 0 ] * x + matrix[ 0 ][ 1 ] * y + matrix[ 0 ][ 2 ] * z + matrix[ 0 ][ 3 ];
        ny = matrix[ 1 ][ 0 ] * x + matrix[ 1 ][ 1 ] * y + matrix[ 1 ][ 2 ] * z + matrix[ 1 ][ 3 ];
        nz = matrix[ 2 ][ 0 ] * x + matrix[ 2 ][ 1 ] * y + matrix[ 2 ][ 2 ] * z + matrix[ 2 ][ 3 ];

        sx += nx;
        sy += ny;
        sz += nz;

        sxx += nx * nx;
        syy += ny * ny;
        szz += nz * nz;
    }

    double r;

    if( n > 1 )
    {
        r = sqrt( ( sxx - sx * sx / n ) / ( n - 1 ) + ( syy - sy * sy / n ) / ( n - 1 ) +
                  ( szz - sz * sz / n ) / ( n - 1 ) );
    }
    else
    {
        r = 0;
    }

    self->m_minimizer->SetFunctionValue( r );
    /*
    cerr << self->Minimizer->GetIterations() << " (" << x << ", " << y << ", " << z << ")" << " " << r << " " << (sxx -
    sx*sx/n)/(n-1) << (syy - sy*sy/n)/(n-1) << (szz - sz*sz/n)/(n-1) << "\n";
    */
}

void PointerObject::UpdateTipCalibration()
{
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    // do tip calibration:
    this->m_minimizer->Initialize();
    this->m_minimizer->SetFunction( vtkTrackerToolCalibrationFunction, this );
    this->m_minimizer->SetFunctionArgDelete( NULL );
    this->m_minimizer->SetParameterValue( "x", 0 );
    this->m_minimizer->SetParameterScale( "x", 1000 );
    this->m_minimizer->SetParameterValue( "y", 0 );
    this->m_minimizer->SetParameterScale( "y", 1000 );
    this->m_minimizer->SetParameterValue( "z", 0 );
    this->m_minimizer->SetParameterScale( "z", 1000 );

    this->m_minimizer->Minimize();
    m_lastTipCalibrationRMS = this->m_minimizer->GetFunctionValue();

    double x = this->m_minimizer->GetParameterValue( "x" );
    double y = this->m_minimizer->GetParameterValue( "y" );
    double z = this->m_minimizer->GetParameterValue( "z" );

    mat->SetElement( 0, 3, x );
    mat->SetElement( 1, 3, y );
    mat->SetElement( 2, 3, z );

    SetCalibrationMatrix( mat );
    emit ObjectModified();
}

bool PointerObject::IsCalibratingTip() { return m_calibrating > 0 ? true : false; }

double PointerObject::GetTipCalibrationRMSError() { return m_lastTipCalibrationRMS; }

void PointerObject::CancelTipCalibration()
{
    m_calibrating           = 0;
    m_lastTipCalibrationRMS = m_backupCalibrationRMS;
    SetCalibrationMatrix( m_backupCalibrationMatrix );
}

void PointerObject::StopTipCalibration() { m_calibrating = 0; }

void PointerObject::CreatePointerPickedPointsObject()
{
    Q_ASSERT( this->GetManager() );
    int n = PointerPickedPointsObjectList.count();
    QString name( "PointerPoints" );
    name.append( QString::number( n ) );
    this->CurrentPointerPickedPointsObject = vtkSmartPointer<PointsObject>::New();
    this->CurrentPointerPickedPointsObject->SetName( name );
    this->CurrentPointerPickedPointsObject->SetCanAppendChildren( false );
    this->CurrentPointerPickedPointsObject->SetCanChangeParent( false );
    connect( this->CurrentPointerPickedPointsObject, SIGNAL( NameChanged() ), this, SLOT( UpdateSettings() ) );
    PointerPickedPointsObjectList.append( this->CurrentPointerPickedPointsObject );
}

void PointerObject::ManagerAddPointerPickedPointsObject()
{
    Q_ASSERT( this->GetManager() );
    this->GetManager()->AddObject( this->CurrentPointerPickedPointsObject );
}

void PointerObject::SetCurrentPointerPickedPointsObject( vtkSmartPointer<PointsObject> obj )
{
    this->CurrentPointerPickedPointsObject = obj;
}

void PointerObject::CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets )
{
    PointerObjectSettingsDialog * dlg = new PointerObjectSettingsDialog;
    dlg->setAttribute( Qt::WA_DeleteOnClose );
    if( this->CurrentPointerPickedPointsObject )
        dlg->SetPointerPickedPointsObject( this->CurrentPointerPickedPointsObject );
    dlg->SetPointer( this );
    dlg->setObjectName( "Properties" );
    connect( this, SIGNAL( SettingsChanged() ), dlg, SLOT( UpdateUI() ) );
    widgets->append( dlg );
}

void PointerObject::Hide()
{
    View * view                               = 0;
    PointerObjectViewAssociation::iterator it = this->pointerObjectInstances.begin();
    for( ; it != this->pointerObjectInstances.end(); ++it )
    {
        view = ( *it ).first;
        if( view->GetType() == THREED_VIEW_TYPE )
        {
            PerViewElements * perView = ( *it ).second;
            vtkActor * tipActor       = perView->tipActor;
            tipActor->VisibilityOff();
            emit ObjectModified();
        }
    }
}

void PointerObject::Show()
{
    View * view                               = 0;
    PointerObjectViewAssociation::iterator it = this->pointerObjectInstances.begin();
    for( ; it != this->pointerObjectInstances.end(); ++it )
    {
        view = ( *it ).first;
        if( view->GetType() == THREED_VIEW_TYPE )
        {
            PerViewElements * perView = ( *it ).second;
            vtkActor * tipActor       = perView->tipActor;
            tipActor->VisibilityOn();
            emit ObjectModified();
        }
    }
}

void PointerObject::RemovePointerPickedPointsObject( int objID )
{
    Q_ASSERT( this->GetManager() );
    SceneObject * obj                       = 0;
    PointerPickedPointsObjects::iterator it = PointerPickedPointsObjectList.begin();
    for( ; it != PointerPickedPointsObjectList.end(); ++it )
    {
        obj                          = (SceneObject *)( ( *it ) );
        PointsObject * objectRemoved = 0;
        if( obj->GetObjectID() == objID )
        {
            objectRemoved = PointsObject::SafeDownCast( obj );
            if( objectRemoved == this->CurrentPointerPickedPointsObject.GetPointer() )
                this->CurrentPointerPickedPointsObject = 0;
            PointerPickedPointsObjectList.removeOne( ( *it ) );
            emit SettingsChanged();
            return;
        }
    }
}

void PointerObject::UpdateSettings() { emit SettingsChanged(); }

// simtodo : this should not be needed
void PointerObject::UpdateTip()
{
    if( m_calibrating )
    {
        this->InsertNextCalibrationPoint();
    }
    if( this->ObjectHidden ) return;
    View * view = this->GetManager()->GetMain3DView();
    if( view )
    {
        PointerObjectViewAssociation::iterator itAssociations = this->pointerObjectInstances.find( view );
        if( itAssociations != this->pointerObjectInstances.end() )
        {
            PerViewElements * perView = ( *itAssociations ).second;
            vtkActor * tipActor       = perView->tipActor;
            tipActor->GetMapper()->Update();
            emit ObjectModified();
        }
    }
}

//----------------------------------------------------------------------------
// Insert the latest matrix in the buffer in the calibration array
int PointerObject::InsertNextCalibrationPoint()
{
    int ret                  = -1;
    vtkTransform * transform = this->GetUncalibratedTransform();
    vtkMatrix4x4 * mat       = vtkMatrix4x4::New();
    transform->GetMatrix( mat );
    ret = m_calibrationArray->InsertNextTuple( *mat->Element );
    return ret;
}
