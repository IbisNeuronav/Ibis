/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "imageobjectvolumesettingswidget.h"
#include "ui_imageobjectvolumesettingswidget.h"
#include "imageobject.h"
#include "vtkVolumeProperty.h"
#include "vtkSmartVolumeMapper.h"

ImageObjectVolumeSettingsWidget::ImageObjectVolumeSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageObjectVolumeSettingsWidget)
{
    m_imageObject = 0;
    ui->setupUi(this);
}

ImageObjectVolumeSettingsWidget::~ImageObjectVolumeSettingsWidget()
{
    delete ui;
}

void ImageObjectVolumeSettingsWidget::SetImageObject( ImageObject * img )
{
    m_imageObject = img;
    Q_ASSERT( m_imageObject );

    vtkVolumeProperty * prop = m_imageObject->GetVolumeProperty();
    ui->scalarOpacityFunctionWidget->SetFunction( prop->GetScalarOpacity() );
    ui->scalarOpacityFunctionWidget->SetYRange( 0.0, 1.0 );
    ui->gradientOpacityFunctionWidget->SetFunction( prop->GetGradientOpacity() );
    ui->gradientOpacityFunctionWidget->SetYRange( 0.0, 1.0 );
    ui->colorFunctionWidget->SetColorTransferFunction( prop->GetRGBTransferFunction() );
    ui->colorFunctionWidget->SetXRange( 0.0, 255.0 );

    UpdateUi();
}

void ImageObjectVolumeSettingsWidget::UpdateUi()
{
    Q_ASSERT( m_imageObject );

    ui->enableVolumeRenderingCheckBox->blockSignals( true );
    ui->enableVolumeRenderingCheckBox->setChecked( m_imageObject->GetVtkVolumeRenderingEnabled() );
    ui->enableVolumeRenderingCheckBox->blockSignals( false );

    ui->renderModeComboBox->blockSignals( true );
    ui->renderModeComboBox->clear();
    ui->renderModeComboBox->addItem( "GPU Raycast", QVariant( (int)vtkSmartVolumeMapper::GPURenderMode ) );
    if( m_imageObject->GetVtkVolumeRenderMode() == (int)vtkSmartVolumeMapper::GPURenderMode )
        ui->renderModeComboBox->setCurrentIndex( 0 );
    ui->renderModeComboBox->addItem( "3D Texture", QVariant( (int)vtkSmartVolumeMapper::TextureRenderMode ) );
    if( m_imageObject->GetVtkVolumeRenderMode() == (int)vtkSmartVolumeMapper::TextureRenderMode )
        ui->renderModeComboBox->setCurrentIndex( 1 );
    ui->renderModeComboBox->addItem( "3D Texture + Raycast", QVariant( (int)vtkSmartVolumeMapper::RayCastAndTextureRenderMode ) );
    if( m_imageObject->GetVtkVolumeRenderMode() == (int)vtkSmartVolumeMapper::RayCastAndTextureRenderMode )
        ui->renderModeComboBox->setCurrentIndex( 2 );
    ui->renderModeComboBox->blockSignals( false );

    ui->levelSpinBox->blockSignals( true );
    ui->levelSpinBox->setValue( m_imageObject->GetVolumeRenderingLevel() );
    ui->levelSpinBox->blockSignals( false );

    ui->windowSpinBox->blockSignals( true );
    ui->windowSpinBox->setValue( m_imageObject->GetVolumeRenderingWindow() );
    ui->windowSpinBox->blockSignals( false );

    ui->autoSampleDistanceCheckBox->blockSignals( true );
    ui->autoSampleDistanceCheckBox->setChecked( m_imageObject->GetAutoSampleDistance() );
    ui->autoSampleDistanceCheckBox->blockSignals( false );

    ui->sampleDistanceSpinBox->blockSignals( true );
    ui->sampleDistanceSpinBox->setValue( m_imageObject->GetSampleDistance() );
    ui->sampleDistanceSpinBox->blockSignals( false );

    vtkVolumeProperty * prop = m_imageObject->GetVolumeProperty();

    ui->shadingCheckBox->blockSignals( true );
    ui->shadingCheckBox->setChecked( prop->GetShade() == 1 ? true : false );
    ui->shadingCheckBox->blockSignals( false );

    ui->ambiantSpinBox->blockSignals( true );
    ui->ambiantSpinBox->setValue( prop->GetAmbient() );
    ui->ambiantSpinBox->blockSignals( false );

    ui->diffuseSpinBox->blockSignals( true );
    ui->diffuseSpinBox->setValue( prop->GetDiffuse() );
    ui->diffuseSpinBox->blockSignals( false );

    ui->specularSpinBox->blockSignals( true );
    ui->specularSpinBox->setValue( prop->GetSpecular() );
    ui->specularSpinBox->blockSignals( false );

    ui->specularPowerSpinBox->blockSignals( true );
    ui->specularPowerSpinBox->setValue( prop->GetSpecularPower() );
    ui->specularPowerSpinBox->blockSignals( false );

    ui->gradientOpacityCheckBox->blockSignals( true );
    ui->gradientOpacityCheckBox->setChecked( prop->GetDisableGradientOpacity() == 1 ? false : true );
    ui->gradientOpacityCheckBox->blockSignals( false );

    // Hide control that are useless in certain render mode
    if( m_imageObject->GetVtkVolumeRenderMode() == (int)vtkSmartVolumeMapper::GPURenderMode )
    {
        ui->windowLevelFrame->setHidden( false );
        ui->shadingParamsFrame->setHidden( true );
        ui->gradientOpacityCheckBox->setHidden( true );
        ui->gradientOpacityFunctionWidget->setHidden( true );
    }
    else if( m_imageObject->GetVtkVolumeRenderMode() == (int)vtkSmartVolumeMapper::TextureRenderMode )
    {
        ui->windowLevelFrame->setHidden( true );
        ui->shadingParamsFrame->setHidden( false );
        ui->gradientOpacityCheckBox->setHidden( false );
        ui->gradientOpacityFunctionWidget->setHidden( false );
    }
    else if( m_imageObject->GetVtkVolumeRenderMode() == (int)vtkSmartVolumeMapper::RayCastAndTextureRenderMode )
    {
        ui->windowLevelFrame->setHidden( true );
        ui->shadingParamsFrame->setHidden( false );
        ui->gradientOpacityCheckBox->setHidden( false );
        ui->gradientOpacityFunctionWidget->setHidden( false );
    }
}

void ImageObjectVolumeSettingsWidget::on_enableVolumeRenderingCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->SetVtkVolumeRenderingEnabled( checked );
}

void ImageObjectVolumeSettingsWidget::on_shadingCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_imageObject );
    m_imageObject->GetVolumeProperty()->SetShade( checked ? 1 : 0 );
}

void ImageObjectVolumeSettingsWidget::on_ambiantSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->GetVolumeProperty()->SetAmbient( val );
}

void ImageObjectVolumeSettingsWidget::on_diffuseSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->GetVolumeProperty()->SetDiffuse( val );
}

void ImageObjectVolumeSettingsWidget::on_specularSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->GetVolumeProperty()->SetSpecular( val );
}

void ImageObjectVolumeSettingsWidget::on_specularPowerSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->GetVolumeProperty()->SetSpecularPower( val );
}

void ImageObjectVolumeSettingsWidget::on_gradientOpacityCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_imageObject );
    m_imageObject->GetVolumeProperty()->SetDisableGradientOpacity( checked ? 0 : 1 );
}

void ImageObjectVolumeSettingsWidget::on_renderModeComboBox_currentIndexChanged( int index )
{
    Q_ASSERT( m_imageObject );
    int renderMode = ui->renderModeComboBox->itemData( index ).toInt();
    m_imageObject->SetVtkVolumeRenderMode( renderMode );
    UpdateUi();
}

void ImageObjectVolumeSettingsWidget::on_levelSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->SetVolumeRenderingLevel( val );
}

void ImageObjectVolumeSettingsWidget::on_windowSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->SetVolumeRenderingWindow( val );
}

void ImageObjectVolumeSettingsWidget::on_autoSampleDistanceCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_imageObject );
    m_imageObject->SetAutoSampleDistance( checked );
}

void ImageObjectVolumeSettingsWidget::on_sampleDistanceSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_imageObject );
    m_imageObject->SetSampleDistance( val );
}
