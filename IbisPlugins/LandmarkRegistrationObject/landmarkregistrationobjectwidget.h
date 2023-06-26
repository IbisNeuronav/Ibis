/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef LANDMARKREGISTRATIONOBJECTWIDGET_H
#define LANDMARKREGISTRATIONOBJECTWIDGET_H

#include <QStandardItemModel>
#include <QWidget>
#include "landmarkregistrationobject.h"

namespace Ui
{
class LandmarkRegistrationObjectWidget;
}

class LandmarkRegistrationObject;

class LandmarkRegistrationObjectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LandmarkRegistrationObjectWidget( QWidget * parent = 0, const char * name = 0, Qt::WindowFlags f = 0 );
    ~LandmarkRegistrationObjectWidget();

    void SetLandmarkRegistrationObject( LandmarkRegistrationObject * obj );

public slots:
    void UpdateUI();
    void PointDataChanged( QStandardItem * );

protected:
    LandmarkRegistrationObject * m_registrationObject;

private slots:
    virtual void on_tagTreeView_clicked( QModelIndex idx );
    virtual void on_tagSizeSpinBox_valueChanged( int tagSize );

private:
    QStandardItemModel * m_model;

    Ui::LandmarkRegistrationObjectWidget * ui;
};

#endif  // LANDMARKREGISTRATIONOBJECTWIDGET_H
