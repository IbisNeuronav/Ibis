/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "opendatafiledialog.h"
#include "ui_opendatafiledialog.h"
#include <QFileDialog>
#include "vtkTransform.h"
#include "sceneobject.h"
#include "filereader.h"
#include "scenemanager.h"
    
OpenDataFileDialog::OpenDataFileDialog( QWidget* parent, Qt::WindowFlags fl, SceneManager * man, OpenFileParams *params )
    : QDialog( parent, fl ),
      m_fileParams( params ),
      m_sceneManager( man ),
      ui(new Ui::OpenDataFileDialog)
{
    m_isLabel = false;
    ui->setupUi(this);
    Update();
}

OpenDataFileDialog::~OpenDataFileDialog()
{
}

void OpenDataFileDialog::BrowsePushButtonClicked()
{
    QString filter = tr("All valid files(*.mnc *.mnc2 *.mnc.gz *.MNC *.MNC2 *.MNC.GZ *.nii *.obj *.tag *.vtk *vtp);;Minc file (*.mnc *.mnc2 *.mnc.gz *.MNC *.MNC2 *.MNC.GZ);;Nifti file (*.nii);;Object file (*.obj);;Tag file (*.tag);;VTK file (*.vtk);;VTP file (*.vtp)");
    QStringList inputFiles = QFileDialog::getOpenFileNames( this, tr("Open Files"), m_fileParams->lastVisitedDir, filter );
    for( int i = 0; i < inputFiles.size(); ++i )
    {
        OpenFileParams::SingleFileParam param;
        param.fileName = inputFiles[i];
        m_fileParams->filesParams.push_back( param );
    }

    Update();
}

void OpenDataFileDialog::on_parentComboBox_activated( int index )
{
    QVariant v = ui->parentComboBox->itemData( index );
    bool ok = false;
    int objectId = v.toInt( &ok );
    Q_ASSERT_X( ok, "OpenDataFileDialog::on_parentComboBox_activated()", "Invalid object id in combo box item." );
    SceneObject * parent = m_sceneManager->GetObjectByID( objectId );
    Q_ASSERT_X( parent, "OpenDataFileDialog::on_parentComboBox_activated()", "Invalid parent" );
    m_fileParams->defaultParent = parent;
}

void OpenDataFileDialog::Update()
{
    // list of files in the textbox
    QString tmp;
    for( int i = 0; i < m_fileParams->filesParams.size(); ++i )
    {
        QString filename = m_fileParams->filesParams[i].fileName;
        tmp += filename + " ";
    }
    ui->openFileLineEdit->setText(tmp);

    // Fill the parent combo box
    QList<SceneObject*> allObjects;
    m_sceneManager->GetAllListableNonTrackedObjects( allObjects );
    ui->parentComboBox->addItem( m_sceneManager->GetSceneRoot()->GetName(), m_sceneManager->GetSceneRoot()->GetObjectID()  );
    int index = 1;
    for( int i = 0; i < allObjects.size(); ++i )
    {
        if( allObjects[i]->CanAppendChildren() )
        {
            ui->parentComboBox->addItem( allObjects[i]->GetName(), QVariant( allObjects[i]->GetObjectID() ) );
            if( allObjects[i] == m_fileParams->defaultParent )
                ui->parentComboBox->setCurrentIndex( index );
            ++index;
        }
    }

    // update label file checkbox
    ui->lablelImageCheckBox->setChecked( m_isLabel );
}

void OpenDataFileDialog::on_lablelImageCheckBox_toggled(bool checked)
{
    m_isLabel = checked;
}

void OpenDataFileDialog::on_buttonOk_clicked()
{
    for( int i = 0; i < m_fileParams->filesParams.size(); ++i )
    {
        m_fileParams->filesParams[i].isLabel = m_isLabel;
    }
    accept();
}
