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

#include "stereotacticframewidget.h"
#include "ui_stereotacticframewidget.h"
#include "stereotacticframeplugininterface.h"

StereotacticFrameWidget::StereotacticFrameWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StereotacticFrameWidget)
{
    ui->setupUi(this);
}

StereotacticFrameWidget::~StereotacticFrameWidget()
{
    delete ui;
}

void StereotacticFrameWidget::SetPluginInterface( StereotacticFramePluginInterface * pi )
{
    m_pluginInterface = pi;
    connect( m_pluginInterface, SIGNAL(CursorMoved()), this, SLOT(OnCursorMoved()) );
    UpdateUi();
}

void StereotacticFrameWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );
    ui->show3DFrameCheckBox->blockSignals( true );
    ui->show3DFrameCheckBox->setChecked( m_pluginInterface->IsShowing3DFrame() );
    ui->show3DFrameCheckBox->blockSignals( false );
    ui->showManipulatorsCheckBox->blockSignals( true );
    ui->showManipulatorsCheckBox->setChecked( m_pluginInterface->IsShowingManipulators() );
    ui->showManipulatorsCheckBox->blockSignals( false );
    UpdateCursor();
}

void StereotacticFrameWidget::OnCursorMoved()
{
    Q_ASSERT_X( m_pluginInterface, "StereotacticFrameWidget::OnCursorMoved()", "No plugin interface specified" );
    UpdateCursor();
}

void StereotacticFrameWidget::on_show3DFrameCheckBox_toggled(bool checked)
{
    Q_ASSERT_X( m_pluginInterface, "StereotacticFrameWidget::on_show3DFrameCheckBox_toggled()", "No plugin interface specified" );
    m_pluginInterface->SetShow3DFrame( checked );
}

void StereotacticFrameWidget::on_showManipulatorsCheckBox_toggled(bool checked)
{
    Q_ASSERT_X( m_pluginInterface, "StereotacticFrameWidget::on_showManipulatorsCheckBox_toggled()", "No plugin interface specified" );
    m_pluginInterface->SetShowManipulators( checked );
}

void StereotacticFrameWidget::on_lineEditFrameX_editingFinished()
{
    Q_ASSERT_X( m_pluginInterface, "StereotacticFrameWidget::on_lineEditFrameX_editingFinished()", "No plugin interface specified" );
    double framePos[3];
    ParseFramePosition(framePos);
    m_pluginInterface->SetCursorFromFramePosition(framePos);
}

void StereotacticFrameWidget::on_lineEditFrameY_editingFinished()
{
    Q_ASSERT_X( m_pluginInterface, "StereotacticFrameWidget::on_lineEditFrameY_editingFinished()", "No plugin interface specified" );
    double framePos[3];
    ParseFramePosition(framePos);
    m_pluginInterface->SetCursorFromFramePosition(framePos);
}

void StereotacticFrameWidget::on_lineEditFrameZ_editingFinished()
{
    Q_ASSERT_X( m_pluginInterface, "StereotacticFrameWidget::on_lineEditFrameZ_editingFinished()", "No plugin interface specified" );
    double framePos[3];
    ParseFramePosition(framePos);
    m_pluginInterface->SetCursorFromFramePosition(framePos);
}

void StereotacticFrameWidget::ParseFramePosition(double pos[3])
{
    pos[0] = ui->lineEditFrameX->text().toDouble();
    pos[1] = ui->lineEditFrameY->text().toDouble();
    pos[2] = ui->lineEditFrameZ->text().toDouble();
}

void StereotacticFrameWidget::UpdateCursor()
{
    double cursorPosition[3];
    m_pluginInterface->GetCursorFramePosition( cursorPosition );
    ui->lineEditFrameX->setText( QString::number(cursorPosition[0]) );
    ui->lineEditFrameY->setText( QString::number(cursorPosition[1]) );
    ui->lineEditFrameZ->setText( QString::number(cursorPosition[2]) );
}
