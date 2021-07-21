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
#include "ibisapi.h"
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <QVTKRenderWidget.h>
#include <QSize>
#include <QMap>
#include <QComboBox>
#include "view.h"
#include "guiutilities.h"

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
    connect( m_pluginInterface, SIGNAL(PluginModified()), this, SLOT(UpdateUi()) );
    UpdateUi();
}

void FrameRateTesterWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );
    Q_ASSERT( m_pluginInterface->GetCurrentViewID() != IbisAPI::InvalidId );

    // Current view combo
    ui->currentViewComboBox->blockSignals( true );
    ui->currentViewComboBox->clear();
    QMap<View*, int> allViews = m_pluginInterface->GetIbisAPI()->GetAllViews();
    GuiUtilities::UpdateObjectComboBox( ui->currentViewComboBox, allViews, m_pluginInterface->GetCurrentViewID() );
    int currentIndex = GuiUtilities::ObjectComboBoxIndexFromObjectId( ui->currentViewComboBox, m_pluginInterface->GetCurrentViewID() );
    ui->currentViewComboBox->setCurrentIndex( currentIndex );
    ui->currentViewComboBox->blockSignals( false );

    // View size
    View * v = m_pluginInterface->GetIbisAPI()->GetViewByID( m_pluginInterface->GetCurrentViewID() );
    vtkRenderer *ren = v->GetRenderer();
    vtkRenderWindow * win = vtkRenderWindow::SafeDownCast( ren->GetRenderWindow() );
    QSize s;
    s.setWidth( win->GetSize()[0] );

    s.setHeight( win->GetSize()[1] );
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
    int currentID = GuiUtilities::ObjectIdFromObjectComboBox( ui->currentViewComboBox, index );
    m_pluginInterface->SetCurrentViewId( currentID );
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
