/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef VTKQTPIECEWISEFUNCTIONWIDGET_H
#define VTKQTPIECEWISEFUNCTIONWIDGET_H

#include <QObject>
#include <QWidget>
#include <vector>

class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;
class QColor;

class vtkPiecewiseFunction;

class vtkQtPiecewiseFunctionWidget : public QWidget
{
    Q_OBJECT

public:
    vtkQtPiecewiseFunctionWidget( QWidget * parent );
    ~vtkQtPiecewiseFunctionWidget();
    void SetFunction( vtkPiecewiseFunction * func );
    void SetXRange( double xMin, double xMax );
    void SetYRange( double yMin, double yMax );

public slots:

    void resetGraph();

private:
    void drawGraph( QPainter & painter, const QColor & drawColor );

    void paintEvent( QPaintEvent * event );

    // Handling of mouse events
    void mouseReleaseEvent( QMouseEvent * event );
    void mouseMoveEvent( QMouseEvent * event );
    void mousePressEvent( QMouseEvent * event );
    void mouseDoubleClickEvent( QMouseEvent * event );
    void leaveEvent( QEvent * event );

    void AddPoint( int x, int y );
    void RemovePoint( int index );
    int PointIndexFromPixCoord( int x, int y );
    void GetPixCoordFromNodeCoord( int index, int point[2] );
    void SetNodeCoordFromPixCoord( int x, int y, int index );
    void NodeCoordFromPixCoord( int x, int y, double & xNode, double & yNode );

    static const int m_pointRadius;
    double m_xRange[2];
    double m_yRange[2];
    int m_movingPointIndex;
    vtkPiecewiseFunction * m_function;
};

#endif
