/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "pointercalibrationdialog.h"
#include <qpushbutton.h>
#include <qlineedit.h>
#include "vtkMatrix4x4.h"
#include "tracker.h"

PointerCalibrationDialog::PointerCalibrationDialog(QWidget * parent, const char * name )
    : QWidget(parent)
{
    setupUi(this);
    cancelButton->setEnabled( false );
    
    m_prevRMS = 0;
    m_prevCalibration = vtkMatrix4x4::New();
    
    m_currentToolIndex = -1;
}


PointerCalibrationDialog::~PointerCalibrationDialog()
{
    this->Cancel();
    m_prevCalibration->Delete();
}


void PointerCalibrationDialog::CalibrateButtonClicked()
{
    if( !m_tracker->IsToolCalibrating( m_currentToolIndex ) )
    {
        // backup the calibration and rms in case we have to cancel
        m_prevRMS = m_tracker->GetToolLastCalibrationRMS( m_currentToolIndex );
        m_tracker->SetToolLastCalibrationRMS( m_currentToolIndex, 0 );
        m_prevCalibration->DeepCopy( m_tracker->GetToolCalibrationMatrix( m_currentToolIndex ) );
        
        // start the calibration
        m_tracker->StartToolTipCalibration( m_currentToolIndex );
        connect( m_tracker, SIGNAL( Updated() ), this, SLOT( TrackerUpdateEvent() ) );
        
        cancelButton->setEnabled( true );
        calibrateButton->setText("Stop");
    }
    else
    {
        m_tracker->StopToolTipCalibration( m_currentToolIndex );
        m_tracker->disconnect( this );
        cancelButton->setEnabled( false );
        calibrateButton->setText("Calibrate");
    }
}


void PointerCalibrationDialog::CancelButtonClicked()
{
    this->Cancel();
    cancelButton->setEnabled( false );
    calibrateButton->setText("Calibrate");
    this->Update();
}


void PointerCalibrationDialog::TrackerUpdateEvent( )
{
    this->Update();
}


void PointerCalibrationDialog::CurrentToolChanged()
{
    this->Cancel();
    m_currentToolIndex = m_tracker->GetCurrentToolIndex();
    this->Update();
}


void PointerCalibrationDialog::Update()
{
    double rms = m_tracker->GetToolLastCalibrationRMS( m_currentToolIndex );
    vtkMatrix4x4 * mat = m_tracker->GetToolCalibrationMatrix( m_currentToolIndex );
    if( mat && rms != 0 )
    {
        tipRMSEdit->setText( QString::number( rms, 'f', 8 ) );
        QString x( QString::number( mat->GetElement( 0, 3 ), 'f', 8 ) );
        QString y( QString::number( mat->GetElement( 1, 3 ), 'f', 8 ) );
        QString z( QString::number( mat->GetElement( 2, 3 ), 'f', 8 ) );
        tipVectorEdit->setText( "( " + x + ", " + y + ", " + z + " )" );
    }
    else
    {
        if( m_tracker->IsToolCalibrating( m_currentToolIndex ) )
        {
            tipRMSEdit->setText("---Not enough points---");
            tipVectorEdit->setText("---Not enough points---");
        }
        else
        {
            tipRMSEdit->setText("-------");
            tipVectorEdit->setText("------");
        }
    }
}


void PointerCalibrationDialog::Cancel()
{
    if( m_tracker->IsToolCalibrating( m_currentToolIndex ) )
    {
        m_tracker->disconnect( this );
        m_tracker->StopToolTipCalibration( m_currentToolIndex );
        m_tracker->GetToolCalibrationMatrix( m_currentToolIndex )->DeepCopy( m_prevCalibration );
        m_tracker->SetToolLastCalibrationRMS( m_currentToolIndex, m_prevRMS );
    }
}


void PointerCalibrationDialog::SetTracker( Tracker * t )
{
    this->m_tracker = t;
    if( t )
    {
        m_currentToolIndex = t->GetCurrentToolIndex();
    }
    this->Update();
}
