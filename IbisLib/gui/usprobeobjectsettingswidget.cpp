/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "usprobeobjectsettingswidget.h"
#include "ui_usprobeobjectsettingswidget.h"
#include "usprobeobject.h"
#include "vtkQtMatrixDialog.h"
#include "vtkTransform.h"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLayout>

UsProbeObjectSettingsWidget::UsProbeObjectSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UsProbeObjectSettingsWidget)
{
    ui->setupUi(this);

    QPalette palette;
    QColor backColor( 255, 0, 0 );
    palette.setColor( ui->probeStatusLabel->backgroundRole(), backColor );
    ui->probeStatusLabel->setPalette( palette );
    ui->probeStatusLabel->setFrameShape( QLabel::Box );
    ui->probeStatusLabel->setAlignment( Qt::AlignCenter );
    ui->probeStatusLabel->setText("");

    m_usProbeObject = 0;
    m_matrixDialog = 0;
}

UsProbeObjectSettingsWidget::~UsProbeObjectSettingsWidget()
{
    delete ui;
    if( m_matrixDialog )
    {
        m_matrixDialog->close();
        m_matrixDialog = 0;
    }
}

void UsProbeObjectSettingsWidget::SetUsProbeObject( UsProbeObject * probeObject )
{
    Q_ASSERT( probeObject );
    m_usProbeObject = probeObject;

    connect( m_usProbeObject, SIGNAL( Modified() ), this, SLOT( UpdateToolStatus() ) );

    this->UpdateToolStatus();
    this->UpdateDepth();
    this->UpdateUI();
}

void UsProbeObjectSettingsWidget::DepthComboBoxSelectionChanged( int newSelection )
{
    Q_ASSERT( m_usProbeObject );
    m_usProbeObject->SetCurrentCalibrationMatrixIndex( newSelection );
}

void UsProbeObjectSettingsWidget::UpdateToolStatus()
{
    Q_ASSERT( m_usProbeObject );

    TrackerToolState newState = m_usProbeObject->GetState();
    switch( newState )
    {
    case Ok:
        ui->probeStatusLabel->setText( "OK" );
        ui->probeStatusLabel->setStyleSheet( "background-color: lightGreen" );
        break;
    case Missing:
        ui->probeStatusLabel->setText( "Missing" );
        ui->probeStatusLabel->setStyleSheet( "background-color: red" );
        break;
    case OutOfVolume:
        ui->probeStatusLabel->setText( "Out of volume" );
        ui->probeStatusLabel->setStyleSheet( "background-color: yellow" );
        break;
    case OutOfView:
        ui->probeStatusLabel->setText( "Out of view" );
        ui->probeStatusLabel->setStyleSheet( "background-color: red" );
        break;
    case Undefined:
        ui->probeStatusLabel->setText( "Ultrasound probe not initialized" );
        ui->probeStatusLabel->setStyleSheet( "background-color: red" );
        break;
    }
}

void UsProbeObjectSettingsWidget::UpdateDepth()
{
    Q_ASSERT( m_usProbeObject );
    ui->depthComboBox->blockSignals(true);
    ui->depthComboBox->clear();

    int currentIndex = -1;
    QString currentScaleFactor = m_usProbeObject->GetCurrentCalibrationMatrixName();
    int numberOfScaleFactors = m_usProbeObject->GetNumberOfCalibrationMatrices();
    for( int i = 0; i < numberOfScaleFactors; ++i )
    {
        QString name = m_usProbeObject->GetCalibrationMatrixName( i );
        ui->depthComboBox->addItem( name );
        if( name == currentScaleFactor )
        {
            currentIndex = i;
        }
    }
    if( currentIndex != -1 )
    {
        ui->depthComboBox->setCurrentIndex( currentIndex );
    }

    ui->depthComboBox->blockSignals(false);
}

void UsProbeObjectSettingsWidget::BModeRadioButtonClicked()
{
    m_usProbeObject->SetAcquisitionType( UsProbeObject::ACQ_B_MODE );
    UpdateUI();
}

void UsProbeObjectSettingsWidget::ColorDopplerRadioButtonClicked()
{
    m_usProbeObject->SetAcquisitionType( UsProbeObject::ACQ_DOPPLER );
    UpdateUI();
}

void UsProbeObjectSettingsWidget::PowerDopplerRadioButtonClicked()
{
    m_usProbeObject->SetAcquisitionType( UsProbeObject::ACQ_POWER_DOPPLER );
    UpdateUI();
}

void UsProbeObjectSettingsWidget::UpdateUI()
{
    ui->useMaskCheckBox->blockSignals( true );
    ui->useMaskCheckBox->setChecked( m_usProbeObject->GetUseMask() );
    ui->useMaskCheckBox->blockSignals( false );

    ui->bModeRadioButton->blockSignals( true );
    ui->colorDopplerRadioButton->blockSignals( true );
    ui->powerDopplerRadioButton->blockSignals( true );
    ui->colorMapComboBox->blockSignals( true );
    if( m_usProbeObject->GetAcquisitionType() == UsProbeObject::ACQ_B_MODE )
    {
       ui->colorMapGroupBox->setHidden( false );
       ui->colorMapComboBox->clear();
       for( int i = 0; i < m_usProbeObject->GetNumberOfAvailableLUT(); ++i )
       {
           ui->colorMapComboBox->addItem( m_usProbeObject->GetLUTName( i ) );
       }
       ui->colorMapComboBox->setCurrentIndex( m_usProbeObject->GetCurrentLUTIndex() );
    }
    else
    {
        ui->colorMapGroupBox->setHidden( true );
        if( m_usProbeObject->GetAcquisitionType() == UsProbeObject::ACQ_DOPPLER )
        {
            ui->colorDopplerRadioButton->setChecked( true );
        }
        else // ACQ_POWER_DOPPLER
        {
            ui->powerDopplerRadioButton->setChecked( true );
        }
    }
    ui->bModeRadioButton->blockSignals( false );
    ui->colorDopplerRadioButton->blockSignals( false );
    ui->powerDopplerRadioButton->blockSignals( false );
    ui->colorMapComboBox->blockSignals( false );
}

void UsProbeObjectSettingsWidget::on_useMaskCheckBox_toggled( bool checked )
{
    m_usProbeObject->SetUseMask( checked );
}

void UsProbeObjectSettingsWidget::on_colorMapComboBox_currentIndexChanged(int index)
{
    m_usProbeObject->SetCurrentLUTIndex( index );
}

void UsProbeObjectSettingsWidget::on_calibrationMatrixPushButton_toggled( bool on )
{
    if( on )
    {
        Q_ASSERT_X( m_usProbeObject, "TransformEditWidget::on_calibrationMatrixPushButton_toggled", "Can't call this function without setting UsProbeObject." );
        Q_ASSERT( !m_matrixDialog );

        QString dialogTitle = m_usProbeObject->GetName();
        dialogTitle += ": US Probe Calibration Matrix";

        m_matrixDialog = new vtkQtMatrixDialog( true, 0 );
        m_matrixDialog->setWindowTitle( dialogTitle );
        m_matrixDialog->setAttribute( Qt::WA_DeleteOnClose );
        m_matrixDialog->SetTransform( m_usProbeObject->GetCalibrationTransform() );
        m_matrixDialog->show();
        connect( m_matrixDialog, SIGNAL(destroyed()), this, SLOT(OnCalibrationMatrixDialogClosed()) );
    }
    else
    {
        if( m_matrixDialog )
        {
            m_matrixDialog->close();
            m_matrixDialog = 0;
        }
    }
}

void UsProbeObjectSettingsWidget::OnCalibrationMatrixDialogClosed()
{
    m_matrixDialog = 0;
    ui->calibrationMatrixPushButton->setChecked( false );
}
