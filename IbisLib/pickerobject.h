/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_PICKEROBJECT_h_
#define TAG_PICKEROBJECT_h_

#include <QObject>
#include "vtkObject.h"

class QWidget;
class QString;
class vtkTextMapper;
class vtkPoints;
class vtkActor;
class vtkCellPicker;
class vtkRenderer;
class vtkEventQtSlotConnect;
class View;
class PointsObject;
class SceneManager;

class PickerObject : public QObject, public vtkObject
{

    Q_OBJECT

public:

    static PickerObject * New() { return new PickerObject; }
    vtkTypeMacro(PickerObject,vtkObject);

    void SetManager( SceneManager * man ) { m_manager = man; }

    void SetCurrentPointIndex(int index) {m_currentPointIndex = index;}
    int GetCurrentPointIndex() {return m_currentPointIndex;}

    vtkCellPicker * GetPicker() {return m_picker;}
    PointsObject * GetSelectedPoints() {return m_selectedPoints;}
    void  SetSelectedPoints(PointsObject *);
    void HighlightPoint(int index);

    virtual void Setup();
    virtual void Release();

    void MovePoint(int, double*);
    
    void AddNewPoint( double *xyz );
    void ConnectObservers(bool connect);

public slots:

    void InteractorMouseEvent( vtkObject * caller, unsigned long vtk_event, void * client_data, void * call_data, vtkCommand * command );

protected:

    bool DoPicking( int x, int y, vtkRenderer * ren, double pickedPoint[3] );

    int m_currentPointIndex;
    int m_lastPickedPointIndex;
    vtkActor *m_lastPickedActor;
    double m_lastPickedPosition[3];
    vtkCellPicker *m_picker;
    double m_pickingPriority;
    bool m_addPoint;
    SceneManager * m_manager;

    // capture vtkRenderWindowInteractor's mouse events
    vtkEventQtSlotConnect * m_mouseCallback;
    
    PointsObject *m_selectedPoints;

    PickerObject();
    virtual ~PickerObject();
    
};

#endif //TAG_PICKEROBJECT_h_
