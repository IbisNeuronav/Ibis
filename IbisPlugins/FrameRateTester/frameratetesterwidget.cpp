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

#include "frameratetesterwidget.h"
#include "ui_frameratetesterwidget.h"
#include "frameratetesterplugininterface.h"
#include "scenemanager.h"
#include "vtkqtrenderwindow.h"
#include <QSize>

FrameRateTesterWidget::FrameRateTesterWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FrameRateTesterWidget)
{
    ui->setupUi(this);
}

FrameRateTesterWidget::~FrameRateTesterWidget()
{
    delete ui;
}

void FrameRateTesterWidget::SetPluginInterface( FrameRateTesterPluginInterface * pi )
{
    m_pluginInterface = pi;
    connect( m_pluginInterface, SIGNAL(PeriodicSignal()), this, SLOT(UpdateStats()) );
    connect( m_pluginInterface, SIGNAL(Modified()), this, SLOT(UpdateUi()) );
    UpdateUi();
}

void FrameRateTesterWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );

    // Current view combo
    ui->currentViewComboBox->blockSignals( true );
    ui->currentViewComboBox->clear();
    int nbViews = m_pluginInterface->GetSceneManager()->GetNumberOfViews();
    for( int i = 0; i < nbViews; ++i )
    {
        View * v = m_pluginInterface->GetSceneManager()->GetViewByIndex( i );
        ui->currentViewComboBox->addItem( v->GetName() );
    }
    ui->currentViewComboBox->setCurrentIndex( m_pluginInterface->GetCurrentViewIndex() );
    ui->currentViewComboBox->blockSignals( false );

    // View size
    View * v = m_pluginInterface->GetSceneManager()->GetViewByIndex( m_pluginInterface->GetCurrentViewIndex() );
    QSize s = v->GetQtRenderWindow()->size();
    ui->windowSizeLabel->setText( QString("Size : %1 x %2").arg( s.width() ).arg( s.height() ) );

    // Run button
    ui->runButton->blockSignals( true );
    ui->runButton->setChecked( m_pluginInterface->IsRunning() );
    ui->runButton->setText(  m_pluginInterface->IsRunning() ? "Stop" : "Run" );
    ui->runButton->blockSignals( false );

    // Period
    ui->periodSpinBox->blockSignals( true );
    ui->periodSpinBox->setValue( m_pluginInterface->GetNumberOfFrames() );
    ui->periodSpinBox->blockSignals( false );

    UpdateStats();

}

void FrameRateTesterWidget::UpdateStats()
{
    ui->lastPeriodLabel->setText( QString("Period : %1").arg( m_pluginInterface->GetLastPeriod() ) );
    ui->numberOfFramesLabel->setText( QString("Nb Frames : %1").arg( m_pluginInterface->GetLastNumberOfFrames() ) );
    ui->frameRateLabel->setText( QString("Fps : %1").arg( m_pluginInterface->GetLastFrameRate() ) );
}

void FrameRateTesterWidget::on_currentViewComboBox_currentIndexChanged(int index)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetCurrentViewIndex( index );
}

void FrameRateTesterWidget::on_periodSpinBox_valueChanged(int arg1)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetNumberOfFrames( arg1 );
}

void FrameRateTesterWidget::on_runButton_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetRunning( checked );
}
