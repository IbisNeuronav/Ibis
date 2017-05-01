/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __StereotacticFramePluginInterface_h_
#define __StereotacticFramePluginInterface_h_

#include <QObject>
#include "toolplugininterface.h"
#include "vtkSmartPointer.h"

class StereotacticFrameWidget;
class vtkNShapeCalibrationWidget;
class vtkTransform;
class vtkLandmarkTransform;
//class vtkAxesActor;
class PolyDataObject;
class vtkEventQtSlotConnect;
class Vec3;

class StereotacticFramePluginInterface : public QObject, public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.StereotacticFramePluginInterface" )

public:

    StereotacticFramePluginInterface();
    virtual ~StereotacticFramePluginInterface();
    virtual QString GetPluginName() { return QString("StereotacticFrame"); }
    virtual bool CanRun();
    virtual QString GetMenuEntryString() { return QString("Stereotactic Frame"); }

    virtual QWidget * CreateTab();
    virtual bool WidgetAboutToClose();

    bool IsShowingManipulators() { return m_manipulatorsOn; }
    void SetShowManipulators( bool on );
    bool IsShowing3DFrame() { return m_3DFrameOn; }
    void SetShow3DFrame( bool on );

    void GetCursorFramePosition( double pos[3] );
    void SetCursorFromFramePosition( double pos[3] );

public slots:

    void OnCursorMoved();
    void OnManipulatorsModified();
    void OnReferenceTransformChanged();

signals:

    void CursorMoved();

protected:

    void Initialize();
    void EnableManipulators();
    void DisableManipulators();
    void UpdateManipulators();
    void Enable3DFrame();
    void Disable3DFrame();
    void UpdateLandmarkTransform();
    vtkNShapeCalibrationWidget * m_manipulators[4];
    vtkEventQtSlotConnect * m_manipulatorsCallbacks;
    vtkSmartPointer<vtkTransform> m_frameTransform;
    vtkSmartPointer<vtkTransform> m_adjustmentTransform;
    vtkLandmarkTransform * m_landmarkTransform;
    PolyDataObject * m_frameRepresentation;

    bool m_manipulatorsOn;
    bool m_3DFrameOn;

    // 4 points of each of the 4 N of the frame. (x,y,z) for each.
    const Vec3 & GetNPointPos( int nIndex, int pointIndex );
    void ComputeInitialTransform();

};

#endif
