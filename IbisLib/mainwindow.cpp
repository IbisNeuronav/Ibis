/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "mainwindow.h"
#include "application.h"
#include "hardwaremodule.h"
#include "imageobject.h"
#include "pointsobject.h"
#include "scenemanager.h"
#include "aboutbicigns.h"
#include "quadviewwindow.h"
#include "opendatafiledialog.h"
#include "ibisapi.h"
#include "worldobject.h"
#include "ibisconfig.h"
#include "toolplugininterface.h"
#include "objectplugininterface.h"
#include "generatorplugininterface.h"
#include "aboutpluginswidget.h"
#include "serializer.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSplitter>
#include <QDir>
#include <QLayout>
#include <QFileInfo>
#include <QCloseEvent>
#include <QFileDialog>
#include <QPluginLoader>
#include <QScrollArea>
#include <QMimeData>
#include <QSettings>
#include <QPoint>


ObjectSerializationMacro( MainWindow );

const QString MainWindow::m_appName( "Intraoperative Brain Imaging System" );

MainWindow::MainWindow( QWidget * parent )
    : QMainWindow( parent )
    , m_4Views(nullptr)
    , m_viewXPlaneAction(nullptr)
    , m_viewYPlaneAction(nullptr)
    , m_viewZPlaneAction(nullptr)
    , m_showAllPlanesAction(nullptr)
    , m_hideAllPlanesAction(nullptr)
    , m_pluginMenu(nullptr)
    , m_rightPanel(nullptr)
    , m_mainSplitter(nullptr)
    , m_leftPanelSize(150)
    , m_rightPanelSize(150)
    , m_leftFrame(nullptr)
    , m_leftLayout(nullptr)
    , m_objectSettingsScrollArea(nullptr)
    , m_leftEndSpacer(nullptr)
    , m_windowClosing( false )
{
    // Set main window title
    setWindowTitle( m_appName );
    setAcceptDrops(true);

    bool viewerOnly = Application::GetInstance().IsViewerOnly();

    // -----------------------------------------
    // Creates a file menu
    // -----------------------------------------
    QMenu * fileMenu = menuBar()->addMenu( "&File" );
    fileMenu->addAction( tr("&Open File"), this, SLOT( fileOpenFile() ), QKeySequence::Open );
    QMenu *newObjectFileMenu = fileMenu->addMenu("&New Object");
    newObjectFileMenu->addAction( tr("&Point Set"), this, SLOT( NewPointSet() ));
    this->CreateNewObjectPluginsUi(newObjectFileMenu);
    QMenu * fileGenerateMenu = fileMenu->addMenu( tr("Generate") );
    fileMenu->addAction( tr("E&xport"), this, SLOT( fileExportFile() ) );
    QMenu * importFileMenu = fileMenu->addMenu("Import");
    importFileMenu->addAction( tr("US Acquisition"), this, SLOT(fileImportUsAcquisition()) );
    importFileMenu->addAction( tr("Camera"), this, SLOT(fileImportCamera()) );
    fileMenu->addSeparator();
    fileMenu->addAction( tr("Save S&cene"), this, SLOT( fileSaveScene() ), QKeySequence::Save );
    fileMenu->addAction( tr("Save S&cene As..."), this, SLOT( fileSaveSceneAs() ) );
    fileMenu->addAction( tr("&Load Scene"), this, SLOT( fileLoadScene() ), QKeySequence(Qt::CTRL + Qt::Key_L));
    fileMenu->addAction( tr("&New Scene"), this, SLOT( fileNewScene() ));
    fileMenu->addSeparator();
    fileMenu->addAction( tr("&Exit"), this, SLOT( close() ), QKeySequence::Quit );

    connect( fileMenu, SIGNAL( aboutToShow() ), this, SLOT( ModifyFileMenu() ) );
    connect( newObjectFileMenu, SIGNAL( aboutToShow() ), this, SLOT( ModifyNewObjectFileMenu() ) );
    connect( fileGenerateMenu, SIGNAL(aboutToShow()), this, SLOT(FileGenerateMenuAboutToShow()) );

    // -----------------------------------------
    // Creates settings menu
    // -----------------------------------------
        QMenu * settingsMenu = menuBar()->addMenu( tr("&Settings") );
        if( !viewerOnly )
        {
            Application::GetInstance().AddHardwareSettingsMenuEntries( settingsMenu );
        }
        QAction *preferences = settingsMenu->addAction( tr("&Preferences"), this, SLOT( Preferences() ), QKeySequence::Preferences );
        preferences->setMenuRole( QAction::PreferencesRole );

    // -----------------------------------------
    // Creates a View menu
    // -----------------------------------------
    QMenu * viewMenu = menuBar()->addMenu(tr("Vie&w"));
    m_viewXPlaneAction = AddToggleAction( viewMenu, tr("&Sagittal Plane"), SLOT( ViewXPlaneToggled(bool)), QKeySequence("Alt+x"), true );
    m_viewYPlaneAction = AddToggleAction( viewMenu, tr("&Coronal Plane"), SLOT( ViewYPlaneToggled(bool)), QKeySequence("Alt+y"), true );
    m_viewZPlaneAction = AddToggleAction( viewMenu, tr("&Transverse Plane"), SLOT( ViewZPlaneToggled(bool)), QKeySequence("Alt+z"), true );
    m_showAllPlanesAction = viewMenu->addAction( tr("View All Planes"), this,  SLOT( ViewAllPlanes() ), QKeySequence("Alt+p") );
    m_hideAllPlanesAction = viewMenu->addAction( tr("Hide All Planes"), this, SLOT( HideAllPlanes() ), QKeySequence("Shift+Alt+p") );
    viewMenu->addSeparator();
    viewMenu->addAction( tr("&Front"), this, SLOT(View3DFront()), QKeySequence("Shift+Alt+f") );
    viewMenu->addAction( tr("&Left"), this, SLOT( View3DLeft() ), QKeySequence("Shift+Alt+l") );
    viewMenu->addAction( tr("&Right"), this, SLOT( View3DRight() ), QKeySequence("Shift+Alt+r") );
    viewMenu->addAction( tr("&Back"), this, SLOT( View3DBack() ), QKeySequence("Shift+Alt+b") );
    viewMenu->addAction( tr("&Top"), this, SLOT( View3DTop() ), QKeySequence("Shift+Alt+t") );
    viewMenu->addAction( tr("Botto&m"), this,  SLOT( View3DBottom() ), QKeySequence("Shift+Alt+m") );
    viewMenu->addSeparator();
    viewMenu->addAction( tr("Re&set Planes"), this,  SLOT( ViewResetPlanes() ), QKeySequence("Shift+Alt+s") );
    viewMenu->addAction( "Full Screen", this, SLOT( ViewFullscreen() ), Qt::CTRL + Qt::Key_F );
    connect( viewMenu, SIGNAL( aboutToShow() ), this, SLOT( ModifyViewMenu() ) );
    
    // -----------------------------------------
    // Create a Plugins menu
    // -----------------------------------------
    m_pluginMenu = menuBar()->addMenu( tr("Plugins") );

    // -----------------------------------------
    // Create a Help menu
    // -----------------------------------------
    menuBar()->addSeparator();
    QMenu * helpMenu = menuBar()->addMenu( tr("&Help") );
    helpMenu->addSeparator();
    helpMenu->addAction( tr("About..."), this, SLOT(about()) );
    QAction * aboutPluginAction = helpMenu->addAction( tr("About Plugins..."), this, SLOT(AboutPlugins()) );
    aboutPluginAction->setMenuRole( QAction::ApplicationSpecificRole );
    
    // -----------------------------------------
    // Create left panel
    // -----------------------------------------
    m_leftFrame = new QFrame(this);
    m_leftFrame->setMinimumWidth( 320 );
    m_leftLayout = new QVBoxLayout( m_leftFrame );
    m_leftLayout->setContentsMargins( 0, 0, 0, 0 );

    if( !viewerOnly )
    {
        QWidget * trackerStatus = Application::GetSceneManager()->CreateTrackedToolsStatusWidget( m_leftFrame );
        trackerStatus->setMinimumWidth( 300 );
        m_leftLayout->addWidget( trackerStatus );
    }

    QWidget * objectListWidget = Application::GetSceneManager()->CreateObjectTreeWidget( m_leftFrame );
    m_leftLayout->addWidget( objectListWidget );
    m_objectSettingsScrollArea = new QScrollArea( m_leftFrame );
    m_objectSettingsScrollArea->setFrameShape( QFrame::NoFrame );
    m_objectSettingsScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_objectSettingsScrollArea->setWidgetResizable( true );
    m_leftLayout->addWidget( m_objectSettingsScrollArea );
    connect( objectListWidget, SIGNAL( ObjectSettingsWidgetChanged(QWidget*)), this, SLOT(ObjectListWidgetChanged(QWidget*)) );

    // -----------------------------------------
    // Create main QuadView window
    // -----------------------------------------
    m_4Views = dynamic_cast<QuadViewWindow*>(Application::GetSceneManager()->CreateQuadViewWindow( this ));

    // -----------------------------------------
    // Create right panel (1 tab per plugin)
    // -----------------------------------------
    m_rightPanel = new QTabWidget( this );
    m_rightPanel->setTabsClosable( true );
    connect( m_rightPanel, SIGNAL(tabCloseRequested(int)), this, SLOT(PluginTabClosed(int)) );

    // -----------------------------------------
    // Create main splitter
    // -----------------------------------------
    m_mainSplitter = new QSplitter(this);
    m_mainSplitter->setContentsMargins( 5, 5, 5, 5 );
    m_mainSplitter->addWidget( m_leftFrame );
    m_mainSplitter->addWidget( m_4Views );
    m_mainSplitter->addWidget( m_rightPanel );
    setCentralWidget( m_mainSplitter );
    connect( m_mainSplitter, SIGNAL(splitterMoved(int,int)), this, SLOT(MainSplitterMoved(int,int) ));

    // -----------------------------------------
    // Setup program icon
    // -----------------------------------------
    QPixmap icon;
    bool validIcon = icon.load( ":/Icons/ibis.png" );
    if (validIcon)
        setWindowIcon( icon );
    
    statusBar()->hide();

    // -----------------------------------------
    // Get window settings from the application
    // -----------------------------------------
    Application::GetSceneManager()->UpdateBackgroundColor();

    UpdateMainSplitter();

    // Tell the qApp unique instance to send event to MainWindow::eventFilter before anyone else
    // so that we can grab global keyboard shortcuts.
    qApp->installEventFilter(this);
}

void MainWindow::OnStartMainLoop()
{
    // Open all files specified before init (command-line)
    OpenFileParams params;
    params.SetAllFileNames( Application::GetInstance().GetInitialDataFiles() );
    OpenFiles( &params );

    Application::GetInstance().OnStartMainLoop();
}


void MainWindow::CreatePluginsUi()
{
    // Add menu entries
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    for( int i = 0; i < allTools.size(); ++i )
    {
        ToolPluginInterface * toolPlugin = allTools[i];
        if( toolPlugin->CanRun() )
        {
            QString menuEntry = toolPlugin->GetMenuEntryString();
            QAction * action = new QAction( menuEntry, this );
            action->setData( QVariant(toolPlugin->GetPluginName()) );
            action->setCheckable( true );
            connect( action, SIGNAL(toggled(bool)), this, SLOT(ToolPluginsMenuActionToggled(bool)) );
            m_pluginMenu->addAction(action);
            m_pluginActions[ toolPlugin ] = action;
            if( toolPlugin->GetSettings().active )
               action->toggle();
        }
    }
    connect( &(Application::GetInstance()), SIGNAL(QueryActivatePluginSignal(ToolPluginInterface*,bool)), this, SLOT(ToggleToolPlugin(ToolPluginInterface*,bool)) );
}

void MainWindow::CreateNewObjectPluginsUi(QMenu *menu)
{
    QList<ObjectPluginInterface*> allObjectPlugins;
    Application::GetInstance().GetAllObjectPlugins( allObjectPlugins );
    for( int i = 0; i < allObjectPlugins.size(); ++i )
    {
        ObjectPluginInterface * objectPlugin = allObjectPlugins[i];
        QString menuEntry = objectPlugin->GetMenuEntryString();
        QAction * action = new QAction( menuEntry, this );
        action->setData( QVariant(objectPlugin->GetPluginName()) );
        connect( action, SIGNAL(triggered()), this, SLOT(ObjectPluginsMenuActionTriggered()) );
        menu->addAction(action);
    }
}

QAction * MainWindow::AddToggleAction( QMenu * menu, const QString & title, const char * member, const QKeySequence & shortcut, bool checked )
{
    QAction * action = menu->addAction( title, this, member, shortcut );
    action->setCheckable( true );
    action->setChecked( checked );
    return action;
}

MainWindow::~MainWindow()
{
}

void MainWindow::about()
{
    AboutBICIgns *a = new AboutBICIgns(this, "AboutIbis");
    a->setAttribute( Qt::WA_DeleteOnClose, true );
    QString version = Application::GetInstance().GetFullVersionString();
    QString buildDate;  // simtodo : figure out if we need this and implement it.
    a->Initialize( "IBIS NeuroNav\nIntraoperative Brain Imaging System", version, buildDate );
    a->show();   
}

void MainWindow::AboutPlugins()
{
    AboutPluginsWidget * w = new AboutPluginsWidget;
    w->setAttribute( Qt::WA_DeleteOnClose, true );
    w->setWindowFlags( Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint );
    w->show();
}

void MainWindow::fileOpenFile()
{
    // Get filenames
    QString lastVisitedDir = Application::GetInstance().GetSettings()->WorkingDirectory;
    if(!QFile::exists(lastVisitedDir))
    {
        lastVisitedDir = QDir::homePath();
    }
    OpenFileParams params;
    params.lastVisitedDir = lastVisitedDir;
    params.defaultParent = Application::GetSceneManager()->GetSceneRoot();
    OpenDataFileDialog * dialog = new OpenDataFileDialog( this, nullptr, Application::GetSceneManager(), &params );

    int result = dialog->exec();

    // Process filenames
    if( result == QDialog::Accepted )
    {
        OpenFiles( &params );
    }

    delete dialog;
}

void MainWindow::OpenFiles( OpenFileParams * params )
{
    Application::GetInstance().OpenFiles( params );
}

void MainWindow::fileExportFile()
{
    SceneObject *obj = Application::GetSceneManager()->GetCurrentObject();
    if (obj)
        obj->Export();
}

void MainWindow::fileImportUsAcquisition()
{
    Application::GetInstance().ImportUsAcquisition();
}

void MainWindow::fileImportCamera()
{
    Application::GetInstance().ImportCamera();
}

void MainWindow::NewPointSet()
{
    SceneManager * manager = Application::GetSceneManager();
    PointsObject *pointsObject = PointsObject::New();
    pointsObject->SetName( tr("PointSetToRename") );
    manager->AddObject( pointsObject );
    manager->SetCurrentObject( pointsObject );
    pointsObject->Delete();
}

void MainWindow::ViewXPlaneToggled(bool viewOn)
{
    SceneManager * manager = Application::GetSceneManager();
    manager->ViewPlane( 0, viewOn );
}

void MainWindow::ViewYPlaneToggled(bool viewOn)
{
    SceneManager * manager = Application::GetSceneManager();
    manager->ViewPlane( 1, viewOn );
}

void MainWindow::ViewZPlaneToggled(bool viewOn)
{
    SceneManager * manager = Application::GetSceneManager();
    manager->ViewPlane( 2, viewOn );
}

void MainWindow::ViewAllPlanes()
{
    SceneManager * manager = Application::GetSceneManager();
    manager->ViewAllPlanes( 1 );
}

void MainWindow::HideAllPlanes()
{
    SceneManager * manager = Application::GetSceneManager();
    manager->ViewAllPlanes( 0 );
}

void MainWindow::View3DFront( )
{
    SceneManager * manager = Application::GetSceneManager();
    manager->SetStandardView(SV_FRONT);
}

void MainWindow::View3DLeft( )
{
    SceneManager * manager = Application::GetSceneManager();
    manager->SetStandardView(SV_LEFT);
}

void MainWindow::View3DRight( )
{
    SceneManager * manager = Application::GetSceneManager();
    manager->SetStandardView(SV_RIGHT);
}

void MainWindow::View3DBack( )
{
    SceneManager * manager = Application::GetSceneManager();
    manager->SetStandardView(SV_BACK);
}

void MainWindow::View3DTop( )
{
    SceneManager * manager = Application::GetSceneManager();
    manager->SetStandardView(SV_TOP);
}

void MainWindow::View3DBottom( )
{
    SceneManager * manager = Application::GetSceneManager();
    manager->SetStandardView(SV_BOTTOM);
}

void MainWindow::ViewResetPlanes( )
{
    SceneManager * manager = Application::GetSceneManager();
    manager->ResetCursorPosition();
}

void MainWindow::ViewFullscreen()
{
    if( isFullScreen() )
    {
        showNormal();
    }
    else
    {
        showFullScreen();
    }
}

void MainWindow::ModifyFileMenu()
{
    QMenu * fileMenu = qobject_cast<QMenu *>(sender());
    SceneObject *obj = Application::GetSceneManager()->GetCurrentObject();
    QList <QAction*> a = fileMenu->actions();
    for (int i = 0; i < a.count(); i++)
    {
        if (a[i]->text() == "E&xport")
        {
            if ( obj && (obj->IsExportable()) )
            { // enable export
                a[i]->setEnabled(true);
            }
            else // disable export
            {
                a[i]->setEnabled(false);
            }
        }
    }
}

void MainWindow::ModifyNewObjectFileMenu()
{
    QMenu * fileMenu = qobject_cast<QMenu *>(sender());
    fileMenu->clear();
    fileMenu->addAction( tr("&Point Set"), this, SLOT( NewPointSet() ));
    QList<ObjectPluginInterface*> allObjectPlugins;
    Application::GetInstance().GetAllObjectPlugins( allObjectPlugins );
    for( int i = 0; i < allObjectPlugins.size(); ++i )
    {
        ObjectPluginInterface * objectPlugin = allObjectPlugins[i];
        if( objectPlugin->CanBeActivated() )
        {
            QString menuEntry = objectPlugin->GetMenuEntryString();
            QAction * action = new QAction( menuEntry, this );
            action->setData( QVariant(objectPlugin->GetPluginName()) );
            connect( action, SIGNAL(triggered()), this, SLOT(ObjectPluginsMenuActionTriggered()) );
            fileMenu->addAction(action);
        }
    }
}

void MainWindow::FileGenerateMenuAboutToShow()
{
    QMenu * generateMenu = qobject_cast<QMenu *>(sender());
    generateMenu->clear();
    QList<GeneratorPluginInterface*> allGeneratePlugins;
    Application::GetInstance().GetAllGeneratorPlugins( allGeneratePlugins );
    foreach( GeneratorPluginInterface * plugin, allGeneratePlugins )
    {
        QAction * action = new QAction( plugin->GetMenuEntryString(), this );
        action->setData( QVariant(plugin->GetPluginName()) );
        action->setEnabled( plugin->CanRun() );
        connect( action, SIGNAL(triggered()), this, SLOT(GeneratePluginsMenuActionTriggered()) );
        generateMenu->addAction(action);
    }
}

void MainWindow::ModifyViewMenu()
{
    SceneManager * manager = Application::GetSceneManager();
    m_viewXPlaneAction->setChecked(manager->IsPlaneVisible(0));
    m_viewYPlaneAction->setChecked(manager->IsPlaneVisible(1));
    m_viewZPlaneAction->setChecked(manager->IsPlaneVisible(2));
}

void MainWindow::fileSaveScene()
{
    this->SaveScene(false);
}

void MainWindow::fileSaveSceneAs()
{
    this->SaveScene(true);
}

void MainWindow::SaveScene(bool asFile)
{
    SceneManager * manager = Application::GetSceneManager();
    QString sceneDir = manager->GetSceneDirectory();
    QString fileName = manager->GetSceneFile();
    if (asFile || sceneDir.isEmpty() )
    {
        QString initialFile = manager->GetSceneDirectory() + "/scene.xml";
        fileName = Application::GetInstance().GetFileNameSave( tr("Save Scene"), initialFile, tr("xml file (*.xml)") );
        if (!fileName.isEmpty())
        {
            QFileInfo info( fileName );
            sceneDir = info.dir().absolutePath();
            manager->SetSceneDirectory( sceneDir );
            manager->SetSceneFile( fileName );
            Application::GetInstance().GetSettings()->WorkingDirectory = sceneDir;
        }
        else
            return;
    }

    Application::GetInstance().SaveScene( fileName );
}

void MainWindow::fileLoadScene()
{
    SceneManager * manager = Application::GetSceneManager();
    bool sceneNotEmpty = manager->GetNumberOfUserObjects() > 0;
    if( sceneNotEmpty )
    {
        int ret = QMessageBox::warning(this, "Load Scene",
                                       tr("All the objects currently in the scene will be removed."),
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No | QMessageBox::Escape);
        if (ret == QMessageBox::No)
            return;
    }
    manager->NewScene();

    QString workingDir = Application::GetInstance().GetSettings()->WorkingDirectory;
    if(!QFile::exists(workingDir))
    {
        workingDir = QDir::homePath();
    }

    QString fileName = Application::GetInstance().GetFileNameOpen( tr("Load Scene"), workingDir, tr("Scene files (*.xml)") );

    if( !fileName.isEmpty() )
    {
        QFileInfo info( fileName );
        QString newDir = info.dir().absolutePath();
        Application::GetInstance().GetSettings()->WorkingDirectory = newDir;
        Application::GetInstance().LoadScene( fileName );
    }
}

void MainWindow::fileNewScene()
{
    SceneManager * manager = Application::GetSceneManager();
    bool sceneNotEmpty = manager->GetNumberOfUserObjects() > 0;
    if( sceneNotEmpty )
    {
        int ret = QMessageBox::warning(this, "New Scene",
                                       tr("All the objects currently in the scene will be removed."),
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No | QMessageBox::Escape);
        if (ret == QMessageBox::No)
            return;
    }
    Application::GetInstance().GetSettings()->WorkingDirectory = QDir::homePath();
    manager->SetSceneDirectory( "" ); // to force directory selection
    manager->NewScene();
}

void MainWindow::ObjectListWidgetChanged( QWidget * newWidget )
{
    if( newWidget )
    {
        m_objectSettingsScrollArea->setWidget( newWidget );
    }
}

void MainWindow::ToggleToolPlugin( ToolPluginInterface * toolPlugin, bool isOn )
{
    QAction * action = m_pluginActions[ toolPlugin ];
    if( action->isChecked() != isOn )
        action->toggle();
}

void MainWindow::ToolPluginsMenuActionToggled( bool isOn )
{
    QAction * action = qobject_cast<QAction *>(sender());
    ToolPluginInterface * toolPlugin = Application::GetInstance().GetToolPluginByName( action->data().toString() );
    Q_ASSERT_X( toolPlugin, "MainWindow::ToolPluginsMenuActionToggled()", "Plugin doesn't exist but should." );

    if( isOn )
    {
        // try creating a tab first
        QWidget * pluginWidget = toolPlugin->CreateTab();
        if( pluginWidget )
        {
            pluginWidget->setAttribute( Qt::WA_DeleteOnClose );
            m_pluginTabs[ action ] = pluginWidget;
            m_rightPanel->addTab( pluginWidget, toolPlugin->GetMenuEntryString() );
            if( m_pluginTabs.size() == 1 )
            {
                UpdateMainSplitter();
            }
            toolPlugin->GetSettings().active = true;
        }
        else
        {
            pluginWidget = toolPlugin->CreateFloatingWidget();
            if( pluginWidget )
            {
                pluginWidget->setAttribute( Qt::WA_DeleteOnClose );
                pluginWidget->setWindowFlags( Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint );
                pluginWidget->setWindowTitle( toolPlugin->GetMenuEntryString() );
                connect( pluginWidget, SIGNAL(destroyed()), this, SLOT(FloatingPluginWidgetClosed()) );
                m_pluginWidgets[ action ] = pluginWidget;
                toolPlugin->GetSettings().active = true;
                if( toolPlugin->GetSettings().winSize != QSize( -1, -1 ) )
                {
                    pluginWidget->resize( toolPlugin->GetSettings().winSize );
                    pluginWidget->move( toolPlugin->GetSettings().winPos );
                }
                this->ShowFloatingDock( pluginWidget );
            }
        }
    }
    else
    {
        if( m_pluginTabs.contains( action ) )
        {
            QWidget * w = m_pluginTabs.value( action );
            int tabIndex = m_rightPanel->indexOf( w );
            ClosePluginTab( action, tabIndex );
        }
        else if( m_pluginWidgets.contains( action ) )
        {
            QWidget * w = m_pluginWidgets.value( action );
            toolPlugin->GetSettings().winPos = w->pos();
            toolPlugin->GetSettings().winSize = w->size();
            toolPlugin->GetSettings().active = false;
            w->close();
        }
    }
}

void MainWindow::FloatingPluginWidgetClosed()
{
    QWidget * closedWidget = qobject_cast<QWidget *>(sender());
    Q_ASSERT_X( closedWidget, "MainWindow::FloatingPluginWidgetClosed()", "Widget closed is no longer valid." );

    QAction * pluginAction = m_pluginWidgets.key( closedWidget );
    Q_ASSERT_X( pluginAction, "MainWindow::FloatingPluginWidgetClosed()", "No action associated with closed widget." );

    QString pluginName = pluginAction->data().toString();
    ToolPluginInterface * toolPlugin = Application::GetInstance().GetToolPluginByName( pluginName );
    Q_ASSERT_X( toolPlugin, "MainWindow::ClosePluginTab()", "Plugin doesn't exist but should." );

    // give plugin a chance to survive
    if( !toolPlugin->WidgetAboutToClose() )
        return;

    // remove the association between action and widget and uncheck action without generating a signal
    m_pluginWidgets.remove( pluginAction );
    pluginAction->blockSignals( true );
    pluginAction->setChecked( false );
    pluginAction->blockSignals( false );

    // Tell the plugin it is not active anymore (for settings)
    if( !m_windowClosing )
        toolPlugin->GetSettings().active = false;
    toolPlugin->GetSettings().winPos = closedWidget->pos();
    toolPlugin->GetSettings().winSize = closedWidget->size();
}

void MainWindow::PluginTabClosed( int tabIndex )
{
    QWidget * w = m_rightPanel->widget( tabIndex );
    Q_ASSERT_X( w, "MainWindow::PluginTabClosed( int tabIndex )", "Closed tab not part of the QTabWidget.");

    QAction * pluginAction = m_pluginTabs.key( w );
    Q_ASSERT_X( pluginAction, "MainWindow::PluginTabClosed( int tabIndex )", "No action associated with closed widget." );

    // close the tab and cleanup
    ClosePluginTab( pluginAction, tabIndex );

    // reset the action without generating a signal
    pluginAction->blockSignals( true );
    pluginAction->setChecked( false );
    pluginAction->blockSignals( false );
}

void MainWindow::ClosePluginTab( QAction * action, int index )
{
    // give plugin a chance to survive
    QString pluginName = action->data().toString();
    ToolPluginInterface * toolPlugin = Application::GetInstance().GetToolPluginByName( pluginName );
    Q_ASSERT_X( toolPlugin, "MainWindow::ClosePluginTab()", "Plugin doesn't exist but should." );
    if( !toolPlugin->WidgetAboutToClose() )
        return;

    // remove tab
    QWidget * w = m_rightPanel->widget( index );
    m_rightPanel->removeTab( index );

    // close the widget to destroy it
    w->close();

    // remove action/widget association
    m_pluginTabs.remove( action );

    // hide panel if no more tabs are open
    if( m_pluginTabs.size() == 0 )
        UpdateMainSplitter();

    // Tell the plugin it is not active anymore (for settings)
    toolPlugin->GetSettings().active = false;
}

void MainWindow::ObjectPluginsMenuActionTriggered()
{
    QAction * action = qobject_cast<QAction *>(sender());
    QString pluginName = action->data().toString();
    ObjectPluginInterface * p = Application::GetInstance().GetObjectPluginByName( pluginName );
    Q_ASSERT( p );
    SceneObject *obj = p->CreateObject();
    if( obj )
        Application::GetSceneManager()->SetCurrentObject( obj );
}

void MainWindow::GeneratePluginsMenuActionTriggered()
{
    QAction * action = qobject_cast<QAction *>(sender());
    QString pluginName = action->data().toString();
    GeneratorPluginInterface * p = Application::GetInstance().GetGeneratorPluginByName( pluginName );
    Q_ASSERT( p );
    p->Run();
}

void MainWindow::MainSplitterMoved( int /*pos*/, int /*index*/ )
{
    QList<int> sizes = m_mainSplitter->sizes();
    m_leftPanelSize = sizes[0];
    if( m_pluginTabs.size() )
        m_rightPanelSize = sizes[2];
}

void MainWindow::UpdateMainSplitter()
{
    QList<int> sizes;
    if( m_pluginTabs.size() == 0 )
    {
        sizes.push_back( m_leftPanelSize );
        sizes.push_back( width() - m_leftPanelSize );
        sizes.push_back( 0 );
    }
    else
    {
        sizes.push_back( m_leftPanelSize );
        sizes.push_back( width() - m_leftPanelSize - m_rightPanelSize );
        sizes.push_back( m_rightPanelSize );
    }
    m_mainSplitter->setSizes( sizes );
}

void MainWindow::closeEvent( QCloseEvent * event )
{
    m_windowClosing = true;

    // Tell all plugins their window/tab is about to close
    QList<ToolPluginInterface*> allTools;
    Application::GetInstance().GetAllToolPlugins( allTools );
    foreach( ToolPluginInterface * toolPlugin, allTools )
    {
        if( toolPlugin->IsPluginActive() )
            toolPlugin->WidgetAboutToClose();
    }

    // Close all open windows appart from the main window
    foreach (QWidget *widget, QApplication::allWidgets())
    {
        if (widget != this)
            widget->close();
    }

    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
 {
     event->acceptProposedAction();
 }

 void MainWindow::dragMoveEvent(QDragMoveEvent *event)
 {
     event->acceptProposedAction();
 }

#include <QUrl>

 void MainWindow::dropEvent( QDropEvent * e )
 {
     const QMimeData * mimeData = e->mimeData();
     OpenFileParams params;
     if( mimeData->hasUrls() )
     {
         QList<QUrl> urlList = mimeData->urls();
         for (int i = 0; i < urlList.size(); ++i)
         {
             QString url = urlList.at(i).toLocalFile();
             params.AddInputFile( url );
         }
     }
     params.defaultParent = Application::GetSceneManager()->GetSceneRoot();
     OpenFiles( &params );
     e->acceptProposedAction();
 }

 void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
 {
     event->accept();
 }

// Capture OpenFile event sent to QApplication: mainly to support OpenWith functionality on OSX
bool MainWindow::eventFilter( QObject * obj, QEvent * event )
{
    bool handled = false;
    if( event->type() == QEvent::KeyPress )
    {
        QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
        handled = Application::GetInstance().GlobalKeyEvent( keyEvent );
    }
    else if( event->type() == QEvent::FileOpen )
	{
		QFileOpenEvent * foe = static_cast<QFileOpenEvent *> (event);
		QString filename = foe->file();
        OpenFileParams params;
        params.AddInputFile( filename );
        params.defaultParent = Application::GetSceneManager()->GetSceneRoot();
		OpenFiles( &params );
        handled = true;
	}

    if( !handled )
		return QObject::eventFilter(obj, event);		

    return true;
}

void MainWindow::AddBottomWidget( QWidget * w )
{
    m_4Views->AddBottomWidget( w );
}

void MainWindow::RemoveBottomWidget( QWidget * w )
{
    m_4Views->RemoveBottomWidget( w );
}

void MainWindow::ShowFloatingDock( QWidget * w, QFlags<QDockWidget::DockWidgetFeature> features )
{
    QDockWidget * dock = new QDockWidget( this );
    dock->setWindowTitle( w->windowTitle() );
    dock->resize( w->size() );
    dock->setAttribute( Qt::WA_DeleteOnClose );
    dock->setFeatures( features );
    dock->setAllowedAreas( Qt::NoDockWidgetArea );
    dock->setFloating( true );
    dock->move(this->pos().x() + this->size().width()/2 - dock->size().width()/2,
               this->pos().y() + this->size().height()/2 - dock->size().height()/2 );
    dock->setWidget( w );
    connect( w, SIGNAL(destroyed()), dock, SLOT(close()) );
    dock->show();
    dock->activateWindow();
}

void MainWindow::SetShowToolbar( bool show )
{
    m_4Views->SetShowToolbar( show );
}

void MainWindow::SetShowLeftPanel( bool show )
{
    m_leftFrame->setHidden( !show );
}

void MainWindow::SetShowRightPanel( bool show )
{
    m_rightPanel->setHidden( !show );
}

void MainWindow::Serialize( Serializer * ser )
{
    m_4Views->Serialize( ser );
}

void MainWindow::LoadSettings( QSettings & s )
{
    s.beginGroup( "MainWindow" );

    const QByteArray geometry = s.value( "geometry", QByteArray() ).toByteArray();
    if( geometry.isEmpty() )
    {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        int w = static_cast<int>(availableGeometry.width() * 0.7);
        int h = static_cast<int>(availableGeometry.height() * 0.6);
        resize( w , h );
        move( (availableGeometry.width() - width()) / 2, (availableGeometry.height() - height()) / 2 );
    }
    else
    {
        restoreGeometry(geometry);
    }

    m_leftPanelSize = s.value( "MainWindowLeftPanelSize", 150 ).toInt();
    m_rightPanelSize = s.value( "MainWindowRightPanelSize", 150 ).toInt();
    m_4Views->LoadSettings( s );
    s.endGroup();
    UpdateMainSplitter();
}

void MainWindow::SaveSettings( QSettings & s )
{
    s.beginGroup( "MainWindow" );
    s.setValue("geometry", saveGeometry());
    s.setValue( "MainWindowLeftPanelSize", m_leftPanelSize );
    s.setValue( "MainWindowRightPanelSize", m_rightPanelSize );
    m_4Views->SaveSettings( s );
    s.endGroup();
}

void MainWindow::Preferences()
{
   Application::GetInstance().Preferences();
}
