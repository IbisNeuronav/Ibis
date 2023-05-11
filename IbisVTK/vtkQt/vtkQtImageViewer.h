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

#ifndef VTKQTIMAGEVIEWER_H
#define VTKQTIMAGEVIEWER_H

#include <QVTKRenderWidget.h>

#include <QObject>

class vtkImageActor;
class vtkRenderer;
class vtkInteractorStyle;
class vtkImageData;
class QWidget;

class vtkQtImageViewer : public QVTKRenderWidget
{
    Q_OBJECT

public:
    vtkQtImageViewer( QWidget * parent = 0 );
    ~vtkQtImageViewer();

    static vtkQtImageViewer * New();
    void PrintSelf( ostream & os, vtkIndent indent );

    void SetInput( vtkImageData * input );
    void RenderFirst();

private:
    vtkImageActor * m_actor;
    vtkRenderer * m_renderer;
};

#endif
