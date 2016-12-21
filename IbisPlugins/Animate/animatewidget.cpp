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

#include "animatewidget.h"
#include "ui_animatewidget.h"
#include "animateplugininterface.h"

static const int minDomeAngle = 180;
static const int maxDomeAngle = 270;

AnimateWidget::AnimateWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AnimateWidget)
{
    ui->setupUi(this);
    ui->domeViewAngleSlider->blockSignals( true );
    ui->domeViewAngleSlider->setRange( minDomeAngle, maxDomeAngle );
    ui->domeViewAngleSlider->blockSignals( false );
    ui->domeViewAngleSpinBox->blockSignals( true );
    ui->domeViewAngleSpinBox->setRange( minDomeAngle, maxDomeAngle );
    ui->domeViewAngleSpinBox->blockSignals( false );
}

AnimateWidget::~AnimateWidget()
{
    delete ui;
}

void AnimateWidget::SetPluginInterface( AnimatePluginInterface * pi )
{
    m_pluginInterface = pi;
    connect( m_pluginInterface, SIGNAL(CurrentFrameChanged()), this, SLOT(UpdateUi()) );
    UpdateUi();
}

void AnimateWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );

    // Cube Texture size
    ui->cubeTextureSizeSlider->blockSignals( true );
    ui->cubeTextureSizeSlider->setValue( (int)m_pluginInterface->GetDomeCubeTextureSize() );
    ui->cubeTextureSizeSlider->blockSignals( false );

    ui->cubeTextureSizeSpinBox->blockSignals( true );
    ui->cubeTextureSizeSpinBox->setValue( (int)m_pluginInterface->GetDomeCubeTextureSize() );
    ui->cubeTextureSizeSpinBox->blockSignals( false );

    ui->domeModeButton->blockSignals( true );
    ui->domeModeButton->setChecked( m_pluginInterface->GetRenderDome() );
    ui->domeModeButton->blockSignals( false );

    // Dome view angle
    ui->domeViewAngleSlider->blockSignals( true );
    ui->domeViewAngleSlider->setValue( (int)m_pluginInterface->GetDomeViewAngle() );
    ui->domeViewAngleSlider->blockSignals( false );

    ui->domeViewAngleSpinBox->blockSignals( true );
    ui->domeViewAngleSpinBox->setValue( (int)m_pluginInterface->GetDomeViewAngle() );
    ui->domeViewAngleSpinBox->blockSignals( false );

    // Render
    int size = m_pluginInterface->GetRenderSize()[0];
    ui->renderSizeButtonGroup->blockSignals( true );
    if( size == 1920 )
        ui->renderHDRadioButton->setChecked( true );
    else if( size == 1024 )
        ui->render1KradioButton->setChecked( true );
    else if( size == 2048 )
        ui->render2KRadioButton->setChecked( true );
    else if( size == 4096 )
        ui->render4KradioButton->setChecked( true );
    ui->renderSizeButtonGroup->blockSignals( false );

    // Camera min distance
    ui->cameraMinDistanceGroupBox->blockSignals( true );
    ui->cameraMinDistanceGroupBox->setChecked( m_pluginInterface->IsUsingMinCamDistance() );
    ui->cameraMinDistanceGroupBox->blockSignals( false );

    ui->cameraMinDistanceSpinBox->blockSignals( true );
    ui->cameraMinDistanceSpinBox->setValue( m_pluginInterface->GetMinCamDistance() );
    ui->cameraMinDistanceSpinBox->blockSignals( false );

    // Number of frames
    ui->nbFramesSpinBox->blockSignals( true );
    ui->nbFramesSpinBox->setValue( m_pluginInterface->GetNumberOfFrames() );
    ui->nbFramesSpinBox->blockSignals( false );

    // Camera keyframe
    ui->cameraKeyCheckBox->blockSignals( true );
    ui->cameraKeyCheckBox->setChecked( m_pluginInterface->HasCameraKey() );
    ui->cameraKeyCheckBox->blockSignals( false );

    // Tf keyframe
    ui->transferFuncKeyframeCheckBox->blockSignals( true );
    ui->transferFuncKeyframeCheckBox->setChecked( m_pluginInterface->HasTFKey() );
    ui->transferFuncKeyframeCheckBox->blockSignals( false );

    // Cam min dist keyframe
    ui->cameraMinDistCheckBox->blockSignals( true );
    ui->cameraMinDistCheckBox->setChecked( m_pluginInterface->HasCamMinDistKey() );
    ui->cameraMinDistCheckBox->blockSignals( false );
}

void AnimateWidget::on_domeModeButton_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );

    m_pluginInterface->SetRenderDome( checked );
}

void AnimateWidget::on_cubeTextureSizeSlider_valueChanged(int value)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetDomeCubeTextureSize( value );
    UpdateUi();
}

void AnimateWidget::on_cubeTextureSizeSpinBox_valueChanged(int value)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetDomeCubeTextureSize( value );
    UpdateUi();
}

void AnimateWidget::on_cameraKeyCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetCameraKey( checked );
}

void AnimateWidget::on_renderHDRadioButton_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    if( checked )
        m_pluginInterface->SetRenderSize( 1920, 1080 );
}

void AnimateWidget::on_render1KradioButton_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    if( checked )
        m_pluginInterface->SetRenderSize( 1024, 1024 );
}

void AnimateWidget::on_render2KRadioButton_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    if( checked )
        m_pluginInterface->SetRenderSize( 2048, 2048 );
}

void AnimateWidget::on_render4KradioButton_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    if( checked )
        m_pluginInterface->SetRenderSize( 4096, 4096 );
}

void AnimateWidget::on_renderSnapshotButton_clicked()
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->RenderCurrentFrame();
}

void AnimateWidget::on_renderAnimationButton_clicked()
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->RenderAnimation();
}

void AnimateWidget::on_transferFuncKeyframeCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetTFKey( checked );
}

void AnimateWidget::on_nbFramesSpinBox_valueChanged(int arg1)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetNumberOfFrames( arg1 );
}

void AnimateWidget::on_cameraMinDistanceGroupBox_toggled(bool arg1)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetUseMinCamDistance( arg1 );
    UpdateUi();
}

void AnimateWidget::on_cameraMinDistanceSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_pluginInterface );
        m_pluginInterface->SetMinCamDistance( arg1 );
}

void AnimateWidget::on_cameraMinDistCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetCamMinDistKey( checked );
}

void AnimateWidget::on_nextKeyframeButton_clicked()
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->NextKeyframe();
}

void AnimateWidget::on_prevKeyframeButton_clicked()
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->PrevKeyframe();
}

void AnimateWidget::on_domeViewAngleSlider_valueChanged(int value)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetDomeViewAngle( value );
    UpdateUi();
}

void AnimateWidget::on_domeViewAngleSpinBox_valueChanged(int arg1)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetDomeViewAngle( arg1 );
    UpdateUi();
}

