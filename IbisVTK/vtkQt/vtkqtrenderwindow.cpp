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
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>
#include <QPainter>
#include <QGestureEvent>
#include <QPanGesture>
#include <QPinchGesture>

#ifdef Q_OS_OSX
#include "retinadisplayhack.h"
#endif

vtkQtRenderWindow::vtkQtRenderWindow( QWidget * parent ) : QVTKWidget( parent )
{
    m_renderingEnabled = true;

    // This is a hack to make sure the window behaves properly on retina displays
#ifdef Q_OS_OSX
    disableGLHiDPI( this->winId() );
#endif

}

void vtkQtRenderWindow::paintEvent( QPaintEvent* )
{
  if( !this->mRenWin )
    return;

  // if we have a saved image, use it
  if( this->paintCachedImage() )
    return;

  if( m_renderingEnabled )
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
