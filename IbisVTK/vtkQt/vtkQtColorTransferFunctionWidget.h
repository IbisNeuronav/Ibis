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

#ifndef __vtkQtColorTransferFunctionWidget_h_
#define __vtkQtColorTransferFunctionWidget_h_

#include <QWidget>

class vtkColorTransferFunction;

class vtkQtColorTransferFunctionWidget : public QWidget
{
    Q_OBJECT

public:

    explicit vtkQtColorTransferFunctionWidget(QWidget * parent = 0);
    ~vtkQtColorTransferFunctionWidget();

    double minSliderValue();
    double maxSliderValue();
    void setMinSliderValue( double );
    void setMaxSliderValue( double );
    void setColorSliderValue( int index, double val );

    void SetColorTransferFunction( vtkColorTransferFunction * func );
    void SetXRange( double min, double max );

signals:

    void slidersValueChanged( double minValue, double maxValue );

protected:

    void    mouseMoveEvent(QMouseEvent *event);
    void    mousePressEvent(QMouseEvent *event);
    void    mouseReleaseEvent(QMouseEvent *event);
    void    mouseDoubleClickEvent(QMouseEvent *event);
    void    enterEvent(QEvent *event);
    void    leaveEvent(QEvent *event);
    void    paintEvent(QPaintEvent *event);

private:

    void   MouseMove( int x, int y );
    void   DrawCursor( QPainter & painter, double value, QColor & in, QColor & out );
    void   DrawColorCursor( QPainter & painter, double value, QColor & c );
    void   DrawValue( QPainter & painter, double value, QColor & lineColor );
    double getMinSliderValue();
    double getMaxSliderValue();
    void   UpdateColorFunctionRange( double prevMin, double prevMax, double min, double max );
    void   setMinSliderValue( int );
    void   setMaxSliderValue( int );
    void   setColorSliderValue( int, int );
	double widgetPosToSliderValue( int );
	int    sliderValueToWidgetPos( double );
    double PixelSize();
    int    MinDistanceCursor( int mousePosition );
    int    MinDistanceColor( int mousePosition );
    void   UpdateCurrentCursorInformation( int x, int y );
    bool   IsInColorCursorZone( int x, int y );
    bool   IsInRangeCursorZone( int x, int y );
    double GetColorNodeValue( int nodeIndex );
    int    GetColorNodePixPosition( int nodeIndex );
    QColor GetColorNodeColor( int nodeIndex );
    void   SetColorNodeColor( int nodeIndex, QColor & color );

	int     m_cursorWidth;
	int		m_cursorHeight;

    bool m_useColorCursor;  // is the current cursor a color cursor or a range cursor?
    int m_currentCursor;
    bool    m_sliderMoving; // is any slider moving?

    double m_xMin;
    double m_xMax;

    static const int m_hotspotRadius;

    vtkColorTransferFunction * m_colorTransferFunction;
};

#endif
