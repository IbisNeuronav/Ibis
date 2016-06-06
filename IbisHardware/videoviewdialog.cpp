/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "videoviewdialog.h"

#include <QVBoxLayout>
#include "vtkQtImageViewer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage2.h"
#include "vtkImageData.h"
#include "trackedvideosource.h"

VideoViewDialog::VideoViewDialog( QWidget * parent ) : QWidget( parent )
{
    setWindowTitle("Video view");
    this->setContentsMargins( 0, 0, 0, 0 );
    QVBoxLayout * layout  = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_vtkWindow = new vtkQtImageViewer( this );
    m_vtkWindow->setObjectName("m_vtkWindow");
    m_vtkWindow->setMinimumSize( QSize( 640, 480 ) );
    vtkRenderWindowInteractor *interactor = m_vtkWindow->GetInteractor();
    vtkInteractorStyleImage2 *style = vtkInteractorStyleImage2::New();
    interactor->SetInteractorStyle(style);
    style->Delete();
    layout->addWidget( m_vtkWindow );
    resize( QSize(640, 480).expandedTo(minimumSizeHint()) );
	setAttribute(Qt::WA_WState_Polished, false);
    m_currentFrameSize[0] = 640;
    m_currentFrameSize[1] = 480;
    m_currentFrameSize[2] = 1;
    m_source = 0;
}

VideoViewDialog::~VideoViewDialog()
{
}

void VideoViewDialog::SetSource( TrackedVideoSource * source )
{
    if( m_source != source )
    {
        disconnect(m_source);

        m_source = source;

        // watch the modified event of the output of the source
        connect( m_source, SIGNAL( OutputUpdatedSignal() ), this, SLOT( VideoChanged() ), Qt::QueuedConnection );
    }
    if (m_source)
    {
        // connect the video source to the image window
        m_vtkWindow->SetInput( m_source->GetVideoOutput() );
        m_vtkWindow->RenderFirst();
        m_source->GetVideoOutput()->GetDimensions( m_currentFrameSize );
    }
}

void VideoViewDialog::VideoChanged()
{
    int * dims = m_source->GetVideoOutput()->GetDimensions();
    if( dims[0] != m_currentFrameSize[0] || dims[1] != m_currentFrameSize[1] )
    {
        m_vtkWindow->RenderFirst();
        m_currentFrameSize[0] = dims[0];
        m_currentFrameSize[1] = dims[1];
    }
    else
        m_vtkWindow->GetRenderWindow()->Render();
}

