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

#include <QObject>
#include "ui_trackersettingsdialog.h"

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

    virtual void InitializeTrackerButtonClicked();
    virtual void ToolsListSelectionChanged();
    virtual void AddToolButtonClicked();
    virtual void RemoveToolButtonClicked();
    virtual void RenameToolButtonClicked();
    virtual void ReferenceToolComboChanged( int index );
    virtual void ActiveCheckBoxToggled( bool isOn );
    virtual void BrowseRomFileButtonClicked();
    virtual void UpdateToolDescription();
    virtual void UpdateToolList();
    virtual void ToolUseComboChanged( int index );

protected:

    void UpdateUI();
    Tracker * m_tracker;

};

#endif
