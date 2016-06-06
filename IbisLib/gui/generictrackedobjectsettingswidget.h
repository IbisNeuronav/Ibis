/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef GENERICTRACKEOBJECTSETTINGSWIDGET_H
#define GENERICTRACKEOBJECTSETTINGSWIDGET_H

#include <QWidget>
#include "hardwaremodule.h"

class GenericTrackedObject;

namespace Ui {
class GenericTrackedObjectSettingsWidget;
}

class GenericTrackedObjectSettingsWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit GenericTrackedObjectSettingsWidget(QWidget *parent = 0);
    ~GenericTrackedObjectSettingsWidget();

    void SetGenericTrackedObject( GenericTrackedObject *obj );

public slots:
    virtual void UpdateToolStatus();

protected:
    GenericTrackedObject *m_genericObject;
    TrackerToolState m_previousStatus;

private:
    Ui::GenericTrackedObjectSettingsWidget *ui;
};

#endif
