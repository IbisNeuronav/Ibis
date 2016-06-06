/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __trackerstatusdialog_h_
#define __trackerstatusdialog_h_

#include <vector>
#include <qvariant.h>
#include <qwidget.h>
#include "trackerflags.h"
#include "hardwaremodule.h"

class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QCustomEvent;
class QComboBox;
class QCheckBox;
class QPushButton;
class Tracker;
class vtkQtMatrixDialog;


class ToolUI : public QWidget
{
 
Q_OBJECT
            
public:
    
    ToolUI( QWidget * parent = 0 );
    ~ToolUI();
    
    void SetTool( int index, Tracker * tracker );
    void Update();

protected slots:

    void SnapshotButtonClicked();
    void SnapshotMatrixWidgetClosed();
    
protected:

    TrackerToolState PreviousState;
    int ToolIndex;
    Tracker * Track;
    
    QHBoxLayout * ToolLayout;
    QPushButton * SnapshotButton;
    QLabel * ToolNameLabel;
    QLabel * ToolStateLabel;
    vtkQtMatrixDialog * SnapshotMatrixWidget;
    
};


class TrackerStatusDialog : public QWidget
{
    Q_OBJECT

public:
    
    TrackerStatusDialog( QWidget * parent = 0 );
    ~TrackerStatusDialog();
    
    void SetTracker( Tracker * tracker );
    
    QLineEdit * m_updateRateEdit;
    QLabel * m_updateRateLabel;
    QLabel * m_warningLabel;
    QComboBox *m_pointerToolCombo;
    QLabel *m_pointersLabel;
    QCheckBox *m_navigationCheckBox;

public slots:
    
    void TrackerUpdateEvent();
    void TrackerToolActivatedEvent();
    void TrackerStoppedEvent();
    void PointerToolComboChangedEvent(int);
    void NavigationStateChangedEvent(bool);
    void UpdateToolList();

protected:
    
    void UpdateToolStatus();
    
    void AppendTool( int toolIndex );
    void ClearAllTools();

    SceneManager * GetSceneManager();
    
    QVBoxLayout * TrackerStatusDialogLayout;
    std::vector< ToolUI * > ToolsWidget;
    
    Tracker * m_tracker;

};

#endif
