/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "ibishardwareIGSIO.h"
#include "plusserverinterface.h"
#include "configio.h"

#include "application.h"
#include "scenemanager.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"

#include <QDir>
#include <QMenu>
#include <QSettings>

#include "qIGTLIOLogicController.h"
#include "qIGTLIOClientWidget.h"

#include "vtkTransform.h"
#include "vtkImageData.h"
#include "vtkEventQtSlotConnect.h"

#include "igtlioLogic.h"
#include "igtlioTransformDevice.h"
#include "igtlioImageDevice.h"
#include "igtlioVideoDevice.h"
#include "igtlioImageConverter.h"
#include "igtlioTransformConverter.h"
#include "igtlioVideoConverter.h"

// Default directory where all config files needed for Plus support in Ibis are located
static const QString DefaultPlusConfigDirectory = Application::GetConfigDirectory() + QString("PlusToolkit/");

// Default directory where IbisPlus configuration files, i.e. files that contain the specification of plus servers, are located.
static const QString DefaultIbisPlusConfigFilesDirectory = DefaultPlusConfigDirectory + QString("IbisPlusConfigFiles/");

// Name and path of the text file that contains the location of the Plus server
static const QString PlusToolkitPathsFilename = DefaultPlusConfigDirectory + QString("plus-toolkit-paths.txt");

// Default directory where Plus Toolkit config files are located
static const QString DefaultPlusConfigFilesDirectory = DefaultPlusConfigDirectory + QString("PlusConfigFiles/");


IbisHardwareIGSIO::IbisHardwareIGSIO()
{
    m_logicController = 0;
    m_logic = 0;
    m_clientWidget = 0;
    m_plusConfigDir = DefaultPlusConfigFilesDirectory;
    m_logicCallbacks = vtkSmartPointer<vtkEventQtSlotConnect>::New();

    // Look for the Plus Toolkit path
    if( QFile::exists( PlusToolkitPathsFilename ) )
    {
        QFile pathFile( PlusToolkitPathsFilename );
        pathFile.open( QIODevice::ReadOnly | QIODevice::Text );
        QTextStream pathIn( &pathFile );
        pathIn >> m_plusServerExec;
    }
}

IbisHardwareIGSIO::~IbisHardwareIGSIO()
{
    ClearConfig();
}

void IbisHardwareIGSIO::LoadSettings( QSettings & s )
{
    m_lastIbisPlusConfigFile = s.value( "LastConfigFile", "" ).toString();
}

void IbisHardwareIGSIO::SaveSettings( QSettings & s )
{
    s.setValue( "LastConfigFile", m_lastIbisPlusConfigFile );
}

void IbisHardwareIGSIO::AddSettingsMenuEntries( QMenu * menu )
{
    menu->addAction( tr("&IGSIO Settings"), this, SLOT( OpenSettingsWidget() ) );
}

void IbisHardwareIGSIO::Init()
{
    m_logicController = new qIGTLIOLogicController;
    m_logic = vtkSmartPointer<igtlioLogic>::New();
    m_logicController->setLogic( m_logic );
    m_logicCallbacks->Connect( m_logic, igtlioLogic::NewDeviceEvent, this, SLOT(OnDeviceNew(vtkObject*, unsigned long, void*, void*)) );
    m_logicCallbacks->Connect( m_logic, igtlioLogic::RemovedDeviceEvent, this, SLOT(OnDeviceRemoved(vtkObject*, unsigned long, void*, void*)) );

    // Temporarily encode config file until we have gui to do it
    m_lastIbisPlusConfigFile = "AutoLocalServerConfig.xml";

    // Initialize with last config file
    QString configFile = DefaultIbisPlusConfigFilesDirectory + m_lastIbisPlusConfigFile;
    StartConfig( configFile );
}

void IbisHardwareIGSIO::StartConfig( QString configFile )
{
    // Read config
    ConfigIO in( configFile );

    // Instanciate Ibis scene objects specified in config file
    for( int i = 0; i < in.GetNumberOfTools(); ++i )
    {
        Tool * newTool = new Tool;
        newTool->sceneObject = InstanciateSceneObjectFromType( in.GetToolName(i), in.GetToolType( i ) );
        m_tools.append( newTool );
        GetSceneManager()->AddObject( newTool->sceneObject );
    }

    // Keep track of Plus device to Ibis tool association
    m_deviceToolAssociations = in.GetAssociations();

    // Try to launch or connect with all servers in the config file or connect to existing servers
    for( int i = 0; i < in.GetNumberOfServers(); ++i )
    {
        // If the server is local and should be launched automatically, then launch it
        if( in.GetServerIPAddress(i) == "localhost" && in.GetStartAuto(i) )
        {
            LauchLocalServer( in.GetServerPort(i), in.GetPlusConfigFile(i) );
        }

        // Now try to connect to server
        Connect( in.GetServerIPAddress(i), in.GetServerPort(i) );
    }
}

void IbisHardwareIGSIO::ClearConfig()
{
    RemoveToolObjectsFromScene();
    DisconnectAllServers();
    foreach( Tool * tool, m_tools )
    {
        delete tool;
    }
    ShutDownLocalServers();
}

TrackerToolState StatusStringToState( const std::string & status )
{
    if( status == "true" )
        return Ok;
    if( status == "false" )
        return OutOfView;
    return Undefined;
}

void IbisHardwareIGSIO::Update()
{
    // Update everything
    m_logic->PeriodicProcess();

    // Push images, transforms and states to the TrackedSceneObjects
    foreach( Tool * tool, m_tools )
    {
        if( tool->transformDevice == 0 ) continue;
        if( tool->transformDevice->GetDeviceType() == igtlioImageConverter::GetIGTLTypeName() )
        {
            igtlioImageDevice * imageDevice = igtlioImageDevice::SafeDownCast( tool->transformDevice );
            tool->sceneObject->SetInputMatrix( imageDevice->GetContent().transform );
            tool->sceneObject->SetState( Ok );
        }
        else if( tool->transformDevice->GetDeviceType() == igtlioTransformConverter::GetIGTLTypeName() )
        {
            igtlioTransformDevice * transformDevice = igtlioTransformDevice::SafeDownCast( tool->transformDevice );
            tool->sceneObject->SetInputMatrix( transformDevice->GetContent().transform );
            //tool->sceneObject->SetState( StatusStringToState( transformDevice->GetContent().transformStatus ) );
            tool->sceneObject->SetState( Ok );
        }
        tool->sceneObject->MarkModified();
    }
}

bool IbisHardwareIGSIO::ShutDown()
{
    ClearConfig();

    delete m_logicController;
    m_logicController = 0;
    m_logic = 0;

    return true;
}

void IbisHardwareIGSIO::AddToolObjectsToScene()
{
    foreach( Tool * tool, m_tools )
    {
        if( !tool->sceneObject->IsObjectInScene() )
        {
            GetSceneManager()->AddObject( tool->sceneObject );
        }
    }
}

void IbisHardwareIGSIO::RemoveToolObjectsFromScene()
{
    foreach( Tool * tool, m_tools )
    {
        GetSceneManager()->RemoveObject( tool->sceneObject );
    }
}

vtkTransform * IbisHardwareIGSIO::GetReferenceTransform()
{
    return 0;
}

bool IbisHardwareIGSIO::IsTransformFrozen( TrackedSceneObject * obj )
{
    return false;
}

void IbisHardwareIGSIO::FreezeTransform( TrackedSceneObject * obj, int nbSamples )
{
}

void IbisHardwareIGSIO::UnFreezeTransform( TrackedSceneObject * obj )
{
}

void IbisHardwareIGSIO::AddTrackedVideoClient( TrackedSceneObject * obj )
{
}

void IbisHardwareIGSIO::RemoveTrackedVideoClient( TrackedSceneObject * obj )
{
}

void IbisHardwareIGSIO::StartTipCalibration( PointerObject * p )
{
}

double IbisHardwareIGSIO::DoTipCalibration( PointerObject * p, vtkMatrix4x4 * calibMat )
{
    return 0.0;
}

bool IbisHardwareIGSIO::IsCalibratingTip( PointerObject * p )
{
    return false;
}

void IbisHardwareIGSIO::StopTipCalibration( PointerObject * p )
{
}

void IbisHardwareIGSIO::OpenSettingsWidget()
{
    m_clientWidget = new qIGTLIOClientWidget;
    m_clientWidget->setLogic( m_logic );

    m_clientWidget->setGeometry(0,0, 859, 811);
    m_clientWidget->show();
}

void IbisHardwareIGSIO::OnDeviceNew( vtkObject*, unsigned long, void*, void * callData )
{
    igtlioDevicePointer device = reinterpret_cast<igtlioDevice*>( callData );
    QString toolName, toolPart;
    m_deviceToolAssociations.ToolAndPartFromDevice( QString(device->GetDeviceName().c_str()), toolName, toolPart );
    if( toolName.isEmpty() )
        return;
    int toolIndex = FindToolByName( toolName );
    if( toolIndex == -1 )
        return;

    // Assign image data of new device to the video input of the scene object
    if( toolPart == "ImageAndTransform" || toolPart == "Image" )
    {
        vtkImageData * imageContent = 0;
        if( IsDeviceImage( device ) )
        {
            igtlioImageDevicePointer imageDev = igtlioImageDevice::SafeDownCast( device );
            imageContent = imageDev->GetContent().image;
        }
        else if( IsDeviceVideo( device ) )
        {
            igtlioVideoDevicePointer videoDev = igtlioVideoDevice::SafeDownCast( device );
            imageContent = videoDev->GetContent().image;
        }

        if( m_tools[ toolIndex ]->sceneObject->IsA( "CameraObject" ) )
        {
            vtkSmartPointer<CameraObject> cam = CameraObject::SafeDownCast( m_tools[ toolIndex ]->sceneObject );
            cam->SetVideoInputData( imageContent );
        }
        else if( m_tools[ toolIndex ]->sceneObject->IsA( "UsProbeObject" ) )
        {
            vtkSmartPointer<UsProbeObject> probe = UsProbeObject::SafeDownCast( m_tools[ toolIndex ]->sceneObject );
            probe->SetVideoInputData( imageContent );
        }
    }

    // Specify the device should be used to recover transform on every update
    if( toolPart == "ImageAndTransform" || toolPart == "Transform" )
    {
        m_tools[ toolIndex ]->transformDevice = device;
    }
}

void IbisHardwareIGSIO::OnDeviceRemoved( vtkObject*, unsigned long, void*, void * callData )
{
    igtlioDevice * device = reinterpret_cast<igtlioDevice*>( callData );
    QString toolName, toolPart;
    m_deviceToolAssociations.ToolAndPartFromDevice( QString(device->GetDeviceName().c_str()), toolName, toolPart );
    if( toolName.isEmpty() )
        return;
    int toolIndex = FindToolByName( toolName );
    if( toolIndex == -1 )
        return;

    if( toolPart == "ImageAndTransform" )
    {
        m_tools[ toolIndex ]->imageDevice = NULL;
    }
    else if( toolPart == "Transform" )
    {
        m_tools[ toolIndex ]->transformDevice = NULL;
    }
}

bool IbisHardwareIGSIO::LauchLocalServer( int port, QString plusConfigFile )
{
    vtkSmartPointer<PlusServerInterface> plusLauncher = vtkSmartPointer<PlusServerInterface>::New();
    plusLauncher->SetServerExecutable( m_plusServerExec );
    m_plusLaunchers.push_back( plusLauncher );

    QString plusConfiFile = m_plusConfigDir + plusConfigFile;
    bool didLaunch = plusLauncher->StartServer( plusConfiFile );
    if( !didLaunch )
    {
        QString msg = QString("Coulnd't launch the Plus Server to communicate with hardware: %1").arg( plusLauncher->GetLastErrorMessage() );
        GetApplication()->Warning("Error", msg);
        return false;
    }
    return true;
}

void IbisHardwareIGSIO::Connect( std::string ip, int port )
{
    igtlioConnectorPointer c = m_logic->CreateConnector();
    c->SetTypeClient( ip, port );
    c->Start();
}

void IbisHardwareIGSIO::DisconnectAllServers()
{
    for( int i = 0; i < m_logic->GetNumberOfConnectors(); ++i )
    {
        m_logic->GetConnector( (unsigned)i )->Stop();
    }
}

void IbisHardwareIGSIO::ShutDownLocalServers()
{
    for( int i = 0; i < m_plusLaunchers.size(); ++i )
    {
        m_plusLaunchers[i]->StopServer();
    }
    m_plusLaunchers.clear();
}

int IbisHardwareIGSIO::FindToolByName( QString name )
{
    for( int i = 0; i < m_tools.size(); ++i )
        if( m_tools[i]->sceneObject->GetName() == name )
            return i;
    return -1;
}

TrackedSceneObject * IbisHardwareIGSIO::InstanciateSceneObjectFromType( QString objectName, QString objectType )
{
    TrackedSceneObject * res = 0;
    if( objectType == "Camera" )
        res = CameraObject::New();
    else if( objectType == "UsProbe" )
        res = UsProbeObject::New();
    else if( objectType == "Pointer" )
        res = PointerObject::New();
    else
        res = TrackedSceneObject::New();

    res->SetName( objectName );
    res->SetObjectManagedBySystem( true );
    res->SetCanAppendChildren( false );
    res->SetObjectManagedByTracker(true);
    res->SetCanChangeParent( false );
    res->SetCanEditTransformManually( false );
    res->SetHardwareModule( this );

    return res;
}

bool IbisHardwareIGSIO::IsDeviceImage( igtlioDevicePointer device )
{
    return device->GetDeviceType() == igtlioImageConverter::GetIGTLTypeName();
}

bool IbisHardwareIGSIO::IsDeviceTransform( igtlioDevicePointer device )
{
    return device->GetDeviceType() == igtlioTransformConverter::GetIGTLTypeName();
}

bool IbisHardwareIGSIO::IsDeviceVideo( igtlioDevicePointer device )
{
    return device->GetDeviceType() == igtlioVideoConverter::GetIGTLTypeName();
}
