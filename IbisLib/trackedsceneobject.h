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

    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;
    virtual void Hide() override;
    virtual void Show() override;

    virtual void Serialize( Serializer * ser ) override;
    virtual void SerializeTracked( Serializer * ser );

    TrackerToolState GetState() { return m_state; }
    void SetState( TrackerToolState state );
    bool IsMissing() { return m_state == Missing; }
    bool IsOutOfView() { return m_state == OutOfView; }
    bool IsOutOfVolume() { return m_state == OutOfVolume; }
    bool IsOk() { return m_state == Ok; }

    void SetInputMatrix( vtkMatrix4x4 * m );
    void SetInputTransform( vtkTransform * t );
    vtkTransform * GetUncalibratedTransform() { return m_transform; }
    void SetCalibrationMatrix( vtkMatrix4x4 * mat );
    void SetTimestamp(double timestamp) { (timestamp < 0) ? m_timestamp = -1 : m_timestamp = timestamp; }
    vtkMatrix4x4 * GetCalibrationMatrix();

    vtkTransform * GetCalibrationTransform() { return m_calibrationTransform; }
    vtkTransform * GetUncalibratedWorldTransform() { return m_uncalibratedWorldTransform; }
    vtkTransform * GetReferenceToolTransform();
    double GetLastTimestamp() { return m_timestamp; }

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

    double m_timestamp;
};

ObjectSerializationHeaderMacro( TrackedSceneObject );

#endif
