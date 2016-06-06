/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POLYDATACLIPPER_H
#define POLYDATACLIPPER_H

#include <QObject>
#include <vtkObject.h>
#include <vtkTransform.h>

class SceneManager;
class vtkPolyData;
class vtkImplicitFunction;
class vtkActor;
class vtkClipPolyData;
class vtkDataSetMapper;
class vtkProperty;

class PolyDataClipper : public QObject, public vtkObject
{
    Q_OBJECT
public:
    static PolyDataClipper * New() { return new PolyDataClipper; }
    vtkTypeMacro(PolyDataClipper,vtkObject);

    // Description:
    // Set scene manager
    void SetManager( SceneManager *manager );

    // Description:
    // Set PolyData input and prepare cutting planes
    void SetInput(vtkPolyData *);

    // Description:
    // Set world transform
    void SetWorldTransform(vtkTransform *);

    // Description:
    // Set actor property
    void SetActorProperty(vtkProperty *prop);

    // Description:
    // Set implicit function for clipper
    void SetClipFunction(vtkImplicitFunction *);

    // Description:
    // Remove actor
    void RemoveClipActor();

    // Description:
    // Show/hide actor
    void HideClipActor();
    void ShowClipActor();

    // Description:
    // Get output polydata
    vtkPolyData * GetClipperOutput();

    // Description:
    // Set scalar visibility
    void SetScalarsVisible( int use );

protected:

    SceneManager *m_manager;
    vtkTransform * m_worldTransform;
    vtkPolyData *m_polyDataToClip;
    vtkActor *m_clipperActor;
    vtkClipPolyData *m_clipper;
    vtkDataSetMapper *m_clipperMapper;
    vtkImplicitFunction *m_clipFunction;
    vtkProperty *m_clipperActorProperty;
    PolyDataClipper();
    virtual ~PolyDataClipper();

};

#endif // POLYDATACLIPPER_H
