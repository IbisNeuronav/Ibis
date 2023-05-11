/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POINTCLOUDOBJECTSETTINGSDIALOG_H
#define POINTCLOUDOBJECTSETTINGSDIALOG_H

#include <QObject>
#include <vector>

#include "ui_pointcloudobjectsettingsdialog.h"

class PointCloudObject;
class UserTransformations;

class PointCloudObjectSettingsDialog : public QWidget, public Ui::PointCloudObjectSettingsDialog
{
    Q_OBJECT

public:
    PointCloudObjectSettingsDialog( QWidget * parent = 0, Qt::WindowFlags fl = 0 );
    virtual ~PointCloudObjectSettingsDialog();

    void SetPointCloudObject( PointCloudObject * object );

public slots:

    virtual void ColorSwatchClicked();
    virtual void OpacitySliderValueChanged( int );
    virtual void OpacityEditTextChanged( const QString & );

    virtual void UpdateSettings();

protected:
    PointCloudObject * m_object;
    QButtonGroup * vertexColorButtonGroup;
    QButtonGroup * displayModeButtonGroup;

    void UpdateUI();
    void UpdateOpacityUI();
};

#endif
