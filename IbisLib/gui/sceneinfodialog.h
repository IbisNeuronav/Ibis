/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef SCENEINFODIALOG_H
#define SCENEINFODIALOG_H

#include <QDialog>
#include <QString>

class SceneInfo;

namespace Ui {
    class SceneInfoDialog;
}

class SceneInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SceneInfoDialog(QWidget *parent = 0);
    ~SceneInfoDialog();

    void SetSceneInfo(SceneInfo * info);
    void SetSceneDirectory(QString);

public slots:
    void ButtonBoxAccepted();

protected:
    SceneInfo *m_sceneinfo;
    QString m_sceneDirectory;

private:
    Ui::SceneInfoDialog *ui;
};

#endif // SCENEINFODIALOG_H
