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

#ifndef __vtkQtHistogramWidget_h_
#define __vtkQtHistogramWidget_h_

#include "QObject"
#include <QWidget>

class vtkImageAccumulate;
class vtkColorTransferFunction;

class vtkQtHistogramWidget : public QWidget
{
    Q_OBJECT

public:

    explicit vtkQtHistogramWidget(QWidget * parent = 0);
    ~vtkQtHistogramWidget();

    double minSliderValue() {return m_minSliderValue;}
    double maxSliderValue() {return m_maxSliderValue;}
    void setMinSliderValue( double );
    void setMaxSliderValue( double );
    void setMidSliderEnabled( bool on ) { m_midSliderEnabled = on; }
    void setMidSliderValue( double value );
    double getMidSliderValue() { return m_midSliderValue; }

    void SetHistogram( vtkImageAccumulate * hist );
    void SetColorTransferFunction( vtkColorTransferFunction * func );
    void SetImageRange( double min, double max );

signals:

    void slidersValueChanged( double minValue, double maxValue );
    void midSliderValueChanged( double value );

protected:

    void    mouseMoveEvent(QMouseEvent *event);
    void    mousePressEvent(QMouseEvent *event);
    void    mouseReleaseEvent(QMouseEvent *event);
    void    enterEvent(QEvent *event);
    void    leaveEvent(QEvent *event);
    void    paintEvent(QPaintEvent *event);

private:

    void   DrawCursor( QPainter & painter, double value, QColor & in, QColor & out );
    void   DrawValue( QPainter & painter, double value, QColor & lineColor );
    void   setMinSliderValue( int );
    void   setMaxSliderValue( int );
    void   setMidSliderValue( int val );
	double widgetPosToSliderValue( int );
	int    sliderValueToWidgetPos( double );
    int    MinDistanceCursor( int mousePosition );

	int     m_cursorWidth;
	int		m_cursorHeight;

    int m_currentCursor;

    double  m_minSliderValue;
    double  m_maxSliderValue;

    bool    m_minSliderMoving;
    bool    m_maxSliderMoving;

    bool    m_midSliderEnabled;
    bool    m_midSliderMoving;
    double  m_midSliderValue;

    double m_imageMin;
    double m_imageMax;

    static const int m_hotspotRadius;
    static const int m_scalarBarHeight;
    static const int m_histogramScalarBarSpacing;

    vtkImageAccumulate * m_histogram;
    vtkColorTransferFunction * m_colorTransferFunction;
};

#endif
