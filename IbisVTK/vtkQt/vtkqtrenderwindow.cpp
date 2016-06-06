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

#include "vtkqtrenderwindow.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharArray.h"
#include <QPainter>
#include <QGestureEvent>
#include <QPanGesture>
#include <QPinchGesture>

vtkQtRenderWindow::vtkQtRenderWindow( QWidget * parent ) : QVTKWidget( parent )
{
    grabGesture( Qt::PanGesture );
    grabGesture( Qt::PinchGesture );
}

bool vtkQtRenderWindow::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));
    return QVTKWidget::event(event);
}

bool vtkQtRenderWindow::gestureEvent( QGestureEvent * event )
{
    if( QGesture *pan = event->gesture(Qt::PanGesture) )
        panTriggered(static_cast<QPanGesture *>(pan));
    if( QGesture *pinch = event->gesture(Qt::PinchGesture) )
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    return true;
}

void vtkQtRenderWindow::panTriggered(QPanGesture *gesture)
{
#ifndef QT_NO_CURSOR
    switch (gesture->state())
    {
    case Qt::GestureStarted:
    case Qt::GestureUpdated:
        setCursor(Qt::SizeAllCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
    }
#endif
    update();
}

void vtkQtRenderWindow::pinchTriggered( QPinchGesture * gesture )
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    /*if (changeFlags & QPinchGesture::RotationAngleChanged)
    {
        qreal value = gesture->property("rotationAngle").toReal();
        qreal lastValue = gesture->property("lastRotationAngle").toReal();
    }*/
    if (changeFlags & QPinchGesture::ScaleFactorChanged)
    {
        qreal value = gesture->property("scaleFactor").toReal();
        qreal lastValue = gesture->property("lastScaleFactor").toReal();
        double scaleFactor = 1.0 + ( value - lastValue );
        emit ZoomView( scaleFactor );
    }
    update();
}

void vtkQtRenderWindow::paintEvent( QPaintEvent* )
{
    if( !this->mRenWin )
        return;

  // if we have a saved image, use it
  if( this->paintCachedImage() )
    return;

  this->mRenWin->Render();

  // In Qt 4.1+ let's support redirected painting
  // if redirected, let's grab the image from VTK, and paint it to the device
  QPaintDevice* device = QPainter::redirected(this);
  if(device != NULL && device != this)
  {
      int w = this->width();
      int h = this->height();
      QImage img(w, h, QImage::Format_RGB32);
      vtkUnsignedCharArray* pixels = vtkUnsignedCharArray::New();
      pixels->SetArray(img.bits(), w*h*4, 1);
      this->mRenWin->GetRGBACharPixelData(0, 0, w-1, h-1, 1, pixels);
      pixels->Delete();
      img = img.rgbSwapped();
      img = img.mirrored();

      QPainter painter(this);
      painter.drawImage(QPointF(0.0,0.0), img);
      return;
  }
}
