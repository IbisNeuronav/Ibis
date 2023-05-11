/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef POINTCOLORWIDGET_H
#define POINTCOLORWIDGET_H

#include <QObject>
#include <QWidget>

#include "pointsobject.h"

namespace Ui
{
class PointColorWidget;
}

class PointColorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PointColorWidget( QWidget * parent = 0 );
    ~PointColorWidget();

public slots:
    virtual void OpacitySliderValueChanged( int );
    virtual void OpacityTextEditChanged( const QString & );
    virtual void DisabledColorSetButtonClicked();
    virtual void EnabledColorSetButtonClicked();
    virtual void SelectedColorSetButtonClicked();
    virtual void LineColorSetButtonClicked();

    void SetPointsObject( PointsObject * obj );

protected:
    void UpdateUI();
    PointsObject * m_points;
    void UpdateOpacityUI();

private:
    Ui::PointColorWidget * ui;
};

#endif  // POINTCOLORWIDGET_H
