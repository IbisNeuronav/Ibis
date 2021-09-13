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
#include "ibisapi.h"
#include "mainwindow.h"
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
#include "pointsobject.h"
#include "ibisconfig.h"
#include "ibisplugin.h"
#include "objectplugininterface.h"
#include "toolplugininterface.h"
#include "globalobjectplugininterface.h"
#include "generatorplugininterface.h"
#include "filereader.h"
#include "imageobject.h"
#include "lookuptablemanager.h"
#include "ibispreferences.h"
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
#include <QMenu>
#include <QDirIterator>

Application * Application::m_uniqueInstance = nullptr;
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
    QString workDir(QDir::homePath());
    workDir.append("/");
    workDir.append(IBIS_CONFIGURATION_SUBDIRECTORY);
    WorkingDirectory = settings.value( "WorkingDirectory", workDir).toString();

    // Should be able to load and save a QColor directly, but doesn't work well on linux: to check: anything to do with the fact that destructor can be called after QApplication's destructor?
    // first all the 2D views
    ViewBackgroundColor.setRed( settings.value("ViewBackgroundColor_r", 50 ).toInt() );
    ViewBackgroundColor.setGreen( settings.value("ViewBackgroundColor_g", 50 ).toInt() );
    ViewBackgroundColor.setBlue( settings.value("ViewBackgroundColor_b", 50 ).toInt() );
    //then 3D view, if not found in settings, set the same as 2D.
    double bgColor[3];
    ViewBackgroundColor.getRgbF( &bgColor[0], &bgColor[1], &bgColor[2] );
    View3DBackgroundColor.setRed( settings.value("View3DBackgroundColor_r", int(bgColor[0]*255) ).toInt() );
    View3DBackgroundColor.setGreen( settings.value("View3DBackgroundColor_g", int(bgColor[1]*255) ).toInt() );
    View3DBackgroundColor.setBlue( settings.value("View3DBackgroundColor_b", int(bgColor[2]*255) ).toInt() );

    CutPlanesCursorColor.setRed( settings.value("CutPlanesCursorColor_r", 50 ).toInt() );
    CutPlanesCursorColor.setGreen( settings.value("CutPlanesCursorColor_g", 50 ).toInt() );
    CutPlanesCursorColor.setBlue( settings.value("CutPlanesCursorColor_b", 50 ).toInt() );

    InteractorStyle3D = static_cast<InteractorStyle>(settings.value( "InteractorStyle3D", static_cast<int>(InteractorStyleTerrain) ).toInt());
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
}

void ApplicationSettings::SaveSettings( QSettings & settings )
{
    settings.setValue( "WorkingDirectory", WorkingDirectory );

    // Should be able to load and save a QColor directly, but doesn't work well on linux: to check: anything to do with the fact that destructor can be called after QApplication's destructor?
    settings.setValue( "ViewBackgroundColor_r", ViewBackgroundColor.red() );
    settings.setValue( "ViewBackgroundColor_g", ViewBackgroundColor.green() );
    settings.setValue( "ViewBackgroundColor_b", ViewBackgroundColor.blue() );
    settings.setValue( "View3DBackgroundColor_r", View3DBackgroundColor.red() );
    settings.setValue( "View3DBackgroundColor_g", View3DBackgroundColor.green() );
    settings.setValue( "View3DBackgroundColor_b", View3DBackgroundColor.blue() );
    settings.setValue( "CutPlanesCursorColor_r", CutPlanesCursorColor.red() );
    settings.setValue( "CutPlanesCursorColor_g", CutPlanesCursorColor.green() );
    settings.setValue( "CutPlanesCursorColor_b", CutPlanesCursorColor.blue() );

    settings.setValue( "InteractorStyle3D", static_cast<int>(InteractorStyle3D) );
    settings.setValue( "CameraViewAngle3D", CameraViewAngle3D );
    settings.setValue( "ShowCursor", ShowCursor );
    settings.setValue( "ShowAxes", ShowAxes );
    settings.setValue( "ViewFollowsReference", ViewFollowsReference );
    settings.setValue( "ShowXPlane", ShowXPlane );
    settings.setValue( "ShowYPlane", ShowYPlane );
    settings.setValue( "ShowZPlane", ShowZPlane );
    settings.setValue( "TripleCutPlaneResliceInterpolationType", TripleCutPlaneResliceInterpolationType );
    settings.setValue( "TripleCutPlaneDisplayInterpolationType", TripleCutPlaneDisplayInterpolationType );
    settings.setValue( "ShowMINCConversionWarning", ShowMINCConversionWarning );
    settings.setValue( "UpdateFrequency", UpdateFrequency );
}

Application::Application( )
{
    m_mainWindow = nullptr;
    m_sceneManager = nullptr;
    m_viewerOnly = false;
    m_fileReader = nullptr;
    m_fileOpenProgressDialog = nullptr;
    m_progressDialogUpdateTimer = nullptr;
    m_ibisAPI = nullptr;
    m_updateManager = nullptr;
    m_lookupTableManager = nullptr;
    m_preferences = nullptr;
}

void Application::SetMainWindow( MainWindow * mw )
{
    m_mainWindow = mw;
    // Create UI elements from the plugins
    m_mainWindow->CreatePluginsUi();
}

void Application::LoadWindowSettings()
{
    Q_ASSERT( m_mainWindow );
    QSettings settings( m_appOrganisation, m_appName );
    m_mainWindow->LoadSettings( settings );
    //Apply settings to scene
    this->ApplyApplicationSettings();
}

void Application::Init( bool viewerOnly )
{
    m_viewerOnly = viewerOnly;


    // Create the object that will manage the 3D scene in the visualizer
    m_sceneManager = SceneManager::New();

    // Load application settings
    QSettings settings( m_appOrganisation, m_appName );
    m_settings.LoadSettings( settings );

    // Load custom paths and other preferences
    m_preferences = new IbisPreferences;
    m_preferences->LoadSettings( settings );

    m_updateManager = UpdateManager::New();

    m_lookupTableManager = new LookupTableManager;

    // Get instance of the hardware module
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        HardwareModule * mod = HardwareModule::SafeDownCast( p );
        if( mod )
            m_hardwareModules.push_back( mod );
    }

    // Despite what the user says, if there is no hardware module, go viewer only.
    if( m_hardwareModules.size() == 0 )
        m_viewerOnly = true;

    // Create programming interface for plugins
    m_ibisAPI = new IbisAPI( this );
}

Application::~Application()
{
    if( !m_viewerOnly )
    {
        // Stop everybody to make sure the order of deletion of objects doesn't matter
        foreach( HardwareModule * module, m_hardwareModules )
            module->ShutDown();
         m_updateManager->Stop();
     }

    // Cleanup
    if( m_updateManager )
        m_updateManager->Delete();

    delete m_ibisAPI;
    m_sceneManager->Destroy();

    delete m_lookupTableManager;

    QList<IbisPlugin*> allPlugins;
    this->GetAllPlugins( allPlugins );
    for( int i = 0; i < allPlugins.size(); ++i )
    {
        allPlugins[i]->Delete(); // this is called because otherwise plugins destructors are never called, Qt bug. The code has to be revised once Qt is fixed.
    }
    delete m_preferences;
}

void Application::ApplyApplicationSettings()
{
    double bgColor[3];
    m_settings.ViewBackgroundColor.getRgbF( &bgColor[0], &bgColor[1], &bgColor[2] );
    m_sceneManager->SetViewBackgroundColor( bgColor );
    m_settings.View3DBackgroundColor.getRgbF( &bgColor[0], &bgColor[1], &bgColor[2] );
    m_sceneManager->SetView3DBackgroundColor( bgColor );
    m_sceneManager->Set3DCameraViewAngle( m_settings.CameraViewAngle3D );

    double cursorColor[3];
    m_settings.CutPlanesCursorColor.getRgbF( &cursorColor[0], &cursorColor[1], &cursorColor[2] );
    WorldObject * world = WorldObject::SafeDownCast(m_sceneManager->GetSceneRoot());
    world->SetCursorColor(cursorColor);

    m_sceneManager->Set3DViewFollowingReferenceVolume( m_settings.ViewFollowsReference );
    m_sceneManager->Set3DInteractorStyle( m_settings.InteractorStyle3D );

    m_sceneManager->SetCursorVisibility( m_settings.ShowCursor );
    m_sceneManager->ViewPlane( 0, m_settings.ShowXPlane );
    m_sceneManager->ViewPlane( 1, m_settings.ShowYPlane );
    m_sceneManager->ViewPlane( 2, m_settings.ShowZPlane );
    m_sceneManager->SetDisplayInterpolationType( m_settings.TripleCutPlaneDisplayInterpolationType );
    m_sceneManager->SetResliceInterpolationType( m_settings.TripleCutPlaneResliceInterpolationType );

    m_sceneManager->GetAxesObject()->SetHidden( !m_settings.ShowAxes );
}

void Application::UpdateApplicationSettings()
{
    double * backgroundColor = m_sceneManager->GetViewBackgroundColor();
    m_settings.ViewBackgroundColor = QColor( int(backgroundColor[0] * 255), int(backgroundColor[1] * 255), int(backgroundColor[2] * 255) );
    backgroundColor = m_sceneManager->GetView3DBackgroundColor();
    m_settings.View3DBackgroundColor = QColor( int(backgroundColor[0] * 255), int(backgroundColor[1] * 255), int(backgroundColor[2] * 255) );
    WorldObject * world = WorldObject::SafeDownCast(m_sceneManager->GetSceneRoot());
    m_settings.CutPlanesCursorColor =  world->GetCursorColor();
    m_settings.InteractorStyle3D = m_sceneManager->Get3DInteractorStyle();
    m_settings.CameraViewAngle3D = m_sceneManager->Get3DCameraViewAngle();
    m_settings.ShowCursor = m_sceneManager->GetCursorVisible();
    m_settings.ShowAxes = !m_sceneManager->GetAxesObject()->IsHidden();
    m_settings.ViewFollowsReference = m_sceneManager->Is3DViewFollowingReferenceVolume();
    m_settings.ShowXPlane = m_sceneManager->IsPlaneVisible( 0 );
    m_settings.ShowYPlane = m_sceneManager->IsPlaneVisible( 1 );
    m_settings.ShowZPlane = m_sceneManager->IsPlaneVisible( 2 );
    m_settings.TripleCutPlaneDisplayInterpolationType = m_sceneManager->GetDisplayInterpolationType();
    m_settings.TripleCutPlaneResliceInterpolationType = m_sceneManager->GetResliceInterpolationType();
}

void Application::SaveSettings()
{
    // Update application settings
    this->UpdateApplicationSettings();

    // Save settings
    QSettings settings( m_appOrganisation, m_appName );
    m_settings.SaveSettings( settings );
    m_preferences->SaveSettings( settings );
    m_mainWindow->SaveSettings( settings );

    // Save plugins settings
    settings.beginGroup("Plugins");
    QList<IbisPlugin*> allPlugins;
    this->GetAllPlugins( allPlugins );
    for( int i = 0; i < allPlugins.size(); ++i )
    {
        allPlugins[i]->BaseSaveSettings( settings );
    }
    settings.endGroup();
}

void Application::CreateInstance( bool viewerOnly )
{
    // Make sure the application hasn't been created yet
    Q_ASSERT( !m_uniqueInstance );

    m_uniqueInstance = new Application;
    m_uniqueInstance->Init( viewerOnly );
}

void Application::DeleteInstance()
{
    Q_ASSERT( m_uniqueInstance );
    delete m_uniqueInstance;
    m_uniqueInstance = nullptr;
}

Application & Application::GetInstance()
{
    Q_ASSERT( m_uniqueInstance );
    return *m_uniqueInstance;
}

void Application::AddBottomWidget( QWidget * w )
{
    m_mainWindow->AddBottomWidget( w );
}

void Application::RemoveBottomWidget( QWidget * w )
{
    m_mainWindow->RemoveBottomWidget( w );
}

void Application::ShowFloatingDock( QWidget * w, QFlags<QDockWidget::DockWidgetFeature> features )
{
    m_mainWindow->ShowFloatingDock( w, features );
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

void Application::InitHardware()
{
    // Make sure the clock is stopped
    m_updateManager->Stop();

    // Init hardware
    foreach( HardwareModule * module, m_hardwareModules )
        module->Init();

    if( m_hardwareModules.size() > 0 )
    {
        m_updateManager->SetUpdatePeriod( static_cast<int>( 1000.0 / m_settings.UpdateFrequency )  );
        m_updateManager->Start();
    }
}

void Application::AddHardwareSettingsMenuEntries( QMenu * menu )
{
    int index = 0;
    foreach( HardwareModule * module, m_hardwareModules )
    {
        if( index > 0 )
            menu->addSeparator();
        module->AddSettingsMenuEntries( menu );
        ++index;
    }
    menu->addSeparator();
}

void Application::AddToolObjectsToScene()
{
    foreach( HardwareModule * module, m_hardwareModules )
        module->AddToolObjectsToScene();
}

void Application::RemoveToolObjectsFromScene()
{
    foreach( HardwareModule * module, m_hardwareModules )
        module->RemoveToolObjectsFromScene();
}

QString Application::GetFullVersionString()
{
    QString versionQualifier( IBIS_VERSION_QUALIFIER );
    QString buildQualifier( IBIS_BUILD_QUALIFIER );
    QString version;
    version = QString("%1.%2.%3 %4 %5\nrev. %6").arg(IBIS_MAJOR_VERSION)
                                                .arg(IBIS_MINOR_VERSION)
                                                .arg(IBIS_PATCH_VERSION)
                                                .arg(versionQualifier)
                                                .arg(buildQualifier)
                                                .arg(GetGitHash());
    return version;
}

QString Application::GetVersionString()
{
    QString versionQualifier( IBIS_VERSION_QUALIFIER );
    QString version;
    version = QString("%1.%2.%3  %4").arg(IBIS_MAJOR_VERSION).arg(IBIS_MINOR_VERSION).arg(IBIS_PATCH_VERSION).arg(versionQualifier);
    return version;
}

QString Application::GetConfigDirectory()
{
    QString configDir = QDir::homePath() + "/" + IBIS_CONFIGURATION_SUBDIRECTORY + "/";
    return configDir;
}

QString Application::GetGitHash()
{
    return QString(GIT_HEAD_SHA1);
}

QString Application::GetGitHashShort()
{
    QString hash = GetGitHash();
    return hash.left(7);
}

bool Application::GitCodeHasModifications()
{
    return GIT_IS_DIRTY;
}

bool Application::GitInfoIsValid()
{
    return GIT_RETRIEVED_STATE;
}

void Application::OpenFiles( OpenFileParams * params, bool addToScene )
{
    if( params->filesParams.size() == 0 )
        return;

    // Create Reader that reads files in another thread
    m_fileReader = new FileReader;
    m_fileReader->SetParams( params );
    m_fileReader->SetIbisAPI( m_ibisAPI );
    for( int i = 0; i < params->filesParams.size(); ++i )
    {
        OpenFileParams::SingleFileParam & cur = params->filesParams[i];
        QFileInfo fi( cur.fileName );
        if( !(fi.isReadable()) )
        {
            QString message( "No read permission on file: " );
            message.append( cur.fileName );
            QMessageBox::critical( nullptr, "Error", message, 1, 0 );
            return;
        }
        if( m_fileReader->IsMINC1( cur.fileName.toUtf8().data() ) )
        {
            if( m_fileReader->HasMincConverter() )
            {
                if( m_settings.ShowMINCConversionWarning )
                    this->ShowMinc1Warning( true );
            }
            else
            {
                this->ShowMinc1Warning( false );
                delete m_fileReader;
                m_fileReader = nullptr;
                return;
            }
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
    m_progressDialogUpdateTimer = nullptr;

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
            QMessageBox::warning( nullptr, "Error", message );
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
                {
                    Application::GetSceneManager()->AddObject( cur.loadedObject, parent );
                    cur.loadedObject->Delete();
                    if( !cur.secondaryObject )
                        Application::GetSceneManager()->SetCurrentObject( cur.loadedObject );
                }
                if( cur.secondaryObject )
                {
                    Application::GetSceneManager()->AddObject( cur.secondaryObject, parent );
                    cur.secondaryObject->Delete();
                    Application::GetSceneManager()->SetCurrentObject( cur.loadedObject );
                }
            }
            // See if it is the first batch of objects loaded/created
            // adding first user object has to call ResetAllCameras
            int initialNumberOfUserObjects = GetSceneManager()->GetNumberOfUserObjects();

            if( initialNumberOfUserObjects == 1 )
            {
                Application::GetSceneManager()->ResetAllCameras();
            }
        }

        if( params->filesParams.size() )
        {
            QFileInfo info( params->filesParams[0].fileName );
            QString newDirVisited = info.dir().absolutePath();
            Application::GetInstance().GetSettings()->WorkingDirectory = newDirVisited;
        }
    }

    delete m_fileOpenProgressDialog;
    m_fileOpenProgressDialog = nullptr;
    delete m_fileReader;
    m_fileReader = nullptr;
    delete m_progressDialogUpdateTimer;
    m_progressDialogUpdateTimer = nullptr;
}

bool Application::GetPointsFromTagFile( QString fileName, PointsObject *pts1, PointsObject *pts2 )
{
    Q_ASSERT(pts1);
    Q_ASSERT(pts2);
    m_fileReader = new FileReader;
    QFileInfo fi( fileName );
    if( !(fi.isReadable()) )
    {
        QString message( "No read permission on file: " );
        message.append( fileName );
        QMessageBox::critical( nullptr, "Error", message, 1, 0 );
        return false;
    }
    bool ok = m_fileReader->GetPointsDataFromTagFile( fileName, pts1, pts2 );
    delete m_fileReader;
    return ok;
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

void Application::ImportUsAcquisition()
{
    if( !m_sceneManager->ImportUsAcquisition() )
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
        {
            m_sceneManager->AddObject( cam );
            m_sceneManager->SetCurrentObject( cam );
        }
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
QString Application::GetFileNameOpen( const QString & caption, const QString & dir, const QString & filter )
{
    QWidget *parent = nullptr;
    if (m_mainWindow)
        parent = m_mainWindow;
    bool running = PreModalDialog();
    QString filename = QFileDialog::getOpenFileName( parent, caption, dir, filter, nullptr, QFileDialog::DontUseNativeDialog );
    if( running )
        PostModalDialog();
    return filename;
}

QString Application::GetFileNameSave( const QString & caption, const QString & dir, const QString & filter )
{
    Q_ASSERT_X( m_mainWindow, "Application::GetFileNameSave()", "MainWindow was not set" );
    bool running = PreModalDialog();
    QString filename = QFileDialog::getSaveFileName( m_mainWindow, caption, dir, filter, nullptr, QFileDialog::DontUseNativeDialog );
    if( running )
        PostModalDialog();
    return filename;
}

QString Application::GetExistingDirectory( const QString & caption, const QString & dir )
{
    Q_ASSERT_X( m_mainWindow, "Application::GetExistingDirectory()", "MainWindow was not set" );
    bool running = PreModalDialog();
    QString dirname = QFileDialog::getExistingDirectory( m_mainWindow, caption, dir, QFileDialog::ShowDirsOnly
                                                         | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog );
    if( running )
        PostModalDialog();
    return dirname;
}

bool Application::GetOpenFileSequence( QStringList & filenames, QString, const QString &, const QString & dir, const QString & )
{
    // Get Base directory and file pattern
    QString firstFilename = Application::GetInstance().GetFileNameOpen( "Select first file of acquisition", dir, "Minc file (*.mnc)" );
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

    QString basePath = firstFilename.left( it + 1 );

    // get absolute directory path
    QFileInfo fi(firstFilename);
    QString absoluteDirPath = fi.absoluteDir().absolutePath();

    // creating a pattern reg expression by replacing the digits with "?" symbols
    fi.setFile(basePath);
    QString  pattern = fi.fileName();
    for (int i = 0; i < nbDigits; ++i)
    {
        pattern += "?";
    }
    pattern += trailer;

    // iterate though folder to get files matching the pattern
    QDirIterator dit(absoluteDirPath, QStringList() << pattern, QDir::Files, QDirIterator::NoIteratorFlags);

    // add the files to temporary QStringList
    QStringList allUSSequence;
    while (dit.hasNext())
    {
        allUSSequence.push_back( dit.next() );
    }

    // sort file names alphabetically
    allUSSequence.sort();
    int idx = allUSSequence.indexOf(firstFilename);

    for (int i = idx; i < allUSSequence.size(); ++i) {
       filenames.push_back( allUSSequence.at(i) );
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
    int percent = static_cast<int>( round( progress * 100 ) );
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
    foreach( HardwareModule * module, m_hardwareModules )
        module->Update();
    emit IbisClockTick();
}

SceneManager * Application::GetSceneManager()
{
    return GetInstance().m_sceneManager;
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
    m_updateManager->SetUpdatePeriod( static_cast<int>( 1000.0 / fps ) );
}

void Application::LoadPlugins()
{
    QSettings settings( m_appOrganisation, m_appName );
    settings.beginGroup("Plugins");
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p )
        {
            p->SetIbisAPI( m_ibisAPI );
            p->BaseLoadSettings( settings );
            p->InitPlugin();
        }
    }
    settings.endGroup();
}

void Application::SerializePlugins( Serializer * ser )
{
    ser->BeginSection("Plugins");
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p )
        {
            ::Serialize( ser, p->GetPluginName().toUtf8().data(), p );
        }
    }
    ser->EndSection();
}

IbisPlugin * Application::GetPluginByName( QString name )
{
    IbisPlugin * ret = nullptr;
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p && p->GetPluginName() == name )
        {
            ret = p;
            break;
        }
    }
    return ret;
}

void Application::ActivatePluginByName( const char * name, bool active )
{
    IbisPlugin * p = GetPluginByName( name );
    if( p && p->GetPluginType() == IbisPluginTypeTool )
    {
        ToolPluginInterface * tool = ToolPluginInterface::SafeDownCast( p );
        emit QueryActivatePluginSignal( tool, active );
    }
}

ObjectPluginInterface * Application::GetObjectPluginByName( QString name )
{
    IbisPlugin * p = GetPluginByName( name );
    if( p && p->GetPluginType() == IbisPluginTypeObject )
        return ObjectPluginInterface::SafeDownCast( p );
    return nullptr;
}

ToolPluginInterface * Application::GetToolPluginByName( QString name )
{
    IbisPlugin * p = GetPluginByName( name );
    if( p && p->GetPluginType() == IbisPluginTypeTool )
        return ToolPluginInterface::SafeDownCast( p );
    return nullptr;
}

GeneratorPluginInterface * Application::GetGeneratorPluginByName( QString name )
{
    IbisPlugin * p = GetPluginByName( name );
    if( p && p->GetPluginType() == IbisPluginTypeGenerator )
        return GeneratorPluginInterface::SafeDownCast( p );
    return nullptr;
}

SceneObject * Application::GetGlobalObjectInstance( const QString & className )
{
    SceneObject * ret = nullptr;
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p && p->GetPluginType() == IbisPluginTypeGlobalObject )
        {
            GlobalObjectPluginInterface * objectPlugin = GlobalObjectPluginInterface::SafeDownCast( p );
            SceneObject * globalObj = objectPlugin->GetGlobalObjectInstance();
            if( globalObj->IsA( className.toUtf8().data() ) )
            {
                ret = globalObj;
                break;
            }
        }
    }
    return ret;
}

void Application::GetAllGlobalObjectInstances( QList<SceneObject*> & allInstances )
{
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p && p->GetPluginType() == IbisPluginTypeGlobalObject )
        {
            GlobalObjectPluginInterface * objectPlugin = GlobalObjectPluginInterface::SafeDownCast( p );
            allInstances.push_back( objectPlugin->GetGlobalObjectInstance() );
        }
    }
}

void Application::GetAllPlugins( QList<IbisPlugin*> & allPlugins )
{
    allPlugins.clear();
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p )
            allPlugins.push_back( p );
    }
}

void Application::GetAllToolPlugins( QList<ToolPluginInterface*> & allTools )
{
    allTools.clear();
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p && p->GetPluginType() == IbisPluginTypeTool )
        {
            ToolPluginInterface * t = ToolPluginInterface::SafeDownCast( p );
            allTools.push_back( t );
        }
    }
}

void Application::GetAllObjectPlugins( QList<ObjectPluginInterface*> & allObjects )
{
    allObjects.clear();
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p && p->GetPluginType() == IbisPluginTypeObject )
        {
            ObjectPluginInterface * o = ObjectPluginInterface::SafeDownCast( p );
            allObjects.push_back( o );
        }
    }
}

void Application::GetAllGeneratorPlugins( QList<GeneratorPluginInterface*> & allObjects )
{
    allObjects.clear();
    foreach( QObject * plugin, QPluginLoader::staticInstances() )
    {
        IbisPlugin * p = qobject_cast< IbisPlugin* >( plugin );
        if( p && p->GetPluginType() == IbisPluginTypeGenerator )
        {
            GeneratorPluginInterface * o = GeneratorPluginInterface::SafeDownCast( p );
            allObjects.push_back( o );
        }
    }
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
        msgBox.exec();
        if( msgBox.clickedButton() == doNotShow )
            m_settings.ShowMINCConversionWarning = false;
    }
    else
    {
        msgBox.setText( tr("Tool mincconvert not found,\nOpen Settings/Preferences and set path to the directory containing MINC tools.\n") );
        msgBox.exec();
    }
}

void Application::LoadScene(QString fileName)
{
    m_sceneManager->LoadScene( fileName );
}

void Application::SaveScene( QString fileName )
{
    m_sceneManager->SaveScene( fileName );
}

void Application::Preferences()
{
    m_preferences->ShowPreferenceDialog();
}

int Application::GetNumberOfComponents( QString filename )
{
    m_fileReader = new FileReader;
    m_fileReader->SetIbisAPI( m_ibisAPI );
    QFileInfo fi( filename );
    if( !(fi.isReadable()) )
    {
        QString message( "No read permission on file: " );
        message.append( filename );
        QMessageBox::critical( nullptr, "Error", message, 1, 0 );
        return false;
    }
    int n = m_fileReader->GetNumberOfComponents( filename );
    delete m_fileReader;
    return n;
}
bool Application::GetGrayFrame( QString filename, IbisItkUnsignedChar3ImageType::Pointer itkImage )
{
    Q_ASSERT(itkImage);
    m_fileReader = new FileReader;
    m_fileReader->SetIbisAPI( m_ibisAPI );
    QFileInfo fi( filename );
    if( !(fi.isReadable()) )
    {
        QString message( "No read permission on file: " );
        message.append( filename );
        QMessageBox::critical( nullptr, "Error", message, 1, 0 );
        return false;
    }
    bool ok = m_fileReader->GetGrayFrame( filename, itkImage );
    delete m_fileReader;
    return ok;
}

bool Application::GetRGBFrame( QString filename, IbisRGBImageType::Pointer itkImage )
{
    Q_ASSERT(itkImage);
    m_fileReader = new FileReader;
    m_fileReader->SetIbisAPI( m_ibisAPI );
    QFileInfo fi( filename );
    if( !(fi.isReadable()) )
    {
        QString message( "No read permission on file: " );
        message.append( filename );
        QMessageBox::critical( nullptr, "Error", message, 1, 0 );
        return false;
    }
    bool ok = m_fileReader->GetRGBFrame( filename, itkImage );
    delete m_fileReader;
    return ok;
}

