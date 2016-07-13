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
#include "sceneobject.h"

class vtkActor;
class vtkProperty;
class vtkSphereSource;
class vtkVectorText;
class vtkFollower;
class vtkCircleWithCrossSource;
class View;

#define INITIAL_TAG_LABEL_SCALE 8.0

class PointRepresentation : public SceneObject
{
    
Q_OBJECT

public:
        
    static PointRepresentation * New() { return new PointRepresentation; }
    vtkTypeMacro(PointRepresentation,SceneObject);
    
    PointRepresentation();
    virtual ~PointRepresentation();

    virtual bool Setup( View * view );
    virtual bool Release( View * view );

    void CreatePointRepresentation();

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
    void GetPosition(double p[3]);

    // Description
    // Enable/disable picking
    void SetActive(bool yes);
    bool GetActive() { return m_active; }

    // Description
    // Check if actor is used in any of the views. This is mainly for picking in 3D views
    bool HasActor( vtkActor * actor );

    // Description:
    // Set/Get the label of the point
    void SetLabel( const std::string & text );
    const char* GetLabel();
    // Description:
    // Show/hide the label of the point
    void ShowLabel( bool show);

    // Description:
    // Set the Label size
    void SetLabelScale(double s);

    // Update the visibility of the point in 2D
    // according to whether it is in a plane
    void UpdateVisibility();

protected:

    int m_pointIndex;
    vtkSphereSource * m_sphere;
    vtkCircleWithCrossSource *m_circle;
    bool m_active;

    // transforms to keep point aligned in 2D
    vtkTransform * m_point2DTransform;
    vtkTransform * m_point3DTransform;
    vtkTransform * m_invWorldRotTransform;
    vtkTransform * m_posTransform;
    vtkTransform * m_labelOffset;
    vtkTransform * m_labelTransform;

    virtual void Hide();
    virtual void Show();

    void InternalWorldTransformChanged();
    void UpdateAllTransforms();

    struct PerViewElements
    {
        PerViewElements();
        ~PerViewElements();
        vtkActor * pointRepresentationActor;
        vtkFollower * labelActor;
    };
    typedef std::map< View*, PerViewElements* > PerViewContainer;
    PerViewContainer m_perViewContainer;

    // Property used to control the appearance of selected objects and
    // the manipulator in general.
    vtkProperty *m_property;

    // The label
    vtkVectorText *m_label;
    double m_labelScale;
    bool m_labelVisible;
};

ObjectSerializationHeaderMacro( PointRepresentation );

#endif
