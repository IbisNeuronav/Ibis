/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "usmasksettingswidget.h"

#include "ui_usmasksettingswidget.h"

USMaskSettingsWidget::USMaskSettingsWidget( QWidget * parent ) : QWidget( parent ), ui( new Ui::USMaskSettingsWidget )
{
    ui->setupUi( this );
    m_mask = 0;
}

USMaskSettingsWidget::~USMaskSettingsWidget() { delete ui; }

void USMaskSettingsWidget::SetMask( USMask * mask )
{
    Q_ASSERT( mask );
    m_mask = mask;
    this->UpdateUI();
}

void USMaskSettingsWidget::on_widthSpinBox_valueChanged( int val )
{
    Q_ASSERT( m_mask );
    int size[2];
    size[0] = val;
    size[1] = m_mask->GetMaskSize()[1];
    m_mask->SetMaskSize( size );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_heightSpinBox_valueChanged( int val )
{
    Q_ASSERT( m_mask );
    int size[2];
    size[0] = m_mask->GetMaskSize()[0];
    size[1] = val;
    m_mask->SetMaskSize( size );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_topDoubleSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_mask );
    m_mask->SetMaskDepthTop( val / m_mask->GetMaskSize()[1] );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_bottomDoubleSpinBox_valueChanged( double val )
{
    m_mask->SetMaskDepthBottom( val / m_mask->GetMaskSize()[1] );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_leftDoubleSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_mask );
    double crop[2];
    crop[0] = val / m_mask->GetMaskSize()[0];
    crop[1] = m_mask->GetMaskCrop()[1];
    m_mask->SetMaskCrop( crop );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_rightDoubleSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_mask );
    double crop[2];
    crop[0] = m_mask->GetMaskCrop()[0];
    crop[1] = val / m_mask->GetMaskSize()[0];
    m_mask->SetMaskCrop( crop );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_leftAngleDoubleSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_mask );
    double angle[2];
    angle[0] = to_radians( val );
    angle[1] = m_mask->GetMaskAngles()[1];
    m_mask->SetMaskAngles( angle );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_rightAngleDoubleSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_mask );
    double angle[2];
    angle[0] = m_mask->GetMaskAngles()[0];
    angle[1] = to_radians( val );
    m_mask->SetMaskAngles( angle );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_originXDoubleSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_mask );
    double origin[2];
    origin[0] = val / m_mask->GetMaskSize()[0];
    origin[1] = m_mask->GetMaskOrigin()[1];
    m_mask->SetMaskOrigin( origin );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_originYDoubleSpinBox_valueChanged( double val )
{
    Q_ASSERT( m_mask );
    double origin[2];
    origin[0] = m_mask->GetMaskOrigin()[0];
    origin[1] = val / m_mask->GetMaskSize()[1];
    m_mask->SetMaskOrigin( origin );
    this->UpdateUI();
}

void USMaskSettingsWidget::on_defaultPushButton_clicked()
{
    Q_ASSERT( m_mask );
    m_mask->ResetToDefault();
    this->UpdateUI();
}

void USMaskSettingsWidget::on_setDefaultPushButton_clicked()
{
    Q_ASSERT( m_mask );
    m_mask->SetAsDefault();
}

void USMaskSettingsWidget::DisableSetASDefault() { ui->setDefaultPushButton->hide(); }

void USMaskSettingsWidget::UpdateUI()
{
    Q_ASSERT( m_mask );
    ui->widthSpinBox->setValue( m_mask->GetMaskSize()[0] );
    ui->heightSpinBox->setValue( m_mask->GetMaskSize()[1] );
    ui->originXDoubleSpinBox->setValue( m_mask->GetMaskOrigin()[0] * m_mask->GetMaskSize()[0] );
    ui->originYDoubleSpinBox->setValue( m_mask->GetMaskOrigin()[1] * m_mask->GetMaskSize()[1] );
    ui->topDoubleSpinBox->setValue( m_mask->GetMaskTop() * m_mask->GetMaskSize()[1] );
    ui->bottomDoubleSpinBox->setValue( m_mask->GetMaskBottom() * m_mask->GetMaskSize()[1] );
    ui->leftDoubleSpinBox->setValue( m_mask->GetMaskCrop()[0] * m_mask->GetMaskSize()[0] );
    ui->rightDoubleSpinBox->setValue( m_mask->GetMaskCrop()[1] * m_mask->GetMaskSize()[0] );
    ui->leftAngleDoubleSpinBox->setValue( to_degrees( m_mask->GetMaskAngles()[0] ) );
    ui->rightAngleDoubleSpinBox->setValue( to_degrees( m_mask->GetMaskAngles()[1] ) );
}
