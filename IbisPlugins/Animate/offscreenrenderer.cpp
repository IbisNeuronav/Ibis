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
#include "vtkqtrenderwindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkgl.h"
#include "vtkImageImport.h"
#include "vtkPNGWriter.h"
#include "vtkOffscreenCamera.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkRenderer.h"
#include "scenemanager.h"
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
    // Initialize GL and Make GL context current
    //-------------------------------
    vtkOpenGLRenderWindow * win = vtkOpenGLRenderWindow::SafeDownCast( m_animate->GetSceneManager()->GetMain3DView()->GetQtRenderWindow()->GetRenderWindow() );
    Q_ASSERT( win );
    if( !m_glInit )
    {
        LoadGlExtensions( win );
        m_glInit = true;
    }
    win->SetForceMakeCurrent();
    win->MakeCurrent();

    //-------------------------------
    // Setup framebuffer
    //-------------------------------

    // Create and bind frame buffer object
    vtkgl::GenFramebuffersEXT( 1, &m_fbId );
    vtkgl::BindFramebufferEXT( vtkgl::FRAMEBUFFER_EXT, m_fbId );

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
    vtkgl::FramebufferTexture2DEXT( vtkgl::FRAMEBUFFER_EXT, vtkgl::COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_textureId, 0 );

    // Create and bind depth buffer used for all faces of the cube and attach it to FBO
    vtkgl::GenRenderbuffersEXT( 1, &m_depthId );
    vtkgl::BindRenderbufferEXT( vtkgl::RENDERBUFFER_EXT, m_depthId );
    vtkgl::RenderbufferStorageEXT( vtkgl::RENDERBUFFER_EXT, vtkgl::DEPTH_COMPONENT24, m_renderSize[0], m_renderSize[1] );
    vtkgl::BindRenderbufferEXT( vtkgl::RENDERBUFFER_EXT, 0 );

    // Attach depth render buffer to the FBO
    vtkgl::FramebufferRenderbufferEXT( vtkgl::FRAMEBUFFER_EXT, vtkgl::DEPTH_ATTACHMENT_EXT, vtkgl::RENDERBUFFER_EXT, m_depthId );

    // Check if FBO is complete and config supported by system
    GLenum ret = vtkgl::CheckFramebufferStatusEXT( vtkgl::FRAMEBUFFER_EXT );
    if( ret != vtkgl::FRAMEBUFFER_COMPLETE_EXT )
    {
        std::cerr << "Render Snapshot: couldn't initialize framebuffer" << std::endl;
    }

    //-------------------------------
    // Setup cameras and delegates
    //-------------------------------
    m_cam = vtkOffscreenCamera::New();
    m_cam->SetRenderSize( m_renderSize );
    vtkRenderer * ren = m_animate->GetSceneManager()->GetMain3DView()->GetRenderer();
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
    vtkRenderer * ren = m_animate->GetSceneManager()->GetMain3DView()->GetRenderer();
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
    vtkRenderer * ren = m_animate->GetSceneManager()->GetMain3DView()->GetRenderer();
    DomeRenderer * domeDelegate = DomeRenderer::SafeDownCast( ren->GetDelegate() );
    if( domeDelegate )
        domeDelegate->SetOverideWindowSize( false );
    ren->SetActiveCamera( m_backupCam );
    m_backupCam = 0;
    m_cam->Delete();
    m_cam = 0;
    vtkgl::BindFramebufferEXT( vtkgl::FRAMEBUFFER_EXT, 0 );

    delete[] m_openglRawImage;
    m_openglRawImage = 0;

    glDeleteTextures( 1, &m_textureId );
    m_textureId = 0;
    vtkgl::DeleteFramebuffersEXT( 1, &m_fbId );
    m_fbId = 0;
    vtkgl::DeleteRenderbuffersEXT( 1, &m_depthId );
    m_depthId = 0;
    m_importer->Delete();
    m_importer = 0;
    m_writer->Delete();
    m_writer = 0;
}

bool OffscreenRenderer::LoadGlExtensions( vtkOpenGLRenderWindow * win )
{
    vtkOpenGLExtensionManager * man = win->GetExtensionManager();

    bool canLoad = true;
    if( !man->ExtensionSupported("GL_VERSION_2_0") )
    {
        canLoad = false;
        std::cerr << "DomeRenderer: OpenGL 2.0 required but not supported" << std::endl;
    }

    if( !man->ExtensionSupported("GL_EXT_framebuffer_object" ) )
    {
        canLoad = false;
        std::cerr << "DomeRenderer: GL_EXT_framebuffer_object is required but not supported" << std::endl;
    }

    // Really load now that we know everything is supported
    if( canLoad )
    {
        man->LoadExtension( "GL_VERSION_1_2" );
        man->LoadExtension( "GL_VERSION_1_3" );
        man->LoadExtension( "GL_VERSION_2_0" );
        man->LoadExtension( "GL_EXT_framebuffer_object" );
    }

    return canLoad;
}
