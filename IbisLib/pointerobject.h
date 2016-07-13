/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PointerObject_h_
#define __PointerObject_h_

#include <QList>
#include "trackedsceneobject.h"
#include "hardwaremodule.h"
#include <map>
#include <QVector>

class vtkImageData;
class vtkMatrix4x4;
class vtkActor;
class PointsObject;
class Tracker;

class PointerObject : public TrackedSceneObject
{
    
Q_OBJECT

public:
        
    static PointerObject * New() { return new PointerObject; }
    vtkTypeMacro( PointerObject, TrackedSceneObject );
    
    PointerObject();
    virtual ~PointerObject();
    
    // Implementation of parent virtual method
	virtual bool Setup( View * view );
	virtual bool Release( View * view );

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

    double * GetTipPosition();
    void GetMainAxisPosition( double pos[3] );

    // Tip calibration
    void StartTipCalibration();
    bool IsCalibratingTip();
    double GetTipCalibrationRMSError();
    void CancelTipCalibration();
    void StopTipCalibration();

    void CreatePointerPickedPointsObject();
    void ManagerAddPointerPickedPointsObject();
    PointsObject * GetCurrentPointerPickedPointsObject() {return this->CurrentPointerPickedPointsObject;}
    void SetCurrentPointerPickedPointsObject(PointsObject *obj){this->CurrentPointerPickedPointsObject = obj;}
    const QList<PointsObject*> & GetPointerPickedPointsObjects() { return PointerPickedPointsObjectList; }

public slots:

    void UpdateTipCalibration();
    void RemovePointerPickedPointsObject(int objID);
    void UpdateSettings();

signals:
    void SettingsChanged();
       
private slots:
    void UpdateTip();

protected:

    virtual void Hide();
    virtual void Show();
    void ObjectAddedToScene();
    void ObjectRemovedFromScene();

    double m_lastTipCalibrationRMS;
    double m_backupCalibrationRMS;
    vtkMatrix4x4 * m_backupCalibrationMatrix;

    PointsObject * CurrentPointerPickedPointsObject;

    double m_pointerAxis[3];
    double m_pointerUpDir[3];
    double m_tipLength;

    struct PerViewElements
    {
        PerViewElements();
        ~PerViewElements();
        vtkActor * tipActor;
    };
    
    typedef std::map<View*,PerViewElements*> PointerObjectViewAssociation;
    PointerObjectViewAssociation pointerObjectInstances;

    typedef QList <PointsObject*> PointerPickedPointsObjects;
    PointerPickedPointsObjects PointerPickedPointsObjectList;
};

#endif
