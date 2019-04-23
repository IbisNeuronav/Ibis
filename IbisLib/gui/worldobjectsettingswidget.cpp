/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "worldobjectsettingswidget.h"
#include "ui_worldobjectsettingswidget.h"
#include "worldobject.h"

#include <QColorDialog>

const char * UpdateFrequencyStrings[4] = { "7.5", "15", "30", "60" };
const double UpdateFrequencies[4] = { 7.5, 15.0, 30.0, 60.0 };

WorldObjectSettingsWidget::WorldObjectSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WorldObjectSettingsWidget),
    m_worldObject(0)
{
    ui->setupUi(this);
    ui->interactionStyleButtonGroup->setId( ui->trackballInteractionRadioButton, (int)InteractorStyleTrackball );
    ui->interactionStyleButtonGroup->setId( ui->joystickInteractionRadioButton, (int) InteractorStyleJoystick );
    ui->interactionStyleButtonGroup->setId( ui->terrainInteractionRadioButton, (int)InteractorStyleTerrain );
}

WorldObjectSettingsWidget::~WorldObjectSettingsWidget()
{
    delete ui;
}

void WorldObjectSettingsWidget::SetWorldObject( WorldObject * obj )
{
    if( obj == m_worldObject )
        return;

    m_worldObject = obj;

    if( m_worldObject )
    {
        UpdateUi();
    }
}

void WorldObjectSettingsWidget::on_changeColorButton_clicked()
{
    QColor initial( m_worldObject->GetBackgroundColor() );
    QColor newColor = QColorDialog::getColor( initial );
    if( newColor.isValid() )
    {
        m_worldObject->SetBackgroundColor( newColor );
        UpdateUi();
    }
}

void WorldObjectSettingsWidget::on_change3DColorButton_clicked()
{
    QColor initial( m_worldObject->GetBackgroundColor() );
    QColor newColor = QColorDialog::getColor( initial );
    if( newColor.isValid() )
    {
        m_worldObject->Set3DBackgroundColor( newColor );
        UpdateUi();
    }
}

void WorldObjectSettingsWidget::on_changeCursorColorButton_clicked()
{
    QColor initial( m_worldObject->GetCursorColor() );
    QColor newColor = QColorDialog::getColor( initial );
    if( newColor.isValid() )
    {
        m_worldObject->SetCursorColor( newColor );
        UpdateUi();
    }
}

void WorldObjectSettingsWidget::on_showAxesCheckBox_toggled( bool checked )
{
    if( m_worldObject )
        m_worldObject->SetAxesHidden( !checked );
    UpdateUi();
}

void WorldObjectSettingsWidget::on_showCursorCheckBox_toggled( bool checked )
{
    if( m_worldObject )
        m_worldObject->SetCursorVisible( checked );
    UpdateUi();
}

void WorldObjectSettingsWidget::On3DInteractionButtonClicked( int buttonId )
{
    if( m_worldObject )
    {
        InteractorStyle style = (InteractorStyle)buttonId;
        m_worldObject->Set3DInteractorStyle( style );
    }
}

void WorldObjectSettingsWidget::UpdateUi()
{
    // color swatch button
    QColor color = m_worldObject->GetBackgroundColor();
    QString styleColor = QString("background-color: rgb(%1,%2,%3);").arg( color.red() ).arg( color.green() ).arg( color.blue() );
    QString style = QString("border-width: 2px; border-style: solid; border-radius: 7;" );
    styleColor += style;
    ui->changeColorButton->setStyleSheet( styleColor );
    color = m_worldObject->Get3DBackgroundColor();
    styleColor = QString("background-color: rgb(%1,%2,%3);").arg( color.red() ).arg( color.green() ).arg( color.blue() );
    styleColor += style;
    ui->change3DColorButton->setStyleSheet( styleColor );
    color = m_worldObject->GetCursorColor();
    styleColor = QString("background-color: rgb(%1,%2,%3);").arg( color.red() ).arg( color.green() ).arg( color.blue() );
    style = QString("border-width: 2px; border-style: solid; border-radius: 7;" );
    styleColor += style;
    ui->changeCursorColorButton->setStyleSheet( styleColor );

    if( m_worldObject )
    {
        ui->showAxesCheckBox->blockSignals( true );
        ui->showAxesCheckBox->setChecked( !m_worldObject->AxesHidden() );
        ui->showAxesCheckBox->blockSignals( false );

        ui->showCursorCheckBox->blockSignals( true );
        ui->showCursorCheckBox->setChecked( m_worldObject->GetCursorVisible() );
        ui->showCursorCheckBox->blockSignals( false );

        ui->viewFollowsReferenceCheckBox->blockSignals( true );
        ui->viewFollowsReferenceCheckBox->setChecked( m_worldObject->Is3DViewFollowingReferenceVolume() );
        ui->viewFollowsReferenceCheckBox->blockSignals( false );

        InteractorStyle style = m_worldObject->Get3DInteractorStyle();
        QAbstractButton * button = ui->interactionStyleButtonGroup->button( (int)style );
        button->setChecked( true );

        // Camera angle
        ui->cameraAngleSlider->blockSignals( true );
        ui->cameraAngleSlider->setValue( (int)m_worldObject->Get3DCameraViewAngle() );
        ui->cameraAngleSlider->blockSignals( false );

        ui->cameraAngleSpinBox->blockSignals( true );
        ui->cameraAngleSpinBox->setValue( (int)m_worldObject->Get3DCameraViewAngle() );
        ui->cameraAngleSpinBox->blockSignals( false );

        int updateFrequencyIndex = GetUpdateFrequencyIndex( m_worldObject->GetUpdateFrequency() );
        ui->updateMaxFrequencySlider->blockSignals( true );
        ui->updateMaxFrequencySlider->setValue( updateFrequencyIndex );
        ui->updateMaxFrequencySlider->blockSignals( false );
        ui->fpsLineEdit->setText( UpdateFrequencyStrings[ updateFrequencyIndex ] );
    }
}

int WorldObjectSettingsWidget::GetUpdateFrequencyIndex( double fps )
{
    for( int i = 0; i < 4; ++i )
    {
        if( UpdateFrequencies[i] == fps )
            return i;
    }
    return 0;
}

void WorldObjectSettingsWidget::on_updateMaxFrequencySlider_valueChanged( int value )
{
    m_worldObject->SetUpdateFrequency( UpdateFrequencies[value] );
    UpdateUi();
}

void WorldObjectSettingsWidget::on_viewFollowsReferenceCheckBox_toggled(bool checked)
{
    m_worldObject->Set3DViewFollowsReferenceVolume( checked );
}

void WorldObjectSettingsWidget::on_cameraAngleSlider_valueChanged(int value)
{
    m_worldObject->Set3DCameraViewAngle( (double)value );
    UpdateUi();
}

void WorldObjectSettingsWidget::on_cameraAngleSpinBox_valueChanged(int value)
{
    m_worldObject->Set3DCameraViewAngle( (double)value );
    UpdateUi();
}
