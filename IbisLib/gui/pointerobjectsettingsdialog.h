/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POINTEROBJECTSETTINGSDIALOG_H
#define POINTEROBJECTSETTINGSDIALOG_H

#include <QWidget>
#include <QList>
#include <QObject>
#include <vtkSmartPointer.h>

class vtkQtMatrixDialog;

namespace Ui {
    class PointerObjectSettingsDialog;
}

class PointerObject;
class PointsObject;
class PointerCalibrationDialog;

class PointerObjectSettingsDialog : public QWidget
{
    Q_OBJECT
public:
    PointerObjectSettingsDialog(QWidget *parent = 0);
    virtual ~PointerObjectSettingsDialog();

    void SetPointer(PointerObject *);
    void SetPointerPickedPointsObject(vtkSmartPointer<PointsObject>);

public slots:
    void UpdateUI();

protected:
    PointerObject *m_pointer;
    vtkSmartPointer<PointsObject> m_pointerPickedPointsObject;

    void UpdatePointSetsComboBox();

private slots:

    virtual void on_savePositionPushButton_clicked();
    virtual void on_newPointsObjectPushButton_clicked();
    virtual void on_pointSetsComboBox_activated(int);
    virtual void on_calibrateTipButton_toggled(bool checked);
    virtual void on_calibrationMatrixPushButton_toggled( bool on );
    virtual void OnTipCalibrationDialogClosed();
    void OnCalibrationMatrixDialogClosed();

private:

    vtkQtMatrixDialog * m_matrixDialog;

    Ui::PointerObjectSettingsDialog *ui;

    PointerCalibrationDialog *  m_tipCalibrationWidget;

};

#endif // POINTEROBJECTSETTINGSDIALOG_H
