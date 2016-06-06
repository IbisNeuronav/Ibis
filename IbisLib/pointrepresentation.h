/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PointRepresentation_h_
#define __PointRepresentation_h_

#include "serializer.h"
#include <QObject>
#include <vtkObject.h>

class vtkTransform;
class vtkMatrix4x4;
class vtkActor;
class vtkCutter;
class vtkProperty;
class vtkSphereSource;
class vtkPlane;
class vtkPolyDataMapper;
class vtkVectorText;
class vtkFollower;
class vtkCylinderWithPlanesSource;
class SceneManager;

#define INITIAL_TAG_LABEL_SCALE 8.0

class PointRepresentation : public QObject, public vtkObject
{
    
Q_OBJECT

public:
        
    static PointRepresentation * New() { return new PointRepresentation; }
    vtkTypeMacro(PointRepresentation,vtkObject);
    
    PointRepresentation();
    virtual ~PointRepresentation();

    virtual void Serialize( Serializer * ser );

    void SetSceneManager(SceneManager *);

    void CreatePointRepresentation(double size2D, double size3D);

    // Description:
    // set property colory
    void SetPropertyColor(double color[3]);

    // Description:
    // set respective properties colors
    void SetOpacity(double opacity);

    // Description:
    // Set point size
    void SetPointSizeIn3D(double);
    void SetPointSizeIn2D(double);

    // Description:
    // Set/get point index
    void SetPointIndex(int i) {m_pointIndex = i;}
    int GetPointIndex() {return m_pointIndex;}

    // Description:
    // Set point position
    void SetPosition(double p[3]);
    void SetPosition(double x, double y, double z);
    void GetPosition(double p[3]) {p[0] = m_position[0]; p[1] = m_position[1]; p[2] = m_position[2];}
    double *GetPosition() {return m_position;}

    // Description:
    // Set world transform
    void SetWorldTransform(vtkTransform *);

    // Description
    // Enable/disable picking
    void SetPickable(bool yes);
    bool GetPickable() {return m_pickable;}

    // Description
    // Get specified actor
    vtkActor *GetPointActor(int i);

    // Description:
    // Set/Get the label of the point
    void SetLabel( const std::string & text );
    void SetNumericLabel(int n);
    const char* GetLabel();
    // Description:
    // Show/hide the label of the point
    void ShowLabel( bool show);
    void UpdateLabelPosition();

    // Description:
    // Set the Label size
    void SetLabelScale(double s);
    double *GetLabelScale();

    // Description:
    // Enable/disable point - will add or remove from view
    void Enable(bool show);

    // Description
    // Activate/de-activate point will set the flag and color of the point
    void SetPointActive(bool active) {m_active = active;}
    bool GetPointActive() {return m_active;}

    // Description
    // Rotate cylinders to set them along cutting plane normal and adjust cutting plane
    void SetCylindersTransform(vtkMatrix4x4 *mat);
    void SetCuttingPlaneTransform(vtkTransform *tr);

    // Description
    // copy point
    void ShallowCopy(PointRepresentation *source);

    // Description
    //
    void Update();

public slots:
    void UpdateCuttingPlane(int);

protected:

    SceneManager *m_manager;
    vtkTransform * m_worldTransform;
    vtkSphereSource * m_sphere;
    vtkCylinderWithPlanesSource *m_cylinder[3];
    vtkActor *m_pointRepresentationActor[4];
    vtkCutter *m_cutter[3];
    vtkPlane *m_cuttingPlane[3];
    double m_sizeIn3D;
    double m_sizeIn2D;
    double m_position[3];
    int m_pointIndex;
    bool m_representedInAllViews;
    bool m_active;
    bool m_pickable;

    // Property used to control the appearance of selected objects and
    // the manipulator in general.
    vtkProperty *m_property;

    // The label
    vtkVectorText *m_label;
    vtkFollower *m_labelActor;
    vtkPolyDataMapper *m_labelMapper;
};

ObjectSerializationHeaderMacro( PointRepresentation );

#endif
