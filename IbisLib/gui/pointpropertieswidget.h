/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PointPropertiesrWidget_h_
#define __PointPropertiesrWidget_h_

#include <QWidget>
#include <QObject>

class PointsObject;
class QTreeWidgetItem;

namespace Ui {
    class PointPropertiesWidget;
}

class PointPropertiesWidget : public QWidget
{
    Q_OBJECT

public:

    explicit PointPropertiesWidget(QWidget *parent = 0);
    ~PointPropertiesWidget();

    void SetPointsObject(PointsObject *obj);

public slots:

    void UpdateUI();

private slots:

    virtual void on_labelSizeSpinBox_valueChanged(int);
    virtual void on_pointRadiusSpinBox_valueChanged(int);
    virtual void on_pointRadius2DSpinBox_valueChanged(int);
    virtual void on_showLabelsCheckBox_toggled(bool);
    virtual void on_showDistanceCheckBox_toggled(bool);

    void on_pointsTreeWidget_itemChanged(QTreeWidgetItem *item, int column);
    void on_deletePointButton_clicked();
    void on_pointsTreeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_pointsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

protected:

    PointsObject * m_points;

private:

    Ui::PointPropertiesWidget *ui;
};

#endif
