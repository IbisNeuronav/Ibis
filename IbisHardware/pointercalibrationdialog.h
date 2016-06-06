/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_POINTERCALIBRATIONDIALOG_H
#define TAG_POINTERCALIBRATIONDIALOG_H

#include "ui_pointercalibrationdialog.h"

class vtkMatrix4x4;
class Tracker;

class PointerCalibrationDialog : public QWidget, public Ui::PointerCalibrationDialog
{
    Q_OBJECT

public:
	PointerCalibrationDialog( QWidget* parent = 0, const char* name = 0 );
    virtual ~PointerCalibrationDialog();
    
    void CurrentToolChanged();
    void SetTracker( Tracker * t );

public slots:
    virtual void CalibrateButtonClicked();
    virtual void CancelButtonClicked();
    virtual void TrackerUpdateEvent();

protected:

    void Update();
    void Cancel();
    
    vtkMatrix4x4 *m_prevCalibration;
    double m_prevRMS;
    Tracker * m_tracker;
    int m_currentToolIndex;
};

#endif // TAG_POINTERCALIBRATIONDIALOG_H
