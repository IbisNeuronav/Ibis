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

namespace Ui {
    class PointerObjectSettingsDialog;
}

class PointerObject;
class PointsObject;

class PointerObjectSettingsDialog : public QWidget
{
    Q_OBJECT
public:
    PointerObjectSettingsDialog(QWidget *parent = 0);
    ~PointerObjectSettingsDialog();

    void SetPointer(PointerObject *);
    void SetPointerPickedPointsObject(PointsObject *);

public slots:
    void UpdateSettings();

protected:
    PointerObject *m_pointer;
    PointsObject *m_pointerPickedPointsObject;
    typedef QList <PointsObject*> PointerPickedPointsObjects;
    PointerPickedPointsObjects m_pointerPickedPointsObjectList;

    void UpdatePointSetsComboBox();

private slots:
    virtual void SaveTipPosition();
    virtual void EnableNavigation(bool);
    virtual void NewPointsObject();
    virtual void PointSetsComboBoxActivated(int);

private:
    Ui::PointerObjectSettingsDialog *ui;

};

#endif // POINTEROBJECTSETTINGSDIALOG_H
