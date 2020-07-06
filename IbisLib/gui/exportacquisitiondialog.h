/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef EXPORTACQUISITIONDIALOG_H
#define EXPORTACQUISITIONDIALOG_H

#include <QDialog>
#include <QString>
#include <QObject>

#include "usacquisitionobject.h"

namespace Ui {
class ExportAcquisitionDialog;
}

class ExportAcquisitionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportAcquisitionDialog(QWidget *parent = 0,  Qt::WindowFlags fl = Qt::WindowStaysOnTopHint );
    ~ExportAcquisitionDialog();

    void SetUSAcquisitionObject( USAcquisitionObject * acq );
    void SetExportParams( ExportParams * params ) { m_params = params; }

private slots:
    void on_browsePushButton_clicked( );
    void on_buttonBox_accepted();

private:
    Ui::ExportAcquisitionDialog *ui;

    USAcquisitionObject * m_acquisitionObject;

    void UpdateUi();
    ExportParams *m_params;
};

#endif // EXPORTACQUISITIONDIALOG_H

/*
QLineEdit *outputDirLineEdit; // read only
QPushButton *browsePushButton;
QCheckBox *maskedFramesCheckBox;
QCheckBox *calibratedFramesCheckBox;
QComboBox *relativeToComboBox;
*/
