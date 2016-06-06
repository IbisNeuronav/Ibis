/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "sceneinfodialog.h"
#include "ui_sceneinfodialog.h"
#include "sceneinfo.h"

SceneInfoDialog::SceneInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SceneInfoDialog)
{
    ui->setupUi(this);
    m_sceneinfo = 0;
}

SceneInfoDialog::~SceneInfoDialog()
{
    delete ui;
}

void SceneInfoDialog::SetSceneInfo(SceneInfo *info)
{
    m_sceneinfo = info;
    if (m_sceneinfo)
    {
        this->ui->patientNameLineEdit->setText( m_sceneinfo->GetPatientName());
        this->ui->patientIdentificationLineEdit->setText( m_sceneinfo->GetPatientIdentification());
        this->ui->surgeonLineEdit->setText( m_sceneinfo->GetSurgeonName());
        this->ui->studentLineEdit->setText( m_sceneinfo->GetStudentName());
        this->ui->commentsTextEdit->setPlainText( m_sceneinfo->GetComment());
    }
}

void SceneInfoDialog::SetSceneDirectory(QString dirName)
{
    this->ui->sceneDirectoryLineEdit->setText(dirName);
    m_sceneDirectory = dirName;
}

void SceneInfoDialog::ButtonBoxAccepted()
{
    if (m_sceneinfo)
    {
        QString tmp;
        tmp = this->ui->patientNameLineEdit->text();
        if (!tmp.isEmpty())
            m_sceneinfo->SetPatientName(tmp);
        tmp = this->ui->patientIdentificationLineEdit->text();
        if (!tmp.isEmpty())
            m_sceneinfo->SetPatientIdentification(tmp);
        tmp = this->ui->surgeonLineEdit->text();
        if (!tmp.isEmpty())
            m_sceneinfo->SetSurgeonName(tmp);
        tmp = this->ui->studentLineEdit->text();
        if (!tmp.isEmpty())
            m_sceneinfo->SetStudentName(tmp);
        tmp = this->ui->commentsTextEdit->toPlainText();
        if (!tmp.isEmpty())
            m_sceneinfo->SetComment(tmp);
    }
    accept();
}
