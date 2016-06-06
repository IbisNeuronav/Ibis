/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
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
    m_sceneManager(0)
{
    m_needVolumeSlotUpdate = true;
    m_shaderInitWidget = 0;
    ui->setupUi(this);
    ui->interactionPointTypeButtonGroup->setId( ui->pointTypeRadioButton, 0 );
    ui->interactionPointTypeButtonGroup->setId( ui->lineTypeRadioButton, 1 );
}

VolumeRenderingObjectSettingsWidget::~VolumeRenderingObjectSettingsWidget()
{
    delete ui;
}

void VolumeRenderingObjectSettingsWidget::SetSceneManager( SceneManager * man )
{
    this->m_sceneManager = man;
    if( this->m_sceneManager )
    {
        connect( man->GetMainVolumeRenderer(), SIGNAL(FinishedReading()), this, SLOT(UpdateUI()) );
        connect( man->GetMainVolumeRenderer(), SIGNAL(Modified()), this, SLOT(UpdateUI()) );
        connect( man->GetMainVolumeRenderer(), SIGNAL(VolumeSlotModified()), this, SLOT(VolumeSlotModified()) );
        UpdateUI();
    }
}

void VolumeRenderingObjectSettingsWidget::UpdateUI()
{
    if( m_sceneManager )
    {
        ui->animateCheckBox->blockSignals( true );
        ui->animateCheckBox->setEnabled( true );
        VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
        ui->animateCheckBox->setChecked( vr->IsAnimating() );
        ui->animateCheckBox->blockSignals( false );

        ui->samplingSpinBox->blockSignals( true );
        ui->samplingSpinBox->setValue( vr->GetSamplingDistance() );
        ui->samplingSpinBox->blockSignals( false );

        ui->multFactorSlider->blockSignals( true );
        double factor = vr->GetMultFactor();
        int factorSlideVal = (int)( ( factor - minFactor ) / ( maxFactor - minFactor ) * 100 );
        ui->multFactorSlider->setValue( factorSlideVal );
        ui->multFactorSlider->blockSignals( false );

        ui->multFactorSpinBox->blockSignals( true );
        ui->multFactorSpinBox->setValue( vr->GetMultFactor() );
        ui->multFactorSpinBox->blockSignals( false );

        ui->enableFogCheckBox->blockSignals( true );
        ui->enableFogCheckBox->setChecked( vr->IsFogEnabled() );
        ui->enableFogCheckBox->blockSignals( false );

        ui->fogMinDistanceSpinBox->blockSignals( true );
        ui->fogMinDistanceSpinBox->setValue( vr->GetFogMinDistance() );
        ui->fogMinDistanceSpinBox->blockSignals( false );

        ui->fogMaxDistanceSpinBox->blockSignals( true );
        ui->fogMaxDistanceSpinBox->setValue( vr->GetFogMaxDistance() );
        ui->fogMaxDistanceSpinBox->blockSignals( false );

        ui->showLightCheckBox->blockSignals( true );
        ui->showLightCheckBox->setChecked( vr->GetShowInteractionWidget() );
        ui->showLightCheckBox->blockSignals( false );

        ui->trackLightCheckBox->blockSignals( true );
        ui->trackLightCheckBox->setEnabled( !(Application::GetInstance().IsViewerOnly()) );
        ui->trackLightCheckBox->setChecked( vr->GetPointerTracksInteractionPoints() );
        ui->trackLightCheckBox->blockSignals( false );

        ui->pointTypeRadioButton->blockSignals( true );
        ui->pointTypeRadioButton->setChecked( !vr->IsInteractionWidgetLine() );
        ui->pointTypeRadioButton->blockSignals( false );

        ui->lineTypeRadioButton->blockSignals( true );
        ui->lineTypeRadioButton->setChecked( vr->IsInteractionWidgetLine() );
        ui->lineTypeRadioButton->blockSignals( false );

        ui->pickPosCheckBox->blockSignals( true );
        ui->pickPosCheckBox->setChecked( vr->GetPickPos() );
        ui->pickPosCheckBox->blockSignals( false );

        ui->pickValueSpinBox->blockSignals( true );
        ui->pickValueSpinBox->setValue( vr->GetPickValue() );
        ui->pickValueSpinBox->blockSignals( false );

        ui->removeRayInitShaderButton->setEnabled( vr->IsRayInitShaderTypeCustom( vr->GetRayInitShaderType() ) );
        ui->rayInitComboBox->blockSignals( true );
        ui->rayInitComboBox->clear();
        for( int i = 0; i < vr->GetNumberOfRayInitShaderTypes(); ++i )
        {
            ui->rayInitComboBox->addItem( vr->GetRayInitShaderTypeName( i ) );
        }
        ui->rayInitComboBox->setCurrentIndex( vr->GetRayInitShaderType() );
        ui->rayInitComboBox->blockSignals( false );

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
        VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
        for( int i = 0; i < vr->GetNumberOfImageSlots(); ++i )
        {
            VolumeRenderingSingleVolumeSettingsWidget * volSettings = new VolumeRenderingSingleVolumeSettingsWidget;
            volSettings->SetSceneManager( this->m_sceneManager, i );
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
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->Animate( checked );
}

void VolumeRenderingObjectSettingsWidget::on_addVolumeButton_clicked()
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->AddImageSlot();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_removeVolumeButton_clicked()
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->RemoveImageSlot();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_fogMinDistanceSpinBox_valueChanged( double dist )
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetFogMinDistance( dist );
}

void VolumeRenderingObjectSettingsWidget::on_fogMaxDistanceSpinBox_valueChanged( double dist )
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetFogMaxDistance( dist );
}

void VolumeRenderingObjectSettingsWidget::on_enableFogCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetFogEnabled( checked );
}

void VolumeRenderingObjectSettingsWidget::on_samplingSpinBox_valueChanged( double sampling )
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetSamplingDistance( sampling );
}

void VolumeRenderingObjectSettingsWidget::on_showLightCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetShowInteractionWidget( checked );
}

void VolumeRenderingObjectSettingsWidget::on_trackLightCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetPointerTracksInteractionPoints( checked );
}

void VolumeRenderingObjectSettingsWidget::on_interactionPointTypeButtonGroup_buttonClicked( int button )
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    if( button == 0 )
        vr->SetInteractionWidgetAsLine( false );
    else
        vr->SetInteractionWidgetAsLine( true );
}

void VolumeRenderingObjectSettingsWidget::on_shaderInitButton_toggled(bool checked)
{
    if( checked )
    {
        Q_ASSERT( m_shaderInitWidget == 0 );
        m_shaderInitWidget = new VolumeShaderEditorWidget( 0 );
        m_shaderInitWidget->setAttribute( Qt::WA_DeleteOnClose );
        m_shaderInitWidget->SetVolumeRenderer( m_sceneManager->GetMainVolumeRenderer(), -1 );
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
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetPickPos( checked );
}

void VolumeRenderingObjectSettingsWidget::on_pickValueSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetPickValue( arg1 );
}


void VolumeRenderingObjectSettingsWidget::on_addRayInitShaderButton_clicked()
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->DuplicateRayInitShaderType();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_removeRayInitShaderButton_clicked()
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->DeleteRayInitShaderType();
    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_rayInitComboBox_currentIndexChanged(int index)
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetRayInitShaderType( index );
}

void VolumeRenderingObjectSettingsWidget::on_multFactorSlider_valueChanged(int value)
{
    Q_ASSERT( m_sceneManager );
    double percent = (double)value * 0.01;
    double newFactor = minFactor + percent * ( maxFactor - minFactor );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetMultFactor( newFactor );

    UpdateUI();
}

void VolumeRenderingObjectSettingsWidget::on_multFactorSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_sceneManager );
    VolumeRenderingObject * vr = this->m_sceneManager->GetMainVolumeRenderer();
    vr->SetMultFactor( arg1 );

    UpdateUI();
}
