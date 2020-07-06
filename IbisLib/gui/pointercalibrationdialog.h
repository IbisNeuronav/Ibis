/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PointerCalibrationDialog_h_
#define __PointerCalibrationDialog_h_

#include <QObject>
#include "ui_pointercalibrationdialog.h"

class vtkMatrix4x4;
class PointerObject;

class PointerCalibrationDialog : public QWidget, public Ui::PointerCalibrationDialog
{
    Q_OBJECT

public:

    PointerCalibrationDialog( QWidget * parent = 0 );
    virtual ~PointerCalibrationDialog();
    
    void SetPointer( PointerObject * p );

public slots:

    void CalibrateButtonClicked();
    void CancelButtonClicked();
    void Update();

protected:
    
    PointerObject * m_pointer;
};

#endif
