/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POINTSOBJECT_H
#define POINTSOBJECT_H

#include <vtkPoints.h>
#include <vtkSmartPointer.h>

#include <QList>
#include <QObject>
#include <QVector>

#include "pointrepresentation.h"
#include "sceneobject.h"
#include "serializer.h"

class View;
class vtkActor;
class vtkMatrix4x4;
class vtkProperty;
class vtkCellPicker;
class PolyDataObject;

/**
 * @class   PointsObject
 * @brief   Set of points
 *
 * PointsObject defines a set of points and all their common properties:
 * color, size, visibility, pickability, transformations.\n
 * Points are displayed using PointRepresentation, in 3D view a sphere, in 2D view a circle with a cross.
 * Points coordinates are stored as vtkPoints.\n
 * Points are read in and saved in a .tag file format.\n
 * Points may be enabled (active), or disabled. Disabled points cannot be picked.\n
 * Enabled, active and selected points are shown in different colors predefined in the constructor PointsObject().\n
 * Disabling points is useful when we want to exclude some points from calculations, e.g. landmark registration.
 * @sa
 * Application SceneManager View SceneObject PolyDataObject PointRepresentation
 */
class PointsObject : public SceneObject
{
    Q_OBJECT

public:
    static PointsObject * New() { return new PointsObject; }
    vtkTypeMacro( PointsObject, SceneObject );

    PointsObject();
    virtual ~PointsObject();

    // overwritten SceneObject methods
    virtual void Serialize( Serializer * ser ) override;
    virtual void Export() override;
    virtual bool IsExportable() override { return true; }
    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;

    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override;

    /** Index used to indicate that the point is not set. */
    static const int InvalidPointIndex;
    /** Minimum 3D sphere radius. */
    static const int MinRadius;
    /** Maximum 3D sphere radius. */
    static const int MaxRadius;
    /** Minimum label size, labels show only in 3D. */
    static const int MinLabelSize;
    /** Maximum label size, labels show only in 3D. */
    static const int MaxLabelSize;

    // actions on the object
    /** Get points coordinates. */
    vtkPoints * GetPoints();
    /** Get the list of all point names. */
    QStringList * GetPointsNames() { return &m_pointNames; }
    /** Get the list of all point time stamps. */
    QStringList * GetTimeStamps() { return &m_timeStamps; }
    /** Get the total number of points. */
    int GetNumberOfPoints() { return m_pointCoordinates->GetNumberOfPoints(); }
    /** Add a new point. */
    void AddPoint( const QString & name, double coords[3] );
    /** Allow/disallow picking points with mouse. */
    void SetPickable( bool p ) { m_pickable = p; }
    /** Allow/disallow picking points with mouse. */
    void SetPickabilityLocked( bool c ) { m_pickabilityLocked = c; }
    /** Get index of currently selected point. */
    int GetSelectedPointIndex() { return m_selectedPointIndex; }
    /** Select point using its index. */
    void SetSelectedPoint( int index );
    /** Unselect points */
    void UnselectAllPoints();
    /** Show cursor at the point specidfied by its index. */
    void MoveCursorToPoint( int index );

    // properties
    /** @name   Points Properties
     *  @brief  Basic properties - color and size of the points.
     */
    ///@{
    /** Set size of the point in 3D view. */
    void Set3DRadius( double r );
    /** Get size of the point in 3D view. */
    double Get3DRadius() { return m_pointRadius3D; }
    /** Set size of the point in 2D view. */
    void Set2DRadius( double r );
    /** Get size of the point in 2D view. */
    double Get2DRadius() { return m_pointRadius2D; }
    /** Set size of point's label. */
    void SetLabelSize( double s );
    /** Get size of point's label. */
    double GetLabelSize() { return m_labelSize; }
    /** Set color of enabled points. */
    void SetEnabledColor( double color[3] );
    /** Get color of enabled points. */
    double * GetEnabledColor() { return m_activeColor; }
    /** Get color of enabled points. */
    void GetEnabledColor( double color[3] );
    /** Set color of disabled points. */
    void SetDisabledColor( double color[3] );
    /** Get color of disabled points. */
    double * GetDisabledColor() { return m_inactiveColor; }
    /** Get color of disabled points. */
    void GetDisabledColor( double color[3] );
    /** Set color of selected point. */
    void SetSelectedColor( double color[3] );
    /** Get color of selected point. */
    double * GetSelectedColor() { return m_selectedColor; }
    /** Get color of selected point. */
    void GetSelectedColor( double color[3] );
    /** Set points opacity. */
    void SetOpacity( double opacity );
    /** Get points opacity. */
    double GetOpacity() { return m_opacity; }
    /** Show/hide points labels . */
    void ShowLabels( bool on );
    ///@}

    /** @name   Line to pointer tip
     *  @brief  When tracking, a line from a given point to the pointer tip may be traced in 3D view.
     */
    ///@{
    /** Set line color. */
    void SetLineToPointerColor( double color[3] );
    /** Get line color. */
    double * GetLineToPointerColor() { return m_lineToPointerColor; }
    /** Get line color. */
    void GetLineToPointerColor( double color[3] );
    /** Enable computing distance from the point to the pointer tip. */
    void EnableComputeDistance( bool enable );
    /** Check if computing distance between point and pointer tip is enabled. */
    bool GetComputeDistanceEnabled() { return m_computeDistance; }
    /** Compute distance from the selected point to the pointer tip and join them wth a line. */
    void ComputeDistanceFromSelectedPointToPointerTip();

    ///@}

    // access to individual points
    /** Remove point of the given index. */
    void RemovePoint( int index );
    /** Enable/disable point of the given index. */
    void EnableDisablePoint( int index, bool enable );
    /** Update color, size position and label of the point at the given index. */
    void UpdatePointProperties( int index );
    /** Label point of the given index. */
    void SetPointLabel( int index, const QString & label );
    /** Get label of point of the given index. */
    const QString GetPointLabel( int index );
    /** Get point coordinates. */
    double * GetPointCoordinates( int index );
    /** Get point coordinates. */
    void GetPointCoordinates( int index, double coords[3] );
    /** Set point coordinates. */
    void SetPointCoordinates( int index, double coords[3] );
    /** Set point timestamp - time when point was created. */
    void SetPointTimeStamp( int index, const QString & stamp );

signals:
    void PointAdded();
    void PointRemoved( int );
    void PointsChanged();

public slots:

    /** Set visibility of all points according to PointsObject visibility. */
    void UpdatePointsVisibility();
    /** React to the signal ObjectModified() emitted by PointsObject. */
    void OnCurrentObjectChanged();
    /** Update distance while poiter is moving. */
    virtual void UpdateDistance();

protected:
    vtkActor * DoPicking( int x, int y, vtkRenderer * ren, double pickedPoint[3] );
    virtual bool OnLeftButtonPressed( View * v, int x, int y, unsigned modifiers ) override;
    virtual bool OnLeftButtonReleased( View * v, int x, int y, unsigned modifiers ) override;
    virtual bool OnRightButtonPressed( View * v, int x, int y, unsigned modifiers ) override;
    virtual bool OnMouseMoved( View * v, int x, int y, unsigned modifiers ) override;

    void AddPointLocal( double coords[3], QString name = QString(), QString timestamp = QString() );
    int FindPoint( vtkActor * actor, double * pos, int viewType );

    virtual void Hide() override;
    virtual void Show() override;
    void UpdatePoints();

    // SceneObject overrides
    virtual void ObjectAddedToScene() override;
    virtual void ObjectAboutToBeRemovedFromScene() override;

    // Description
    //  structures to hold points coordinates and names
    vtkSmartPointer<vtkPoints> m_pointCoordinates;
    QStringList m_pointNames;
    QStringList m_timeStamps;
    int m_selectedPointIndex;

    double m_pointRadius3D;
    double m_pointRadius2D;
    double m_labelSize;
    double m_activeColor[3];
    double m_inactiveColor[3];
    double m_selectedColor[3];
    double m_opacity;
    bool m_pickable;
    bool m_pickabilityLocked;
    bool m_showLabels;
    bool m_computeDistance;

private:
    void LineToPointerTip( double selectedPoint[3], double pointerTip[3] );

    typedef QList<vtkSmartPointer<PointRepresentation> > PointList;
    PointList m_pointList;

    // Point manipulation with the mouse
    vtkSmartPointer<vtkCellPicker> m_picker;
    int m_movingPointIndex;

    vtkSmartPointer<PolyDataObject> m_lineToPointerTip;
    double m_lineToPointerColor[3];
};

ObjectSerializationHeaderMacro( PointsObject );

#endif
