/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "exportacquisitiondialog.h"
#include "ui_exportacquisitiondialog.h"
#include "scenemanager.h"
#include <QFileDialog>
#include <QDir>

ExportAcquisitionDialog::ExportAcquisitionDialog(QWidget *parent, Qt::WindowFlags fl) :
    QDialog(parent,fl),
    ui(new Ui::ExportAcquisitionDialog)
{
    ui->setupUi(this);
    m_acquisitionObject = 0;
    m_params = 0;
}

ExportAcquisitionDialog::~ExportAcquisitionDialog()
{
    delete ui;
}


void ExportAcquisitionDialog::SetUSAcquisitionObject( USAcquisitionObject * acq )
{
    if( m_acquisitionObject == acq )
        return;

    m_acquisitionObject = acq;
    if( m_acquisitionObject )
        this->UpdateUi();
}

void ExportAcquisitionDialog::on_browsePushButton_clicked( )
{
    Q_ASSERT( m_acquisitionObject );
    QString outputDir = QFileDialog::getExistingDirectory( this, "Output Folder", m_acquisitionObject->GetManager()->GetSceneDirectory(),  QFileDialog::DontUseNativeDialog );
    ui->outputDirLineEdit->setText( outputDir );
}

void ExportAcquisitionDialog::on_buttonBox_accepted()
{
    Q_ASSERT( m_acquisitionObject );
    Q_ASSERT( m_params );
    m_params->masked = ui->maskedFramesCheckBox->isChecked();
    m_params->useCalibratedTransform = ui->calibratedFramesCheckBox->isChecked();
    m_params->outputDir = ui->outputDirLineEdit->text();
    QVariant v = ui->relativeToComboBox->itemData( ui->relativeToComboBox->currentIndex() );
    m_params->relativeToID = v.toInt();
    accept();
}

void ExportAcquisitionDialog::UpdateUi()
{
    Q_ASSERT( m_acquisitionObject );
    QList<SceneObject*> objects;
    m_acquisitionObject->GetManager()->GetAllUserObjects( objects );

    ui->relativeToComboBox->blockSignals( true );
    ui->relativeToComboBox->clear();

    ui->relativeToComboBox->addItem( "None", QVariant( SceneManager::InvalidId ));
    for( int i = 0; i < objects.size(); ++i )
    {
        ui->relativeToComboBox->addItem( objects[i]->GetName(), QVariant( objects[i]->GetObjectID() ) );
    }
    ui->relativeToComboBox->blockSignals( false );
}

