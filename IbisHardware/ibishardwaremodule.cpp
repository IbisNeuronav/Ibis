/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "ibishardwaremodule.h"
#include "application.h"
#include "trackedvideosource.h"
#include "trackertoolsdisplaymanager.h"
#include "scenemanager.h"
#include "vtkTracker.h"
#include "vtkTrackerTool.h"
#include "vtkTransform.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"
#include "generictrackedobject.h"
#include <QDir>
#include <QMenu>
#include <QVBoxLayout>

//Q_EXPORT_STATIC_PLUGIN2( IbisHardware, IbisHardwareModule );

IbisHardwareModule::IbisHardwareModule()
{
    m_tracker = Tracker::New();
    m_trackedVideoSource = TrackedVideoSource::New();
    m_toolDisplayManager = TrackerToolDisplayManager::New();
}

IbisHardwareModule::~IbisHardwareModule()
{
    m_toolDisplayManager->Delete();
    m_tracker->Delete();
    m_trackedVideoSource->Delete();
}

void IbisHardwareModule::AddSettingsMenuEntries( QMenu * menu )
{
    menu->addAction( tr("&Video Settings"), this, SLOT( OpenVideoSettingsDialog() ), QKeySequence("Ctrl+v") );
    menu->addAction( tr("&Tracker Settings"), this, SLOT( OpenTrackerSettingsDialog() ), QKeySequence("Ctrl+t"));
}

QWidget * IbisHardwareModule::CreateTrackerStatusDialog( QWidget * parent )
{
    return m_tracker->CreateStatusDialog( parent );
}

// Implementation of the HardwareModule interface
bool IbisHardwareModule::Init( const char * configfilename )
{
    // Create the tracker if it doesn't exist. Otherwise, make sure tracking is stopped
    m_tracker->StopTracking();

    // Read hardware configuration if we have a valid filename
    QString configName( configfilename );
    QFileInfo configFileInfo( configName );
    if( configFileInfo.exists() && configFileInfo.isReadable() )
        ReadHardwareConfig( configfilename );

    // Init tracker
    m_tracker->Initialize();

    // Init video
    m_trackedVideoSource->SetTracker( m_tracker );
    m_trackedVideoSource->InitializeVideo();

    // Init display manager
    m_toolDisplayManager->Settracker( m_tracker );
    m_toolDisplayManager->SetvideoSource( m_trackedVideoSource );
    m_toolDisplayManager->SetSceneManager( m_application->GetSceneManager() );
    m_toolDisplayManager->Initialize();

    // simtodo : don't always return true
    return true;
}

void IbisHardwareModule::Update()
{
    // update the matrices of all tracked objects and the video images
    m_trackedVideoSource->UnlockUpdates();
    m_tracker->Update(); // tracked video update will only update the tool it is using.
    m_trackedVideoSource->Update();
    m_trackedVideoSource->LockUpdates();
}

bool IbisHardwareModule::ShutDown()
{
    m_toolDisplayManager->ShutDown();

    m_trackedVideoSource->RemoveClient();
    m_tracker->StopTracking();

    // simtodo : don't always return true
    return true;
}

void IbisHardwareModule::AddObjectsToScene()
{
    m_toolDisplayManager->AddAllObjectsToScene();
}

void IbisHardwareModule::RemoveObjectsFromScene()
{
    m_toolDisplayManager->RemoveAllObjectsFromScene();
}

bool IbisHardwareModule::CanCaptureTrackedVideo()
{
    if( !m_tracker )
        return false;
    if( !m_tracker->IsInitialized() )
        return false;
    return CanCaptureVideo();
}

bool IbisHardwareModule::CanCaptureVideo()
{
    if( !m_trackedVideoSource )
        return false;
    return true;
}

int IbisHardwareModule::GetNavigationPointerObjectID()
{
    if( !m_tracker )
        return SceneObject::InvalidObjectId;
    int navPointerIndex = m_tracker->GetNavigationPointerIndex();
    if( navPointerIndex == -1 )
        return SceneObject::InvalidObjectId;
    return m_tracker->GetToolObjectId( navPointerIndex );
}

vtkImageData * IbisHardwareModule::GetTrackedVideoOutput()
{
    return m_trackedVideoSource->GetVideoOutput();
}

int IbisHardwareModule::GetVideoFrameWidth()
{
    return m_trackedVideoSource->GetFrameWidth();
}

int IbisHardwareModule::GetVideoFrameHeight()
{
    return m_trackedVideoSource->GetFrameHeight();
}

TrackerToolState IbisHardwareModule::GetVideoTrackerState()
{
    return m_trackedVideoSource->GetState();
}

vtkTransform * IbisHardwareModule::GetTrackedVideoTransform()
{
    return m_trackedVideoSource->GetTransform();
}

vtkTransform * IbisHardwareModule::GetTrackedVideoUncalibratedTransform()
{
    return m_trackedVideoSource->GetUncalibratedTransform();
}

vtkMatrix4x4 * IbisHardwareModule::GetVideoCalibrationMatrix()
{
    return m_trackedVideoSource->GetCurrentCalibrationMatrix();
}

void IbisHardwareModule::SetVideoCalibrationMatrix( vtkMatrix4x4 * mat )
{
    m_trackedVideoSource->SetCurrentCalibrationMatrix( mat );
}

int IbisHardwareModule::GetNumberOfVideoCalibrationMatrices()
{
    return m_trackedVideoSource->GetNumberOfCalibrationMatrices();
}

QString IbisHardwareModule::GetVideoCalibrationMatrixName( int index )
{
    return m_trackedVideoSource->GetCalibrationMatrixName( index );
}

void IbisHardwareModule::SetCurrentVideoCalibrationMatrixName( QString name )
{
    m_trackedVideoSource->SetCurrentCalibrationMatrixName( name );
}

QString IbisHardwareModule::GetCurrentVideoCalibrationMatrixName()
{
    return m_trackedVideoSource->GetCurrentCalibrationMatrixName();
}

bool IbisHardwareModule::IsVideoTransformFrozen()
{
    return m_trackedVideoSource->IsTransformFrozen();
}

void IbisHardwareModule::FreezeVideoTransform( int nbSamples )
{
    m_trackedVideoSource->FreezeTransform( nbSamples );
}

void IbisHardwareModule::UnFreezeVideoTransform()
{
    m_trackedVideoSource->UnFreezeTransform();
}

const CameraIntrinsicParams & IbisHardwareModule::GetCameraIntrinsicParams()
{
    return m_trackedVideoSource->GetCameraIntrinsicParams();
}

void IbisHardwareModule::SetCameraIntrinsicParams( CameraIntrinsicParams & p )
{
    m_trackedVideoSource->SetCameraIntrinsicParams( p );
}

void IbisHardwareModule::AddTrackedVideoClient()
{
    m_trackedVideoSource->AddClient();
}

void IbisHardwareModule::RemoveTrackedVideoClient()
{
    m_trackedVideoSource->RemoveClient();
}

int IbisHardwareModule::GetReferenceToolIndex()
{
    return m_tracker->GetReferenceToolIndex();
}

vtkTransform * IbisHardwareModule::GetTrackerToolTransform( int toolIndex )
{
    vtkTrackerTool * tool = m_tracker->GetTool( toolIndex );
    Q_ASSERT( tool );
    return tool->GetTransform();
}

TrackerToolState IbisHardwareModule::GetTrackerToolState( int toolIndex )
{
    Q_ASSERT( m_tracker );

    vtkTrackerTool * tool = m_tracker->GetTool( toolIndex );
    Q_ASSERT( tool );

    int flags = tool->GetFlags();
    if( flags & TR_MISSING ) return Missing;
    if( flags & TR_OUT_OF_VIEW ) return OutOfView;
    if( flags & TR_OUT_OF_VOLUME ) return OutOfVolume;
    if( flags == -1 ) return Undefined;
    return Ok;
}

void IbisHardwareModule::OpenVideoSettingsDialog()
{
    QDialog * dialog = new QDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle( "Video Settings" );
    QVBoxLayout * layout = new QVBoxLayout( dialog );
    QWidget * dlg = m_trackedVideoSource->CreateVideoSettingsDialog( dialog );
    layout->addWidget( dlg );
    dialog->show();
}

void IbisHardwareModule::OpenTrackerSettingsDialog()
{
    QDialog * dialog = new QDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle( "Tracker Settings" );
    QVBoxLayout * layout = new QVBoxLayout( dialog );
    QWidget * dlg = m_tracker->CreateSettingsDialog( dialog );
    layout->addWidget( dlg );
    dialog->show();
}

void IbisHardwareModule::ReadHardwareConfig( const char * filename )
{
    // make sure the file exists and is readable
    QFileInfo fileInfo( filename );
    Q_ASSERT( fileInfo.exists() && fileInfo.isReadable() );

    QString name( filename );
    SerializerReader reader;
    reader.SetFilename( name.toUtf8().data() );
    reader.Start();

    Serialize( &reader, "TrackerSettings", m_tracker );
    Serialize( &reader, "TrackedVideoSourceSettings", m_trackedVideoSource );

    reader.Finish();
}

#include <QDateTime>
#include <QMessageBox>

void IbisHardwareModule::WriteHardwareConfig(const char * filename , bool backupOnly)
{
    // make sure directory exists. Possible filename doesn't exist, but dir has to exist and be writable
    QFileInfo info( filename );
    QFileInfo dirInfo( info.dir().path() );  // get the parent dir
    QString generalConfigFileName( filename );

    if( !dirInfo.exists() || !dirInfo.isWritable() || (info.exists() && !info.isWritable()) )
    {
        QString message;
        if( !dirInfo.exists() )
            message = QString( "Directory %1 used to store hardware settings doesn't exist." ).arg( info.dir().path() );
        else if( !dirInfo.isWritable() )
            message = QString( "Directory %1 used to store hardware settings is not writable." ).arg( info.dir().path() );
        else
            message = QString( "Hardware config file %1 is not writable." ).arg( filename );

        generalConfigFileName = QDir::homePath() + "/hardware_config.xml";
        message += QString(" Hardware config will be saved in %1").arg( generalConfigFileName );

        QMessageBox::warning( 0, "Warning", message );
    }

    if( !backupOnly )
    {
        // write the config file
        InternalWriteHardwareConfig( generalConfigFileName );
    }

    // make a backup of the config file
    QString backupDir = Application::GetConfigDirectory() + "/BackupConfig/";
    if( !QFile::exists( backupDir ) )
    {
        QDir dir;
        dir.mkpath( backupDir );
    }
    QString dateAndTime( QDateTime::currentDateTime().toString() );
    QString backupFileName = QString( "%1Rev-%2-%3-%4" ).arg( backupDir ).arg( Application::GetGitHashShort() ).arg( dateAndTime ).arg( info.fileName() );
    InternalWriteHardwareConfig( backupFileName );
}

void IbisHardwareModule::InternalWriteHardwareConfig( QString filename )
{
    SerializerWriter writer0;
    writer0.SetFilename( filename.toUtf8().data() );
    writer0.Start();
    Serialize( &writer0, "TrackerSettings", m_tracker );
    Serialize( &writer0, "TrackedVideoSourceSettings", m_trackedVideoSource );
    writer0.EndSection();
    writer0.Finish();
}
