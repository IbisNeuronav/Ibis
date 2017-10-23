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

#include "volumeshadereditorwidget.h"
#include "ui_volumeshadereditorwidget.h"
#include "volumerenderingobject.h"
#include "imageobject.h"

VolumeShaderEditorWidget::VolumeShaderEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VolumeShaderEditorWidget)
{
    ui->setupUi(this);
    this->setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
    ui->shaderCodeTextEdit->setFontFamily( "Monospace" );
}

VolumeShaderEditorWidget::~VolumeShaderEditorWidget()
{
    delete ui;
}

void VolumeShaderEditorWidget::SetVolumeRenderer( VolumeRenderingObject * ren, int volumeIndex )
{
    m_volumeRenderer = ren;
    connect( m_volumeRenderer, SIGNAL(Modified()), this, SLOT(VolumeRendererModifiedSlot()));
    m_volumeIndex = volumeIndex;
    m_backupShaderTypeName = GetCurrentShaderTypeName();
    m_backupShaderCode = GetShaderCode();
    UpdateUi();
}

void VolumeShaderEditorWidget::SetShaderCode( QString shaderCode )
{
    if( m_volumeIndex >= 0 )
        m_volumeRenderer->SetCustomShaderCode( m_volumeIndex, shaderCode );
    else if( m_volumeIndex == -1 )
        m_volumeRenderer->SetRayInitShaderCode( shaderCode );
    else
        m_volumeRenderer->SetStopConditionShaderCode( shaderCode );
}

QString VolumeShaderEditorWidget::GetShaderCode()
{
    QString shaderText;
    if( m_volumeIndex >= 0 )
        shaderText = m_volumeRenderer->GetCustomShaderContribution( m_volumeIndex );
    else if( m_volumeIndex == -1 )
        shaderText = m_volumeRenderer->GetRayInitShaderCode();
    else
        shaderText = m_volumeRenderer->GetStopConditionShaderCode();
    return shaderText;
}

bool VolumeShaderEditorWidget::IsShaderCustom()
{
    if( m_volumeIndex >= 0 )
        return m_volumeRenderer->IsShaderTypeCustom( m_volumeRenderer->GetShaderContributionType( m_volumeIndex ) );
    if( m_volumeIndex == -1 )
        return m_volumeRenderer->IsRayInitShaderTypeCustom( m_volumeRenderer->GetRayInitShaderType() );
    return m_volumeRenderer->IsStopConditionShaderTypeCustom( m_volumeRenderer->GetStopConditionShaderType() );
}

void VolumeShaderEditorWidget::DuplicateShader()
{
    if( m_volumeIndex >= 0 )
    {
        int newShaderType = m_volumeRenderer->DuplicateShaderContribType( m_volumeRenderer->GetShaderContributionType( m_volumeIndex ) );
        if( newShaderType != -1 )
            m_volumeRenderer->SetShaderContributionType( m_volumeIndex, newShaderType );
    }
    else if( m_volumeIndex == -1 )
        m_volumeRenderer->DuplicateRayInitShaderType();
    else
        m_volumeRenderer->DuplicateStopConditionShaderType();
}

void VolumeShaderEditorWidget::UpdateUi()
{
    Q_ASSERT( m_volumeRenderer );

    // update window caption
    QString header;
    if( m_volumeIndex >= 0 )
    {
        ImageObject * img = m_volumeRenderer->GetImage( m_volumeIndex );
        if( img )
        {
            header = m_volumeRenderer->GetImage( m_volumeIndex )->GetName();
            header += " - ";
        }
    }
    else if( m_volumeIndex == -1 )
    {
        header = "Ray Init - ";
    }
    else
    {
        header = "Stop Condition - ";
    }
    header += GetCurrentShaderTypeName();
    this->setWindowTitle( header );

    // Update code text
    bool custom = IsShaderCustom();
    ui->shaderCodeTextEdit->setReadOnly( !custom );
    ui->shaderCodeTextEdit->setText( GetShaderCode() );
    ui->duplicateShaderButton->setVisible( m_volumeIndex < 0 );  // init shader only, with other shaders, this causes the dialog to close because the ui is updated in the settings widget.
}

QString VolumeShaderEditorWidget::GetCurrentShaderTypeName()
{
    if( m_volumeIndex >= 0 )
        return m_volumeRenderer->GetShaderContributionTypeName( m_volumeRenderer->GetShaderContributionType( m_volumeIndex ) );
    else if( m_volumeIndex == -1 )
        return m_volumeRenderer->GetRayInitShaderTypeName( m_volumeRenderer->GetRayInitShaderType() );
    else
        return m_volumeRenderer->GetStopConditionShaderTypeName( m_volumeRenderer->GetStopConditionShaderType() );
}

void VolumeShaderEditorWidget::VolumeRendererModifiedSlot()
{
    if( m_backupShaderTypeName != GetCurrentShaderTypeName() )
    {
        m_backupShaderTypeName = GetCurrentShaderTypeName();
        m_backupShaderCode = GetShaderCode();
        UpdateUi();
    }
}

void VolumeShaderEditorWidget::on_applyButton_clicked()
{
    QString newShaderText = ui->shaderCodeTextEdit->toPlainText();
    SetShaderCode( newShaderText );
}

void VolumeShaderEditorWidget::on_revertButton_clicked()
{
    SetShaderCode( m_backupShaderCode );
    UpdateUi();
}

void VolumeShaderEditorWidget::on_duplicateShaderButton_clicked()
{
    DuplicateShader();
}
