/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef DTIOBJECTSETTINGSWIDGET_H
#define DTIOBJECTSETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
    class DTIObjectSettingsWidget;
}

class DTIObject;

class DTIObjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:

    explicit DTIObjectSettingsWidget(QWidget *parent = 0);
    virtual ~DTIObjectSettingsWidget();

    void SetDTIObject( DTIObject * object );

protected:

    DTIObject *m_tracks;
    void UpdateUI();

private slots:

    void on_radiusSpinBox_valueChanged(double arg1);
    void on_resolutionSpinBox_valueChanged(int arg1);

    void on_generateTubesCheckBox_toggled(bool checked);

private:

    Ui::DTIObjectSettingsWidget *ui;
};

#endif
