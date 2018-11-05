/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __MainWindow_h_
#define __MainWindow_h_

#include <QMainWindow>
#include <QMap>
#include "serializer.h"

class QAction;
class QFrame;
class QVBoxLayout;
class QSpacerItem;
class QProgressDialog;
class QSplitter;
class QScrollArea;
class QSettings;
class ImageObject;
class SceneObject;
class ImageMixerWidget;
class OpenFileParams;
class ToolPluginInterface;
class QuadViewWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow( QWidget * parent = 0 );
    ~MainWindow();

    virtual void Serialize( Serializer * ser );
    void AddBottomWidget( QWidget * w );
    void RemoveBottomWidget( QWidget * w );
    void SetShowToolbar( bool show );
    void SetShowLeftPanel( bool show );
    void SetShowRightPanel( bool show );

    void LoadSettings( QSettings & s );
    void SaveSettings( QSettings & s );

public slots:

    void OnStartMainLoop();

private slots:

    void about();
    void AboutPlugins();
    void fileOpenFile(); 
    void fileExportFile();
    void fileImportUsAcquisition();
    void fileImportCamera();
    void fileSaveScene();
    void fileSaveSceneAs();
    void fileLoadScene();
    void fileNewScene();
    void NewPointSet();
    void ModifyFileMenu( );
    void ModifyNewObjectFileMenu( );
    void FileGenerateMenuAboutToShow();
    void ModifyViewMenu( );
    void ViewXPlaneToggled( bool );
    void ViewYPlaneToggled( bool );
    void ViewZPlaneToggled( bool );
    void ViewAllPlanes();
    void HideAllPlanes();
    void View3DFront( );
    void View3DLeft( );
    void View3DRight( );
    void View3DBack( );
    void View3DTop( );
    void View3DBottom( );
    void ViewResetPlanes( );
    void ViewFullscreen();
    void ObjectListWidgetChanged(QWidget*);
    void ToggleToolPlugin( ToolPluginInterface * toolPlugin, bool isOn );
    void ToolPluginsMenuActionToggled(bool);
    void FloatingPluginWidgetClosed();
    void PluginTabClosed( int tabIndex );
    void ObjectPluginsMenuActionTriggered();
    void GeneratePluginsMenuActionTriggered();
    void MainSplitterMoved( int pos, int index );
    void SaveScene(bool);
    void Preferences();

protected:

    void OpenFiles( OpenFileParams * params );
    void CreatePluginsUi();
    void CreateNewObjectPluginsUi(QMenu *);
    QAction * AddToggleAction( QMenu * menu, const QString & title, const char * member, const QKeySequence & shortcut, bool checked );
    void closeEvent( QCloseEvent * event );
    void ClosePluginTab( QAction * action, int index );
    void UpdateMainSplitter();

    // Handling of Drag and Drop
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
	
	// Capture events sent to application (ex.:fileopen event on OSX)
	bool eventFilter(QObject *obj, QEvent *event);

    QuadViewWindow *m_4Views;
    QAction * m_viewXPlaneAction;
    QAction * m_viewYPlaneAction;
    QAction * m_viewZPlaneAction;
    QAction * m_showAllPlanesAction;
    QAction * m_hideAllPlanesAction;

    QMenu * m_pluginMenu;

    static const QString m_appName;
    QString m_pathSettings;

    QTabWidget * m_rightPanel;
    QSplitter * m_mainSplitter;
    int m_leftPanelSize;
    int m_rightPanelSize;
    QFrame * m_leftFrame;
    QVBoxLayout * m_leftLayout;
    QScrollArea * m_objectSettingsScrollArea;
    QSpacerItem * m_leftEndSpacer;

    typedef QMap< QAction*, QWidget* > PluginWidgetMap;
    PluginWidgetMap m_pluginWidgets;
    PluginWidgetMap m_pluginTabs;

    typedef QMap< ToolPluginInterface*, QAction* > PluginActionMap;
    PluginActionMap m_pluginActions;

    bool m_windowClosing;

};

ObjectSerializationHeaderMacro( MainWindow );

#endif
