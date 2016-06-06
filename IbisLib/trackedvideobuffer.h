/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __TrackedVideoBuffer_h_
#define __TrackedVideoBuffer_h_

#include <QList>

class vtkImageData;
class vtkMatrix4x4;

class TrackedVideoBuffer
{

public:

    TrackedVideoBuffer();
    ~TrackedVideoBuffer();

    void Clear();

    // frame properties
    int GetFrameWidth();
    int GetFrameHeight();
    int GetFrameNumberOfComponents();


    void AddFrame( vtkImageData * frame, vtkMatrix4x4 * mat );
    int GetNumberOfFrames() { return m_frames.size(); }

    void SetCurrentFrame( int index );
    int GetCurrentFrame() { return m_currentFrame; }

    vtkMatrix4x4 * GetCurrentMatrix();
    vtkImageData * GetCurrentImage();

    vtkMatrix4x4 * GetMatrix( int index );
    vtkImageData * GetImage( int index );

    vtkImageData * GetVideoOutput() { return m_videoOutput; }

protected:

    vtkImageData * m_videoOutput;
    int m_currentFrame;
    QList< vtkImageData * > m_frames;
    QList< vtkMatrix4x4 * > m_matrices;
};

#endif
