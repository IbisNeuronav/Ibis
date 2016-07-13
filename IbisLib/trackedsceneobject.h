#ifndef __TrackedSceneObject_h_
#define __TrackedSceneObject_h_

#include <map>
#include "sceneobject.h"
#include "ibistypes.h"

class vtkActor;
class HardwareModule;

class TrackedSceneObject : public SceneObject
{
    Q_OBJECT

public:

    static TrackedSceneObject * New() { return new TrackedSceneObject; }
    vtkTypeMacro( TrackedSceneObject, SceneObject );

    TrackedSceneObject();
    virtual ~TrackedSceneObject();

    void SetHardwareModule( HardwareModule * hw ) { m_hardwareModule = hw; }
    bool IsDrivenByHardware() { return m_hardwareModule != 0; }
    HardwareModule * GetHardwareModule()  { return m_hardwareModule; }

    virtual bool Setup( View * view );
    virtual bool Release( View * view );
    virtual void Hide();
    virtual void Show();

    virtual void Serialize( Serializer * ser );
    virtual void SerializeTracked( Serializer * ser );

    TrackerToolState GetState() { return m_state; }
    void SetState( TrackerToolState state );
    bool IsMissing() { return m_state == Missing; }
    bool IsOutOfView() { return m_state == OutOfView; }
    bool IsOutOfVolume() { return m_state == OutOfVolume; }
    bool IsOk() { return m_state == Ok; }

    void SetInputTransform( vtkTransform * t );
    vtkTransform * GetUncalibratedTransform() { return m_transform; }
    void SetCalibrationMatrix( vtkMatrix4x4 * mat );
    vtkMatrix4x4 * GetCalibrationMatrix();

    vtkTransform * GetCalibrationTransform() { return m_calibrationTransform; }
    vtkTransform * GetUncalibratedWorldTransform() { return m_uncalibratedWorldTransform; }
    vtkTransform * GetReferenceToolTransform();

    bool IsTransformFrozen();
    void FreezeTransform();
    void UnFreezeTransform();

protected:

    virtual void InternalUpdateWorldTransform();

    HardwareModule * m_hardwareModule;

    // Generic representation
    typedef std::map< View *, vtkActor* > PerViewActors;
    PerViewActors m_genericActors;

    TrackerToolState m_state;
    vtkTransform * m_transform;
    vtkTransform * m_calibrationTransform;
    vtkTransform * m_uncalibratedWorldTransform;
};

ObjectSerializationHeaderMacro( TrackedSceneObject );

#endif
