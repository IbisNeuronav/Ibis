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

class View;
class PickerObject;
class vtkActor;
class vtkMatrix4x4;
class vtkProperty;
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
    virtual void PostSceneRead();
    virtual void Export();
    virtual bool IsExportable()  { return true; }
    virtual bool Release( View * view );
    virtual void ShallowCopy(SceneObject *source);

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets );

    static const int InvalidPointIndex;
    // actions on the object
    void SetPoints( vtkPoints *pt );
    vtkPoints *GetPoints() { return m_pointCoordinates; }
    void SetPointsNames( QStringList & names );
    QStringList *GetPointsNames() { return & m_pointNames; }
    QStringList *GetTimeStamps() { return & m_timeStamps; }
    int GetNumberOfPoints() { return m_pointCoordinates->GetNumberOfPoints(); }
    void AddPoint( const QString &name, double coords[3], bool show = false );
    void SetPickable( bool pickable );
    bool GetPickable() { return m_pickable; }
    void SetEnabled( bool enabled );
    bool GetEnabled() { return m_enabled; }
    void Reset();
    int GetSelectedPointIndex() { return m_selectedPointIndex; }
    void SetSelectedPoint( int index, bool movePlanes = true );
    int FindPoint(vtkActor **actor, double *pos);

    // properties
#define MIN_RADIUS 1
#define MAX_RADIUS 16
#define MIN_LABEl_SIZE 6
#define MAX_LABEL_SIZE 16
    void Set3DRadius( double r );
    double Get3DRadius() { return m_pointRadius3D; }
    void Set2DRadius( double r );
    double Get2DRadius() { return m_pointRadius2D; }
    void SetLabelSize( double s );
    double GetLabelSize() { return m_labelSize; }
    void SetEnabledColor( double color[3] );
    double *GetEnabledColor() { return m_enabledColor; }
    void GetEnabledColor(double color[3]);
    void SetDisabledColor( double color[3] );
    double *GetDisabledColor() { return m_disabledColor; }
    void GetDisabledColor(double color[3]);
    void SetSelectedColor( double color[3] );
    double *GetSelectedColor() { return m_selectedColor; }
    void GetSelectedColor(double color[3]);
    void SetLineToPointerColor(double color[3]);
    double *GetLineToPointerColor() { return m_lineToPointerColor; }
    void GetLineToPointerColor( double color[3] );
    void SetOpacity( double opacity );
    double GetOpacity() { return m_opacity; }
    double GetHotSpotSize() {return m_hotSpotSize;}
    void ShowLabels( bool on );

    // access to individual points
    void RemovePoint( int index );
    void EnableDisablePoint( int index, bool enable );
    void UpdatePointInScene( int index, bool movePlanes = true );
    void SetPointLabel( int index, const QString & label );
    const QString GetPointLabel(int index);
    double *GetPointCoordinates( int index );
    void GetPointCoordinates( int index, double coords[3] );
    void SetPointCoordinates( int index, double coords[3] );
    void SetPointTimeStamp( int index, const QString & stamp);
    vtkSetClampMacro(m_maxNumberOfPoints,int,0,10000);
    vtkSetClampMacro(m_minNumberOfPoints,int,0,10000);
    void SetMaxNumberOfPoints( int num ) { m_maxNumberOfPoints = num; }
    void SetMinNumberOfPoints( int num ) { m_minNumberOfPoints = num; }

    void EnableComputeDistance( bool enable );
    bool GetComputeDistanceEnabled() { return m_computeDistance; }

    void ComputeDistanceFromSelectedPointToPointerTip();

signals:
    void PointAdded();
    void PointRemoved( int );
    void PointsChanged();

public slots:
    void UpdatePoints();
    void UpdateSettingsWidget();
    void RotatePointsInScene( );
    virtual void OnCloseSettingsWidget();
    virtual void UpdateDistance();


protected:

    virtual void Hide();
    virtual void Show();
    void UpdatePickability();

    //Description
    // structures to hold points coordinates and names
   vtkPoints *m_pointCoordinates;
   QStringList m_pointNames;
   QStringList m_timeStamps;
   int m_selectedPointIndex;
   int m_maxNumberOfPoints;
   int m_minNumberOfPoints;

   double m_pointRadius3D;
   double m_pointRadius2D;
   double m_labelSize;
   double m_enabledColor[3];
   double m_disabledColor[3];
   double m_selectedColor[3];
   double m_opacity;
   bool   m_pickable;
   bool   m_enabled;
   bool   m_showLabels;
   bool   m_computeDistance;

private:
   void SetPointsInScene();
   void AddPointToScene(const QString & label, double * coords);
   void DeleteAllPointsFromScene();
   void LineToPointerTip( double selectedPoint[3], double pointerTip[3] );

   double m_hotSpotSize;

   typedef QList< PointRepresentation* > PointList;
   PointList m_pointList;

   PickerObject *m_mousePicker;

   PolyDataObject *m_lineToPointerTip;
   vtkProperty * m_lineToPointerProperty;
   double m_lineToPointerColor[3];
};

ObjectSerializationHeaderMacro( PointsObject );

#endif
