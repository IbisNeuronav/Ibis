/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POINTREPRESENTATION_H
#define POINTREPRESENTATION_H

#include <vtkSmartPointer.h>

#include <QObject>

#include "sceneobject.h"
#include "serializer.h"

class vtkActor;
class vtkProperty;
class vtkSphereSource;
class vtkVectorText;
class vtkFollower;
class vtkCircleWithCrossSource;
class View;

#define INITIAL_TAG_LABEL_SCALE 8.0

/**
 * @class   PointRepresentation
 * @brief   Graphic point representation
 *
 * PointRepresentation is a collection of actors that represent a point in all 4 main views.
 * The appeareance of the point is controlled using vtkProperty.
 * @sa
 * PointsObject SceneObject
 */
class PointRepresentation : public SceneObject
{
    Q_OBJECT

public:
    static PointRepresentation * New() { return new PointRepresentation; }
    vtkTypeMacro( PointRepresentation, SceneObject );

    PointRepresentation();
    virtual ~PointRepresentation();

    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;

    /** Set point's color. */
    void SetPropertyColor( double color[3] );

    /** Set point's opacity. */
    void SetOpacity( double opacity );

    /** Set point's size in 3D view. */
    void SetPointSizeIn3D( double );
    /** Set point's size in 2D view. */
    void SetPointSizeIn2D( double );

    /** Set point's index. */
    void SetPointIndex( int i ) { m_pointIndex = i; }
    /** Get point's index. */
    int GetPointIndex() { return m_pointIndex; }

    /** Set point's position. */
    ///@{
    void SetPosition( double p[3] );
    void SetPosition( double x, double y, double z );
    void GetPosition( double p[3] );
    ///@}

    /** Enable/disable picking. */
    void SetActive( bool yes );
    /** Check if the point is active. */
    bool GetActive() { return m_active; }

    /** Check if actor is used in any of the views. This is mainly for picking in 3D views.*/
    bool HasActor( vtkActor * actor );

    /** Set the label of the point. */
    void SetLabel( const std::string & text );
    /** Get the label of the point. */
    const char * GetLabel();

    /** Show/hide the label of the point. */
    void ShowLabel( bool show );

    /** Set label size. */
    void SetLabelScale( double s );

    /** Update the visibility of the point in 2D according to whether it is in a plane. */
    void UpdateVisibility();
    /** Check visibility of the point in 2D according to whether it is in a plane. */
    bool CheckVisibility();

protected:
    int m_pointIndex;
    vtkSmartPointer<vtkSphereSource> m_sphere;
    vtkSmartPointer<vtkCircleWithCrossSource> m_circle;
    bool m_active;

    // transforms to keep point aligned in 2D
    vtkSmartPointer<vtkTransform> m_point2DTransform;
    vtkSmartPointer<vtkTransform> m_point3DTransform;
    vtkSmartPointer<vtkTransform> m_invWorldRotTransform;
    vtkSmartPointer<vtkTransform> m_posTransform;
    vtkSmartPointer<vtkTransform> m_labelOffset;
    vtkSmartPointer<vtkTransform> m_labelTransform;

    virtual void Hide() override;
    virtual void Show() override;

    void InternalWorldTransformChanged() override;
    void UpdateAllTransforms();

    struct PerViewElements
    {
        PerViewElements();
        ~PerViewElements();
        vtkSmartPointer<vtkActor> pointRepresentationActor;
        vtkSmartPointer<vtkFollower> labelActor;
    };
    typedef std::map<View *, PerViewElements *> PerViewContainer;
    PerViewContainer m_perViewContainer;

    // Property used to control the appearance of selected objects and
    // the manipulator in general.
    vtkSmartPointer<vtkProperty> m_property;

    // The label
    vtkSmartPointer<vtkVectorText> m_label;
    double m_labelScale;
    bool m_labelVisible;
};

ObjectSerializationHeaderMacro( PointRepresentation );

#endif
