/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "volumerenderingsinglevolumesettingswidget.h"
#include "ui_volumerenderingsinglevolumesettingswidget.h"
#include "scenemanager.h"
#include "volumerenderingobject.h"
#include <vtkVolumeProperty.h>
#include "imageobject.h"
#include "volumeshadereditorwidget.h"

VolumeRenderingSingleVolumeSettingsWidget::VolumeRenderingSingleVolumeSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VolumeRenderingSingleVolumeSettingsWidget)
{
    m_vr = 0;
    m_volumeIndex = -1;
    ui->setupUi(this);
    m_shaderWidget = 0;
}

VolumeRenderingSingleVolumeSettingsWidget::~VolumeRenderingSingleVolumeSettingsWidget()
{
    if( m_shaderWidget )
        m_shaderWidget->close();
    delete ui;
}

void VolumeRenderingSingleVolumeSettingsWidget::SetVolumeRenderingObject( VolumeRenderingObject * vr, int volumeIndex )
{
    m_vr = vr;
    m_volumeIndex = volumeIndex;
    vtkVolumeProperty * vp = m_vr->GetVolumeProperty( m_volumeIndex );
    ui->colorFunctionWidget->SetColorTransferFunction( vp->GetRGBTransferFunction() );
    ui->colorFunctionWidget->SetXRange( 0.0, 255.0 );
    ui->opacityFunctionWidget->SetFunction( vp->GetScalarOpacity() );
    ui->opacityFunctionWidget->SetYRange( 0.0, 1.0 );
    UpdateUi();
}

void VolumeRenderingSingleVolumeSettingsWidget::UpdateUi()
{
    Q_ASSERT( m_vr );
    Q_ASSERT( m_vr->GetManager() );

    ui->volumeGroupBox->setTitle( QString("Volume %1").arg( m_volumeIndex ) );

    ui->enableVolumeCheckBox->blockSignals( true );
    ui->volumeComboBox->blockSignals( true );

    // Update enable checkbox
    ui->enableVolumeCheckBox->setChecked( m_vr->IsVolumeEnabled( m_volumeIndex ) );

    // Update use 16 bits volume
    ui->use16BitsVolumeCheckBox->blockSignals( true );
    ui->use16BitsVolumeCheckBox->setChecked( m_vr->GetUse16BitsVolume( m_volumeIndex ) );
    ui->use16BitsVolumeCheckBox->blockSignals( false );

    // Update use linear sampling checkbox
    ui->useLinearSamplingCheckBox->blockSignals( true );
    ui->useLinearSamplingCheckBox->setChecked( m_vr->GetUseLinearSampling( m_volumeIndex ) );
    ui->useLinearSamplingCheckBox->blockSignals( false );

    // Update image object combo box
    ui->volumeComboBox->clear();
    QList< ImageObject* > allObjects;
    m_vr->GetManager()->GetAllImageObjects( allObjects );
    ImageObject * currentImage = m_vr->GetImage( m_volumeIndex );
    bool none = true;
    for( int i = 0; i < allObjects.size(); ++i )
    {
        ImageObject * im = allObjects[i];
        ui->volumeComboBox->addItem( im->GetName(), QVariant( im->GetObjectID() ) );
        if( im == currentImage )
        {
            ui->volumeComboBox->setCurrentIndex( i );
            ui->enableVolumeCheckBox->setEnabled( true );
            none = false;
        }
    }
    ui->volumeComboBox->addItem( "None", QVariant((int)-1) );
    if( none )
    {
        ui->volumeComboBox->setCurrentIndex( allObjects.size() );
        ui->enableVolumeCheckBox->setEnabled( false );
    }

    ui->enableVolumeCheckBox->blockSignals( false );
    ui->volumeComboBox->blockSignals( false );

    // Disable interpolation and 16 bit volume checkboxes when there is no valid volume
    ui->use16BitsVolumeCheckBox->blockSignals( true );
    ui->use16BitsVolumeCheckBox->setEnabled( !none );
    ui->use16BitsVolumeCheckBox->blockSignals( false );

    ui->useLinearSamplingCheckBox->blockSignals( true );
    ui->useLinearSamplingCheckBox->setEnabled( !none );
    ui->useLinearSamplingCheckBox->blockSignals( false );

    // Update shader contribution combo box
    ui->contributionTypeComboBox->blockSignals( true );
    ui->contributionTypeComboBox->clear();
    for( int i = 0; i < m_vr->GetNumberOfShaderContributionTypes(); ++i )
    {
        ui->contributionTypeComboBox->addItem( QString( m_vr->GetShaderContributionTypeName( i ) ) );
    }
    ui->contributionTypeComboBox->setCurrentIndex( m_vr->GetShaderContributionType( m_volumeIndex ) );
    ui->contributionTypeComboBox->blockSignals( false );

    // disable remove shader button if shader type is not custom
    int currentShaderType = m_vr->GetShaderContributionType( m_volumeIndex );
    bool shaderIsCustom = m_vr->IsShaderTypeCustom( currentShaderType );
    ui->removeVolumeShaderButton->setEnabled( shaderIsCustom );

    // make sure we repaint transfer func widgets
    ui->colorFunctionWidget->update();
    ui->opacityFunctionWidget->update();
}

void VolumeRenderingSingleVolumeSettingsWidget::on_enableVolumeCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_vr );
    m_vr->EnableVolume( m_volumeIndex, checked );
}

void VolumeRenderingSingleVolumeSettingsWidget::on_volumeComboBox_currentIndexChanged( int index )
{
    Q_ASSERT( m_vr );
    Q_ASSERT( m_vr->GetManager() );
    int objectId = ui->volumeComboBox->itemData( index ).toInt();
    ImageObject * newObject = 0;
    if( objectId != SceneManager::InvalidId )
        newObject =  ImageObject::SafeDownCast( m_vr->GetManager()->GetObjectByID( objectId ) );
    m_vr->SetImage( m_volumeIndex, newObject );
    UpdateUi();
}

void VolumeRenderingSingleVolumeSettingsWidget::on_contributionTypeComboBox_currentIndexChanged(int index)
{
    Q_ASSERT( m_vr );
    m_vr->SetShaderContributionType( m_volumeIndex, index );
}

void VolumeRenderingSingleVolumeSettingsWidget::OnVolumeShaderEditorWidgetClosed()
{
    m_shaderWidget = 0;
    ui->shaderPushButton->blockSignals( true );
    ui->shaderPushButton->setChecked( false );
    ui->shaderPushButton->blockSignals( false );
}

void VolumeRenderingSingleVolumeSettingsWidget::on_shaderPushButton_toggled( bool checked )
{
    Q_ASSERT( m_vr );
    if( checked )
    {
        Q_ASSERT( m_shaderWidget == 0 );
        m_shaderWidget = new VolumeShaderEditorWidget( 0 );
        m_shaderWidget->setAttribute( Qt::WA_DeleteOnClose );
        m_shaderWidget->SetVolumeRenderer( m_vr, m_volumeIndex );
        connect( m_shaderWidget, SIGNAL(destroyed()), this, SLOT(OnVolumeShaderEditorWidgetClosed()) );
        m_shaderWidget->show();
    }
    else
    {
        Q_ASSERT( m_shaderWidget );
        m_shaderWidget->close();
    }
}

void VolumeRenderingSingleVolumeSettingsWidget::on_use16BitsVolumeCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_vr );
    m_vr->SetUse16BitsVolume( m_volumeIndex, checked );
}

void VolumeRenderingSingleVolumeSettingsWidget::on_useLinearSamplingCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_vr );
    m_vr->SetUseLinearSampling( m_volumeIndex, checked );
}

void VolumeRenderingSingleVolumeSettingsWidget::on_addShaderContribTypeButton_clicked()
{
    Q_ASSERT( m_vr );
    int contribType = m_vr->GetShaderContributionType( m_volumeIndex );
    int newShaderType = m_vr->DuplicateShaderContribType( contribType );
    if( newShaderType != -1 )
        m_vr->SetShaderContributionType( m_volumeIndex, newShaderType );
    UpdateUi();
}

void VolumeRenderingSingleVolumeSettingsWidget::on_removeVolumeShaderButton_clicked()
{
    Q_ASSERT( m_vr );
    int contribType = m_vr->GetShaderContributionType( m_volumeIndex );
    m_vr->DeleteShaderContributionType( contribType );
}
