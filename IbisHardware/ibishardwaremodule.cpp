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
#include "tracker.h"
#include "trackedvideosource.h"
#include "vtkTracker.h"
#include "vtkTrackerTool.h"
#include "vtkTransform.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"
#include "ibisapi.h"
#include <QDir>
#include <QMenu>
#include <QDialog>
#include <QVBoxLayout>

static const QString IbisHardwareModuleConfigFilename("IbisHardwareModuleConfig.xml");

IbisHardwareModule::IbisHardwareModule()
{
    m_tracker = Tracker::New();
    m_tracker->SetHardwareModule( this );
    m_trackedVideoSource = TrackedVideoSource::New();
}

IbisHardwareModule::~IbisHardwareModule()
{
    m_tracker->Delete();
    m_trackedVideoSource->Delete();
}

void IbisHardwareModule::AddSettingsMenuEntries( QMenu * menu )
{
    menu->addAction( tr("Video Settings"), this, SLOT( OpenVideoSettingsDialog() ) );
    menu->addAction( tr("Tracker Settings"), this, SLOT( OpenTrackerSettingsDialog() ) );
    menu->addAction( tr("Save Hardware Settings"), this, SLOT( OnWriteHardwareSettingsMenuActivated() ) );
}

// Implementation of the HardwareModule interface
void IbisHardwareModule::Init()
{
    // Create the tracker if it doesn't exist. Otherwise, make sure tracking is stopped
    m_tracker->SetIbisAPI( GetIbisAPI() );
    m_tracker->StopTracking();

    // Read hardware configuration
    ReadHardwareConfig();

    // Init tracker
    m_tracker->Initialize();

    // Init video
    m_trackedVideoSource->InitializeVideo();
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
    WriteHardwareConfig();

    m_trackedVideoSource->RemoveClient();
    m_tracker->StopTracking();

    // simtodo : don't always return true
    return true;
}

vtkTransform * IbisHardwareModule::GetReferenceTransform()
{
    return m_tracker->GetReferenceTransform();
}

bool IbisHardwareModule::IsTransformFrozen( TrackedSceneObject * obj )
{
    return m_tracker->GetVideoSource( obj )->IsTransformFrozen();
}

void IbisHardwareModule::FreezeTransform( TrackedSceneObject * obj, int nbSamples )
{
    return m_tracker->GetVideoSource( obj )->FreezeTransform( nbSamples );
}

void IbisHardwareModule::UnFreezeTransform( TrackedSceneObject * obj )
{
    return m_tracker->GetVideoSource( obj )->UnFreezeTransform();
}

void IbisHardwareModule::AddTrackedVideoClient( TrackedSceneObject * obj )
{
    return m_tracker->GetVideoSource( obj )->AddClient();
}

void IbisHardwareModule::RemoveTrackedVideoClient( TrackedSceneObject * obj )
{
    return m_tracker->GetVideoSource( obj )->RemoveClient();
}

void IbisHardwareModule::StartTipCalibration( PointerObject * p )
{
    vtkTrackerTool * tool = m_tracker->GetTool( p );
    Q_ASSERT( tool );
    tool->StartTipCalibration();
}

double IbisHardwareModule::DoTipCalibration( PointerObject * p, vtkMatrix4x4 * calibMat )
{
    vtkTrackerTool * tool = m_tracker->GetTool( p );
    Q_ASSERT( tool );
    return tool->DoToolTipCalibration( calibMat );
}

bool IbisHardwareModule::IsCalibratingTip( PointerObject * p )
{
    vtkTrackerTool * tool = m_tracker->GetTool( p );
    Q_ASSERT( tool );
    return tool->GetCalibrating() > 0 ? true : false;
}

void IbisHardwareModule::StopTipCalibration( PointerObject * p )
{
    vtkTrackerTool * tool = m_tracker->GetTool( p );
    Q_ASSERT( tool );
    tool->StopTipCalibration();
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

void IbisHardwareModule::OnWriteHardwareSettingsMenuActivated()
{
    this->WriteHardwareConfig();
}

void IbisHardwareModule::AddToolObjectsToScene()
{
    m_tracker->AddAllToolsToScene();
}

void IbisHardwareModule::RemoveToolObjectsFromScene()
{
    m_tracker->RemoveAllToolsFromScene();
}

void IbisHardwareModule::ReadHardwareConfig()
{
    QString filename = GetIbisAPI()->GetConfigDirectory() + IbisHardwareModuleConfigFilename;

    // make sure the file exists and is readable
    QFileInfo fileInfo( filename );
    if( !fileInfo.exists() || !fileInfo.isReadable() )
        return;

    SerializerReader reader;
    reader.SetFilename( filename.toUtf8().data() );
    reader.Start();

    ::Serialize( &reader, "TrackerSettings", m_tracker );
    ::Serialize( &reader, "TrackedVideoSourceSettings", m_trackedVideoSource );

    reader.Finish();
}

#include <QDateTime>
#include <QMessageBox>

void IbisHardwareModule::WriteHardwareConfig()
{
    QString filename = GetIbisAPI()->GetConfigDirectory() + IbisHardwareModuleConfigFilename;

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


    // write the config file
    InternalWriteHardwareConfig( generalConfigFileName );

    // make a backup of the config file
    QString backupDir = GetIbisAPI()->GetConfigDirectory() + "/BackupConfig/";
    if( !QFile::exists( backupDir ) )
    {
        QDir dir;
        dir.mkpath( backupDir );
    }
    QString dateAndTime( QDateTime::currentDateTime().toString() );
    QString backupFileName = QString( "%1Rev-%2-%3-%4" ).arg( backupDir ).arg( GetIbisAPI()->GetGitHashShort() ).arg( dateAndTime ).arg( info.fileName() );
    InternalWriteHardwareConfig( backupFileName );
}

void IbisHardwareModule::InternalWriteHardwareConfig( QString filename )
{
    SerializerWriter writer0;
    writer0.SetFilename( filename.toUtf8().data() );
    writer0.Start();
    ::Serialize( &writer0, "TrackerSettings", m_tracker );
    ::Serialize( &writer0, "TrackedVideoSourceSettings", m_trackedVideoSource );
    writer0.EndSection();
    writer0.Finish();
}
