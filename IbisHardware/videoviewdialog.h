/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __VideoViewDialog_h_
#define __VideoViewDialog_h_

#include <QWidget>
#include <QObject>

class vtkQtImageViewer;
class TrackedVideoSource;

class VideoViewDialog : public QWidget
{
    Q_OBJECT

public:
    
    VideoViewDialog( QWidget* parent = 0 );
    virtual ~VideoViewDialog();

    vtkQtImageViewer * m_vtkWindow;

    virtual void SetSource( TrackedVideoSource * source );

public slots:
    
    virtual void VideoChanged();

protected:
    
    TrackedVideoSource * m_source;
    int m_currentFrameSize[3];

};

#endif
