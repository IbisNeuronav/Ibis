/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Francois Rheau for writing this class

#include "fiberobjectsettingswidget.h"
#include "ui_fiberobjectsettingswidget.h"
#include "fibernavigatorplugininterface.h"
#include "sceneobject.h"
#include <QtGui>

FiberObjectSettingsWidget::FiberObjectSettingsWidget( QWidget * parent )
    : QWidget(parent)
    , ui( new Ui::FiberObjectSettingsWidget )
{
    ui->setupUi(this);
}

FiberObjectSettingsWidget::~FiberObjectSettingsWidget()
{
    delete ui;
}

void FiberObjectSettingsWidget::SetPluginInterface( FiberNavigatorPluginInterface * interf )
{
    m_pluginInterface = interf;
    connect( interf, SIGNAL(RoiModifiedSignal()), this, SLOT(UpdateUI()) );
    this->UpdateUI();
    ui->roiGroupBox->setDisabled(true);
}

void FiberObjectSettingsWidget::UpdateUI()
{
    Q_ASSERT( m_pluginInterface );
    insertReferenceImage(m_pluginInterface->GetObjectsName());

    ui->seedPerAxisSlider->blockSignals( true );
    ui->seedPerAxisSlider->setValue( m_pluginInterface->GetSeedPerAxis() );
    ui->seedPerAxisSlider->blockSignals( false );
    ui->seedPerAxisLineEdit->setText( QString::number( m_pluginInterface->GetSeedPerAxis() ) );

    ui->stepSizeSlider->blockSignals( true );
    ui->stepSizeSlider->setValue( m_pluginInterface->GetStepSize()*10 );
    ui->stepSizeSlider->blockSignals( false );
    ui->stepSizeLineEdit->setText( QString::number( m_pluginInterface->GetStepSize() ) );

    ui->minimumLengthSlider->blockSignals( true );
    ui->minimumLengthSlider->setValue( m_pluginInterface->GetMinimumLength() );
    ui->minimumLengthSlider->blockSignals( false );
    ui->minimumLengthLineEdit->setText( QString::number( m_pluginInterface->GetMinimumLength() ) );

    ui->maximumLengthSlider->blockSignals( true );
    ui->maximumLengthSlider->setValue( m_pluginInterface->GetMaximumLength() );
    ui->maximumLengthSlider->blockSignals( false );
    ui->maximumLengthLineEdit->setText( QString::number( m_pluginInterface->GetMaximumLength() ) );

    ui->maximumAngleSlider->blockSignals( true );
    ui->maximumAngleSlider->setValue( m_pluginInterface->GetMaximumAngle() );
    ui->maximumAngleSlider->blockSignals( false );
    ui->maximumAngleLineEdit->setText( QString::number( m_pluginInterface->GetMaximumAngle() ) );

    ui->FAThresholdSlider->blockSignals( true );
    ui->FAThresholdSlider->setValue( m_pluginInterface->GetFAThreshold()*100 );
    ui->FAThresholdSlider->blockSignals( false );
    ui->FAThresoldlineEdit->setText( QString::number( m_pluginInterface->GetFAThreshold() ) );

    ui->InertiaSlider->blockSignals( true );
    ui->InertiaSlider->setValue( m_pluginInterface->GetInertia()*100 );
    ui->InertiaSlider->blockSignals( false );
    ui->InertiaEdit->setText( QString::number( m_pluginInterface->GetInertia() ) );

    ui->useTubesCheckBox->blockSignals( true );
    ui->useTubesCheckBox->setChecked( m_pluginInterface->IsUsingTubes() );
    ui->useTubesCheckBox->blockSignals( false );

    ui->roiGroupBox->blockSignals( true );
    ui->roiGroupBox->setChecked( m_pluginInterface->IsRoiVisible() );
    ui->roiGroupBox->blockSignals( false );

    double* roiCenter = m_pluginInterface->GetRoiCenter();
    double* roiSpacing = m_pluginInterface->GetRoiSpacing();
    ui->xCenterLineEdit->setText( QString::number( roiCenter[0] ) );
    ui->yCenterLineEdit->setText( QString::number( roiCenter[1] ) );
    ui->zCenterLineEdit->setText( QString::number( roiCenter[2] ) );
    ui->xSpacingLineEdit->setText( QString::number( roiSpacing[0] ) );
    ui->ySpacingLineEdit->setText( QString::number( roiSpacing[1] ) );
    ui->zSpacingLineEdit->setText( QString::number( roiSpacing[2] ) );
}

void FiberObjectSettingsWidget::on_seedPerAxisSlider_valueChanged( int value )
{
    Q_ASSERT( m_pluginInterface );

    m_pluginInterface->SetSeedPerAxis( value );
    UpdateUI();
}

void FiberObjectSettingsWidget::on_stepSizeSlider_valueChanged( int value )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetStepSize( value/10.0 );
    UpdateUI();
}

void FiberObjectSettingsWidget::on_minimumLengthSlider_valueChanged( int value )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetMinimumLength( value );
    UpdateUI();
}

void FiberObjectSettingsWidget::on_maximumLengthSlider_valueChanged( int value )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetMaximumLength( value );
    UpdateUI();
}

void FiberObjectSettingsWidget::on_maximumAngleSlider_valueChanged( int value )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetMaximumAngle( value );
    UpdateUI();
}

void FiberObjectSettingsWidget::on_FAThresholdSlider_valueChanged( int value )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetFAThreshold( value/100.0 );
    UpdateUI();
}

void FiberObjectSettingsWidget::on_InertiaSlider_valueChanged(int value)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetInertia( value/100.0 );
    UpdateUI();
}

void FiberObjectSettingsWidget::on_useTubesCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetUseTubes( checked );
}

void FiberObjectSettingsWidget::on_roiGroupBox_toggled( bool on )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetRoiVisibility( on );
}

void FiberObjectSettingsWidget::on_loadMaximaButton_clicked()
{
    QString lastVisitedDir = Application::GetInstance().GetSettings()->WorkingDirectory;
    QString filePath = QFileDialog::getOpenFileName(0, tr("Open File"), lastVisitedDir, tr("Files (*.nii)"));

    if(!filePath.length())
        return;

    m_pluginInterface->OpenMaximaFile(filePath);
    QString filename = filePath.section("/",-1,-1);
    ui->maximaLineEdit->setText( filename );
    ui->loadFAButton->setDisabled(false);

}


void FiberObjectSettingsWidget::on_loadFAButton_clicked()
{
    QString lastVisitedDir = Application::GetInstance().GetSettings()->WorkingDirectory;
    QString filePath = QFileDialog::getOpenFileName(0, tr("Open File"), lastVisitedDir, tr("Files (*.nii)"));

    if(!filePath.length())
        return;

    m_pluginInterface->OpenFAFile(filePath);
    m_pluginInterface->SetFAMapLoad(true);
    QString filename = filePath.section("/",-1,-1);
    ui->FALineEdit->setText( filename );
    ui->roiGroupBox->setDisabled(false);
}

void FiberObjectSettingsWidget::insertReferenceImage( std::vector< std::pair< int, QString> > images  )
{
    QString qstr("None");
    if(ui->DropDownListReferenceImage->count() == 0)
    {
        ui->DropDownListReferenceImage->addItem( qstr );
        ui->loadMaximaButton->setDisabled(true);
        ui->loadFAButton->setDisabled(true);
    }

    for(int i = 0; i < images.size(); i++)
    {
        bool isPresent = false;
        for(int j = 0; j < m_ReferenceImage.size(); j++)
        {
            isPresent |= (images.at(i).second == m_ReferenceImage.at(j).second);
        }

        if(!isPresent)
        {
            m_ReferenceImage.push_back(images.at(i));
            ui->DropDownListReferenceImage->addItem(images.at(i).second);
            UpdateUI();
        }
    }

}

void FiberObjectSettingsWidget::on_DropDownListReferenceImage_activated(const QString &arg1)
{
    UpdateUI();
}

void FiberObjectSettingsWidget::on_DropDownListReferenceImage_highlighted(int index)
{
    Q_ASSERT( m_pluginInterface );
    if(m_ReferenceImage.size() > 0)
    {
        if(index != 0)
        {
            m_pluginInterface->SetReferenceId(m_ReferenceImage.at(index-1).first);
            ui->DropDownListReferenceImage->setDisabled(false);
            ui->loadMaximaButton->setDisabled(false);
        }
        else
        {
            ui->loadMaximaButton->setDisabled(true);
            ui->loadFAButton->setDisabled(true);
        }
    }
}



void FiberObjectSettingsWidget::on_xCenterLineEdit_textEdited(const QString &arg1)
{
    double xCenter = arg1.toDouble(),
            yCenter = ui->yCenterLineEdit->text().toDouble(),
            zCenter = ui->zCenterLineEdit->text().toDouble(),
            xSpacing = ui->xSpacingLineEdit->text().toDouble(),
            ySpacing = ui->ySpacingLineEdit->text().toDouble(),
            zSpacing = ui->zSpacingLineEdit->text().toDouble();

    m_pluginInterface->SetRoiBounds(xCenter, yCenter, zCenter, xSpacing, ySpacing, zSpacing);
}

void FiberObjectSettingsWidget::on_yCenterLineEdit_textEdited(const QString &arg1)
{
    double xCenter = ui->xCenterLineEdit->text().toDouble(),
            yCenter = arg1.toDouble(),
            zCenter = ui->zCenterLineEdit->text().toDouble(),
            xSpacing = ui->xSpacingLineEdit->text().toDouble(),
            ySpacing = ui->ySpacingLineEdit->text().toDouble(),
            zSpacing = ui->zSpacingLineEdit->text().toDouble();

    m_pluginInterface->SetRoiBounds(xCenter, yCenter, zCenter, xSpacing, ySpacing, zSpacing);
}

void FiberObjectSettingsWidget::on_zCenterLineEdit_textEdited(const QString &arg1)
{
    double xCenter = ui->xCenterLineEdit->text().toDouble(),
            yCenter = ui->yCenterLineEdit->text().toDouble(),
            zCenter = arg1.toDouble(),
            xSpacing = ui->xSpacingLineEdit->text().toDouble(),
            ySpacing = ui->ySpacingLineEdit->text().toDouble(),
            zSpacing = ui->zSpacingLineEdit->text().toDouble();

    m_pluginInterface->SetRoiBounds(xCenter, yCenter, zCenter, xSpacing, ySpacing, zSpacing);
}

void FiberObjectSettingsWidget::on_xSpacingLineEdit_textEdited(const QString &arg1)
{
    double xCenter = ui->xCenterLineEdit->text().toDouble(),
            yCenter = ui->yCenterLineEdit->text().toDouble(),
            zCenter = ui->zCenterLineEdit->text().toDouble(),
            xSpacing = arg1.toDouble(),
            ySpacing = ui->ySpacingLineEdit->text().toDouble(),
            zSpacing = ui->zSpacingLineEdit->text().toDouble();

    m_pluginInterface->SetRoiBounds(xCenter, yCenter, zCenter, xSpacing, ySpacing, zSpacing);
}

void FiberObjectSettingsWidget::on_ySpacingLineEdit_textEdited(const QString &arg1)
{
    double xCenter = ui->xCenterLineEdit->text().toDouble(),
            yCenter = ui->yCenterLineEdit->text().toDouble(),
            zCenter = ui->zCenterLineEdit->text().toDouble(),
            xSpacing = ui->xSpacingLineEdit->text().toDouble(),
            ySpacing = arg1.toDouble(),
            zSpacing = ui->zSpacingLineEdit->text().toDouble();

    m_pluginInterface->SetRoiBounds(xCenter, yCenter, zCenter, xSpacing, ySpacing, zSpacing);
}

void FiberObjectSettingsWidget::on_zSpacingLineEdit_textEdited(const QString &arg1)
{
    double xCenter = ui->xCenterLineEdit->text().toDouble(),
            yCenter = ui->yCenterLineEdit->text().toDouble(),
            zCenter = ui->zCenterLineEdit->text().toDouble(),
            xSpacing = ui->xSpacingLineEdit->text().toDouble(),
            ySpacing = ui->ySpacingLineEdit->text().toDouble(),
            zSpacing = arg1.toDouble();

    m_pluginInterface->SetRoiBounds(xCenter, yCenter, zCenter, xSpacing, ySpacing, zSpacing);
}

void FiberObjectSettingsWidget::on_xCenterLineEdit_editingFinished()
{
    m_pluginInterface->UpdateFibers();
}

void FiberObjectSettingsWidget::on_yCenterLineEdit_editingFinished()
{
    m_pluginInterface->UpdateFibers();
}

void FiberObjectSettingsWidget::on_zCenterLineEdit_editingFinished()
{
    m_pluginInterface->UpdateFibers();
}

void FiberObjectSettingsWidget::on_xSpacingLineEdit_editingFinished()
{
    m_pluginInterface->UpdateFibers();
}

void FiberObjectSettingsWidget::on_ySpacingLineEdit_editingFinished()
{
    m_pluginInterface->UpdateFibers();
}

void FiberObjectSettingsWidget::on_zSpacingLineEdit_editingFinished()
{
    m_pluginInterface->UpdateFibers();
}
