/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef SURFACECUTTINGPLANE_H
#define SURFACECUTTINGPLANE_H

#include <QObject>
#include <vtkObject.h>
#include "sceneobject.h"

class vtkTransform;
class vtkActor;
class vtkCutter;
class vtkPlane;
class vtkProperty;
class vtkPolyData;
class vtkAppendPolyData;
class QWidget;
class SceneManager;
class View;

class SurfaceCuttingPlane: public QObject, public vtkObject
{
    Q_OBJECT

public:
    static SurfaceCuttingPlane * New() { return new SurfaceCuttingPlane; }
    vtkTypeMacro(SurfaceCuttingPlane,vtkObject);

    SurfaceCuttingPlane();
    virtual ~SurfaceCuttingPlane();

    // Description:
    // Set scene manager
    void SetManager( SceneManager *manager );

    // Description:
    // Set PolyData input and prepare cutting planes
    void SetInput(vtkPolyData *);

    // Description:
    // Return cutter output
    vtkPolyData *GetCutterOutput(int);

    // Description:
    // Add PolyData object and update  cutting planes
    void AddInput(vtkPolyData *);

    // Description:
    // Remove PolyData object and update  cutting planes
    void RemoveInput(vtkPolyData *);

    // Description:
    // Set world transform
    void SetWorldTransform(vtkTransform *);

    // Description:
    // Get property
    vtkProperty *GetProperty() {return m_property;}

    // Description:
    // set property colory
    void SetPropertyColor(double color[3]);

    // Description:
    // set line width
    void SetPropertyLineWidth(int n);

    // Description:
    // set visibility
    void SetVisible(bool);

public slots:
    // Description
    // Update cutting plane position when one of the main planes is moved
    void UpdateCuttingPlane(int);

protected:
    SceneManager *m_manager;
    vtkTransform * m_worldTransform;
    vtkAppendPolyData *m_surfaceToCut;
    vtkProperty *m_property;
    vtkActor *m_cutterActor[3];
    vtkCutter *m_cutter[3];
    vtkPlane *m_cuttingPlane[3];

};

#endif // SURFACECUTTINGPLANE_H
