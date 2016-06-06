/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __UsAcquisitionSettingsWidget_h_
#define __UsAcquisitionSettingsWidget_h_

#include <QWidget>

namespace Ui {
class UsAcquisitionSettingsWidget;
}

class USAcquisitionObject;
class vtkQtMatrixDialog;

class UsAcquisitionSettingsWidget : public QWidget
{

    Q_OBJECT
    
public:

    explicit UsAcquisitionSettingsWidget(QWidget *parent = 0);
    ~UsAcquisitionSettingsWidget();  

    void SetUSAcquisitionObject( USAcquisitionObject * acq );
    
private slots:

    void UpdateUi();
    void OnCalibrationMatrixWidgetClosed();
    void OnClose();

    void on_sliceSlider_valueChanged(int value);
    void on_sliceSpinBox_valueChanged(int arg1);
    void on_opacitySlider_valueChanged(int value);
    void on_staticSlicesGroupBox_toggled(bool arg1);
    void on_nbStaticSlicesSpinBox_valueChanged(int arg1);
    void on_calibrationMatrixButton_toggled(bool checked);
    void on_staticSlicesOpacitySlider_valueChanged(int value);
    void on_staticSlicesColorComboBox_currentIndexChanged(int index);
    void on_currentSliceColorComboBox_currentIndexChanged(int index);
    void on_useMaskCheckBox_toggled(bool checked);
    void on_useDopplerCheckBox_toggled(bool checked); //added function

private:

    USAcquisitionObject * m_acquisitionObject;

    vtkQtMatrixDialog * m_calibrationMatrixWidget;

    Ui::UsAcquisitionSettingsWidget * ui;

};

#endif
