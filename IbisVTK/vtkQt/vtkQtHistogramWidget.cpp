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

#include "vtkQtHistogramWidget.h"

#include <math.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageAccumulate.h>
#include <vtkImageData.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <algorithm>

const int vtkQtHistogramWidget::m_hotspotRadius             = 10;
const int vtkQtHistogramWidget::m_scalarBarHeight           = 40;
const int vtkQtHistogramWidget::m_histogramScalarBarSpacing = 10;

vtkQtHistogramWidget::vtkQtHistogramWidget( QWidget * parent )
    : QWidget( parent ),
      m_cursorWidth( 10 ),
      m_cursorHeight( 5 ),
      m_currentCursor( -1 ),
      m_minSliderValue( 0.0 ),
      m_maxSliderValue( 1.0 ),
      m_minSliderMoving( false ),
      m_maxSliderMoving( false ),
      m_midSliderEnabled( false ),
      m_midSliderMoving( false ),
      m_midSliderValue( .5 ),
      m_imageMin( 0.0 ),
      m_imageMax( 1.0 ),
      m_histogram( 0 ),
      m_colorTransferFunction( 0 )
{
    setMouseTracking( true );  // need to track mouse to highlight closest cursor.
}

vtkQtHistogramWidget::~vtkQtHistogramWidget()
{
    if( m_histogram ) m_histogram->UnRegister( 0 );
    if( m_colorTransferFunction ) m_colorTransferFunction->UnRegister( 0 );
}

void vtkQtHistogramWidget::enterEvent( QEvent * event )
{
    Q_UNUSED( event )
    setCursor( Qt::CrossCursor );
    m_currentCursor = MinDistanceCursor( this->cursor().pos().x() );
    update();
}

void vtkQtHistogramWidget::leaveEvent( QEvent * event )
{
    Q_UNUSED( event )
    unsetCursor();
    m_currentCursor = -1;
    update();
}

void vtkQtHistogramWidget::mouseMoveEvent( QMouseEvent * event )
{
    /*if( m_minSliderMoving == false && m_maxSliderMoving == false )
    {
        event->ignore();
        return;
    }*/

    int cursorWindowPos = 0;
    if( event->pos().x() >= rect().width() )
        cursorWindowPos = rect().width() - 1;
    else if( event->pos().x() < 0 )
        cursorWindowPos = 0;
    else
        cursorWindowPos = event->pos().x();

    if( m_minSliderMoving )
        setMinSliderValue( cursorWindowPos );
    else if( m_maxSliderMoving )
        setMaxSliderValue( cursorWindowPos );
    else if( m_midSliderMoving )
        setMidSliderValue( cursorWindowPos );
    else
        m_currentCursor = MinDistanceCursor( event->pos().x() );

    update();

    event->accept();
}

int vtkQtHistogramWidget::MinDistanceCursor( int mousePosition )
{
    std::vector<int> dist;
    dist.reserve( 3 );

    int minPos  = sliderValueToWidgetPos( m_minSliderValue );
    int minDist = abs( minPos - mousePosition );
    dist.push_back( minDist );

    int maxPos  = sliderValueToWidgetPos( m_maxSliderValue );
    int maxDist = abs( maxPos - mousePosition );
    dist.push_back( maxDist );

    if( m_midSliderEnabled )
    {
        int midPos  = sliderValueToWidgetPos( m_midSliderValue );
        int midDist = abs( midPos - mousePosition );
        dist.push_back( midDist );
    }

    int minIndex = std::min_element( dist.begin(), dist.end() ) - dist.begin();
    return minIndex;
}

void vtkQtHistogramWidget::mousePressEvent( QMouseEvent * event )
{
    if( event->button() != Qt::LeftButton )
    {
        event->ignore();
        return;
    }

    m_currentCursor = MinDistanceCursor( event->x() );

    if( m_currentCursor == 0 )
    {
        m_minSliderMoving = true;
        setMinSliderValue( event->x() );
    }
    else if( m_currentCursor == 1 )
    {
        m_maxSliderMoving = true;
        setMaxSliderValue( event->x() );
    }
    else if( m_currentCursor == 2 )
    {
        m_midSliderMoving = true;
        setMidSliderValue( event->x() );
    }

    event->accept();
}

void vtkQtHistogramWidget::mouseReleaseEvent( QMouseEvent * event )
{
    if( event->button() != Qt::LeftButton )
    {
        event->ignore();
        return;
    }

    if( m_minSliderMoving == false && m_maxSliderMoving == false && m_midSliderMoving == false )
    {
        event->ignore();
        return;
    }

    // Test mouse is inside the widget, set value
    if( rect().contains( event->pos() ) )
    {
        if( m_minSliderMoving )
            setMinSliderValue( event->x() );
        else if( m_maxSliderMoving )
            setMaxSliderValue( event->x() );
        else
            setMidSliderValue( event->x() );
    }

    // Stop moving
    m_minSliderMoving = false;
    m_maxSliderMoving = false;
    m_midSliderMoving = false;

    update();

    event->accept();
}

void vtkQtHistogramWidget::paintEvent( QPaintEvent * event )
{
    Q_UNUSED( event )

    QPainter painter( this );
    painter.setRenderHint( QPainter::HighQualityAntialiasing );

    // compute dimensions
    int histHeight = height() - m_cursorHeight - m_scalarBarHeight - m_histogramScalarBarSpacing - 2;

    // Draw histogram background
    QRect histRect( 0, 0, width() - 1, histHeight );
    painter.setBrush( palette().brush( QPalette::Base ) );
    painter.setPen( palette().color( QPalette::Dark ) );
    painter.drawRect( histRect );
    painter.setPen( Qt::NoPen );

    // Draw histogram bars
    if( m_histogram )
    {
        painter.setBrush( palette().brush( QPalette::WindowText ) );
        vtkImageData * hist    = m_histogram->GetOutput();
        int nbBins             = hist->GetDimensions()[0];
        double binWidth        = width() / (double)nbBins;
        double nextBar         = 0.0;
        double maxBinHeight    = log10( hist->GetScalarRange()[1] + 1 );
        double maxBinHeightPix = histHeight * .95;
        double binPixRatio     = maxBinHeightPix / maxBinHeight;
        for( int i = 0; i < nbBins; ++i )
        {
            int binValue     = *( (int *)hist->GetScalarPointer( i, 0, 0 ) );
            double binHeight = log10( binValue + 1 ) * binPixRatio;
            painter.drawRect( QRectF( nextBar, histHeight - binHeight, binWidth, binHeight ) );
            nextBar += binWidth;
        }
    }

    // Draw gradient
    QLinearGradient linearGrad( QPointF( 0, 0 ), QPointF( width(), 0 ) );
    if( m_colorTransferFunction )
    {
        int nbPoints = m_colorTransferFunction->GetSize();
        for( int i = 0; i < nbPoints; ++i )
        {
            double nodeValues[6];
            m_colorTransferFunction->GetNodeValue( i, nodeValues );
            double realNodePos = m_minSliderValue + nodeValues[0] * ( m_maxSliderValue - m_minSliderValue );
            linearGrad.setColorAt( realNodePos, QColor( (int)( nodeValues[1] * 255 ), (int)( nodeValues[2] * 255 ),
                                                        (int)( nodeValues[3] * 255 ) ) );
        }
    }
    else
    {
        linearGrad.setColorAt( m_minSliderValue, QColor( 0, 0, 0 ) );
        linearGrad.setColorAt( m_maxSliderValue, QColor( 255, 255, 255 ) );
    }
    painter.setPen( palette().color( QPalette::Dark ) );
    painter.setBrush( QBrush( linearGrad ) );
    painter.drawRect( 0, histHeight + m_histogramScalarBarSpacing, width() - 1, m_scalarBarHeight );

    QColor inColor  = palette().color( QPalette::WindowText );
    QColor outColor = palette().color( QPalette::Window );
    QColor highlightColor( 255, 0, 0 );

    QColor widgetIn  = inColor;
    QColor widgetOut = outColor;

    // Draw min slider
    if( m_currentCursor == 0 )
    {
        widgetIn  = highlightColor;
        widgetOut = highlightColor;
    }
    else
    {
        widgetIn  = inColor;
        widgetOut = outColor;
    }
    DrawCursor( painter, m_minSliderValue, widgetIn, widgetOut );
    if( m_minSliderMoving ) DrawValue( painter, m_minSliderValue, widgetIn );

    // Draw max slider
    if( m_currentCursor == 1 )
    {
        widgetIn  = highlightColor;
        widgetOut = highlightColor;
    }
    else
    {
        widgetIn  = inColor;
        widgetOut = outColor;
    }
    DrawCursor( painter, m_maxSliderValue, widgetIn, widgetOut );
    if( m_maxSliderMoving ) DrawValue( painter, m_maxSliderValue, widgetIn );

    // Draw mid slider
    if( m_midSliderEnabled )
    {
        if( m_currentCursor == 2 )
        {
            widgetIn  = highlightColor;
            widgetOut = highlightColor;
        }
        else
        {
            widgetIn  = inColor;
            widgetOut = inColor;
        }
        DrawCursor( painter, m_midSliderValue, widgetIn, widgetOut );
        if( m_midSliderMoving ) DrawValue( painter, m_midSliderValue, widgetIn );
    }
}

void vtkQtHistogramWidget::DrawCursor( QPainter & painter, double value, QColor & in, QColor & out )
{
    painter.setBrush( in );
    painter.setPen( out );
    int cursorPosition = sliderValueToWidgetPos( value );
    painter.setBrush( palette().windowText() );
    float bottom = (float)( height() - 1 );
    float top    = (float)( height() - m_cursorHeight - 1 );
    float left   = (float)( cursorPosition - m_cursorWidth * .5 );
    float right  = (float)( cursorPosition + m_cursorWidth * .5 );
    QPolygonF triangle;
    triangle.push_back( QPointF( left, bottom ) );
    triangle.push_back( QPointF( right, bottom ) );
    triangle.push_back( QPointF( float( cursorPosition ), top ) );
    painter.drawPolygon( triangle );
}

void vtkQtHistogramWidget::DrawValue( QPainter & painter, double value, QColor & lineColor )
{
    int cursorPosition = sliderValueToWidgetPos( value );
    int histHeight     = height() - m_cursorHeight - m_scalarBarHeight - m_histogramScalarBarSpacing - 2;
    painter.setPen( lineColor );
    painter.drawLine( cursorPosition, 1, cursorPosition, histHeight - 1 );
    painter.setPen( QPalette::WindowText );
    double histogramX = (double)cursorPosition * ( m_imageMax - m_imageMin ) / width();
    QString valueText = QString( "%1" ).arg( histogramX );
    QRect textRect    = painter.boundingRect( QRect( width() - 60, 10, 50, 30 ), Qt::AlignRight, valueText );
    painter.drawText( textRect, Qt::AlignRight, valueText );
}

void vtkQtHistogramWidget::setMinSliderValue( double val )
{
    if( m_minSliderValue == val ) return;
    m_minSliderValue = val;
    if( m_minSliderValue > m_maxSliderValue ) m_minSliderValue = m_maxSliderValue;
    emit slidersValueChanged( m_minSliderValue, m_maxSliderValue );
    update();
}

void vtkQtHistogramWidget::setMaxSliderValue( double val )
{
    if( m_maxSliderValue == val ) return;
    m_maxSliderValue = val;
    if( m_maxSliderValue < m_minSliderValue ) m_maxSliderValue = m_minSliderValue;
    emit slidersValueChanged( m_minSliderValue, m_maxSliderValue );
    update();
}

void vtkQtHistogramWidget::setMinSliderValue( int val ) { setMinSliderValue( widgetPosToSliderValue( val ) ); }

void vtkQtHistogramWidget::setMaxSliderValue( int val ) { setMaxSliderValue( widgetPosToSliderValue( val ) ); }

void vtkQtHistogramWidget::setMidSliderValue( double value )
{
    m_midSliderValue = value;
    emit midSliderValueChanged( m_midSliderValue );
}
void vtkQtHistogramWidget::setMidSliderValue( int val ) { setMidSliderValue( widgetPosToSliderValue( val ) ); }

void vtkQtHistogramWidget::SetHistogram( vtkImageAccumulate * hist )
{
    if( hist == m_histogram ) return;
    if( m_histogram ) m_histogram->UnRegister( 0 );
    m_histogram = hist;
    if( m_histogram )
    {
        m_histogram->Register( 0 );
    }
    update();
}

void vtkQtHistogramWidget::SetColorTransferFunction( vtkColorTransferFunction * func )
{
    if( func == m_colorTransferFunction ) return;
    if( m_colorTransferFunction ) m_colorTransferFunction->UnRegister( 0 );
    m_colorTransferFunction = func;
    if( m_colorTransferFunction )
    {
        m_colorTransferFunction->Register( 0 );
    }
    update();
}

void vtkQtHistogramWidget::SetImageRange( double min, double max )
{
    Q_ASSERT_X( min < max, "vtkQtHistogramWidget::SetImageRange", "min should be < max" );
    m_imageMin = min;
    m_imageMax = max;
}

double vtkQtHistogramWidget::widgetPosToSliderValue( int widgetPos )
{
    double sliderValue = (double)widgetPos / width();
    return sliderValue;
}

int vtkQtHistogramWidget::sliderValueToWidgetPos( double sliderValue ) { return (int)round( sliderValue * width() ); }
