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

#include "vtkQtImageViewer.h"

#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

vtkQtImageViewer::vtkQtImageViewer( QWidget * parent ) : QVTKRenderWidget( parent )
{
    m_actor = vtkImageActor::New();
    m_actor->InterpolateOff();

    m_renderer = vtkRenderer::New();
    m_renderer->AddViewProp( m_actor );

    GetRenderWindow()->AddRenderer( m_renderer );

    vtkInteractorStyleImage * interactorStyle = vtkInteractorStyleImage::New();
    GetInteractor()->SetInteractorStyle( interactorStyle );
    interactorStyle->Delete();
}

vtkQtImageViewer::~vtkQtImageViewer()
{
    m_actor->Delete();
    m_renderer->Delete();
}

void vtkQtImageViewer::SetInput( vtkImageData * input ) { m_actor->SetInputData( input ); }

void vtkQtImageViewer::RenderFirst()
{
    m_actor->Update();
    int * extent  = m_actor->GetInput()->GetExtent();
    int diffx     = extent[1] - extent[0] + 1;
    double scalex = (double)diffx / 2.0;
    int diffy     = extent[3] - extent[2] + 1;
    double scaley = (double)diffy / 2.0;

    vtkCamera * cam = m_renderer->GetActiveCamera();
    cam->ParallelProjectionOn();
    cam->SetParallelScale( scaley );
    double * prevPos   = cam->GetPosition();
    double * prevFocal = cam->GetFocalPoint();
    cam->SetPosition( scalex, scaley, prevPos[2] );
    cam->SetFocalPoint( scalex, scaley, prevFocal[2] );

    GetRenderWindow()->Render();
}

vtkQtImageViewer * vtkQtImageViewer::New() { return new vtkQtImageViewer; }

void vtkQtImageViewer::PrintSelf( ostream & os, vtkIndent indent ) {}
