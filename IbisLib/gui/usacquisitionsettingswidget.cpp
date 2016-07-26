/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "usacquisitionsettingswidget.h"
#include "ui_usacquisitionsettingswidget.h"

#include "usacquisitionobject.h"
#include "vtkQtMatrixDialog.h"
#include "vtkTransform.h"
#include "application.h"
#include "lookuptablemanager.h"

UsAcquisitionSettingsWidget::UsAcquisitionSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UsAcquisitionSettingsWidget)
{
    m_acquisitionObject = 0;
    m_calibrationMatrixWidget = 0;
    ui->setupUi(this);
    connect( this, SIGNAL(destroyed()), this, SLOT(OnClose()) );
}

UsAcquisitionSettingsWidget::~UsAcquisitionSettingsWidget()
{
    if( m_calibrationMatrixWidget )
        m_calibrationMatrixWidget->close();
    if( m_acquisitionObject )
        m_acquisitionObject->UnRegister(0);
    delete ui;
}

void UsAcquisitionSettingsWidget::SetUSAcquisitionObject( USAcquisitionObject * acq )
{
    if( m_acquisitionObject == acq )
        return;

    if (m_acquisitionObject)
    {
        m_acquisitionObject->UnRegister(0);
        disconnect( m_acquisitionObject, SIGNAL(Modified()), this, SLOT(UpdateUi()) );
    }
    m_acquisitionObject = acq;
    if (m_acquisitionObject)
    {
        if (m_acquisitionObject->GetNumberOfSlices() > 0)
            m_acquisitionObject->SetNumberOfStaticSlices(m_acquisitionObject->GetNumberOfStaticSlices());
        m_acquisitionObject->Register(0);
        connect( m_acquisitionObject, SIGNAL(Modified()), this, SLOT(UpdateUi()) );
        UpdateUi();
    }
}

void UsAcquisitionSettingsWidget::UpdateUi()
{
    Q_ASSERT( m_acquisitionObject );

    // Fill text box with properties
    QString acquisitionPropString( "" );
    acquisitionPropString += QString( "Acquisition type: %1 \n" ).arg( m_acquisitionObject->GetAcquisitionTypeAsString() );
    acquisitionPropString += QString( "Pixel Format: %1 \n" ).arg( m_acquisitionObject->GetAcquisitionColor() );
    acquisitionPropString += QString( "Frame Size: %1 x %2 \n" ).arg( m_acquisitionObject->GetSliceWidth() ).arg( m_acquisitionObject->GetSliceHeight() );
    acquisitionPropString += QString( "Probe Depth: %1 \n" ).arg( m_acquisitionObject->GetUsDepth() );
    acquisitionPropString += QString( "Number of frames: %1\n" ).arg( m_acquisitionObject->GetNumberOfSlices() );
    ui->acquisitionPropertiesTextEdit->setPlainText( acquisitionPropString );

    // Update Slice viewer info
    ui->sliceSlider->blockSignals( true );
    ui->sliceSlider->setRange( 0, m_acquisitionObject->GetNumberOfSlices() - 1 );
    ui->sliceSlider->setValue( m_acquisitionObject->GetCurrentSlice() );
    ui->sliceSlider->blockSignals( false );

    ui->sliceSpinBox->blockSignals( true );
    ui->sliceSpinBox->setRange( 0, m_acquisitionObject->GetNumberOfSlices() - 1 );
    ui->sliceSpinBox->setValue( m_acquisitionObject->GetCurrentSlice() );
    ui->sliceSpinBox->blockSignals( false );

    ui->opacitySlider->blockSignals( true );
    ui->opacitySlider->setValue( (int)( 100 * m_acquisitionObject->GetSliceImageOpacity() ) );
    ui->opacitySlider->blockSignals( false );

    ui->useDopplerCheckBox->blockSignals( true );
    ui->useDopplerCheckBox->setChecked(m_acquisitionObject->IsUsingDoppler());
    ui->useDopplerCheckBox->blockSignals( false );


    ui->currentSliceColorComboBox->blockSignals( true ); // change to toggle between modes Mar 2, 2016, by Xiao
    ui->currentSliceColorComboBox->setEnabled(!m_acquisitionObject->IsUsingDoppler());
    ui->currentSliceColorComboBox->clear();

    for( int i = 0; i < Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables(); i++ )
        ui->currentSliceColorComboBox->addItem( Application::GetLookupTableManager()->GetTemplateLookupTableName(i) );
    ui->currentSliceColorComboBox->setCurrentIndex( m_acquisitionObject->GetSliceLutIndex() );
    ui->currentSliceColorComboBox->blockSignals( false );

    ui->staticSlicesGroupBox->blockSignals( true );
    ui->staticSlicesGroupBox->setChecked( m_acquisitionObject->IsStaticSlicesEnabled() );
    ui->staticSlicesGroupBox->blockSignals( false );

    ui->nbStaticSlicesSpinBox->blockSignals( true );
    ui->nbStaticSlicesSpinBox->setRange( 2, m_acquisitionObject->GetNumberOfSlices() );
    ui->nbStaticSlicesSpinBox->setValue( m_acquisitionObject->GetNumberOfStaticSlices() );
    ui->nbStaticSlicesSpinBox->blockSignals( false );

    ui->staticSlicesOpacitySlider->blockSignals( true );
    ui->staticSlicesOpacitySlider->setValue( (int)( 100 * m_acquisitionObject->GetStaticSlicesOpacity() ) );
    ui->staticSlicesOpacitySlider->blockSignals( false );

    ui->staticSlicesColorComboBox->blockSignals( true );
    ui->staticSlicesColorComboBox->clear();
    for( int i = 0; i < Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables(); i++ )
        ui->staticSlicesColorComboBox->addItem( Application::GetLookupTableManager()->GetTemplateLookupTableName(i) );
    ui->staticSlicesColorComboBox->setCurrentIndex( m_acquisitionObject->GetStaticSlicesLutIndex() );
    ui->staticSlicesColorComboBox->blockSignals( false );

    ui->useMaskCheckBox->blockSignals( true );
    ui->useMaskCheckBox->setChecked( m_acquisitionObject->IsUsingMask() );
    ui->useMaskCheckBox->blockSignals( false );

    ui->useDopplerCheckBox->blockSignals( true ); // added Mar 3, 2016, Xiao
    ui->useDopplerCheckBox->setChecked( m_acquisitionObject->IsUsingDoppler() );
    ui->useDopplerCheckBox->blockSignals( false );
}

void UsAcquisitionSettingsWidget::OnCalibrationMatrixWidgetClosed()
{
    m_calibrationMatrixWidget = 0;
    ui->calibrationMatrixButton->blockSignals( true );
    ui->calibrationMatrixButton->setChecked( false );
    ui->calibrationMatrixButton->blockSignals( false );
}

void UsAcquisitionSettingsWidget::on_sliceSlider_valueChanged( int value )
{
    Q_ASSERT( m_acquisitionObject );
    m_acquisitionObject->SetCurrentFrame( value );
}

void UsAcquisitionSettingsWidget::on_sliceSpinBox_valueChanged( int value )
{
    Q_ASSERT( m_acquisitionObject );
    m_acquisitionObject->SetCurrentFrame( value );
}

void UsAcquisitionSettingsWidget::on_opacitySlider_valueChanged(int value)
{
    Q_ASSERT( m_acquisitionObject );
    m_acquisitionObject->SetSliceImageOpacity( (double)value * 0.01 );
}

void UsAcquisitionSettingsWidget::on_currentSliceColorComboBox_currentIndexChanged( int index )
{
    Q_ASSERT( m_acquisitionObject );
    m_acquisitionObject->SetSliceLutIndex( index );
}

void UsAcquisitionSettingsWidget::on_staticSlicesGroupBox_toggled( bool on )
{
    Q_ASSERT( m_acquisitionObject );
    m_acquisitionObject->SetEnableStaticSlices( on );
}

void UsAcquisitionSettingsWidget::on_nbStaticSlicesSpinBox_valueChanged( int nbSlices )
{
    m_acquisitionObject->SetNumberOfStaticSlices( nbSlices );
}

void UsAcquisitionSettingsWidget::on_staticSlicesOpacitySlider_valueChanged(int value)
{
    Q_ASSERT( m_acquisitionObject );
    m_acquisitionObject->SetStaticSlicesOpacity( (double)value * 0.01 );
}

void UsAcquisitionSettingsWidget::on_staticSlicesColorComboBox_currentIndexChanged( int index )
{
    Q_ASSERT( m_acquisitionObject );
    m_acquisitionObject->SetStaticSlicesLutIndex( index );
}

void UsAcquisitionSettingsWidget::on_useMaskCheckBox_toggled(bool checked)
{
    m_acquisitionObject->SetUseMask( checked );
}

//added Mar 1, 2016, Xiao
void UsAcquisitionSettingsWidget::on_useDopplerCheckBox_toggled(bool checked)
{
    m_acquisitionObject->SetUseDoppler( checked );
}


void UsAcquisitionSettingsWidget::on_calibrationMatrixButton_toggled( bool checked )
{
    if( checked )
    {
        Q_ASSERT( !m_calibrationMatrixWidget );
        m_calibrationMatrixWidget = new vtkQtMatrixDialog( true, 0 );
        m_calibrationMatrixWidget->setAttribute( Qt::WA_DeleteOnClose );
        m_calibrationMatrixWidget->SetMatrix( m_acquisitionObject->GetCalibrationTransform()->GetMatrix() );
        m_calibrationMatrixWidget->setWindowTitle( m_acquisitionObject->GetName() + QString(" Calibration Matrix") );
        connect( m_calibrationMatrixWidget, SIGNAL(destroyed()), this, SLOT(OnCalibrationMatrixWidgetClosed()) );
        m_calibrationMatrixWidget->show();
    }
    else
    {
        Q_ASSERT( m_calibrationMatrixWidget );
        m_calibrationMatrixWidget->close();
        m_calibrationMatrixWidget = 0;
    }
}

void UsAcquisitionSettingsWidget::OnClose()
{
    if( m_calibrationMatrixWidget )
        m_calibrationMatrixWidget->close();
    m_calibrationMatrixWidget = 0;
}
