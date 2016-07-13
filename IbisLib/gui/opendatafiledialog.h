/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef OPENDATAFILEDIALOG_H
#define OPENDATAFILEDIALOG_H

#include <qstring.h>
#include <QDialog>
class QStringList;
class OpenFileParams;
class SceneManager;

namespace Ui {
    class OpenDataFileDialog;
}

class OpenDataFileDialog : public QDialog
{
    Q_OBJECT

public:

    OpenDataFileDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0, SceneManager * man = 0, OpenFileParams * params = 0 );
    virtual ~OpenDataFileDialog();
    
public slots:

    virtual void BrowsePushButtonClicked();

protected:

    void Update();
    OpenFileParams  *m_fileParams;
    SceneManager * m_sceneManager;

private slots:

    void on_parentComboBox_activated(int index);

    void on_lablelImageCheckBox_toggled(bool checked);

    void on_buttonOk_clicked();

private:

    bool m_isLabel;
    Ui::OpenDataFileDialog *ui;
};

#endif
