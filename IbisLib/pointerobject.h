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
#include "sceneobject.h"
#include "hardwaremodule.h"
#include <map>
#include <QVector>

class vtkImageData;
class vtkMatrix4x4;
class vtkActor;
class PointsObject;
class Tracker;

class PointerObject : public SceneObject
{
    
Q_OBJECT

public:
        
    static PointerObject * New() { return new PointerObject; }
     vtkTypeMacro(PointerObject,SceneObject);
    
    PointerObject();
    virtual ~PointerObject();
    
    vtkGetObjectMacro( CalibrationMatrix, vtkMatrix4x4 );
    void SetCalibrationMatrix( vtkMatrix4x4 * calMatrix );
    void SetTrackerToolIndex( int index ) { m_trackerToolIndex = index; }
    
    // Implementation of parent virtual method
	virtual bool Setup( View * view );
	virtual bool Release( View * view );

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

    double * GetTipPosition();
    double * GetMainAxisPosition();
    TrackerToolState GetState();
    bool IsMissing();
    bool IsOutOfView();
    bool IsOutOfVolume();
    bool IsOk();

    void CreatePointerPickedPointsObject();
    void ManagerAddPointerPickedPointsObject();
    PointsObject *GetCurrentPointerPickedPointsObject() {return this->CurrentPointerPickedPointsObject;}
    void SetCurrentPointerPickedPointsObject(PointsObject *obj){this->CurrentPointerPickedPointsObject = obj;}
    const QList<PointsObject*> & GetPointerPickedPointsObjects() { return PointerPickedPointsObjectList; }

public slots:
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

    int m_trackerToolIndex;
    vtkMatrix4x4 * CalibrationMatrix;
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
