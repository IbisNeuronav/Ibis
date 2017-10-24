/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PointsObject_h_
#define __PointsObject_h_

#include <QObject>
#include <QVector>
#include <QList>
#include "sceneobject.h"
#include "serializer.h"
#include "pointrepresentation.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"

class View;
class vtkActor;
class vtkMatrix4x4;
class vtkProperty;
class vtkCellPicker;
class PolyDataObject;

class PointsObject : public SceneObject
{

    Q_OBJECT

public:

    static PointsObject * New() { return new PointsObject; }
    vtkTypeMacro(PointsObject,SceneObject);

    PointsObject();
    virtual ~PointsObject();

    // overwritten SceneObject methods
    virtual void Serialize( Serializer * ser );
    virtual void Export();
    virtual bool IsExportable()  { return true; }
    virtual void Setup( View * view );
    virtual void Release( View * view );

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets );

    static const int InvalidPointIndex;
    static const int MinRadius;
    static const int MaxRadius;
    static const int MinLabelSize;
    static const int MaxLabelSize;

    // actions on the object
    vtkPoints *GetPoints();
    QStringList *GetPointsNames() { return & m_pointNames; }
    QStringList *GetTimeStamps() { return & m_timeStamps; }
    int GetNumberOfPoints() { return m_pointCoordinates->GetNumberOfPoints(); }
    void AddPoint( const QString &name, double coords[3] );
    void SetPickable( bool p ) { m_pickable = p; }
    void SetPickabilityLocked( bool c ) { m_pickabilityLocked = c; }
    int GetSelectedPointIndex() { return m_selectedPointIndex; }
    void SetSelectedPoint( int index );
    void MoveCursorToPoint( int index );

    // properties
    void Set3DRadius( double r );
    double Get3DRadius() { return m_pointRadius3D; }
    void Set2DRadius( double r );
    double Get2DRadius() { return m_pointRadius2D; }
    void SetLabelSize( double s );
    double GetLabelSize() { return m_labelSize; }
    void SetEnabledColor( double color[3] );
    double *GetEnabledColor() { return m_activeColor; }
    void GetEnabledColor(double color[3]);
    void SetDisabledColor( double color[3] );
    double *GetDisabledColor() { return m_inactiveColor; }
    void GetDisabledColor(double color[3]);
    void SetSelectedColor( double color[3] );
    double *GetSelectedColor() { return m_selectedColor; }
    void GetSelectedColor(double color[3]);


    void SetLineToPointerColor(double color[3]);
    double *GetLineToPointerColor() { return m_lineToPointerColor; }
    void GetLineToPointerColor( double color[3] );

    void SetOpacity( double opacity );
    double GetOpacity() { return m_opacity; }

    void ShowLabels( bool on );

    // access to individual points
    void RemovePoint( int index );
    void EnableDisablePoint( int index, bool enable );
    void UpdatePointProperties( int index );
    void SetPointLabel( int index, const QString & label );
    const QString GetPointLabel(int index);
    double *GetPointCoordinates( int index );
    void GetPointCoordinates( int index, double coords[3] );
    void SetPointCoordinates( int index, double coords[3] );
    void SetPointTimeStamp( int index, const QString & stamp);

    void EnableComputeDistance( bool enable );
    bool GetComputeDistanceEnabled() { return m_computeDistance; }

    void ComputeDistanceFromSelectedPointToPointerTip();

signals:
    void PointAdded();
    void PointRemoved( int );
    void PointsChanged();

public slots:

    void UpdatePointsVisibility();
    void OnCurrentObjectChanged();
    virtual void UpdateDistance();

protected:

    vtkActor * DoPicking( int x, int y, vtkRenderer * ren, double pickedPoint[3] );
    virtual bool OnLeftButtonPressed( View * v, int x, int y, unsigned modifiers );
    virtual bool OnLeftButtonReleased( View * v, int x, int y, unsigned modifiers );
    virtual bool OnRightButtonPressed( View * v, int x, int y, unsigned modifiers );
    virtual bool OnMouseMoved( View * v, int x, int y, unsigned modifiers );

    void AddPointLocal( double coords[3], QString name = QString(), QString timestamp = QString() );
    int FindPoint(vtkActor *actor, double *pos, int viewType );

    virtual void Hide();
    virtual void Show();
    void UpdatePoints();

    // SceneObject overrides
    virtual void ObjectAddedToScene();
    virtual void ObjectAboutToBeRemovedFromScene();

    //Description
    // structures to hold points coordinates and names
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
   bool   m_pickable;
   bool   m_pickabilityLocked;
   bool   m_showLabels;
   bool   m_computeDistance;

private:

   void LineToPointerTip( double selectedPoint[3], double pointerTip[3] );

   typedef QList< vtkSmartPointer<PointRepresentation> > PointList;
   PointList m_pointList;

   // Point manipulation with the mouse
   vtkSmartPointer<vtkCellPicker> m_picker;
   int m_movingPointIndex;

   vtkSmartPointer<PolyDataObject> m_lineToPointerTip;
   vtkSmartPointer<vtkProperty> m_lineToPointerProperty;
   double m_lineToPointerColor[3];
};

ObjectSerializationHeaderMacro( PointsObject );

#endif
