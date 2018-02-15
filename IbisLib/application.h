/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __Application_h_
#define __Application_h_

#include <QString>
#include <QSize>
#include <QPoint>
#include <QColor>
#include "serializer.h"
#include <vector>
#include "ibistypes.h"
#include "globaleventhandler.h"
#include "serializer.h"

// forward declarations
class QSettings;
class HardwareModule;
class UpdateManager;
class SceneManager;
class IbisAPI;
class SceneObject;
class WorldObject;
class CameraObject;
class IbisPlugin;
class ToolPluginInterface;
class ObjectPluginInterface;
class GeneratorPluginInterface;
class OpenFileParams;
class FileReader;
class LookupTableManager;
class QProgressDialog;
class QTimer;
class QDialog;
class vtkMatrix4x4;
class vtkImageData;
class MainWindow;
class ImageObject;
class PointsObject;
class QMenu;

struct ApplicationSettings
{
    ApplicationSettings();
    ~ApplicationSettings();
    void LoadSettings( QSettings & settings );
    void SaveSettings( QSettings & settings );
    QString WorkingDirectory;
    QColor ViewBackgroundColor;
    QColor CutPlanesCursorColor;
    InteractorStyle InteractorStyle3D;
    double CameraViewAngle3D;
    bool ShowCursor;
    bool ShowAxes;
    bool ViewFollowsReference;
    int ShowXPlane;
    int ShowYPlane;
    int ShowZPlane;
    int TripleCutPlaneResliceInterpolationType;
    int TripleCutPlaneDisplayInterpolationType;
    bool VolumeRendererEnabled;
    double UpdateFrequency;
    bool ShowMINCConversionWarning;
    QList<QString> PluginsWithOpenWidget;
    QList<QString> PluginsWithOpenTab;
};

//===============================================================================
// Main application class (Singleton)
//===============================================================================
class Application : public QObject
{

    Q_OBJECT

public:

    ~Application();

    static void CreateInstance( bool viewerOnly );
    static void DeleteInstance();
    static Application & GetInstance();

    void SetMainWindow( MainWindow * mw );
    void LoadWindowSettings();
    MainWindow * GetMainWindow() { return m_mainWindow; }
    void AddBottomWidget( QWidget * w );
    void RemoveBottomWidget( QWidget * w );

    void OnStartMainLoop();
    void AddGlobalEventHandler( GlobalEventHandler * h );
    void RemoveGlobalEventHandler( GlobalEventHandler * h );
    bool GlobalKeyEvent( QKeyEvent * e );

    void InitHardware();
    void InitAPI();

    void AddHardwareSettingsMenuEntries( QMenu * menu );
    void AddToolObjectsToScene();
    void RemoveToolObjectsFromScene();

    bool IsViewerOnly() { return m_viewerOnly; }

    // Info on version
    QString GetFullVersionString();
    QString GetVersionString();
    static QString GetConfigDirectory();
    static QString GetGitHashShort();

    static SceneManager * GetSceneManager();
    static LookupTableManager * GetLookupTableManager();

    // Application Settings (saved automatically at each run )
    ApplicationSettings * GetSettings();
    void SetUpdateFrequency( double fps );
    double GetUpdateFrequency() { return GetSettings()->UpdateFrequency; }

    // Ibis Plugin management
    void LoadPlugins();
    IbisPlugin * GetPluginByName( QString name );
    void ActivatePluginByName( const char * name, bool active );
    ObjectPluginInterface *GetObjectPluginByName( QString className );
    ToolPluginInterface * GetToolPluginByName( QString name );
    GeneratorPluginInterface * GetGeneratorPluginByName( QString name );
    SceneObject * GetGlobalObjectInstance( const QString & className );
    void GetAllGlobalObjectInstances( QList<SceneObject*> & allInstances );
    void GetAllPlugins( QList<IbisPlugin*> & allPlugins );
    void GetAllToolPlugins( QList<ToolPluginInterface*> & allTools );
    void GetAllObjectPlugins( QList<ObjectPluginInterface*> & allObjects );
    void GetAllGeneratorPlugins( QList<GeneratorPluginInterface*> & allObjects );

    // Data file to load when the application starts up (typically specified on the command line)
    void SetInitialDataFiles( const QStringList & files ) { m_initialDataFiles = files; }
    const QStringList & GetInitialDataFiles() { return m_initialDataFiles; }

    void OpenFiles( OpenFileParams * params, bool addToScene = true );
    bool OpenTransformFile( const char * filename, SceneObject * obj = 0 );
    bool OpenTransformFile( const char * filename, vtkMatrix4x4 * mat );
    void ImportUsAcquisition();
    void ImportCamera();

    // Getting data from files
    bool GetImageDataFromVideoFrame(QString fileName, vtkImageData *img , vtkMatrix4x4 *mat );
    bool GetPointsFromTagFile(QString fileName, PointsObject *pts1, PointsObject *pts2 );

    // Useful modal dialog
    QString GetOpenFileName( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetSaveFileName( const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString() );
    QString GetExistingDirectory( const QString & caption = QString(), const QString & dir = QString() );
    bool GetOpenFileSequence( QStringList & filenames, QString extension, const QString & caption, const QString & dir, const QString & filter );
    int LaunchModalDialog( QDialog * d );
    void Warning( const QString &title, const QString & text );

    QProgressDialog * StartProgress( int max, const QString & caption = QString() );
    void StopProgress( QProgressDialog * progressDialog);

    void ShowMinc1Warning( bool cando);

    void LoadScene( QString fileName );
    void SaveScene( QString fileName );

public slots:
    void UpdateProgress( QProgressDialog*, int current );
    void SaveSettings();

private slots:

    void OpenFilesProgress();

signals:

    void QueryActivatePluginSignal( ToolPluginInterface * toolPlugin, bool isOn );

    // This is the main clock of the application. It tell the rest of IbisLib and plugins that
    // the hardware modules have been updated.
    void IbisClockTick();

private:

    void Init( bool viewerOnly );
    Application();
    static Application  * m_uniqueInstance;

    // Update manager is the only one to call TickIbisClock which will
    // update the hardware module and tell the whole world (emit IbisClockTick())
    friend class UpdateManager;
    void TickIbisClock();
    bool PreModalDialog();
    void PostModalDialog();

    // Open file progress dialog management
    FileReader * m_fileReader;
    QProgressDialog * m_fileOpenProgressDialog;
    QTimer * m_progressDialogUpdateTimer;

    MainWindow                  * m_mainWindow;
    SceneManager                * m_sceneManager;
    IbisAPI                     * m_ibisAPI;
    UpdateManager               * m_updateManager;
    QList<HardwareModule*>        m_hardwareModules;
    LookupTableManager          * m_lookupTableManager;
    QList<GlobalEventHandler*>    m_globalEventHandlers;

    ApplicationSettings m_settings;

    // Data file to load when the application starts up (typically specified on the command line)
    QStringList m_initialDataFiles;

    bool m_viewerOnly;

    static const QString m_appName;
    static const QString m_appOrganisation;

};


#endif
