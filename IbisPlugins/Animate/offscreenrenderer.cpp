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

#include "offscreenrenderer.h"
#include <QApplication>
#include <QProgressDialog>
#include "animateplugininterface.h"
#include <QDir>
#include <QUrl>
#include <QDateTime>
#include <vtkOpenGLRenderWindow.h>
#include <vtk_glew.h>
#include <vtkImageImport.h>
#include <vtkPNGWriter.h>
#include <vtkRenderer.h>
#include "vtkOffscreenCamera.h"
#include "ibisapi.h"
#include "view.h"
#include "DomeRenderer.h"


OffscreenRenderer::OffscreenRenderer( AnimatePluginInterface * anim )
{
    m_glInit = false;
    m_animate = anim;
    m_backupCam = 0;
    m_cam = 0;
    m_openglRawImage = 0;
    m_renderSize[0] = 1;
    m_renderSize[1] = 1;
    m_fbId = 0;
    m_textureId = 0;
    m_depthId = 0;
}

void OffscreenRenderer::RenderCurrent()
{
    Setup();

    QString snapshotDir = QDir::homePath() + "/IbisRenders/Snapshots/";
    if( !QFile::exists( snapshotDir ) )
    {
        QDir dir;
        dir.mkpath( snapshotDir );
    }

    QString dateAndTime( QDateTime::currentDateTime().toString() );
    QString fileName = QString( "%1/%2.png" ).arg( snapshotDir ).arg( dateAndTime );

    RenderOneFrame( fileName );

    Cleanup();
}

void OffscreenRenderer::RenderAnimation()
{
    Setup();

    QString dateAndTime( QDateTime::currentDateTime().toString() );
    QString animationDir = QDir::homePath() + "/IbisRenders/" + dateAndTime;
    if( !QFile::exists( animationDir ) )
    {
        QDir dir;
        dir.mkpath( animationDir );
    }

    QProgressDialog * progress = new QProgressDialog("Exporting frames", "Cancel", 0, m_animate->GetNumberOfFrames() );
    progress->show();

    int backupCurrentFrame = m_animate->GetCurrentFrame();

    for( int i = 0; i < m_animate->GetNumberOfFrames(); ++i )
    {
        m_animate->SetCurrentFrame( i );
        qApp->processEvents();
        QString filename = QString("%1/frame_%2.png").arg( animationDir ).arg( i, 5, 10, QLatin1Char('0') );
        RenderOneFrame( filename );

        progress->setValue( i + 1 );
        if( progress->wasCanceled() )
            break;
    }

    Cleanup();

    progress->close();
    delete progress;

    m_animate->SetCurrentFrame( backupCurrentFrame );
}

void OffscreenRenderer::Setup()
{
    //-------------------------------
    // Make GL context current
    //-------------------------------
    vtkRenderer *ren = m_animate->GetIbisAPI()->GetMain3DView()->GetRenderer();
    vtkOpenGLRenderWindow * win = vtkOpenGLRenderWindow::SafeDownCast( ren->GetRenderWindow() );
    Q_ASSERT( win );
    win->SetForceMakeCurrent();
    win->MakeCurrent();

    //-------------------------------
    // Setup framebuffer
    //-------------------------------

    // Create and bind frame buffer object
    glGenFramebuffersEXT( 1, &m_fbId );
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_fbId );

    // Create and initialize texture
    glGenTextures( 1, &m_textureId );
    glBindTexture( GL_TEXTURE_2D, m_textureId );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, m_renderSize[0], m_renderSize[1], 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture( GL_TEXTURE_2D, 0 );

    // Attach one of the faces of the Cubemap texture to this FBO
    glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_textureId, 0 );

    // Create and bind depth buffer used for all faces of the cube and attach it to FBO
    glGenRenderbuffersEXT( 1, &m_depthId );
    glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, m_depthId );
    glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, m_renderSize[0], m_renderSize[1] );
    glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, 0 );

    // Attach depth render buffer to the FBO
    glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthId );

    // Check if FBO is complete and config supported by system
    GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
    if( ret != GL_FRAMEBUFFER_COMPLETE_EXT )
    {
        std::cerr << "Render Snapshot: couldn't initialize framebuffer" << std::endl;
    }

    //-------------------------------
    // Setup cameras and delegates
    //-------------------------------
    m_cam = vtkOffscreenCamera::New();
    m_cam->SetRenderSize( m_renderSize );
    m_backupCam = ren->GetActiveCamera();
    m_cam->DeepCopy( m_backupCam );
    ren->SetActiveCamera( m_cam );
    DomeRenderer * domeDelegate = DomeRenderer::SafeDownCast( ren->GetDelegate() );
    if( domeDelegate )
    {
        domeDelegate->SetOverideSize( m_renderSize[0], m_renderSize[1] );
        domeDelegate->SetOverideWindowSize( true );
    }

    // Create objects used to export images
    int numberOfPixelComponents = m_renderSize[0] * m_renderSize[1] * 4;
    m_openglRawImage = new unsigned char[ numberOfPixelComponents ];
    m_importer = vtkImageImport::New();
    m_importer->SetDataScalarTypeToUnsignedChar();
    m_importer->SetNumberOfScalarComponents(4);
    m_importer->SetWholeExtent(0,m_renderSize[0]-1,0,m_renderSize[1]-1,0,0);
    m_importer->SetDataExtentToWholeExtent();
    m_writer = vtkPNGWriter::New();
    m_writer->SetInputConnection( m_importer->GetOutputPort() );
}

void OffscreenRenderer::RenderOneFrame( QString filename )
{
    // Render
    vtkRenderer * ren = m_animate->GetIbisAPI()->GetMain3DView()->GetRenderer();
    ren->Render();

    // Get the raw data from OpenGL
    glBindTexture( GL_TEXTURE_2D, m_textureId );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_openglRawImage );
    glBindTexture( GL_TEXTURE_2D, 0 );

    // Import the data to vtk
    int imageSize = m_renderSize[0] * m_renderSize[1] * 4 * sizeof(unsigned char);
    m_importer->CopyImportVoidPointer( m_openglRawImage, imageSize );

    // Write file
    m_writer->SetFileName( filename.toUtf8().data() );
    m_writer->Write();
}

void OffscreenRenderer::Cleanup()
{
    vtkRenderer * ren = m_animate->GetIbisAPI()->GetMain3DView()->GetRenderer();
    DomeRenderer * domeDelegate = DomeRenderer::SafeDownCast( ren->GetDelegate() );
    if( domeDelegate )
        domeDelegate->SetOverideWindowSize( false );
    ren->SetActiveCamera( m_backupCam );
    m_backupCam = 0;
    m_cam->Delete();
    m_cam = 0;
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

    delete[] m_openglRawImage;
    m_openglRawImage = 0;

    glDeleteTextures( 1, &m_textureId );
    m_textureId = 0;
    glDeleteFramebuffersEXT( 1, &m_fbId );
    m_fbId = 0;
    glDeleteRenderbuffersEXT( 1, &m_depthId );
    m_depthId = 0;
    m_importer->Delete();
    m_importer = 0;
    m_writer->Delete();
    m_writer = 0;
}

