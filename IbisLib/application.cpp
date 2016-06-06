/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "application.h"
#include "hardwaremodule.h"
#include "quadviewwindow.h"
#include "githash.h"
#include "version.h"
#include "updatemanager.h"
#include "serializer.h"
#include "scenemanager.h"
#include "view.h"
#include "sceneobject.h"
#include "worldobject.h"
#include "cameraobject.h"
#include "pointerobject.h"
#include "polydataobject.h"
#include "ignsconfig.h"
#include "sceneinfo.h"
#include "objectplugininterface.h"
#include "toolplugininterface.h"
#include "triplecutplaneobject.h"
#include "volumerenderingobject.h"
#include "filereader.h"
#include "imageobject.h"
#include "lookuptablemanager.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QRect>
#include <QLibrary>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QTimer>
#include <QMessageBox>
#include <QPushButton>
#include <QApplication>


Application * Application::m_uniqueInstance = NULL;
const QString Application::m_appName("ibis");
const QString Application::m_appOrganisation("MNI-BIC-IPL");

ApplicationSettings::ApplicationSettings() 
{
}

ApplicationSettings::~ApplicationSettings() 
{
}

void ApplicationSettings::LoadSettings( QSettings & settings )
{
    QRect mainWindowRect( 0, 0, 800, 600 );
    MainWindowPosition = settings.value( "MainWindow_pos", mainWindowRect.topLeft() ).toPoint();
    MainWindowSize = settings.value( "MainWindow_size", mainWindowRect.size() ).toSize();
    MainWindowLeftPanelSize = settings.value( "MainWindowLeftPanelSize", 150 ).toInt();
    MainWindowRightPanelSize = settings.value( "MainWindowRightPanelSize", 150 ).toInt();
    LastVisitedDirectory = settings.value( "LastVisitedDirectory", QDir::homePath() ).toString();
    LastConfigFile = settings.value( "LastConfigFile", QString("") ).toString();
    QString workDir(QDir::homePath());
    workDir.append(IGNS_WORKING_DIRECTORY);
    WorkingDirectory = settings.value( "WorkingDirectory", workDir).toString();

    // Should be able to load and save a QColor directly, but doesn't work well on linux: to check: anything to do with the fact that destructor can be called after QApplication's destructor?
    ViewBackgroundColor.setRed( settings.value("ViewBackgroundColor_r", 50 ).toInt() );
    ViewBackgroundColor.setGreen( settings.value("ViewBackgroundColor_g", 50 ).toInt() );
    ViewBackgroundColor.setBlue( settings.value("ViewBackgroundColor_b", 50 ).toInt() );

    CutPlanesCursorColor.setRed( settings.value("CutPlanesCursorColor_r", 50 ).toInt() );
    CutPlanesCursorColor.setGreen( settings.value("CutPlanesCursorColor_g", 50 ).toInt() );
    CutPlanesCursorColor.setBlue( settings.value("CutPlanesCursorColor_b", 50 ).toInt() );

    ExpandedView = settings.value( "ExpandedView", -1).toInt();

    InteractorStyle3D = (InteractorStyle)settings.value( "InteractorStyle3D", (int)InteractorStyleTerrain ).toInt();
    CameraViewAngle3D = settings.value( "CameraViewAngle3D", 30.0 ).toDouble();
    ShowCursor = settings.value( "ShowCursor", true ).toBool();
    ShowAxes = settings.value( "ShowAxes", true ).toBool();
    ViewFollowsReference = settings.value( "ViewFollowsReference", true ).toBool();
    ShowXPlane = settings.value( "ShowXPlane", 1 ).toInt();
    ShowYPlane = settings.value( "ShowYPlane", 1 ).toInt();
    ShowZPlane = settings.value( "ShowZPlane", 1 ).toInt();
    TripleCutPlaneResliceInterpolationType = settings.value( "TripleCutPlaneResliceInterpolationType", 1 ).toInt();
    TripleCutPlaneDisplayInterpolationType = settings.value( "TripleCutPlaneDisplayInterpolationType", 1 ).toInt();
    VolumeRendererEnabled = settings.value( "VolumeRendererEnabled", false ).toBool();
    ShowMINCConversionWarning = settings.value( "ShowMINCConversionWarning", true ).toBool();
    UpdateFrequency = settings.value( "UpdateFrequency", 15.0 ).toDouble();
    NumberOfImageProcessingThreads = settings.value( "NumberOfImageProcessingThreads", 1 ).toInt();
}

void ApplicationSettings::SaveSettings( QSettings & settings )
{
    settings.setValue( "MainWindow_pos", MainWindowPosition );
    settings.setValue( "MainWindow_size", MainWindowSize );
    settings.setValue( "MainWindowLeftPanelSize", MainWindowLeftPanelSize );
    settings.setValue( "MainWindowRightPanelSize", MainWindowRightPanelSize );
    settings.setValue( "LastVisitedDirectory", LastVisitedDirectory );
    settings.setValue( "LastConfigFile", LastConfigFile );
    settings.setValue( "WorkingDirectory", WorkingDirectory );

    // Should be able to load and save a QColor directly, but doesn't work well on linux: to check: anything to do with the fact that destructor can be called after QApplication's destructor?
    settings.setValue( "ViewBackgroundColor_r", ViewBackgroundColor.red() );
    settings.setValue( "ViewBackgroundColor_g", ViewBackgroundColor.green() );
    settings.setValue( "ViewBackgroundColor_b", ViewBackgroundColor.blue() );
    settings.setValue( "CutPlanesCursorColor_r", CutPlanesCursorColor.red() );
    settings.setValue( "CutPlanesCursorColor_g", CutPlanesCursorColor.green() );
    settings.setValue( "CutPlanesCursorColor_b", CutPlanesCursorColor.blue() );

    settings.setValue( "ExpandedView", ExpandedView );
    settings.setValue( "InteractorStyle3D", (int)InteractorStyle3D );
    settings.setValue( "CameraViewAngle3D", CameraViewAngle3D );
    settings.setValue( "ShowCursor", ShowCursor );
    settings.setValue( "ShowAxes", ShowAxes );
    settings.setValue( "ViewFollowsReference", ViewFollowsReference );
    settings.setValue( "ShowXPlane", ShowXPlane );
    settings.setValue( "ShowYPlane", ShowYPlane );
    settings.setValue( "ShowZPlane", ShowZPlane );
    settings.setValue( "TripleCutPlaneResliceInterpolationType", TripleCutPlaneResliceInterpolationType );
    settings.setValue( "TripleCutPlaneDisplayInterpolationType", TripleCutPlaneDisplayInterpolationType );
    settings.setValue( "VolumeRendererEnabled", VolumeRendererEnabled );
    settings.setValue( "ShowMINCConversionWarning", ShowMINCConversionWarning );
    settings.setValue( "UpdateFrequency", UpdateFrequency );
    settings.setValue( "NumberOfImageProcessingThreads", NumberOfImageProcessingThreads );
}

Application::Application( bool viewerOnly )
{
    m_mainWindow = 0;
    m_quadView = 0;
    m_sceneManager = 0;
    m_viewerOnly = viewerOnly;
    m_fileReader = 0;
    m_fileOpenProgressDialog = 0;
    m_progressDialogUpdateTimer = 0;

    // Load application settings
    QSettings settings( m_appOrganisation, m_appName );
    m_settings.LoadSettings( settings );

    // Create the object that will manage the 3D scene in the visualizer
    // and add axes at its origin
    m_sceneManager = SceneManager::New();
    m_sceneManager->SetSceneDirectory(m_settings.WorkingDirectory);
    double bgColor[3];
    m_settings.ViewBackgroundColor.getRgbF( &bgColor[0], &bgColor[1], &bgColor[2] );
    m_sceneManager->SetViewBackgroundColor( bgColor );
    m_sceneManager->Set3DCameraViewAngle( m_settings.CameraViewAngle3D );

    double cursorColor[3];
    m_settings.CutPlanesCursorColor.getRgbF( &cursorColor[0], &cursorColor[1], &cursorColor[2] );
    WorldObject * world = WorldObject::SafeDownCast(m_sceneManager->GetSceneRoot());
    world->SetCursorColor(cursorColor);

    m_sceneManager->Set3DViewFollowingReferenceVolume( m_settings.ViewFollowsReference );
    m_sceneManager->SetExpandedView(m_settings.ExpandedView);
    m_sceneManager->Set3DInteractorStyle( m_settings.InteractorStyle3D );

    m_sceneManager->GetMainImagePlanes()->SetCursorVisibility( m_settings.ShowCursor );
    m_sceneManager->GetMainImagePlanes()->SetViewPlane( 0, m_settings.ShowXPlane );
    m_sceneManager->GetMainImagePlanes()->SetViewPlane( 1, m_settings.ShowYPlane );
    m_sceneManager->GetMainImagePlanes()->SetViewPlane( 2, m_settings.ShowZPlane );
    m_sceneManager->GetMainImagePlanes()->SetDisplayInterpolationType( m_settings.TripleCutPlaneDisplayInterpolationType );
    m_sceneManager->GetMainImagePlanes()->SetResliceInterpolationType( m_settings.TripleCutPlaneResliceInterpolationType );

    m_sceneManager->GetMainVolumeRenderer()->SetHidden( !m_settings.VolumeRendererEnabled );

    SceneInfo *sceneinfo = m_sceneManager->GetSceneInfo();
    sceneinfo->SetApplicationVersion( this->GetFullVersionString() );
    m_sceneManager->GetAxesObject()->SetHidden( !m_settings.ShowAxes );

    m_updateManager = UpdateManager::New();

    m_lookupTableManager = new LookupTableManager;

    // Get instance of the hardware module
    m_hardwareModule = 0;
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        HardwareModule * mod = qobject_cast< HardwareModule* >( plugin );
        if( mod )
        {
            m_hardwareModule = mod;
            break;
        }
    }
}


Application::~Application()
{
    // Update application settings
    double * backgroundColor = m_sceneManager->GetViewBackgroundColor();
    m_settings.ViewBackgroundColor = QColor( int(backgroundColor[0] * 255), int(backgroundColor[1] * 255), int(backgroundColor[2] * 255) );
    WorldObject * world = WorldObject::SafeDownCast(m_sceneManager->GetSceneRoot());
    m_settings.CutPlanesCursorColor =  world->GetCursorColor();
    m_settings.ExpandedView = m_sceneManager->GetExpandedView();
    m_settings.InteractorStyle3D = m_sceneManager->Get3DInteractorStyle();
    m_settings.CameraViewAngle3D = m_sceneManager->Get3DCameraViewAngle();
    m_settings.ShowCursor = m_sceneManager->GetMainImagePlanes()->GetCursorVisible();
    m_settings.ShowAxes = !m_sceneManager->GetAxesObject()->IsHidden();
    m_settings.ViewFollowsReference = m_sceneManager->Is3DViewFollowingReferenceVolume();
    m_settings.ShowXPlane = m_sceneManager->GetMainImagePlanes()->GetViewPlane( 0 );
    m_settings.ShowYPlane = m_sceneManager->GetMainImagePlanes()->GetViewPlane( 1 );
    m_settings.ShowZPlane = m_sceneManager->GetMainImagePlanes()->GetViewPlane( 2 );
    m_settings.TripleCutPlaneDisplayInterpolationType = m_sceneManager->GetMainImagePlanes()->GetDisplayInterpolationType();
    m_settings.TripleCutPlaneResliceInterpolationType = m_sceneManager->GetMainImagePlanes()->GetResliceInterpolationType();
    m_settings.VolumeRendererEnabled = !(m_sceneManager->GetMainVolumeRenderer()->IsHidden());
    m_sceneManager->GetMainVolumeRenderer()->SaveCustomShaderContribs();
    m_sceneManager->GetMainVolumeRenderer()->SaveCustomRayInitShaders();

    if( !m_viewerOnly )
    {
        // Stop everybody to make sure the order of deletion of objects doesn't matter
         m_hardwareModule->ShutDown();
         m_updateManager->Stop();
     }

    // Cleanup
    if( m_updateManager )
        m_updateManager->Delete();
    if( m_hardwareModule )
        delete m_hardwareModule;

    m_sceneManager->Destroy();

    delete m_lookupTableManager;

    // Save settings
    QSettings settings( m_appOrganisation, m_appName );
    m_settings.SaveSettings( settings );
    settings.beginGroup("Plugins");
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
            toolModule->BaseSaveSettings( settings );
    }
    settings.endGroup();

}

void Application::CreateInstance( bool viewerOnly )
{
    // Make sure the application hasn't been created yet
    Q_ASSERT( !m_uniqueInstance );

    m_uniqueInstance = new Application( viewerOnly );
}

void Application::DeleteInstance()
{
    Q_ASSERT( m_uniqueInstance );
    delete m_uniqueInstance;
    m_uniqueInstance = 0;
}

Application & Application::GetInstance()
{
    Q_ASSERT( m_uniqueInstance );
    return *m_uniqueInstance;
}

void Application::AddBottomWidget( QWidget * w )
{
    m_quadView->AddBottomWidget( w );
}

void Application::RemoveBottomWidget( QWidget * w )
{
    m_quadView->RemoveBottomWidget( w );
}

void Application::OnStartMainLoop()
{
    Q_ASSERT( m_sceneManager );
    m_sceneManager->OnStartMainLoop();
}

void Application::AddGlobalEventHandler( GlobalEventHandler * h )
{
    m_globalEventHandlers.push_back( h );
}

void Application::RemoveGlobalEventHandler( GlobalEventHandler * h )
{
    for( int i = 0; i < m_globalEventHandlers.size(); ++i )
        if( m_globalEventHandlers[i] == h )
            m_globalEventHandlers.removeAt(i);
}

bool Application::GlobalKeyEvent( QKeyEvent * e )
{
    for( int i = 0; i < m_globalEventHandlers.size(); ++i )
    {
        if( m_globalEventHandlers[i]->HandleKeyboardEvent( e ) )
            return true;
    }
    return false;
}

void Application::InitHardware( const char * filename )
{
    // Make sure the clock is stopped
    m_updateManager->Stop();

    // Init hardware
    bool success = m_hardwareModule->Init( filename );

    // Restart the clock if hardware running
    if( success )
    {
        m_updateManager->SetUpdatePeriod( (int)( 1000.0 / m_settings.UpdateFrequency )  );
        m_updateManager->Start();
        m_settings.LastConfigFile = filename;
    }
}

QString Application::GetFullVersionString()
{
    QString versionQualifier( IBIS_VERSION_QUALIFIER );
    QString buildQualifier( IBIS_BUILD_QUALIFIER );
    QString version;
    version = QString("%1.%2.%3 %4 %5\nrev. %6").arg(IBIS_MAJOR_VERSION).arg(IBIS_MINOR_VERSION).arg(IBIS_PATCH_VERSION).arg(versionQualifier).arg(buildQualifier).arg(IBIS_GIT_HASH);
    return version;
}

QString Application::GetConfigDirectory()
{
    QString configDir = QDir::homePath() + "/" + IGNS_CONFIGURATION_SUBDIRECTORY + "/";
    return configDir;
}

QString Application::GetGitHashShort()
{
    QString hash = IBIS_GIT_HASH_SHORT;
    return hash;
}

void Application::OpenFiles( OpenFileParams * params, bool addToScene )
{
    if( params->filesParams.size() == 0 )
        return;

    bool minc1found = false;
    for( int i = 0; i < params->filesParams.size(); ++i )
    {
        OpenFileParams::SingleFileParam & cur = params->filesParams[i];
        QFileInfo fi( cur.fileName );
        if( !(fi.isReadable()) )
        {
            QString message( "No read permission on file: " );
            message.append( cur.fileName );
            QMessageBox::critical( 0, "Error", message, 1, 0 );
            return;
        }
        FILE *fp = fopen( cur.fileName.toUtf8().data(), "rb" );
        if( fp )
        {
            char first4[4];
            size_t count = fread( first4, 4, 1, fp );
            fclose( fp );

            if( count == 1 &&
                first4[0] == 'C' &&
                first4[1] == 'D' &&
                first4[2] == 'F' &&
                first4[3] == '\001' )
            {
                cur.isMINC1 = true;
                minc1found = true;
            }
        }
    }

    // See if it is the first batch of objects loaded/created
    int initialNumberOfUserObjects = GetSceneManager()->GetNumberOfUserObjects();

    // Create Reader that reads files in another thread
    m_fileReader = new FileReader;
    m_fileReader->SetParams( params );
    if( minc1found )
    {
        if( m_fileReader->FindMincConverter() )
        {
            if( m_settings.ShowMINCConversionWarning )
                this->ShowMinc1Warning( true );
        }
        else
        {
            this->ShowMinc1Warning( false );
            delete m_fileReader;
            m_fileReader = 0;
            return;
        }
    }
    m_fileReader->start();

    // Create a progress dialog and a timer to update it
    m_fileOpenProgressDialog = new QProgressDialog( tr(""), tr("Cancel"), 0, 100 );
    m_progressDialogUpdateTimer = new QTimer;
    connect( m_progressDialogUpdateTimer, SIGNAL(timeout()), this, SLOT( OpenFilesProgress() ) );
    m_progressDialogUpdateTimer->start( 100 );

    // Launch the modal progress dialog while reader thread is working
    m_fileOpenProgressDialog->exec();

    // Stop timer
    m_progressDialogUpdateTimer->stop();
    delete m_progressDialogUpdateTimer;
    m_progressDialogUpdateTimer = 0;

    // Make sure the reading thread finished
    m_fileReader->wait();

    // Commit the changes if the operation didn't get cancelled
    if( !m_fileOpenProgressDialog->wasCanceled() )
    {
        // Process warnings generated during reading
        const QStringList & warnings = m_fileReader->GetWarnings();
        if( warnings.size() )
        {
            QString message;
            for( int i = 0; i < warnings.size(); ++i )
            {
                message += warnings[i];
                message += QString("\n");
            }
            QMessageBox::warning( 0, "Error", message );
        }

        if( addToScene )
        {
            for( int i = 0; i < params->filesParams.size(); ++i )
            {
                OpenFileParams::SingleFileParam & cur = params->filesParams[i];
                SceneObject * parent = cur.parent;
                if( !parent )
                    parent = params->defaultParent;
                if( !parent && !m_viewerOnly )
                    parent = Application::GetSceneManager()->GetSceneRoot();
                if( cur.loadedObject )
                    Application::GetSceneManager()->AddObject( cur.loadedObject, parent );
                if( cur.secondaryObject )
                    Application::GetSceneManager()->AddObject( cur.secondaryObject, parent );
            }

            if( initialNumberOfUserObjects == 0 )
            {
                Application::GetSceneManager()->ResetAllCameras();
            }
        }

        if( params->filesParams.size() )
        {
            QFileInfo info( params->filesParams[0].fileName );
            QString newDirVisited = info.dir().absolutePath();
            Application::GetInstance().GetSettings()->LastVisitedDirectory = newDirVisited;
        }
    }

    delete m_fileOpenProgressDialog;
    m_fileOpenProgressDialog = 0;
    delete m_fileReader;
    m_fileReader = 0;
    delete m_progressDialogUpdateTimer;
    m_progressDialogUpdateTimer = 0;
}

#include "vtkXFMReader.h"
#include "vtkTransform.h"

bool Application::OpenTransformFile( const char * filename, SceneObject * obj )
{
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    bool success = OpenTransformFile( filename, mat );
    if( success )
    {
        vtkTransform * transform = vtkTransform::New();
        transform->SetMatrix( mat );
        obj->SetLocalTransform( transform );
        transform->Delete();
    }
    mat->Delete();
    return success;
}

bool Application::OpenTransformFile( const char * filename, vtkMatrix4x4 * mat )
{
    vtkXFMReader * reader = vtkXFMReader::New();
    if( reader->CanReadFile( filename ) )
    {
        reader->SetFileName( filename );
        reader->SetMatrix(mat);
        reader->Update();
        reader->Delete();
        return true;
    }
    reader->Delete();
    return false;
}

#include "usacquisitionobject.h"

void Application::ImportUsAcquisition()
{
    QStringList filenames;
    QString extension( ".mnc" );
    bool success = Application::GetInstance().GetOpenFileSequence( filenames, extension, "Select first file of acquisition", QDir::homePath(), "Minc file (*.mnc)" );
    if( success )
    {
        USAcquisitionObject * acq = USAcquisitionObject::New();
        if( acq->LoadFramesFromMINCFile( filenames ) )
        {
            acq->SetName( "Acquisition" );
            acq->SetCurrentFrame(0);
            m_sceneManager->AddObject( acq );
        }
    }
    else
        QMessageBox::warning( m_mainWindow, "ERROR", "Couldn't read image sequence" );
}

void Application::ImportCamera()
{
    QString directory = Application::GetInstance().GetExistingDirectory( "Camera data dir", QDir::homePath() );
    if( !directory.isEmpty() )
    {
        QFileInfo dirInfo( directory );
        if( !dirInfo.exists() )
        {
            Warning( "Import Camera", "Directory doesn't exist" );
            return;
        }

        if( !dirInfo.isReadable() )
        {
            Warning( "Import Camera", "Directory is not readable" );
            return;
        }

        QProgressDialog * progressDlg = StartProgress( 100, "Importing Camera..." );
        CameraObject * cam = CameraObject::New();
        bool imported = cam->Import( directory, progressDlg );
        if( imported )
            m_sceneManager->AddObject( cam );
        cam->Delete();
        StopProgress( progressDlg );
    }
}

bool Application::PreModalDialog()
{
    bool isRunning = m_updateManager->IsRunning();
    if(m_updateManager && isRunning )
        m_updateManager->Stop();
    return isRunning;
}

void Application::PostModalDialog()
{
    m_updateManager->Start();
}

// Next 2 functions are a workaround for a bug in Qt. The update manager is ran by a Idle Qt timer (timeout = 0)
// and for an unknown reason, Qt can't pop up file dialogs properly when such timer is running in the main loop.
#include <QFileDialog>
QString Application::GetOpenFileName( const QString & caption, const QString & dir, const QString & filter )
{
    QWidget *parent = 0;
    if (m_mainWindow)
        parent = m_mainWindow;
    bool running = PreModalDialog();
    QString filename = QFileDialog::getOpenFileName( parent, caption, dir, filter );
    if( running )
        PostModalDialog();
    return filename;
}

QString Application::GetSaveFileName( const QString & caption, const QString & dir, const QString & filter )
{
    Q_ASSERT_X( m_mainWindow, "Application::GetSaveFileName()", "MainWindow was not set" );
    bool running = PreModalDialog();
    QString filename = QFileDialog::getSaveFileName( m_mainWindow, caption, dir, filter );
    if( running )
        PostModalDialog();
    return filename;
}

QString Application::GetExistingDirectory( const QString & caption, const QString & dir )
{
    Q_ASSERT_X( m_mainWindow, "Application::GetExistingDirectory()", "MainWindow was not set" );
    bool running = PreModalDialog();
    QString dirname = QFileDialog::getExistingDirectory( m_mainWindow, caption, dir, QFileDialog::ShowDirsOnly
                                                         | QFileDialog::DontResolveSymlinks );
    if( running )
        PostModalDialog();
    return dirname;
}

bool Application::GetOpenFileSequence( QStringList & filenames, QString extension, const QString & caption, const QString & dir, const QString & filter )
{
    // Get Base directory and file pattern
    QString firstFilename = Application::GetInstance().GetOpenFileName( "Select first file of acquisition", QDir::homePath(), "Minc file (*.mnc)" );
    if( firstFilename.isEmpty() )
        return false;

    // find first digit
    int it = firstFilename.size() - 1;
    while( !firstFilename.at( it ).isDigit() && it > 0 )
        --it;
    if( it <= 0 )
        return false;

    int nbCharTrailer = firstFilename.size() - it - 1;
    QString trailer = firstFilename.right( nbCharTrailer );

    // find the number of digits
    int nbDigits = 0;
    while( firstFilename.at( it ).isDigit() && it >= 0 )
    {
        --it;
        nbDigits++;
    }

    bool ok = true;
    int firstNumber = firstFilename.mid( it + 1, nbDigits ).toInt( &ok );
    if( !ok )
        return false;

    QString basePath = firstFilename.left( it + 1 );

    // find the number of files
    bool done = false;
    int fileNumber = firstNumber;
    while( !done )
    {
        QString name = QString( basePath + "%1" + trailer ).arg( fileNumber, nbDigits, 10, QChar('0') );
        if( !QFile::exists( name ) )
            done = true;
        else
        {
            ++fileNumber;
            filenames.push_back( name );
        }
    }
    return true;
}

int Application::LaunchModalDialog( QDialog * d )
{
    Q_ASSERT_X( m_mainWindow, "Application::LaunchModalDialog()", "MainWindow was not set" );
    bool running = PreModalDialog();
    d->setParent( m_mainWindow );
    int res = d->exec();
    if( running )
        PostModalDialog();
    return res;
}

void Application::Warning( const QString &title, const QString & text )
{
    bool running = PreModalDialog();
    QMessageBox::warning( m_mainWindow, title, text );
    if( running )
        PostModalDialog();
}

void Application::OpenFilesProgress()
{
    double progress = m_fileReader->GetProgress();
    int percent = (int)round( progress * 100 );
    m_fileOpenProgressDialog->setValue( percent );

    QString labelText = tr("Reading ");
    labelText += m_fileReader->GetCurrentlyReadFile();
    labelText += " ...";
    m_fileOpenProgressDialog->setLabelText( labelText );

    // Manually reset the dialog if processing is done. This shouldn't be necessary
    // but it seems that Qt fails to reset the dialog properly if a file is loaded
    // too quickly. simtodo : look for a better explanation.
    if( percent == 100 )
        m_fileOpenProgressDialog->reset();
}

void Application::TickIbisClock()
{
    m_hardwareModule->Update();
    emit IbisClockTick();
}

SceneManager * Application::GetSceneManager()
{
    return GetInstance().m_sceneManager;
}

HardwareModule * Application::GetHardwareModule()
{
    return GetInstance().m_hardwareModule;
}

LookupTableManager * Application::GetLookupTableManager()
{
    return GetInstance().m_lookupTableManager;
}

ApplicationSettings * Application::GetSettings()
{
    return &m_settings;
}

void Application::SetUpdateFrequency( double fps )
{
    m_settings.UpdateFrequency = fps;
    m_updateManager->SetUpdatePeriod( (int)( 1000.0 / fps ) );
}

void Application::SetNumberOfImageProcessingThreads( int nbThreads )
{
    m_settings.NumberOfImageProcessingThreads = nbThreads;
}

void Application::LoadPlugins()
{
    QSettings settings( m_appOrganisation, m_appName );
    settings.beginGroup("Plugins");
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule )
        {
            toolModule->BaseLoadSettings( settings );
            toolModule->SetApplication( this );
            toolModule->InitPlugin();
        }
    }
    settings.endGroup();
}

ToolPluginInterface * Application::GetPluginByName( const char * name )
{
    ToolPluginInterface * ret = 0;
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ToolPluginInterface * toolModule = qobject_cast< ToolPluginInterface* >( plugin );
        if( toolModule && toolModule->GetPluginName() == name )
        {
            ret = toolModule;
            break;
        }
    }
    return ret;
}

void Application::ActivatePluginByName( const char * name, bool active )
{
    ToolPluginInterface * tool = GetPluginByName( name );
    if( tool )
        emit QueryActivatePluginSignal( tool, active );
}

ObjectPluginInterface * Application::GetObjectPluginByClassName( const char * name )
{
    ObjectPluginInterface *ret = 0;
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        ObjectPluginInterface * objectPlugin = qobject_cast< ObjectPluginInterface* >( plugin );
        if( objectPlugin  && objectPlugin->GetObjectClassName() == name )
        {
            ret = objectPlugin;
            break;
        }
    }
    return ret;
}

QProgressDialog * Application::StartProgress( int max, const QString &caption )
{
    QProgressDialog *progressDialog =  new QProgressDialog( caption, tr("Cancel"), 0, max );
    progressDialog->setLabelText( caption );
    progressDialog->show();
    return progressDialog;
}

void Application::StopProgress(QProgressDialog * progressDialog)
{
    if (progressDialog)
        delete progressDialog;
}


void Application::UpdateProgress( QProgressDialog * progressDialog, int current )
{
    if (!progressDialog) // the process might have been cancelled
        return;
    if( current >= progressDialog->maximum() )
        progressDialog->reset();
    else
        progressDialog->setValue( current );
    QApplication::processEvents();
}



void Application::ShowMinc1Warning( bool cando)
{
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Ok);
    if( cando )
    {
        msgBox.setText( tr("MINC 1 Files will be automatically converted to MINC 2\nand saved in scenes in MINC 2 format.") );
        QPushButton *doNotShow = msgBox.addButton( tr("Do not show this dialog again."), QMessageBox::ActionRole );
        int ret = msgBox.exec();
        if( msgBox.clickedButton() == doNotShow )
            m_settings.ShowMINCConversionWarning = false;
    }
    else
    {
        msgBox.setText( tr("Tool mincconvert not found,\nMINC 1 files cannot be processed.") );
        int ret = msgBox.exec();
    }
}

