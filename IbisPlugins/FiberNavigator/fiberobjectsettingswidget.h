/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Francois Rheau for writing this class

#ifndef __FiberObjectSettingsWidget_h_
#define __FiberObjectSettingsWidget_h_

#include <QWidget>

namespace Ui {
    class FiberObjectSettingsWidget;
}

class FiberNavigatorPluginInterface;

class FiberObjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:

    explicit FiberObjectSettingsWidget( QWidget * parent = 0 );
    virtual ~FiberObjectSettingsWidget();

    void SetPluginInterface( FiberNavigatorPluginInterface * interf );

public slots:

    void UpdateUI();

protected:

    FiberNavigatorPluginInterface * m_pluginInterface;

private slots:

    void on_seedPerAxisSlider_valueChanged(int value);
    void on_stepSizeSlider_valueChanged(int value);
    void on_minimumLengthSlider_valueChanged(int value);
    void on_maximumLengthSlider_valueChanged(int value);
    void on_maximumAngleSlider_valueChanged(int value);
    void on_FAThresholdSlider_valueChanged(int value);

    void on_useTubesCheckBox_toggled(bool checked);

    void on_roiGroupBox_toggled(bool arg1);
    void on_loadMaximaButton_clicked();
    void on_loadFAButton_clicked();
    void insertReferenceImage( std::vector< std::pair< int, QString> >  );

    void on_DropDownListReferenceImage_activated(const QString &arg1);

    void on_DropDownListReferenceImage_highlighted(int index);

    void on_InertiaSlider_valueChanged(int value);

    void on_xCenterLineEdit_textEdited(const QString &arg1);

    void on_yCenterLineEdit_textEdited(const QString &arg1);

    void on_zCenterLineEdit_textEdited(const QString &arg1);

    void on_xSpacingLineEdit_textEdited(const QString &arg1);

    void on_ySpacingLineEdit_textEdited(const QString &arg1);

    void on_zSpacingLineEdit_textEdited(const QString &arg1);

    void on_xCenterLineEdit_editingFinished();

    void on_yCenterLineEdit_editingFinished();

    void on_zCenterLineEdit_editingFinished();

    void on_xSpacingLineEdit_editingFinished();

    void on_ySpacingLineEdit_editingFinished();

    void on_zSpacingLineEdit_editingFinished();

private:
    std::vector< std::pair< int, QString> > m_ReferenceImage;
    Ui::FiberObjectSettingsWidget * ui;
};

#endif
