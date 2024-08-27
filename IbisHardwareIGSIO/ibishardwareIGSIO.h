/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef IBISHARDWAREIGSIO_H
#define IBISHARDWAREIGSIO_H

#include <igtlioDevice.h>

#include "configio.h"
#include "hardwaremodule.h"

class igtlioLogic;
class igtlioConnector;

class QMenu;
class qIGTLIOLogicController;
class qIGTLIOClientWidget;
class PlusServerInterface;
class vtkEventQtSlotConnect;
class IbisHardwareIGSIOSettingsWidget;
class PolyDataObject;
class Logger;

class IbisHardwareIGSIO : public HardwareModule
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.IbisHardwareIGSIO" )

public:
    vtkTypeMacro( IbisHardwareIGSIO, HardwareModule );

    static IbisHardwareIGSIO * New() { return new IbisHardwareIGSIO; }
    IbisHardwareIGSIO();
    ~IbisHardwareIGSIO();

    static const QString PlusServerExecutable;

    // Implementation of IbisPlugin interface
    virtual QString GetPluginName() override { return QString( "IbisHardwareIGSIO" ); }

    // Load/Save settings
    virtual void LoadSettings( QSettings & s ) override;
    virtual void SaveSettings( QSettings & s ) override;

    // Load Ibis-Plus configs
    bool GetAutoStartLastConfig() { return m_autoStartLastConfig; }
    void SetAutoStartLastConfig( bool start ) { m_autoStartLastConfig = start; }
    QString GetIbisPlusConfigDirectory() { return m_ibisPlusConfigFilesDirectory; }
    QString GetLastIbisPlusConfigFilename() { return m_lastIbisPlusConfigFile; }
    void StartConfig( QString configFile );
    void ClearConfig();

    // Implementation of the HardwareModule interface
    virtual void AddSettingsMenuEntries( QMenu * menu ) override;
    virtual void Init() override;
    virtual void Update() override;
    virtual bool ShutDown() override;

    virtual void AddToolObjectsToScene() override;
    virtual void RemoveToolObjectsFromScene() override;

    virtual vtkTransform * GetReferenceTransform() override;

    virtual bool IsTransformFrozen( TrackedSceneObject * obj ) override;
    virtual void FreezeTransform( TrackedSceneObject * obj, int nbSamples ) override;
    virtual void UnFreezeTransform( TrackedSceneObject * obj ) override;

    virtual void AddTrackedVideoClient( TrackedSceneObject * obj ) override;
    virtual void RemoveTrackedVideoClient( TrackedSceneObject * obj ) override;

    virtual void SceneAboutToLoad() override;
    virtual void SceneFinishedLoading() override;

    const Logger * GetLogger() { return m_log; }

private slots:

    void OpenSettingsWidget();
    void OnSettingsWidgetClosed();
    void OpenConfigFileWidget();
    void OnConfigFileWidgetClosed();
    void OnDeviceNew( vtkObject *, unsigned long, void *, void * );
    void OnDeviceRemoved( vtkObject *, unsigned long, void *, void * );
    void OnConnectionEstablished( vtkObject *, unsigned long, void *, void * );

protected:
    virtual void InitPlugin() override;

    // Launch a Plus server and connect
    bool LaunchLocalServer( QString plusConfigFile );
    void Connect( std::string ip, int port, bool start );
    void DisconnectAllServers();
    void ShutDownLocalServers();

    struct Tool
    {
        Tool()
        {
            lastTimeStamp             = 0.0;
            lastTimeStampModifiedTime = 0.0;
        }
        vtkSmartPointer<TrackedSceneObject> sceneObject;
        vtkSmartPointer<PolyDataObject> toolModel;
        igtlioDevicePointer transformDevice;
        igtlioDevicePointer imageDevice;

        // timestamps used to compute tool status when not in Metadata
        double lastTimeStamp;              // The timestamp of the last message we received
        double lastTimeStampModifiedTime;  // The last time the timestamp has changed
    };
    typedef QList<Tool *> toolList;
    toolList m_tools;

    // Utility functions
    TrackerToolState ComputeToolStatus( igtlioDevicePointer dev, Tool * t );
    int FindToolByName( QString name );
    void AssignDeviceImageToTool( igtlioDevicePointer device, Tool * tool );
    TrackedSceneObject * InstanciateSceneObjectFromType( QString objectName, QString objectType );
    vtkSmartPointer<PolyDataObject> InstanciateToolModel( QString filename );
    void ReadToolConfig( QString filename, vtkSmartPointer<TrackedSceneObject> tool );
    bool IsDeviceImage( igtlioDevicePointer device );
    bool IsDeviceTransform( igtlioDevicePointer device );
    bool IsDeviceVideo( igtlioDevicePointer device );

    void WriteToolConfig();
    void InternalWriteToolConfig( QString, Tool * );

    DeviceToolMap m_deviceToolAssociations;

    vtkSmartPointer<igtlioLogic> m_logic;
    vtkSmartPointer<vtkEventQtSlotConnect> m_logicCallbacks;
    QList<vtkSmartPointer<PlusServerInterface> > m_plusLaunchers;
    qIGTLIOLogicController * m_logicController;

    bool m_autoStartLastConfig;

    // log server output
    Logger * m_log;

    // When determining tool status from timestamp, time after which we
    // consider the tool missing
    static const double MaxTimeBetweenTransformSamples;

    // Settings widgets
    qIGTLIOClientWidget * m_clientWidget;
    IbisHardwareIGSIOSettingsWidget * m_settingsWidget;

    // Useful paths

    // Last ibisplus config file loaded
    QString m_lastIbisPlusConfigFile;
    // Default directory where all config files needed for Plus support in Ibis are located
    QString m_plusConfigDirectory;
    // Default directory where IbisPlus configuration files, i.e. files that contain the specification of plus servers,
    // are located.
    QString m_ibisPlusConfigFilesDirectory;
    // Name and path of the text file that contains the location of the Plus server
    QString m_plusToolkitPathsFilename;
    // Default directory where Plus Toolkit config files are located
    QString m_plusConfigFilesDirectory;
    // Current ibisplus config file loaded
    QString m_currentIbisPlusConfigFile;
};

#endif
