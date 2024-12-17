/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POINTEROBJECT_H
#define POINTEROBJECT_H

#include <vtkSmartPointer.h>

#include <QList>
#include <QObject>
#include <QVector>
#include <map>

#include "hardwaremodule.h"
#include "trackedsceneobject.h"

class vtkImageData;
class vtkMatrix4x4;
class vtkActor;
class PointsObject;
class Tracker;
class vtkAmoebaMinimizer;
class vtkDoubleArray;

class PointerObject : public TrackedSceneObject
{
    Q_OBJECT

public:
    static PointerObject * New() { return new PointerObject; }
    vtkTypeMacro( PointerObject, TrackedSceneObject );

    PointerObject();
    virtual ~PointerObject();

    // Implementation of parent virtual method
    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;

    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override;

    double * GetTipPosition();
    void GetMainAxisPosition( double pos[3] );

    // Tip calibration
    void StartTipCalibration();
    int InsertNextCalibrationPoint();
    bool IsCalibratingTip();
    double GetTipCalibrationRMSError();
    void CancelTipCalibration();
    void StopTipCalibration();

    void CreatePointerPickedPointsObject();
    void ManagerAddPointerPickedPointsObject();
    vtkSmartPointer<PointsObject> GetCurrentPointerPickedPointsObject()
    {
        return this->CurrentPointerPickedPointsObject;
    }
    void SetCurrentPointerPickedPointsObject( vtkSmartPointer<PointsObject> obj );
    const QList<vtkSmartPointer<PointsObject> > & GetPointerPickedPointsObjects()
    {
        return PointerPickedPointsObjectList;
    }

public slots:

    void UpdateTipCalibration();
    void RemovePointerPickedPointsObject( int objID );
    void UpdateSettings();

signals:
    void SettingsChanged();

private slots:
    void UpdateTip();

protected:
    virtual void Hide() override;
    virtual void Show() override;
    void ObjectAddedToScene() override;
    void ObjectRemovedFromScene() override;

    double m_lastTipCalibrationRMS;
    double m_backupCalibrationRMS;
    vtkSmartPointer<vtkMatrix4x4> m_backupCalibrationMatrix;

    vtkSmartPointer<PointsObject> CurrentPointerPickedPointsObject;

    double m_pointerAxis[3];
    double m_pointerUpDir[3];
    double m_tipLength;

    int m_calibrating;
    vtkAmoebaMinimizer * m_minimizer;
    vtkDoubleArray * m_calibrationArray;

    friend void vtkTrackerToolCalibrationFunction( void * userData );

    struct PerViewElements
    {
        PerViewElements();
        ~PerViewElements();
        vtkActor * tipActor;
    };

    typedef std::map<View *, PerViewElements *> PointerObjectViewAssociation;
    PointerObjectViewAssociation pointerObjectInstances;

    typedef QList<vtkSmartPointer<PointsObject> > PointerPickedPointsObjects;
    PointerPickedPointsObjects PointerPickedPointsObjectList;
};

#endif
