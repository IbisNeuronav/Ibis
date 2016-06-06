/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef USPROBEOBJECTSETTINGSWIDGET_H
#define USPROBEOBJECTSETTINGSWIDGET_H

#include <QWidget>

class UsProbeObject;

namespace Ui {
class UsProbeObjectSettingsWidget;
}

class UsProbeObjectSettingsWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit UsProbeObjectSettingsWidget(QWidget *parent = 0);
    ~UsProbeObjectSettingsWidget();

    void SetUsProbeObject( UsProbeObject * probe );

public slots:

    virtual void DepthComboBoxSelectionChanged(int newSelection);
    virtual void UpdateToolStatus();
    virtual void UpdateDepth();
    virtual void BModeRadioButtonClicked();
    virtual void ColorDopplerRadioButtonClicked();
    virtual void PowerDopplerRadioButtonClicked();

protected:

    UsProbeObject * m_usProbeObject;
    void UpdateUI();

private slots:

    void on_useMaskCheckBox_toggled( bool checked );

    void on_colorMapComboBox_currentIndexChanged(int index);

private:

    Ui::UsProbeObjectSettingsWidget * ui;
};

#endif
