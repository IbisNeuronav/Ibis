/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef APPLICATION_H
#define APPLICATION_H

#include <QColor>
#include <QDockWidget>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>
#include <vector>

#include "globaleventhandler.h"
#include "ibisitkvtkconverter.h"
#include "ibistypes.h"
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
class IbisPreferences;

/**
 * @class   Application
 * @brief   Main application class (Singleton)
 *
 *  Application will create on start all the main  elements of ibis: the main window, menus, docks etc.
 */

/**
 * @struct  ApplicationSettings
 * @brief   Global application settings stored as QSettings
 *
 * On Ubuntu saved in ~/.config/MNI-BIC-IPL/ibis.conf.
 *
 * On Windows 10 saved in registry HKEY_CURRENT_USER/SOFTWARE/MNI-BIC-IPL.
 *
 * The settings are loaded on ibis start and saved on exit.
 *
 **/
struct ApplicationSettings
{
    ApplicationSettings();
    ~ApplicationSettings();
    void LoadSettings( QSettings & settings );
    void SaveSettings( QSettings & settings );
    QString WorkingDirectory;
    QColor ViewBackgroundColor;
    QColor View3DBackgroundColor;
    QColor CutPlanesCursorColor;
    InteractorStyle InteractorStyle3D;
    double CameraViewAngle3D;
    bool ShowCursor;
    bool ShowAxes;
    bool ViewFollowsReference;
    /** @name Cutting planes
     *  @brief these variables are used to set respective planes visibility
     *
     *  Axis X is red, axis Y is yellow, axis Z is green.
     * */
    ///@{
    /** XPlane formed by axes Y and Z is later known as Sagittal plane in ImagePlane settings and in View menu */
    int ShowXPlane;
    /** YPlane formed by axes X and Z is later known as Coronal plane in ImagePlane settings and in View menu */
    int ShowYPlane;
    /** ZPlane formed by axes X and Y is later known as Transverse plane in ImagePlane settings and in View menu */
    int ShowZPlane;
    ///@}

    int TripleCutPlaneResliceInterpolationType;
    int TripleCutPlaneDisplayInterpolationType;
    bool VolumeRendererEnabled;
    double UpdateFrequency;
    bool ShowMINCConversionWarning;
    QList<QString> PluginsWithOpenWidget;
    QList<QString> PluginsWithOpenTab;
};

//===============================================================================
class Application : public QObject
{
    Q_OBJECT

public:
    ~Application();

    static void CreateInstance( bool viewerOnly );
    static void DeleteInstance();
    /** Get pointer to the unique application instance. */
    static Application & GetInstance();

    /** Set up GUI and plugins. */
    void SetMainWindow( MainWindow * mw );
    /** Restore previous MainWindow settings - position, size, etc. */
    void LoadWindowSettings();
    /** Get pointer to MainWindow. */
    MainWindow * GetMainWindow() { return m_mainWindow; }
    /** Add a widget at the bottom of the main window, mainly used by plugins. */
    void AddBottomWidget( QWidget * w );
    /** Remove the bottom widget. */
    void RemoveBottomWidget( QWidget * w );

    /** Popup a dialog. */
    void ShowFloatingDock( QWidget * w,
                           QFlags<QDockWidget::DockWidgetFeature> features = QDockWidget::DockWidgetClosable |
                                                                             QDockWidget::DockWidgetMovable |
                                                                             QDockWidget::DockWidgetFloatable );

    /** Set up clock connection. */
    void OnStartMainLoop();
    /** @name  Global Events
     *  @brief Manage event hanlers and key events.
     *
     * */
    ///@{
    void AddGlobalEventHandler( GlobalEventHandler * h );
    void RemoveGlobalEventHandler( GlobalEventHandler * h );
    bool GlobalKeyEvent( QKeyEvent * e );
    ///@}

    /** @name  Hardware
     *  @brief Manage hardware modules.
     *
     * */
    ///@{
    void InitHardware();

    void AddHardwareSettingsMenuEntries( QMenu * menu );
    void AddToolObjectsToScene();
    void RemoveToolObjectsFromScene();
    ///@}

    /** Check if the application is in a viewer mode - no tracking. */
    bool IsViewerOnly() { return m_viewerOnly; }

    /** @name  Version
     *   @brief Information on application and git version.
     *
     * */
    ///@{
    QString GetFullVersionString();
    QString GetVersionString();
    static QString GetConfigDirectory();
    static QString GetGitHash();
    static QString GetGitHashShort();
    static bool GitCodeHasModifications();
    static bool GitInfoIsValid();
    ///@}

    /** Get pointer to SceneManager. */
    static SceneManager * GetSceneManager();
    /** Get pointer to LookupTableManager. */
    static LookupTableManager * GetLookupTableManager();

    /** @name  Application Settings
     *   @brief Application Settings are loaded at the start of the application and
     *  saved automatically at each run.
     *
     * @sa ApplicationSettings
     *
     * */
    ///@{
    ApplicationSettings * GetSettings();
    void UpdateApplicationSettings();
    void ApplyApplicationSettings();
    ///@}

    /** @name  Update
     *   @brief Set/Get update frequency.
     *
     * */
    ///@{
    void SetUpdateFrequency( double fps );
    double GetUpdateFrequency() { return GetSettings()->UpdateFrequency; }
    ///@}

    /** @name  Plugins
     *   @brief Plugin  and global objects management.
     *
     * */
    ///@{
    void LoadPlugins();
    void SerializePlugins( Serializer * ser );
    IbisPlugin * GetPluginByName( QString name );
    void ActivatePluginByName( const char * name, bool active );
    ObjectPluginInterface * GetObjectPluginByName( QString className );
    ToolPluginInterface * GetToolPluginByName( QString name );
    GeneratorPluginInterface * GetGeneratorPluginByName( QString name );
    SceneObject * GetGlobalObjectInstance( const QString & className );
    void GetAllGlobalObjectInstances( QList<SceneObject *> & allInstances );
    void GetAllPlugins( QList<IbisPlugin *> & allPlugins );
    void GetAllToolPlugins( QList<ToolPluginInterface *> & allTools );
    void GetAllObjectPlugins( QList<ObjectPluginInterface *> & allObjects );
    void GetAllGeneratorPlugins( QList<GeneratorPluginInterface *> & allObjects );
    ///@}

    /** @name  Files
     *   @brief Manage loading and importing files.
     *
     * */
    ///@{
    /** Set initial data files to load when the application starts up, typically specified on the command line. */
    void SetInitialDataFiles( const QStringList & files ) { m_initialDataFiles = files; }
    /** Get names of data files loaded on application start, typically specified on the command line. */
    const QStringList & GetInitialDataFiles() { return m_initialDataFiles; }
    /** Open any file of a supported format .
     *  Following file types are supported:
     * Minc file: *.mnc *.mnc2 *.mnc.gz *.MNC *.MNC2 *.MNC.GZ;
     * Nifti file *.nii;
     * Object file *.obj PLY file *.ply;
     * Tag file *.tag;
     * VTK file: *.vtk *.vtp;
     * FIB file *.fib.
     */
    void OpenFiles( OpenFileParams * params, bool addToScene = true );
    /** Open a transform file, supported format *xfm, and possibly set as a local transform of obj */
    bool OpenTransformFile( const char * filename, SceneObject * obj = 0 );
    /** Open a transform file, supported format *xfm, and store in a 4x4 matrix. */
    bool OpenTransformFile( const char * filename, vtkMatrix4x4 * mat );
    /** Import a sequence of previously acquired US images. */
    void ImportUsAcquisition();
    /** Import previously saved CameraObject. */
    void ImportCamera();
    ///@}

    /** Getting points from a  tag file. */
    bool GetPointsFromTagFile( QString fileName, PointsObject * pts1, PointsObject * pts2 );

    /** @name  US Acquisitions
     *   @brief Manage loading acquired frames.
     *
     * */
    ///@{
    /** Get the number of components per pixel in the image. For grayscale
     * number of components will be 1 otherwise it will be greater than one.
     * Used to check what type of frames should be loaded.
     */
    int GetNumberOfComponents( QString filename );
    /** Load gray scale frames. */
    bool GetGrayFrame( QString filename, IbisItkUnsignedChar3ImageType::Pointer itkImage );
    /** Load RGB frames. */
    bool GetRGBFrame( QString filename, IbisRGBImageType::Pointer itkImage );
    ///@}

    /** @name  Dialogs
     *   @brief Opening files, getting directories, displaying warnings and progress.
     *
     * */
    ///@{
    QString GetFileNameOpen( const QString & caption = QString(), const QString & dir = QString(),
                             const QString & filter = QString() );
    QString GetFileNameSave( const QString & caption = QString(), const QString & dir = QString(),
                             const QString & filter = QString() );
    QString GetExistingDirectory( const QString & caption = QString(), const QString & dir = QString() );
    bool GetOpenFileSequence( QStringList & filenames, QString extension, const QString & caption, const QString & dir,
                              const QString & filter );
    int LaunchModalDialog( QDialog * d );
    void Warning( const QString & title, const QString & text );

    QProgressDialog * StartProgress( int max, const QString & caption = QString() );
    void StopProgress( QProgressDialog * progressDialog );
    ///@}

    /** Show dialog informing user that MINC1 files will be automatically converted to MINC2 if possible. */
    void ShowMinc1Warning( bool cando );

    /** @name  Scenes
     *   @brief Loading and saving.
     *
     * */
    ///@{
    /** Load saved scene. */
    void LoadScene( QString fileName );
    /** Save current scene. */
    void SaveScene( QString fileName );
    ///@}

    /** @name  Preferences
     *   @brief Preferences are used to save paths to tools used in Ibis.
     *
     * @sa IbisPreferences
     * */
    ///@{
    /** Get preferences in use */
    IbisPreferences * GetIbisPreferences() { return m_preferences; }
    /** Show dialog allowing to set the preferences. */
    void Preferences();
    ///@}

public slots:
    void UpdateProgress( QProgressDialog *, int current );
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
    static Application * m_uniqueInstance;

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

    MainWindow * m_mainWindow;
    SceneManager * m_sceneManager;
    IbisAPI * m_ibisAPI;
    UpdateManager * m_updateManager;
    QList<HardwareModule *> m_hardwareModules;
    LookupTableManager * m_lookupTableManager;
    QList<GlobalEventHandler *> m_globalEventHandlers;

    ApplicationSettings m_settings;
    IbisPreferences * m_preferences;

    // Data file to load when the application starts up (typically specified on the command line)
    QStringList m_initialDataFiles;

    bool m_viewerOnly;

    static const QString m_appName;
    static const QString m_appOrganisation;
};

#endif
