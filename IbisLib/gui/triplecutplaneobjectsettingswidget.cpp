/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "triplecutplaneobjectsettingswidget.h"
#include "ui_triplecutplaneobjectsettingswidget.h"
#include "triplecutplaneobjectmixerwidget.h"
#include "triplecutplaneobject.h"
#include "scenemanager.h"
#include "imageobject.h"
#include <QVBoxLayout>
#include <QComboBox>

TripleCutPlaneObjectSettingsWidget::TripleCutPlaneObjectSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TripleCutPlaneObjectSettingsWidget)
{
    m_cutPlaneObject = 0;
    ui->setupUi(this);
    m_volumeContributionLayout = new QVBoxLayout( ui->volumeContributionGroupBox );
}

TripleCutPlaneObjectSettingsWidget::~TripleCutPlaneObjectSettingsWidget()
{
    delete ui;
}

void TripleCutPlaneObjectSettingsWidget::SetTripleCutPlaneObject( TripleCutPlaneObject * obj )
{
    m_cutPlaneObject = obj;
    connect( m_cutPlaneObject->GetManager(), SIGNAL(ObjectAdded( int )), this, SLOT(UpdateImageSliders()) );
    connect( m_cutPlaneObject->GetManager(), SIGNAL(ObjectRemoved( int )), this, SLOT(UpdateImageSliders()) );
    UpdateUI();
    UpdateImageSliders();
}

void TripleCutPlaneObjectSettingsWidget::UpdateImageSliders()
{
    Q_ASSERT( m_cutPlaneObject );

    // Clear previous widgets
    QLayoutItem * item;
    while( ( item = m_volumeContributionLayout->layout()->takeAt( 0 ) ) != NULL )
    {
        delete item->widget();
        delete item;
    }

    // Add new widgets
    for( int i = 0; i < m_cutPlaneObject->GetNumberOfImages(); ++i )
    {
        TripleCutPlaneObjectMixerWidget * w = new TripleCutPlaneObjectMixerWidget( ui->volumeContributionGroupBox );
        w->SetTripleCutPlaneObject( m_cutPlaneObject, i );
        m_volumeContributionLayout->addWidget( w );
    }

    m_volumeContributionLayout->addStretch();
}

void TripleCutPlaneObjectSettingsWidget::UpdateUI()
{
    Q_ASSERT( m_cutPlaneObject );

    ui->sagitalPlaneCheckBox->blockSignals( true );
    ui->sagitalPlaneCheckBox->setChecked( m_cutPlaneObject->GetViewPlane( 0 ) == 0 ? false : true );
    ui->sagitalPlaneCheckBox->blockSignals( false );
    ui->coronalPlaneCheckBox->blockSignals( true );
    ui->coronalPlaneCheckBox->setChecked( m_cutPlaneObject->GetViewPlane( 1 ) == 0 ? false : true );
    ui->coronalPlaneCheckBox->blockSignals( false );
    ui->transversePlaneCheckBox->blockSignals( true );
    ui->transversePlaneCheckBox->setChecked( m_cutPlaneObject->GetViewPlane( 2 ) == 0 ? false : true );
    ui->transversePlaneCheckBox->blockSignals( false );

    ui->resliceInterpolationComboBox->blockSignals( true );
    ui->resliceInterpolationComboBox->setCurrentIndex( m_cutPlaneObject->GetResliceInterpolationType() );
    ui->resliceInterpolationComboBox->blockSignals( false );

    ui->displayInterpolationComboBox->blockSignals( true );
    ui->displayInterpolationComboBox->setCurrentIndex( m_cutPlaneObject->GetDisplayInterpolationType() );
    ui->displayInterpolationComboBox->blockSignals( false );

    ui->sliceThicknessSpinBox->blockSignals( true );
    ui->sliceThicknessSpinBox->setValue( m_cutPlaneObject->GetSliceThickness() );
    ui->sliceThicknessSpinBox->blockSignals( false );
}

void TripleCutPlaneObjectSettingsWidget::on_sagitalPlaneCheckBox_toggled(bool checked)
{
    m_cutPlaneObject->SetViewPlane( 0, checked ? 1 : 0 );
}

void TripleCutPlaneObjectSettingsWidget::on_coronalPlaneCheckBox_toggled(bool checked)
{
    m_cutPlaneObject->SetViewPlane( 1, checked ? 1 : 0 );
}

void TripleCutPlaneObjectSettingsWidget::on_transversePlaneCheckBox_toggled(bool checked)
{
    m_cutPlaneObject->SetViewPlane( 2, checked ? 1 : 0 );
}

void TripleCutPlaneObjectSettingsWidget::on_viewAllPlanesButton_clicked()
{
    m_cutPlaneObject->SetViewAllPlanes( 1 );
    UpdateUI();
}

void TripleCutPlaneObjectSettingsWidget::on_viewNoPlaneButton_clicked()
{
    m_cutPlaneObject->SetViewAllPlanes( 0 );
    UpdateUI();
}

void TripleCutPlaneObjectSettingsWidget::on_resliceInterpolationComboBox_currentIndexChanged( int index )
{
    m_cutPlaneObject->SetResliceInterpolationType( index );
}

void TripleCutPlaneObjectSettingsWidget::on_displayInterpolationComboBox_currentIndexChanged( int index )
{
    m_cutPlaneObject->SetDisplayInterpolationType( index );
}

void TripleCutPlaneObjectSettingsWidget::on_sliceThicknessSpinBox_valueChanged( int val )
{
    m_cutPlaneObject->SetSliceThickness( val );
}
