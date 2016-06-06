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

#include "timelinewidget.h"
#include "ui_timelinewidget.h"
#include "animateplugininterface.h"
#include "KeyframeEditorWidget.h"

TimelineWidget::TimelineWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TimelineWidget)
{
    ui->setupUi(this);
    m_pluginInterface = 0;
}

TimelineWidget::~TimelineWidget()
{
    delete ui;
}

void TimelineWidget::SetPluginInterface( AnimatePluginInterface * interf )
{
    m_pluginInterface = interf;
    ui->keyframeEditorWidget->SetAnim( interf );
    connect( m_pluginInterface, SIGNAL(CurrentFrameChanged()), this, SLOT(UpdateUi()) );
    connect( m_pluginInterface, SIGNAL(KeyframesChanged()), this, SLOT(UpdateUi()) );
}

void TimelineWidget::on_currentFrameSpinBox_valueChanged(int arg1)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetCurrentFrame( arg1 );
    UpdateUi();
}

void TimelineWidget::on_frameSlider_valueChanged(int value)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetCurrentFrame( value );
    UpdateUi();
}

void TimelineWidget::UpdateUi()
{
    if( m_pluginInterface )
    {
        ui->frameSlider->blockSignals( true );
        ui->frameSlider->setRange( 0, m_pluginInterface->GetNumberOfFrames() - 1 );
        ui->frameSlider->setValue( m_pluginInterface->GetCurrentFrame() );
        ui->frameSlider->blockSignals( false );

        ui->currentFrameSpinBox->blockSignals( true );
        ui->currentFrameSpinBox->setRange( 0, m_pluginInterface->GetNumberOfFrames() - 1 );
        ui->currentFrameSpinBox->setValue( m_pluginInterface->GetCurrentFrame() );
        ui->currentFrameSpinBox->blockSignals( false );

        ui->playButton->blockSignals( true );
        if( m_pluginInterface->IsPlaying() )
        {
            ui->playButton->setChecked( true );
            ui->playButton->setText( "Stop" );
        }
        else
        {
            ui->playButton->setChecked( false );
            ui->playButton->setText( "Play" );
        }
        ui->playButton->blockSignals( false );

        ui->keyframeEditorWidget->update();
    }
}

void TimelineWidget::on_playButton_toggled(bool checked)
{
    Q_ASSERT( checked != m_pluginInterface->IsPlaying() );
    m_pluginInterface->SetPlaying( checked );
    UpdateUi();
}
