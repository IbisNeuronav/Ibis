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
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkQtColorTransferFunctionWidget.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <algorithm>
#include <cmath>

const int vtkQtColorTransferFunctionWidget::m_hotspotRadius = 10;

vtkQtColorTransferFunctionWidget::vtkQtColorTransferFunctionWidget( QWidget * parent )
    : QWidget( parent ),
      m_cursorWidth( 10 ),
      m_cursorHeight( 8 ),
      m_useColorCursor( false ),
      m_currentCursor( -1 ),
      m_sliderMoving( false ),
      m_xMin( 0.0 ),
      m_xMax( 1.0 ),
      m_colorTransferFunction( 0 )
{
    setMouseTracking( true );  // need to track mouse to highlight closest cursor.
}

vtkQtColorTransferFunctionWidget::~vtkQtColorTransferFunctionWidget()
{
    if( m_colorTransferFunction ) m_colorTransferFunction->UnRegister( 0 );
}

void vtkQtColorTransferFunctionWidget::enterEvent( QEvent * event )
{
    Q_UNUSED( event )
    setCursor( Qt::CrossCursor );
    UpdateCurrentCursorInformation( this->cursor().pos().x(), this->cursor().pos().y() );
    update();
}

void vtkQtColorTransferFunctionWidget::leaveEvent( QEvent * event )
{
    Q_UNUSED( event )
    unsetCursor();
    m_currentCursor = -1;
    update();
}

void vtkQtColorTransferFunctionWidget::mouseMoveEvent( QMouseEvent * event )
{
    if( m_sliderMoving )
        MouseMove( event->pos().x(), event->pos().y() );
    else
        UpdateCurrentCursorInformation( event->x(), event->y() );

    update();
    event->accept();
}

void vtkQtColorTransferFunctionWidget::mousePressEvent( QMouseEvent * event )
{
    if( event->button() != Qt::LeftButton )
    {
        event->ignore();
        return;
    }

    UpdateCurrentCursorInformation( event->x(), event->y() );

    if( IsInColorCursorZone( event->x(), event->y() ) || IsInRangeCursorZone( event->x(), event->y() ) )
    {
        m_sliderMoving = true;
        MouseMove( event->x(), event->y() );
    }

    event->accept();
}

void vtkQtColorTransferFunctionWidget::mouseReleaseEvent( QMouseEvent * event )
{
    if( event->button() != Qt::LeftButton )
    {
        event->ignore();
        return;
    }

    if( !m_sliderMoving )
    {
        event->ignore();
        return;
    }

    MouseMove( event->x(), event->y() );

    // Stop moving
    m_sliderMoving = false;

    update();

    event->accept();
}

#include <QColorDialog>

void vtkQtColorTransferFunctionWidget::mouseDoubleClickEvent( QMouseEvent * event )
{
    // In Color cursor zone, choose a new color for a point
    if( IsInColorCursorZone( event->x(), event->y() ) )
    {
        int index  = MinDistanceColor( event->x() );
        int pixPos = GetColorNodePixPosition( index );
        if( abs( pixPos - event->x() ) <= m_hotspotRadius )
        {
            // clang-format off
            QColor currentColor = GetColorNodeColor( index );
            QColor newColor     = QColorDialog::getColor( currentColor, nullptr, tr( "Choose Color" ),
                                                          QColorDialog::DontUseNativeDialog );
            if( newColor.isValid() )
            {
                SetColorNodeColor( index, newColor );
            }
            // clang-format on
        }
    }
    // Anywhere else, remove point if close to one and add one if not
    else
    {
        int index  = MinDistanceColor( event->x() );
        int pixPos = GetColorNodePixPosition( index );
        if( abs( pixPos - event->x() ) <= m_hotspotRadius )
        {
            m_colorTransferFunction->RemovePoint( GetColorNodeValue( index ) );
        }
        else
        {
            double value   = widgetPosToSliderValue( event->x() );
            double * color = m_colorTransferFunction->GetColor( value );
            m_colorTransferFunction->AddRGBPoint( value, color[0], color[1], color[2] );
        }
    }
}

void vtkQtColorTransferFunctionWidget::paintEvent( QPaintEvent * event )
{
    Q_UNUSED( event )

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );
    painter.setPen( Qt::NoPen );

    // Draw gradient
    QLinearGradient linearGrad( QPointF( 0, 0 ), QPointF( width(), 0 ) );
    if( m_colorTransferFunction )
    {
        int nbPoints = m_colorTransferFunction->GetSize();
        for( int i = 0; i < nbPoints; ++i )
        {
            QColor currentColor = GetColorNodeColor( i );
            double nodePos      = GetColorNodeValue( i );
            double realNodePos  = ( nodePos - m_xMin ) / ( m_xMax - m_xMin );
            linearGrad.setColorAt( realNodePos, currentColor );
            DrawColorCursor( painter, nodePos, currentColor );
        }
    }
    else
    {
        linearGrad.setColorAt( m_xMin, QColor( 0, 0, 0 ) );
        linearGrad.setColorAt( m_xMax, QColor( 255, 255, 255 ) );
    }
    int scalarBarHeight = height() - 2 * m_cursorHeight;
    painter.setPen( palette().color( QPalette::Dark ) );
    painter.setBrush( QBrush( linearGrad ) );
    painter.drawRect( 0, m_cursorHeight, width() - 1, scalarBarHeight );

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
    DrawCursor( painter, getMinSliderValue(), widgetIn, widgetOut );

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
    DrawCursor( painter, getMaxSliderValue(), widgetIn, widgetOut );
}

void vtkQtColorTransferFunctionWidget::MouseMove( int x, int y )
{
    int cursorWindowPos = 0;
    if( x >= rect().width() )
        cursorWindowPos = rect().width() - 1;
    else if( x < 0 )
        cursorWindowPos = 0;
    else
        cursorWindowPos = x;

    if( m_useColorCursor )
    {
        setColorSliderValue( m_currentCursor, cursorWindowPos );
    }
    else
    {
        if( m_currentCursor == 0 )
            setMinSliderValue( cursorWindowPos );
        else
            setMaxSliderValue( cursorWindowPos );
    }
}

void vtkQtColorTransferFunctionWidget::DrawCursor( QPainter & painter, double value, QColor & in, QColor & out )
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

void vtkQtColorTransferFunctionWidget::DrawColorCursor( QPainter & painter, double value, QColor & c )
{
    painter.setBrush( c );
    painter.setPen( c );
    int cursorPosition = sliderValueToWidgetPos( value );
    float bottom       = (float)( m_cursorHeight );
    float top          = (float)( 0 );
    float left         = (float)( cursorPosition - m_cursorWidth * .5 );
    float right        = (float)( cursorPosition + m_cursorWidth * .5 );
    QPolygonF triangle;
    triangle.push_back( QPointF( left, top ) );
    triangle.push_back( QPointF( float( cursorPosition ), bottom ) );
    triangle.push_back( QPointF( right, top ) );
    painter.drawPolygon( triangle );
}

double vtkQtColorTransferFunctionWidget::getMinSliderValue()
{
    double minSliderValue = m_xMin;

    if( m_colorTransferFunction )
    {
        int nbNodes = m_colorTransferFunction->GetSize();
        double nodeValues[6];
        if( nbNodes > 0 )
        {
            m_colorTransferFunction->GetNodeValue( 0, nodeValues );
            minSliderValue = nodeValues[0];
        }
    }

    return minSliderValue;
}

double vtkQtColorTransferFunctionWidget::getMaxSliderValue()
{
    double maxSliderValue = m_xMax;

    if( m_colorTransferFunction )
    {
        int nbNodes = m_colorTransferFunction->GetSize();
        double nodeValues[6];
        if( nbNodes > 1 )
        {
            m_colorTransferFunction->GetNodeValue( nbNodes - 1, nodeValues );
            maxSliderValue = nodeValues[0];
        }
    }

    return maxSliderValue;
}

void vtkQtColorTransferFunctionWidget::setMinSliderValue( double val )
{
    double prevMin        = getMinSliderValue();
    double minSliderValue = val;
    double maxSliderValue = getMaxSliderValue();
    if( minSliderValue > maxSliderValue - PixelSize() ) minSliderValue = maxSliderValue - PixelSize();
    UpdateColorFunctionRange( prevMin, maxSliderValue, minSliderValue, maxSliderValue );
    emit slidersValueChanged( minSliderValue, maxSliderValue );
    update();
}

void vtkQtColorTransferFunctionWidget::setMaxSliderValue( double val )
{
    double prevMaxSliderVal = getMaxSliderValue();
    double maxSliderValue   = val;
    double minSliderValue   = getMinSliderValue();
    if( maxSliderValue < minSliderValue + PixelSize() ) maxSliderValue = minSliderValue + PixelSize();
    UpdateColorFunctionRange( minSliderValue, prevMaxSliderVal, minSliderValue, maxSliderValue );
    emit slidersValueChanged( minSliderValue, maxSliderValue );
    update();
}

void vtkQtColorTransferFunctionWidget::setColorSliderValue( int index, double val )
{
    double nodeValues[6];
    m_colorTransferFunction->GetNodeValue( index, nodeValues );
    if( index > 0 )
    {
        double prevNodeValues[6];
        m_colorTransferFunction->GetNodeValue( index - 1, prevNodeValues );
        if( val < prevNodeValues[0] + PixelSize() ) val = prevNodeValues[0] + PixelSize();
    }
    if( index < m_colorTransferFunction->GetSize() - 1 )
    {
        double nextNodeValues[6];
        m_colorTransferFunction->GetNodeValue( index + 1, nextNodeValues );
        if( val > nextNodeValues[0] - PixelSize() ) val = nextNodeValues[0] - PixelSize();
    }
    nodeValues[0] = val;
    m_colorTransferFunction->SetNodeValue( index, nodeValues );
}

void vtkQtColorTransferFunctionWidget::UpdateColorFunctionRange( double prevMin, double prevMax, double min,
                                                                 double max )
{
    if( m_colorTransferFunction )
    {
        // first pass: compute new values. Has to be done in 2 passes because SetNodeValue does is sorting points
        int nbPoints       = m_colorTransferFunction->GetSize();
        double * newValues = new double[nbPoints * 6];
        for( int i = 0; i < nbPoints; ++i )
        {
            m_colorTransferFunction->GetNodeValue( i, &( newValues[i * 6] ) );
            newValues[i * 6] = min + ( newValues[i * 6] - prevMin ) / ( prevMax - prevMin ) * ( max - min );
        }

        // second pass, rebuild function
        m_colorTransferFunction->RemoveAllPoints();
        for( int i = 0; i < nbPoints; ++i )
        {
            int index = i * 6;
            m_colorTransferFunction->AddRGBPoint( newValues[index], newValues[index + 1], newValues[index + 2],
                                                  newValues[index + 3] );
        }
        delete[] newValues;
    }
}

void vtkQtColorTransferFunctionWidget::setMinSliderValue( int val )
{
    setMinSliderValue( widgetPosToSliderValue( val ) );
}

void vtkQtColorTransferFunctionWidget::setMaxSliderValue( int val )
{
    setMaxSliderValue( widgetPosToSliderValue( val ) );
}

void vtkQtColorTransferFunctionWidget::setColorSliderValue( int index, int val )
{
    setColorSliderValue( index, widgetPosToSliderValue( val ) );
}

void vtkQtColorTransferFunctionWidget::SetColorTransferFunction( vtkColorTransferFunction * func )
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

void vtkQtColorTransferFunctionWidget::SetXRange( double min, double max )
{
    Q_ASSERT_X( min < max, "vtkQtColorTransferFunctionWidget::SetImageRange", "min should be > max" );
    m_xMin = min;
    m_xMax = max;
}

double vtkQtColorTransferFunctionWidget::widgetPosToSliderValue( int widgetPos )
{
    double sliderValue = m_xMin + (double)widgetPos / width() * ( m_xMax - m_xMin );
    return sliderValue;
}

int vtkQtColorTransferFunctionWidget::sliderValueToWidgetPos( double sliderValue )
{
    return (int)round( ( sliderValue - m_xMin ) / ( m_xMax - m_xMin ) * width() );
}

// returns the size of a screen pixel in the slider coordinate system
double vtkQtColorTransferFunctionWidget::PixelSize() { return ( (double)width() ) / ( m_xMax - m_xMin ); }

int vtkQtColorTransferFunctionWidget::MinDistanceCursor( int mousePosition )
{
    double mouseValue = widgetPosToSliderValue( mousePosition );
    double minDist    = std::abs( getMinSliderValue() - mouseValue );
    double maxDist    = std::abs( getMaxSliderValue() - mouseValue );

    if( minDist < maxDist ) return 0;
    return 1;
}

#include <limits>

int vtkQtColorTransferFunctionWidget::MinDistanceColor( int mousePosition )
{
    int nbPoints    = m_colorTransferFunction->GetSize();
    int minDistance = std::numeric_limits<int>::max();
    int minIndex    = 0;
    for( int i = 0; i < nbPoints; ++i )
    {
        double nodeValues[6];
        m_colorTransferFunction->GetNodeValue( i, nodeValues );
        int pos = sliderValueToWidgetPos( nodeValues[0] );
        if( abs( pos - mousePosition ) < minDistance )
        {
            minDistance = abs( pos - mousePosition );
            minIndex    = i;
        }
    }
    return minIndex;
}

void vtkQtColorTransferFunctionWidget::UpdateCurrentCursorInformation( int x, int y )
{
    if( IsInColorCursorZone( x, y ) )
    {
        m_useColorCursor = true;
        m_currentCursor  = MinDistanceColor( x );
    }
    else
    {
        m_useColorCursor = false;
        m_currentCursor  = MinDistanceCursor( x );
    }
}

bool vtkQtColorTransferFunctionWidget::IsInColorCursorZone( int x, int y )
{
    if( y < m_cursorHeight ) return true;
    return false;
}

bool vtkQtColorTransferFunctionWidget::IsInRangeCursorZone( int x, int y )
{
    if( y > height() - m_cursorHeight ) return true;
    return false;
}

double vtkQtColorTransferFunctionWidget::GetColorNodeValue( int nodeIndex )
{
    Q_ASSERT( nodeIndex < m_colorTransferFunction->GetSize() );
    double nodeValues[6];
    m_colorTransferFunction->GetNodeValue( nodeIndex, nodeValues );
    return nodeValues[0];
}

int vtkQtColorTransferFunctionWidget::GetColorNodePixPosition( int nodeIndex )
{
    Q_ASSERT( nodeIndex < m_colorTransferFunction->GetSize() );
    double nodeValues[6];
    m_colorTransferFunction->GetNodeValue( nodeIndex, nodeValues );
    return sliderValueToWidgetPos( nodeValues[0] );
}

QColor vtkQtColorTransferFunctionWidget::GetColorNodeColor( int nodeIndex )
{
    Q_ASSERT( nodeIndex < m_colorTransferFunction->GetSize() );
    double nodeValues[6];
    m_colorTransferFunction->GetNodeValue( nodeIndex, nodeValues );
    return QColor( (int)( nodeValues[1] * 255 ), (int)( nodeValues[2] * 255 ), (int)( nodeValues[3] * 255 ) );
}

void vtkQtColorTransferFunctionWidget::SetColorNodeColor( int nodeIndex, QColor & color )
{
    Q_ASSERT( nodeIndex < m_colorTransferFunction->GetSize() );
    double nodeValues[6];
    m_colorTransferFunction->GetNodeValue( nodeIndex, nodeValues );
    nodeValues[1] = color.redF();
    nodeValues[2] = color.greenF();
    nodeValues[3] = color.blueF();
    m_colorTransferFunction->SetNodeValue( nodeIndex, nodeValues );
}
