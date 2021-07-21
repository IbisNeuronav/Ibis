/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TRACTOGRAMOBJECTSETTINGSDIALOG_H
#define TRACTOGRAMOBJECTSETTINGSDIALOG_H

#include <vector>
#include <QObject>
#include "ui_tractogramobjectsettingsdialog.h"

class PolyDataObject;
class TractogramObject;
class UserTransformations;

namespace Ui {
class TractogramObjectSettingsDialog;
}


class TractogramObjectSettingsDialog : public QWidget, public Ui::TractogramObjectSettingsDialog
{
    Q_OBJECT

public:
    
    TractogramObjectSettingsDialog( QWidget * parent = 0, Qt::WindowFlags fl = 0 );
    virtual ~TractogramObjectSettingsDialog();

    void SetTractogramObject( TractogramObject * object );

public slots:
    
    virtual void ColorSwatchClicked();
    virtual void VertexColorModeChanged( int );
    virtual void DisplayModeChanged( int mode );
    virtual void OpacitySliderValueChanged(int);
    virtual void OpacityEditTextChanged( const QString & );
    virtual void CrossSectionCheckBoxToggled(bool);
    virtual void UpdateSettings();

protected:
        
    TractogramObject * m_object;
    QButtonGroup * vertexColorButtonGroup;
    QButtonGroup * displayModeButtonGroup;

    void UpdateUI();
    void UpdateOpacityUI();

private slots:

    void on_sampleVolumeComboBox_currentIndexChanged(int index);
    void on_vertexColorGroupBox_toggled(bool arg1);
    void on_lutComboBox_currentIndexChanged(int index);
    void on_xpRadioButton_toggled(bool checked);
    void on_ypRadioButton_toggled(bool checked);
    void on_zpRadioButton_toggled(bool checked);
    void on_clippingGroupBox_toggled(bool arg1);
};

#endif
