/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POLYDATAOBJECTSETTINGSDIALOG_H
#define POLYDATAOBJECTSETTINGSDIALOG_H

#include <vector>
#include <QObject>
#include "ui_polydataobjectsettingsdialog.h"

class PolyDataObject;
class UserTransformations;

class PolyDataObjectSettingsDialog : public QWidget, public Ui::PolyDataObjectSettingsDialog
{
    Q_OBJECT

public:
    
    PolyDataObjectSettingsDialog( QWidget * parent = 0, Qt::WindowFlags fl = 0 );
    virtual ~PolyDataObjectSettingsDialog();

    void SetPolyDataObject( PolyDataObject * object );

public slots:
    
    virtual void ColorSwatchClicked();
    virtual void VertexColorModeChanged( int );
    virtual void DisplayModeChanged( int mode );
    virtual void OpacitySliderValueChanged(int);
    virtual void OpacityEditTextChanged( const QString & );
    virtual void CrossSectionCheckBoxToggled(bool);
    virtual void UpdateSettings();

protected:
        
    PolyDataObject * m_object;
    QButtonGroup * vertexColorButtonGroup;
    QButtonGroup * displayModeButtonGroup;

    void UpdateUI();
    void UpdateOpacityUI();

private slots:

    void on_clearTextureImageButton_clicked();
    void on_textureImageLineEdit_textChanged(QString );
    void on_textureImageBrowseButton_clicked();
    void on_showTextureCheckBox_toggled(bool checked);
    void on_sampleVolumeComboBox_currentIndexChanged(int index);
    void on_vertexColorGroupBox_toggled(bool arg1);
    void on_lutComboBox_currentIndexChanged(int index);
    void on_xpRadioButton_toggled(bool checked);
    void on_ypRadioButton_toggled(bool checked);
    void on_zpRadioButton_toggled(bool checked);
    void on_clippingGroupBox_toggled(bool arg1);
};

#endif
