/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef ZOOMFACTORDIALOG_H
#define ZOOMFACTORDIALOG_H

#include "ui_zoomfactordialog.h"

class SceneManager;

class ZoomFactorDialog : public QDialog, public Ui::ZoomFactorDialog
{
    Q_OBJECT

public:
    ZoomFactorDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    ~ZoomFactorDialog();

    void SetSceneManager(SceneManager *m);
    
public slots:
    virtual void OkButtonClicked();

protected:
    SceneManager *m_manager;
};

#endif
