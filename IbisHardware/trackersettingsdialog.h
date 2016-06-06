/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_TRACKERSETTINGSDIALOG_H
#define TAG_TRACKERSETTINGSDIALOG_H
#include "ui_trackersettingsdialog.h"

class PointerCalibrationDialog;
class vtkQtMatrixDialog;
class Tracker;

class TrackerSettingsDialog : public QWidget, public Ui::TrackerSettingsDialog
{
    Q_OBJECT

public:
	TrackerSettingsDialog( QWidget* parent = 0, const char* name = 0 );
    virtual ~TrackerSettingsDialog();

    virtual void SetTracker( Tracker * tracker );
    virtual void UpdateGlobalSettings();

public slots:
    virtual void WorldCalibrationMatrixButtonClicked();
    virtual void WorldCalibrationDialogClosed();
    virtual void InitializeTrackerButtonClicked();
    virtual void ToolsListSelectionChanged();
    virtual void AddToolButtonClicked();
    virtual void RemoveToolButtonClicked();
    virtual void RenameToolButtonClicked();
    virtual void ReferenceToolComboChanged( int index );
    virtual void ActiveCheckBoxToggled( bool isOn );
    virtual void BrowseRomFileButtonClicked();
    virtual void TipCalibrationButtonClicked();
    virtual void TipCalibrationDialogClosed();
    virtual void ToolCalibrationMatrixButtonClicked();
    virtual void ToolCalibrationMatrixDialogClosed();
    virtual void UpdateToolDescription();
    virtual void UpdateToolList();
    virtual void ToolActivationEvent();
    virtual void ToolUseComboChanged( int index );
    virtual void NavigationPointerChanged(int);

protected:

    PointerCalibrationDialog * m_tipCalibrationDialog;
    vtkQtMatrixDialog * m_toolCalibrationDialog;
    vtkQtMatrixDialog * m_worldCalibrationDialog;
    Tracker * m_tracker;

};

#endif // TAG_TRACKERSETTINGSDIALOG_H
