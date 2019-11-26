#include "trackedsceneobject.h"
#include "vtkTransform.h"
#include "vtkRenderer.h"
#include "vtkAxes.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "serializerhelper.h"
#include "view.h"
#include "hardwaremodule.h"

ObjectSerializationMacro( TrackedSceneObject );

TrackedSceneObject::TrackedSceneObject()
{
    m_timestamp = -1;
    m_hardwareModule = nullptr;
    m_state = Undefined;
    m_transform = vtkTransform::New();
    m_calibrationTransform = vtkTransform::New();
    vtkTransform * localTransform = vtkTransform::New();
    localTransform->Concatenate( m_transform );
    localTransform->Concatenate( m_calibrationTransform );
    SetLocalTransform( localTransform );
    localTransform->Delete();

    m_uncalibratedWorldTransform = vtkTransform::New();
    m_uncalibratedWorldTransform->Concatenate( m_transform );
}

TrackedSceneObject::~TrackedSceneObject()
{
    m_transform->Delete();
    m_calibrationTransform->Delete();
    m_uncalibratedWorldTransform->Delete();
}

void TrackedSceneObject::Setup( View * view )
{
    SceneObject::Setup( view );

    if( view->GetType() == THREED_VIEW_TYPE )
    {
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
        m_genericActors[ view ] = markerActor;
    }
}

void TrackedSceneObject::Release( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        PerViewActors::iterator it = m_genericActors.find( view );
        if( it != m_genericActors.end() )
        {
            vtkActor * actor = (*it).second;
            view->GetRenderer()->RemoveViewProp( actor );
            actor->Delete();
            m_genericActors.erase( it );
        }
    }
}

void TrackedSceneObject::Hide()
{
    PerViewActors::iterator it = m_genericActors.begin();
    while( it != m_genericActors.end() )
    {
        vtkActor * actor = (*it).second;
        actor->VisibilityOff();
        ++it;
    }
    emit ObjectModified();
}

void TrackedSceneObject::Show()
{
    PerViewActors::iterator it = m_genericActors.begin();
    while( it != m_genericActors.end() )
    {
        vtkActor * actor = (*it).second;
        actor->VisibilityOn();
        ++it;
    }
    emit ObjectModified();
}

void TrackedSceneObject::Serialize( Serializer * ser )
{
    SceneObject::Serialize( ser );
    SerializeTracked( ser );
}

void TrackedSceneObject::SerializeTracked( Serializer * ser )
{
    vtkMatrix4x4 * calibrationMat = vtkMatrix4x4::New();
    m_calibrationTransform->GetMatrix( calibrationMat );
    ::Serialize( ser, "CalibrationMatrix", calibrationMat );

    if (ser->IsReader())
        SetCalibrationMatrix( calibrationMat );

    calibrationMat->Delete();
}

void TrackedSceneObject::SetState( TrackerToolState state )
{
    m_state = state;
    emit ObjectModified();
}

void TrackedSceneObject::SetInputMatrix( vtkMatrix4x4 * m )
{
    m_transform->SetMatrix( m );
    emit ObjectModified();
}

void TrackedSceneObject::SetInputTransform( vtkTransform * t )
{
    m_transform->SetInput( t );
}

void TrackedSceneObject::SetCalibrationMatrix( vtkMatrix4x4 * mat )
{
    m_calibrationTransform->SetMatrix( mat );
    emit ObjectModified();
}

vtkMatrix4x4 * TrackedSceneObject::GetCalibrationMatrix()
{
    return m_calibrationTransform->GetMatrix();
}

vtkTransform * TrackedSceneObject::GetReferenceToolTransform()
{
    Q_ASSERT( m_hardwareModule );
    return m_hardwareModule->GetReferenceTransform();
}

bool TrackedSceneObject::IsTransformFrozen()
{
    // simtodo : implement this
    return false;
}

void TrackedSceneObject::FreezeTransform()
{
    // simtodo : implement this
}

void TrackedSceneObject::UnFreezeTransform()
{
    // simtodo : implement this
}

void TrackedSceneObject::InternalUpdateWorldTransform()
{
    SceneObject::UpdateWorldTransform();

    m_uncalibratedWorldTransform->Identity();
    if( Parent )
        m_uncalibratedWorldTransform->Concatenate( Parent->GetWorldTransform() );
    m_uncalibratedWorldTransform->Concatenate( m_transform );
}
