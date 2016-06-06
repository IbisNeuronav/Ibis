/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "dtiobjectsettingswidget.h"
#include "ui_dtiobjectsettingswidget.h"
#include "dtiobject.h"

DTIObjectSettingsWidget::DTIObjectSettingsWidget(QWidget *parent) :
    QWidget(parent)
    , m_tracks(0)
    , ui(new Ui::DTIObjectSettingsWidget)
{
    ui->setupUi(this);
}

DTIObjectSettingsWidget::~DTIObjectSettingsWidget()
{
    if( m_tracks )
    {
        m_tracks->UnRegister( 0 );
    }
    delete ui;
}

void DTIObjectSettingsWidget::SetDTIObject( DTIObject * object )
{
    if( object == m_tracks )
    {
        return;
    }
    m_tracks = object;
    if( m_tracks )
    {
        m_tracks->Register( 0 );
        this->UpdateUI();
    }
}

void DTIObjectSettingsWidget::UpdateUI()
{
    Q_ASSERT( m_tracks );
    ui->generateTubesCheckBox->blockSignals( true );
    ui->generateTubesCheckBox->setChecked( m_tracks->GetGenerateTubes() );
    ui->generateTubesCheckBox->blockSignals( false );
    ui->radiusSpinBox->setValue( m_tracks->GetTubeRadius() );
    ui->resolutionSpinBox->setValue( m_tracks->GetTubeResolution() );
}

void DTIObjectSettingsWidget::on_radiusSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_tracks );
    m_tracks->SetTubeRadius( arg1 );
}

void DTIObjectSettingsWidget::on_resolutionSpinBox_valueChanged(int arg1)
{
    Q_ASSERT( m_tracks );
    m_tracks->SetTubeResolution(arg1);
}

void DTIObjectSettingsWidget::on_generateTubesCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_tracks );
    m_tracks->SetGenerateTubes( checked );
}
