/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __IbisHardwareIGSIO_h_
#define __IbisHardwareIGSIO_h_

#include "hardwaremodule.h"
#include "igtlioDevice.h"
#include "configio.h"

class igtlioLogic;

class QMenu;
class qIGTLIOLogicController;
class qIGTLIOClientWidget;
class PlusServerInterface;
class vtkEventQtSlotConnect;

class IbisHardwareIGSIO : public HardwareModule
{
    
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA(IID "Ibis.IbisHardwareIGSIO" )
    
public:

    vtkTypeMacro( IbisHardwareIGSIO, HardwareModule );

    static IbisHardwareIGSIO * New() { return new IbisHardwareIGSIO; }
    IbisHardwareIGSIO();
    ~IbisHardwareIGSIO();

    // Implementation of IbisPlugin interface
    virtual QString GetPluginName() override { return QString("IbisHardwareIGSIO"); }

    // Load/Save settings
    virtual void LoadSettings( QSettings & s ) override;
    virtual void SaveSettings( QSettings & s ) override;

    // Load Ibis-Plus configs
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
    virtual void RemoveTrackedVideoClient( TrackedSceneObject * obj) override;

    virtual void StartTipCalibration( PointerObject * p ) override;
    virtual double DoTipCalibration( PointerObject * p, vtkMatrix4x4 * calibMat ) override;
    virtual bool IsCalibratingTip( PointerObject * p ) override;
    virtual void StopTipCalibration( PointerObject * p ) override;

private slots:

    void OpenSettingsWidget();
    void OnDeviceNew( vtkObject*, unsigned long, void*, void* );
    void OnDeviceRemoved( vtkObject*, unsigned long, void*, void* );

protected:

    // Launch a Plus server and connect
    bool LauchLocalServer( int port, QString plusConfigFile );
    void Connect( std::string ip, int port );
    void DisconnectAllServers();
    void ShutDownLocalServers();

    // Utility functions
    int FindToolByName( QString name );
    TrackedSceneObject * InstanciateSceneObjectFromType( QString objectName, QString objectType );
    bool IsDeviceImage( igtlioDevicePointer device );
    bool IsDeviceTransform( igtlioDevicePointer device );
    bool IsDeviceVideo( igtlioDevicePointer device );

    // Plus server exec and config paths
    QString m_plusServerExec;
    QString m_plusConfigDir;
    QString m_lastIbisPlusConfigFile;

    struct Tool
    {
        vtkSmartPointer<TrackedSceneObject> sceneObject;
        igtlioDevicePointer transformDevice;
        igtlioDevicePointer imageDevice;
    };
    typedef QList< Tool* > toolList;
    toolList m_tools;

    DeviceToolMap m_deviceToolAssociations;

    vtkSmartPointer<igtlioLogic> m_logic;
    vtkSmartPointer<vtkEventQtSlotConnect> m_logicCallbacks;
    QList< vtkSmartPointer<PlusServerInterface> > m_plusLaunchers;
    qIGTLIOLogicController * m_logicController;
    qIGTLIOClientWidget * m_clientWidget;
};

#endif
