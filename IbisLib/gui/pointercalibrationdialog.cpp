/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: pointercalibrationdialog.cpp,v $
  Language:  C++
  Date:      $Date: 2010-06-30 18:03:32 $
  Version:   $Revision: 1.4 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill
  Re-make from the old Simon's code
  All rights reserved.

=========================================================================*/
#include "pointercalibrationdialog.h"
#include <QPushButton>
#include <QLineEdit>
#include <vtkMatrix4x4.h>
#include "pointerobject.h"
#include "application.h"

PointerCalibrationDialog::PointerCalibrationDialog( QWidget * parent ) : QWidget(parent)
{
    setupUi(this);
    m_pointer = 0;
}

PointerCalibrationDialog::~PointerCalibrationDialog()
{
    if( m_pointer && m_pointer->IsCalibratingTip() )
        m_pointer->CancelTipCalibration();
}

void PointerCalibrationDialog::SetPointer( PointerObject * p )
{
    m_pointer = p;
    connect( m_pointer, SIGNAL(ObjectModified()), this, SLOT(update()) );
    Update();
}

void PointerCalibrationDialog::CalibrateButtonClicked()
{
    if( !m_pointer->IsCalibratingTip() )
    {
        m_pointer->StartTipCalibration();
        connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(Update()) );
    }
    else
    {
        m_pointer->StopTipCalibration();
        disconnect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(Update()) );
    }
    Update();
}

void PointerCalibrationDialog::CancelButtonClicked()
{
    disconnect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(Update()) );
    m_pointer->CancelTipCalibration();
    this->Update();
}

void PointerCalibrationDialog::Update()
{
    cancelButton->setEnabled( m_pointer->IsCalibratingTip() );
    if( m_pointer->IsCalibratingTip() )
        calibrateButton->setText("Stop");
    else
        calibrateButton->setText("Start");

    double rms = m_pointer->GetTipCalibrationRMSError();
    vtkMatrix4x4 * mat = m_pointer->GetCalibrationMatrix();
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
        if( m_pointer->IsCalibratingTip() )
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
