/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __GenericTrackedObject_h_
#define __GenericTrackedObject_h_

#include <QObject>
#include <QVector>
#include "sceneobject.h"
#include "hardwaremodule.h"

class vtkActor;
class vtkPoints;

class GenericTrackedObject : public SceneObject
{
    Q_OBJECT

public:

    static GenericTrackedObject * New() { return new GenericTrackedObject; }
    vtkTypeMacro(GenericTrackedObject,SceneObject);

    GenericTrackedObject();
    virtual ~GenericTrackedObject();

    void SetTrackerToolIndex( int index ) { m_trackerToolIndex = index; }
    int GetTrackerToolIndex() {return m_trackerToolIndex;}
    TrackerToolState GetToolState();

    // Implementation of parent virtual method
    virtual bool Setup( View * view );
    virtual bool Release( View * view );

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

public slots:

    void ChangeInView();

signals:

    void StatusChanged();

protected:

    virtual void Hide();
    virtual void Show();
    void ObjectAddedToScene();
    void ObjectRemovedFromScene();

    int m_trackerToolIndex;

    struct PerViewElements
    {
        PerViewElements();
        ~PerViewElements();
        vtkActor * genericObjectActor;
    };
    typedef std::map<View*,PerViewElements*> GenericTrackedObjectViewAssociation;
    GenericTrackedObjectViewAssociation m_genericObjectInstances;
};

#endif
