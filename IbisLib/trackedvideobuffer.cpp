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
#include "serializer.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkPassThrough.h"
#include "vtkTransform.h"
#include "application.h"

static int DefaultNumberOfScalarComponents = 1;

TrackedVideoBuffer::TrackedVideoBuffer( int w, int h )
{
    m_defaultImageSize[0] = w;
    m_defaultImageSize[1] = h;
    m_currentFrame = -1;
    m_videoOutput = vtkSmartPointer<vtkImageData>::New();
    m_output = vtkSmartPointer<vtkPassThrough>::New();
    m_output->SetInputData( m_videoOutput);
    m_outputTransform = vtkSmartPointer<vtkTransform>::New();
}

TrackedVideoBuffer::~TrackedVideoBuffer()
{
    Clear();
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
    return m_defaultImageSize[0];
}

int TrackedVideoBuffer::GetFrameHeight()
{
    if( m_frames.size() > 0 )
        return m_frames[0]->GetDimensions()[1];
    return m_defaultImageSize[1];
}

int TrackedVideoBuffer::GetFrameNumberOfComponents()
{
    if( m_frames.size() > 0 )
        return m_frames[0]->GetNumberOfScalarComponents();
    return DefaultNumberOfScalarComponents;
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
    m_outputTransform->SetMatrix( GetCurrentMatrix() );
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

vtkAlgorithmOutput * TrackedVideoBuffer::GetVideoOutputPort()
{
    return m_output->GetOutputPort();
}

bool TrackedVideoBuffer::Serialize( Serializer * ser, QString dataDirectory )
{
    ::Serialize( ser, "CurrentFrame", m_currentFrame );

    // If we are writing and dir already exists, sequence has been saved so we can skip writing
    // todo: find smarter way to handle this
    QFileInfo info(dataDirectory);
    bool needsSerialization = !( !ser->IsReader() && info.exists() && info.isDir() );

    if( needsSerialization )
    {
        if( !ser->IsReader() )
            WriteMatrices( m_matrices, dataDirectory );
        else
            ReadMatrices( m_matrices, dataDirectory );

        if( !ser->IsReader() )
            WriteImages( dataDirectory );
        else
            ReadImages( m_matrices.size(), dataDirectory );
    }

    if( ser->IsReader() && m_currentFrame != -1 )
        SetCurrentFrame( m_currentFrame );

    return true;
}

void TrackedVideoBuffer::Export( QString dirName, QProgressDialog * progress )
{
    WriteImages( dirName, progress );
    WriteMatrices( m_matrices, dirName );
}

void TrackedVideoBuffer::Import( QString dirName, QProgressDialog * progress )
{
    ReadMatrices( m_matrices, dirName );
    ReadImages( m_matrices.size(), dirName, progress );
}

#include "vtkXFMReader.h"

void TrackedVideoBuffer::ReadMatrix( QString filename, vtkMatrix4x4 * mat )
{
    vtkXFMReader * reader = vtkXFMReader::New();
    reader->SetFileName( filename.toUtf8().data() );
    reader->SetMatrix( mat );
    reader->Update();
    reader->Delete();
}

void TrackedVideoBuffer::ReadMatrices( QList< vtkMatrix4x4 * > & matrices, QString dirName )
{
    int index = 0;
    bool done = false;
    while( !done )
    {
        QString uncalMatrixFilename = QString("%1/uncalMat_%2.xfm").arg( dirName ).arg( index, 4, 10, QLatin1Char('0') );
        QFileInfo uncalMatInfo( uncalMatrixFilename );
        if( uncalMatInfo.exists() )
        {
            vtkMatrix4x4 * uncalMat = vtkMatrix4x4::New();
            ReadMatrix( uncalMatrixFilename, uncalMat );
            matrices.push_back( uncalMat );
        }
        else
            done = true;
        ++index;
    }
}

#include "vtkXFMWriter.h"

void TrackedVideoBuffer::WriteMatrix( vtkMatrix4x4 * mat, QString filename )
{
    vtkXFMWriter * writer = vtkXFMWriter::New();
    writer->SetFileName( filename.toUtf8().data() );
    writer->SetMatrix( mat );
    writer->Write();
}

void TrackedVideoBuffer::WriteMatrices( QList< vtkMatrix4x4 * > & matrices, QString dirName )
{
    for( int i = 0; i < matrices.size(); ++i )
    {
        QString matrixFilename = dirName + QString("/uncalMat_%1.xfm").arg( i, 4, 10, QLatin1Char('0') );
        WriteMatrix( matrices[i], matrixFilename );
    }
}

#include "vtkPNGWriter.h"

void TrackedVideoBuffer::WriteImages( QString dirName, QProgressDialog * progressDlg )
{
    vtkPNGWriter * writer = vtkPNGWriter::New();
    for( int i = 0; i < m_frames.size(); ++i )
    {
        QString filename = dirName + QString("/frame_%1").arg( i, 4, 10, QLatin1Char('0') );
        writer->SetFileName( filename.toUtf8().data() );
        writer->SetInputData( m_frames[i] );
        writer->Write();

        if( progressDlg )
            Application::GetInstance().UpdateProgress( progressDlg, (int)round( (float)i / m_frames.size() * 100.0 ) );
    }
    writer->Delete();
}

#include "vtkPNGReader.h"

void TrackedVideoBuffer::ReadImages( int nbImages, QString dirName, QProgressDialog * progressDlg )
{
    vtkPNGReader * reader = vtkPNGReader::New();
    for( int i = 0; i < nbImages; ++i )
    {
        QString filename = dirName + QString("/frame_%1").arg( i, 4, 10, QLatin1Char('0') );
        reader->SetFileName( filename.toUtf8().data() );
        reader->Update();
        vtkImageData * image = vtkImageData::New();
        image->DeepCopy( reader->GetOutput() );
        m_frames.push_back( image );

        if( progressDlg )
            Application::GetInstance().UpdateProgress( progressDlg, (int)round( (float)i / nbImages * 100.0 ) );
    }
    reader->Delete();
}

vtkImageData *TrackedVideoBuffer:: GetVideoOutput()
{
    return m_videoOutput;
}

