/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef USMASKSETTINGSWIDGET_H
#define USMASKSETTINGSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include "usmask.h"
#include <math.h>

namespace Ui {
class USMaskSettingsWidget;
}

class USMaskSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit USMaskSettingsWidget(QWidget *parent = 0);
    ~USMaskSettingsWidget();
    void SetMask( USMask *mask );
    void DisableSetASDefault();

    inline double to_degrees(double radians) { return radians * (180.0 / M_PI); }
    inline double to_radians(double degrees) { return degrees * (M_PI / 180.0); }

private slots:

    void on_widthSpinBox_valueChanged( int val );
    void on_heightSpinBox_valueChanged( int val );
    void on_topDoubleSpinBox_valueChanged( double val );
    void on_bottomDoubleSpinBox_valueChanged( double val );
    void on_leftDoubleSpinBox_valueChanged( double val );
    void on_rightDoubleSpinBox_valueChanged( double val );
    void on_leftAngleDoubleSpinBox_valueChanged( double val );
    void on_rightAngleDoubleSpinBox_valueChanged( double val );
    void on_originXDoubleSpinBox_valueChanged( double val );
    void on_originYDoubleSpinBox_valueChanged( double val );
    void on_defaultPushButton_clicked( );
    void on_setDefaultPushButton_clicked( );

private:
    Ui::USMaskSettingsWidget *ui;
    USMask *m_mask;

    void UpdateUI();
};

#endif // USMASKSETTINGSWIDGET_H
