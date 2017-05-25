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
#include "vtkSmartPointer.h"

class vtkImageData;
class vtkAlgorithmOutput;
class vtkPassThrough;
class vtkMatrix4x4;
class QProgressDialog;
class vtkTransform;
class Serializer;

class TrackedVideoBuffer
{

public:

    TrackedVideoBuffer( int defaultWidth, int defaultHeight );
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

    vtkImageData * GetVideoOutput();
    vtkAlgorithmOutput * GetVideoOutputPort();
    vtkTransform * GetOutputTransform() { return m_outputTransform; }

    bool Serialize( Serializer * ser, QString dataDirectory );
    void Export( QString dirName, QProgressDialog * progress = 0 );
    void Import( QString dirName, QProgressDialog * progress = 0 );

    static void ReadMatrix( QString filename, vtkMatrix4x4 * mat );
    static void WriteMatrix( vtkMatrix4x4 * mat, QString filename );

protected:

    void WriteMatrices( QList< vtkMatrix4x4 * > & matrices, QString dirName );
    void ReadMatrices( QList< vtkMatrix4x4 * > & matrices, QString dirName );
    void WriteImages( QString dirName, QProgressDialog * progressDlg = 0 );
    void ReadImages( int nbImages, QString dirName, QProgressDialog * progressDlg = 0 );

    vtkSmartPointer<vtkImageData> m_videoOutput;
    vtkSmartPointer<vtkPassThrough> m_output;
    vtkSmartPointer<vtkTransform> m_outputTransform;
    int m_currentFrame;
    QList< vtkImageData * > m_frames;
    QList< vtkMatrix4x4 * > m_matrices;

    int m_defaultImageSize[2];
};

#endif
