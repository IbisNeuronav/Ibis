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

#ifndef __vtkQtRenderWindow_h_
#define __vtkQtRenderWindow_h_

#include "QVTKWidget.h"

class QGestureEvent;
class QPanGesture;
class QPinchGesture;

class vtkQtRenderWindow : public QVTKWidget
{
    Q_OBJECT

public:

    explicit vtkQtRenderWindow( QWidget * parent = 0 );

    // overloaded paint handler
    virtual void paintEvent(QPaintEvent* event);

    // Control rendering of the window
    void SetRenderingEnabled( bool b ) {m_renderingEnabled = b;}
    
protected:

    bool m_renderingEnabled;
};

#endif
