/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "trackedvideobuffer.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"

TrackedVideoBuffer::TrackedVideoBuffer()
{
    m_currentFrame = -1;
    m_videoOutput = vtkImageData::New();
}

TrackedVideoBuffer::~TrackedVideoBuffer()
{
    Clear();
    m_videoOutput->Delete();
}

void TrackedVideoBuffer::Clear()
{
    for( int i = 0; i < m_frames.size(); ++i )
    {
        m_frames[i]->Delete();
        m_matrices[i]->Delete();
    }
    m_frames.clear();
    m_matrices.clear();
    m_currentFrame = -1;
    m_videoOutput->Initialize();
}

int TrackedVideoBuffer::GetFrameWidth()
{
    if( m_frames.size() > 0 )
        return m_frames[0]->GetDimensions()[0];
    return 0;
}

int TrackedVideoBuffer::GetFrameHeight()
{
    if( m_frames.size() > 0 )
        return m_frames[0]->GetDimensions()[1];
    return 0;
}

int TrackedVideoBuffer::GetFrameNumberOfComponents()
{
    if( m_frames.size() > 0 )
        return m_frames[0]->GetNumberOfScalarComponents();
    return 0;
}

void TrackedVideoBuffer::AddFrame( vtkImageData * frame, vtkMatrix4x4 * mat )
{
    vtkImageData * im = vtkImageData::New();
    im->DeepCopy( frame );
    m_frames.push_back( im );

    vtkMatrix4x4 * m = vtkMatrix4x4::New();
    m->DeepCopy( mat );
    m_matrices.push_back( m );

    SetCurrentFrame( m_frames.size() - 1 );
}

void TrackedVideoBuffer::SetCurrentFrame( int index )
{
    Q_ASSERT( index >= 0 && index < m_frames.size() );
    m_currentFrame = index;
    m_videoOutput->ShallowCopy( GetCurrentImage() );
}

vtkMatrix4x4 * TrackedVideoBuffer::GetCurrentMatrix()
{
    Q_ASSERT( m_currentFrame != -1 && m_frames.size() > 0 );
    return m_matrices[ m_currentFrame ];
}

vtkImageData * TrackedVideoBuffer::GetCurrentImage()
{
    Q_ASSERT( m_currentFrame != -1 && m_frames.size() > 0 );
    return m_frames[ m_currentFrame ];
}

vtkMatrix4x4 * TrackedVideoBuffer::GetMatrix( int index )
{
    Q_ASSERT( index >= 0 && index < m_frames.size() );
    return m_matrices[index];
}

vtkImageData * TrackedVideoBuffer::GetImage( int index )
{
    Q_ASSERT( index >= 0 && index < m_frames.size() );
    return m_frames[index];
}
