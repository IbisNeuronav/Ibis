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

#include "vtkQtPiecewiseFunctionWidget.h"
#include <vtkPiecewiseFunction.h>

#include <QPainter>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QMouseEvent>

#include <limits>
#include <cmath>

const int vtkQtPiecewiseFunctionWidget::m_pointRadius = 5;

vtkQtPiecewiseFunctionWidget::vtkQtPiecewiseFunctionWidget( QWidget * parent )
    : QWidget( parent )
    , m_function(0)
{
    m_xRange[0] = 0.0;
    m_xRange[1] = 255.0;
    m_yRange[0] = 0.0;
    m_yRange[1] = 255.0;
    m_movingPointIndex = -1;
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setMinimumSize( QSize( 300, 200 ) );
}

vtkQtPiecewiseFunctionWidget::~vtkQtPiecewiseFunctionWidget()
{
    if( m_function )
        m_function->UnRegister( 0 );
}

void vtkQtPiecewiseFunctionWidget::SetFunction( vtkPiecewiseFunction * func )
{
    if( m_function )
        m_function->UnRegister( 0 );
    m_function = func;
    if( m_function )
        m_function->Register( 0 );
}

void vtkQtPiecewiseFunctionWidget::SetXRange( double xMin, double xMax )
{
    m_xRange[0] = xMin;
    m_xRange[1] = xMax;
}

void vtkQtPiecewiseFunctionWidget::SetYRange( double yMin, double yMax )
{
    m_yRange[0] = yMin;
    m_yRange[1] = yMax;
}

void vtkQtPiecewiseFunctionWidget::resetGraph()
{ 
    if( m_function )
    {
        m_function->RemoveAllPoints();
        m_function->AddPoint( m_xRange[0], m_yRange[0] );
        m_function->AddPoint( m_xRange[1], m_yRange[1] );
        update();
    }
}

void vtkQtPiecewiseFunctionWidget::mousePressEvent( QMouseEvent * event )
{
    Q_UNUSED(event)

    m_movingPointIndex = PointIndexFromPixCoord( event->x(), event->y() );
    if( m_movingPointIndex == -1 )
        return;

    SetNodeCoordFromPixCoord( event->x(), event->y(), m_movingPointIndex );

    update();
}

void vtkQtPiecewiseFunctionWidget::mouseReleaseEvent( QMouseEvent * event )
{
    if( m_movingPointIndex == -1 )
        return;

    SetNodeCoordFromPixCoord( event->x(), event->y(), m_movingPointIndex );

    m_movingPointIndex = -1;

    update();
}


void vtkQtPiecewiseFunctionWidget::mouseMoveEvent(QMouseEvent* event)
{
    if( m_movingPointIndex == -1 )
        return;

    SetNodeCoordFromPixCoord( event->x(), event->y(), m_movingPointIndex );

    update();
}

void vtkQtPiecewiseFunctionWidget::mouseDoubleClickEvent ( QMouseEvent * event )
{
    int index = PointIndexFromPixCoord( event->x(), event->y() );

    if( index == -1 )
        AddPoint( event->x(), event->y() );
    else
        RemovePoint( index );

    update();
}

void vtkQtPiecewiseFunctionWidget::leaveEvent(QEvent* event)
{
}

void vtkQtPiecewiseFunctionWidget::paintEvent( QPaintEvent * event )
{
    QPainter p(this);
    p.setRenderHint( QPainter::HighQualityAntialiasing );

    // Draw background
    QRect backRect( 0, 0, width() - 1, height() - 1 );
    p.setBrush( palette().brush( QPalette::Base ) );
    p.setPen( palette().color( QPalette::Dark ) );
    p.drawRect( backRect );
    p.setPen( Qt::NoPen );

    // Draw the graph
    if( m_function )
        drawGraph( p, palette().color( QPalette::WindowText ) );

    p.end();
}


void vtkQtPiecewiseFunctionWidget::drawGraph( QPainter & painter, const QColor & drawColor )
{
    Q_ASSERT_X( m_function, "vtkQtPiecewiseFunctionWidget::drawGraph()", "m_function needs to be defined before calling this" );

    painter.setPen( drawColor );
    painter.setBrush( drawColor );

    // need at least 1 point to draw something
    if( m_function->GetSize() < 1 )
        return;

    int point[2];
    int lastPoint[2];
    GetPixCoordFromNodeCoord( 0, point );
    painter.drawRect( point[0] - m_pointRadius, point[1] - m_pointRadius, 2 * m_pointRadius, 2 * m_pointRadius );

    for( int i = 1; i < m_function->GetSize(); ++i )
    {
        GetPixCoordFromNodeCoord( i, point );
        GetPixCoordFromNodeCoord( i - 1, lastPoint );
        painter.drawLine( lastPoint[0], lastPoint[1], point[0], point[1] );
        painter.drawRect( point[0] - m_pointRadius, point[1] - m_pointRadius, 2 * m_pointRadius, 2 * m_pointRadius );
    }
}

void vtkQtPiecewiseFunctionWidget::AddPoint( int x, int y )
{
    double xNode, yNode;
    NodeCoordFromPixCoord( x, y, xNode, yNode );
    m_function->AddPoint( xNode, yNode );
}

void vtkQtPiecewiseFunctionWidget::RemovePoint( int index )
{
    double point[4];
    m_function->GetNodeValue( index, point );
    m_function->RemovePoint( point[0] );
}

int vtkQtPiecewiseFunctionWidget::PointIndexFromPixCoord( int x, int y )
{
    int pixCoord[2];
    double minDist = std::numeric_limits<double>::max();
    int minIndex = -1;
    for( int i = 0; i < m_function->GetSize(); ++i )
    {
        GetPixCoordFromNodeCoord( i, pixCoord );
        double dist = sqrt( (double) (pow( x - pixCoord[0], 2 ) + pow( y - pixCoord[1], 2 )) );
        if( dist < minDist )
        {
            minDist = dist;
            minIndex = i;
        }
    }
    if( minDist < m_pointRadius + 1 )
        return minIndex;
    return -1;
}

void vtkQtPiecewiseFunctionWidget::GetPixCoordFromNodeCoord( int index, int point[2] )
{
    Q_ASSERT_X( m_function, "vtkQtPiecewiseFunctionWidget::GetPixCoordFromNodeCoord()", "m_function needs to be defined before calling this" );

    double nodeValue[4];
    m_function->GetNodeValue( index, nodeValue );
    point[0] = ( nodeValue[0] - m_xRange[0]) / ( m_xRange[1] - m_xRange[0] ) * width();
    point[1] = ( nodeValue[1] - m_yRange[0]) / ( m_yRange[1] - m_yRange[0] ) * height();
    point[1] = height() - point[1] - 1;
}

void vtkQtPiecewiseFunctionWidget::SetNodeCoordFromPixCoord( int x, int y, int index )
{
    Q_ASSERT_X( m_function, "vtkQtPiecewiseFunctionWidget::SetNodeCoordFromPixCoord()", "m_function needs to be defined before calling this" );

    // convert to node coord
    double newX, newY;
    NodeCoordFromPixCoord( x, y, newX, newY );

    // make sure we don't bypass neighbors
    if( index > 0 )
    {
        double prevCoord[4];
        m_function->GetNodeValue( index - 1, prevCoord );
        if( newX < prevCoord[0] )
            newX = prevCoord[0];
    }
    if( index < m_function->GetSize() - 1 )
    {
        double nextCoord[4];
        m_function->GetNodeValue( index + 1, nextCoord );
        if( newX > nextCoord[0] )
            newX = nextCoord[0];
    }

    double nodeCoord[4];
    nodeCoord[0] = newX;
    nodeCoord[1] = newY;
    nodeCoord[2] = 0.5;
    nodeCoord[3] = 0.0;
    m_function->SetNodeValue( index, nodeCoord );
}

void vtkQtPiecewiseFunctionWidget::NodeCoordFromPixCoord( int x, int y, double & xNode, double & yNode )
{
    xNode = m_xRange[0] + x * ( m_xRange[1] - m_xRange[0] ) / width();
    if( xNode > m_xRange[1] )
        xNode = m_xRange[1];
    if( xNode < m_xRange[0] )
        xNode = m_xRange[0];

    yNode = m_yRange[0] + ( height() - y - 1 ) * ( m_yRange[1] - m_yRange[0] ) / height();
    if( yNode > m_yRange[1] )
        yNode = m_yRange[1];
    if( yNode < m_yRange[0] )
        yNode = m_yRange[0];
}
