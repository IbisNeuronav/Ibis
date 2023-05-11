/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "triplecutplaneobjectmixerwidget.h"

#include "application.h"
#include "imageobject.h"
#include "lookuptablemanager.h"
#include "triplecutplaneobject.h"
#include "ui_triplecutplaneobjectmixerwidget.h"

TripleCutPlaneObjectMixerWidget::TripleCutPlaneObjectMixerWidget( QWidget * parent )
    : QWidget( parent ), ui( new Ui::TripleCutPlaneObjectMixerWidget )
{
    ui->setupUi( this );
}

TripleCutPlaneObjectMixerWidget::~TripleCutPlaneObjectMixerWidget() { delete ui; }

void TripleCutPlaneObjectMixerWidget::SetTripleCutPlaneObject( TripleCutPlaneObject * obj, int imageIndex )
{
    m_cutPlaneObject = obj;
    m_imageIndex     = imageIndex;
    ui->mainGroupBox->setTitle( m_cutPlaneObject->GetImage( imageIndex )->GetName() );
    UpdateUi();
}

void TripleCutPlaneObjectMixerWidget::on_colorTableComboBox_currentIndexChanged( int index )
{
    m_cutPlaneObject->SetColorTable( m_imageIndex, index );
}

void TripleCutPlaneObjectMixerWidget::on_blendModeComboBox_currentIndexChanged( int index )
{
    m_cutPlaneObject->SetBlendingModeIndex( m_imageIndex, index );
}

void TripleCutPlaneObjectMixerWidget::on_sliceMixComboBox_currentIndexChanged( int index )
{
    m_cutPlaneObject->SetSliceMixMode( m_imageIndex, index );
}

void TripleCutPlaneObjectMixerWidget::on_imageIntensitySlider_valueChanged( int value )
{
    m_cutPlaneObject->SetImageIntensityFactor( m_imageIndex, double( value ) / 100.0 );
}

void TripleCutPlaneObjectMixerWidget::UpdateUi()
{
    if( !m_cutPlaneObject ) return;

    // update color table combo box
    ui->colorTableComboBox->blockSignals( true );
    ui->colorTableComboBox->clear();
    for( int i = 0; i < Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables(); i++ )
        ui->colorTableComboBox->addItem( Application::GetLookupTableManager()->GetTemplateLookupTableName( i ) );
    ui->colorTableComboBox->setCurrentIndex( m_cutPlaneObject->GetColorTable( m_imageIndex ) );
    ui->colorTableComboBox->blockSignals( false );

    // Update blending mode combo box
    ui->blendModeComboBox->blockSignals( true );
    ui->blendModeComboBox->clear();
    for( int i = 0; i < m_cutPlaneObject->GetNumberOfBlendingModes(); ++i )
        ui->blendModeComboBox->addItem( m_cutPlaneObject->GetBlendingModeName( i ) );
    ui->blendModeComboBox->setCurrentIndex( m_cutPlaneObject->GetBlendingModeIndex( m_imageIndex ) );
    ui->blendModeComboBox->blockSignals( false );

    // update slice mix combo box
    ui->sliceMixComboBox->blockSignals( true );
    ui->sliceMixComboBox->clear();
    ui->sliceMixComboBox->addItem( "Min" );
    ui->sliceMixComboBox->addItem( "Max" );
    ui->sliceMixComboBox->addItem( "Mean" );
    ui->sliceMixComboBox->addItem( "Sum" );
    ui->sliceMixComboBox->setCurrentIndex( m_cutPlaneObject->GetSliceMixMode( m_imageIndex ) );
    ui->sliceMixComboBox->blockSignals( false );

    // update image intensity factor
    ui->imageIntensitySlider->setValue( (int)( m_cutPlaneObject->GetImageIntensityFactor( m_imageIndex ) * 100 ) );
}
