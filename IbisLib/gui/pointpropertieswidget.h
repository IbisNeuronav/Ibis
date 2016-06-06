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
#include <QModelIndex>
#include <QStandardItemModel>
#include "pointsobject.h"

class Application;
class vtkTextActor;

namespace Ui {
    class PointPropertiesWidget;
}

class PointPropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PointPropertiesWidget(QWidget *parent = 0);
    ~PointPropertiesWidget();

public slots:
    void UpdateSettings();
    void SetPointsObject(PointsObject *obj);

private slots:
    virtual void on_labelSizeSpinBox_valueChanged(int);
    virtual void on_pointRadiusSpinBox_valueChanged(int);
    virtual void on_pointRadius2DSpinBox_valueChanged(int);
    virtual void on_showLabelsCheckBox_toggled(bool);
    virtual void on_showDistanceCheckBox_toggled(bool);
    virtual void on_pointsTreeView_clicked(QModelIndex);
    virtual void DeletePoint();
    virtual void ClosingSettingsWidget();


protected:
    PointsObject *m_points;
    void UpdateUI();
    int m_current_point_index;
    QStandardItemModel *m_model;
    void contextMenuEvent( QContextMenuEvent * event );

private:
    Ui::PointPropertiesWidget *ui;

    QColor              m_show_distance_button_color;
    Application        *m_application;
};

#endif
