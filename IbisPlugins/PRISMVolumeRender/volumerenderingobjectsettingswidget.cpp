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

#include "volumerenderingobjectsettingswidget.h"
#include "ui_volumerenderingobjectsettingswidget.h"
#include "volumerenderingsinglevolumesettingswidget.h"
#include "volumeshadereditorwidget.h"
#include "scenemanager.h"
#include "application.h"
#include "volumerenderingobject.h"
#include "vtkVolumeProperty.h"

static const double minFactor = 0.1;
static const double maxFactor = 10.0;

VolumeRenderingObjectSettingsWidget::VolumeRenderingObjectSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VolumeRenderingObjectSettingsWidget),
    m_vr(0)
{
    m_needVolumeSlotUpdate = true;
    m_shaderInitWidget = 0;
    m_shaderStopConditionWidget = 0;
    ui->setupUi(this);
    ui->interactionPointTypeButtonGroup->setId( ui->pointTypeRadioButton, 0 );
    ui->interactionPointTypeButtonGroup->setId( ui->lineTypeRadioButton, 1 );
}

VolumeRenderingObjectSettingsWidget::~VolumeRenderingObjectSettingsWidget()
{
    delete ui;
}

void VolumeRenderingObjectSettingsWidget::SetVolumeRenderingObject( VolumeRenderingObject * vr )
{
    this->m_vr = vr;
    if( this->m_vr )
    {
        connect( m_vr, SIGNAL(FinishedReading()), this, SLOT(UpdateUI()) );
        connect( m_vr, SIGNAL(Modified()), this, SLOT(UpdateUI()) );
        connect( m_vr, SIGNAL(VolumeSlotModified()), this, SLOT(VolumeSlotModified()) );
        UpdateUI();
    }
}

void VolumeRenderingObjectSettingsWidget::UpdateUI()
{
    if( m_vr )
    {
        ui->animateCheckBox->blockSignals( true );
        ui->animateCheckBox->setEnabled( true );
        ui->animateCheckBox->setChecked( m_vr->IsAnimating() );
        ui->animateCheckBox->blockSignals( false );

        ui->samplingSpinBox->blockSignals( true );
        ui->samplingSpinBox->setValue( m_vr->GetSamplingDistance() );
        ui->samplingSpinBox->blockSignals( false );

        ui->multFactorSlider->blockSignals( true );
        double factor = m_vr->GetMultFactor();
        int factorSlideVal = (int)( ( factor - minFactor ) / ( maxFactor - minFactor ) * 100 );
        ui->multFactorSlider->setValue( factorSlideVal );
        ui->multFactorSlider->blockSignals( false );

        ui->multFactorSpinBox->blockSignals( true );
        ui->multFactorSpinBox->setValue( m_vr->GetMultFactor() );
        ui->multFactorSpinBox->blockSignals( false );

        ui->showLightCheckBox->blockSignals( true );
        ui->showLightCheckBox->setChecked( m_vr->GetShowInteractionWidget() );
        ui->showLightCheckBox->blockSignals( false );

        ui->trackLightCheckBox->blockSignals( true );
        ui->trackLightCheckBox->setEnabled( !(Application::GetInstance().IsViewerOnly()) );
        ui->trackLightCheckBox->setChecked( m_vr->GetPointerTracksInteractionPoints() );
        ui->trackLightCheckBox->blockSignals( false );

        ui->pointTypeRadioButton->blockSignals( true );
        ui->pointTypeRadioButton->setChecked( !m_vr->IsInteractionWidgetLine() );
        ui->pointTypeRadioButton->blockSignals( false );

        ui->lineTypeRadioButton->blockSignals( true );
        ui->lineTypeRadioButton->setChecked( m_vr->IsInteractionWidgetLine() );
        ui->lineTypeRadioButton->blockSignals( false );

        ui->pickPosCheckBox->blockSignals( true );
        ui->pickPosCheckBox->setChecked( m_vr->GetPickPos() );
        ui->pickPosCheckBox->blockSignals( false );

        ui->pickValueSpinBox->blockSignals( true );
        ui->pickValueSpinBox->setValue( m_vr->GetPickValue() );
        ui->pickValueSpinBox->blockSignals( false );

        ui->removeRayInitShaderButton->setEnabled( m_vr->IsRayInitShaderTypeCustom( m_vr->GetRayInitShaderType() ) );
        ui->rayInitComboBox->blockSignals( true );
        ui->rayInitComboBox->clear();
        for( int i = 0; i < m_vr->GetNumberOfRayInitShaderTypes(); ++i )
        {
            ui->rayInitComboBox->addItem( m_vr->GetRayInitShaderTypeName( i ) );
        }
        ui->rayInitComboBox->setCurrentIndex( m_vr->GetRayInitShaderType() );
        ui->rayInitComboBox->blockSignals( false );

        ui->removeStopConditionShaderButton->setEnabled( m_vr->IsStopConditionShaderTypeCustom( m_vr->GetStopConditionShaderType() ) );
        ui->stopConditionComboBox->blockSignals( true );
        ui->stopConditionComboBox->clear();
        for( int i = 0; i < m_vr->GetNumberOfStopConditionShaderTypes(); ++i )
        {
            ui->stopConditionComboBox->addItem( m_vr->GetStopConditionShaderTypeName( i ) );
        }
        ui->stopConditionComboBox->setCurrentIndex( m_vr->GetStopConditionShaderType() );
        ui->stopConditionComboBox->blockSignals( false );

        SetupVolumeWidgets();
    }
    else
    {
        ui->animateCheckBox->setEnabled( false );
    }
}

void VolumeRenderingObjectSettingsWidget::VolumeSlotModified()
{
    m_needVolumeSlotUpdate = true;
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::SetupVolumeWidgets()
{
    // clear previous widgets if needed
    if( m_needVolumeSlotUpdate )
    {
        for( int i = 0; i < m_volumeWidgets.size(); ++i )
        {
            m_volumeWidgets[i]->close();
        }
        m_volumeWidgets.clear();

        // Add new widgets
        for( int i = 0; i < m_vr->GetNumberOfImageSlots(); ++i )
        {
            VolumeRenderingSingleVolumeSettingsWidget * volSettings = new VolumeRenderingSingleVolumeSettingsWidget;
            volSettings->SetVolumeRenderingObject( m_vr, i );
            volSettings->setAttribute( Qt::WA_DeleteOnClose );
            ui->mainLayout->insertWidget( GetVolumeWidgetInsertionIndex(), volSettings );
            m_volumeWidgets.push_back( volSettings );
        }
        m_needVolumeSlotUpdate = false;
    }
    else
    {
        for( int i = 0; i < m_volumeWidgets.size(); ++i )
        {
            m_volumeWidgets[i]->UpdateUi();
        }
    }
}

int VolumeRenderingObjectSettingsWidget::GetVolumeWidgetInsertionIndex()
{
    for( int i = 0; i < ui->mainLayout->count(); ++i )
        if( ui->mainLayout->itemAt( i ) == ui->trailSpacer )
            return i;
    return -1;
}

void VolumeRenderingObjectSettingsWidget::on_animateCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_vr );
    m_vr->Animate( checked );
}

void VolumeRenderingObjectSettingsWidget::on_addVolumeButton_clicked()
{
    Q_ASSERT( m_vr );
    m_vr->AddImageSlot();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_removeVolumeButton_clicked()
{
    Q_ASSERT( m_vr );
    m_vr->RemoveImageSlot();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_samplingSpinBox_valueChanged( double sampling )
{
    Q_ASSERT( m_vr );
    m_vr->SetSamplingDistance( sampling );
}

void VolumeRenderingObjectSettingsWidget::on_showLightCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_vr );
    m_vr->SetShowInteractionWidget( checked );
}

void VolumeRenderingObjectSettingsWidget::on_trackLightCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_vr );
    m_vr->SetPointerTracksInteractionPoints( checked );
}

void VolumeRenderingObjectSettingsWidget::on_interactionPointTypeButtonGroup_buttonClicked( int button )
{
    Q_ASSERT( m_vr );
    if( button == 0 )
        m_vr->SetInteractionWidgetAsLine( false );
    else
        m_vr->SetInteractionWidgetAsLine( true );
}

void VolumeRenderingObjectSettingsWidget::on_shaderInitButton_toggled(bool checked)
{
    if( checked )
    {
        Q_ASSERT( m_shaderInitWidget == 0 );
        m_shaderInitWidget = new VolumeShaderEditorWidget( 0 );
        m_shaderInitWidget->setAttribute( Qt::WA_DeleteOnClose );
        m_shaderInitWidget->SetVolumeRenderer( m_vr, -1 );
        connect( m_shaderInitWidget, SIGNAL(destroyed()), this, SLOT(OnShaderInitEditorWidgetClosed()) );
        m_shaderInitWidget->show();
    }
    else
    {
        Q_ASSERT( m_shaderInitWidget );
        m_shaderInitWidget->close();
    }
}

void VolumeRenderingObjectSettingsWidget::OnShaderInitEditorWidgetClosed()
{
    m_shaderInitWidget = 0;
    ui->shaderInitButton->blockSignals( true );
    ui->shaderInitButton->setChecked( false );
    ui->shaderInitButton->blockSignals( false );
}

void VolumeRenderingObjectSettingsWidget::on_pickPosCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_vr );
    m_vr->SetPickPos( checked );
}

void VolumeRenderingObjectSettingsWidget::on_pickValueSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_vr );
    m_vr->SetPickValue( arg1 );
}


void VolumeRenderingObjectSettingsWidget::on_addRayInitShaderButton_clicked()
{
    Q_ASSERT( m_vr );
    m_vr->DuplicateRayInitShaderType();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_removeRayInitShaderButton_clicked()
{
    Q_ASSERT( m_vr );
    m_vr->DeleteRayInitShaderType();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_rayInitComboBox_currentIndexChanged(int index)
{
    Q_ASSERT( m_vr );
    m_vr->SetRayInitShaderType( index );
}

void VolumeRenderingObjectSettingsWidget::on_stopConditionComboBox_currentIndexChanged(int index)
{
    Q_ASSERT( m_vr );
    m_vr->SetStopConditionShaderType( index );
}

void VolumeRenderingObjectSettingsWidget::on_addStopConditionShaderButton_clicked()
{
    Q_ASSERT( m_vr );
    m_vr->DuplicateStopConditionShaderType();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_removeStopConditionShaderButton_clicked()
{
    Q_ASSERT( m_vr );
    m_vr->DeleteRayInitShaderType();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_shaderStopConditionButton_toggled(bool checked)
{
    if( checked )
    {
        Q_ASSERT( m_shaderStopConditionWidget == 0 );
        m_shaderStopConditionWidget = new VolumeShaderEditorWidget( 0 );
        m_shaderStopConditionWidget->setAttribute( Qt::WA_DeleteOnClose );
        m_shaderStopConditionWidget->SetVolumeRenderer( m_vr, -2 );
        connect( m_shaderStopConditionWidget, SIGNAL(destroyed()), this, SLOT(OnShaderStopConditionEditorWidgetClosed()) );
        m_shaderStopConditionWidget->show();
    }
    else
    {
        Q_ASSERT( m_shaderStopConditionWidget );
        m_shaderStopConditionWidget->close();
    }
}

void VolumeRenderingObjectSettingsWidget::OnShaderStopConditionEditorWidgetClosed()
{
    m_shaderStopConditionWidget = 0;
    ui->shaderStopConditionButton->blockSignals( true );
    ui->shaderStopConditionButton->setChecked( false );
    ui->shaderStopConditionButton->blockSignals( false );
}

void VolumeRenderingObjectSettingsWidget::on_multFactorSlider_valueChanged(int value)
{
    Q_ASSERT( m_vr );
    double percent = (double)value * 0.01;
    double newFactor = minFactor + percent * ( maxFactor - minFactor );
    m_vr->SetMultFactor( newFactor );

    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_multFactorSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_vr );
    m_vr->SetMultFactor( arg1 );

    UpdateUI();
}

