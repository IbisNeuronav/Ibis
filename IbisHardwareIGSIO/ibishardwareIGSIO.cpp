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
#include "application.h"
#include "scenemanager.h"
#include "vtkTransform.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"
#include <QDir>
#include <QMenu>
#include <QSettings>
#include "igtlioLogic.h"
#include "qIGTLIOLogicController.h"
#include "qIGTLIOClientWidget.h"
#include "igtlioTransformDevice.h"
#include "igtlioImageDevice.h"
#include "vtkImageData.h"

#include "igtlioImageConverter.h"
#include "igtlioTransformConverter.h"
#include "igtlioVideoConverter.h"
#include "igtlioVideoDevice.h"
#include "plusserverinterface.h"

static const QString DefaultPlusConfigDirectory = Application::GetConfigDirectory() + QString("PlusToolkit/");
static const QString DefaultPlusConfigFilesDirectory = DefaultPlusConfigDirectory + QString("ConfigFiles/");
static const QString PlusToolkitPathsFilename = DefaultPlusConfigDirectory + QString("plus-toolkit-paths.txt");

IbisHardwareIGSIO::IbisHardwareIGSIO()
{
    m_logicController = 0;
    m_logic = 0;
    m_clientWidget = 0;
    m_plusLauncher = 0;
    m_plusConfigDir = DefaultPlusConfigFilesDirectory;

    // Look for the Plus Toolkit path
    /*if( QFile::exists( PlusToolkitPathsFilename ) )
    {
        QFile pathFile( PlusToolkitPathsFilename );
        pathFile.open( QIODevice::ReadOnly | QIODevice::Text );
        QTextStream pathIn( &pathFile );
        pathIn >> m_plusServerExec;
    }*/
}

IbisHardwareIGSIO::~IbisHardwareIGSIO()
{
}

void IbisHardwareIGSIO::LoadSettings( QSettings & s )
{
    m_plusLastConfigFile = s.value( "LastConfigFile", "" ).toString();
}

void IbisHardwareIGSIO::SaveSettings( QSettings & s )
{
    s.setValue( "LastConfigFile", m_plusLastConfigFile );
}

void IbisHardwareIGSIO::AddSettingsMenuEntries( QMenu * menu )
{
    menu->addAction( tr("&IGSIO Settings"), this, SLOT( OpenSettingsWidget() ) );
}

void IbisHardwareIGSIO::Init()
{
    m_logicController = new qIGTLIOLogicController;
    m_logic = vtkSmartPointer<igtlio::Logic>::New();
    m_logicController->setLogic( m_logic );

    // Temporarily encode config file until we have gui to do it
    m_plusLastConfigFile = "PlusDeviceSet_OpenIGTLinkCommandsTest.xml";

    // Make sure we have a valid config file before trying to launch a server
    /*QFileInfo configInfo( m_plusConfigDir + m_plusLastConfigFile );
    if( configInfo.exists() && configInfo.isReadable() )
    {
        LaunchAndConnectLocal();
    }*/
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
    FindRemovedTools();
    FindNewTools();

    // Push images, transforms and states to the TrackedSceneObjects
    foreach( Tool * tool, m_tools )
    {
        if( tool->transformDevice == 0 ) continue;
        if( tool->transformDevice->GetDeviceType() == igtlio::ImageConverter::GetIGTLTypeName() )
        {
            igtlio::ImageDevice * imageDevice = igtlio::ImageDevice::SafeDownCast( tool->transformDevice );
            tool->sceneObject->SetInputMatrix( imageDevice->GetContent().transform );
            tool->sceneObject->SetState( Ok );
        }
        else if( tool->transformDevice->GetDeviceType() == igtlio::TransformConverter::GetIGTLTypeName() )
        {
            igtlio::TransformDevice * transformDevice = igtlio::TransformDevice::SafeDownCast( tool->transformDevice );
            tool->sceneObject->SetInputMatrix( transformDevice->GetContent().transform );
            //tool->sceneObject->SetState( StatusStringToState( transformDevice->GetContent().transformStatus ) );
            tool->sceneObject->SetState( Ok );
        }
        tool->sceneObject->MarkModified();
    }
}

bool IbisHardwareIGSIO::ShutDown()
{
    RemoveToolObjectsFromScene();

    delete m_logicController;
    m_logicController = 0;
    m_logic = 0;

    foreach( Tool * tool, m_tools )
    {
        tool->sceneObject->Delete();
        delete tool;
    }

    return true;
}

#include "vtkTimerLog.h"
#include "vtksys/SystemTools.hxx"

bool IbisHardwareIGSIO::LaunchAndConnectLocal()
{   
    // First try to launch a server
    if( !m_plusLauncher )
    {
        m_plusLauncher = vtkSmartPointer<PlusServerInterface>::New();
        m_plusLauncher->SetServerExecutable( m_plusServerExec );
    }
    QString plusConfiFile = m_plusConfigDir + m_plusLastConfigFile;
    bool didLaunch = m_plusLauncher->StartServer( plusConfiFile );
    if( !didLaunch )
    {
        QString msg = QString("Coulnd't launch the Plus Server to communicate with hardware: %1").arg( m_plusLauncher->GetLastErrorMessage() );
        GetApplication()->Warning("Error", msg);
        return false;
    }

    // Now, try to connect to it. By default the connector tries local host with port where server is listening
    igtlio::ConnectorPointer c = m_logic->CreateConnector();
    c->Start();

    // Wait for connection (max 3 sec)
    double starttime = vtkTimerLog::GetUniversalTime();
    while (vtkTimerLog::GetUniversalTime() - starttime < 3.0)
    {
        c->PeriodicProcess();
        vtksys::SystemTools::Delay(5);

        if (c->GetState() != igtlio::Connector::STATE_WAIT_CONNECTION)
        {
            break;
        }
    }
    bool connected = c->GetState() == igtlio::Connector::STATE_CONNECTED;

    // If connected, send empty status message to tell plus we are using header v2
    if( connected )
        m_logic->GetConnector(0)->SendEmptyStatusMessage( IGTL_HEADER_VERSION_2 );

    return connected;
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

void IbisHardwareIGSIO::FindNewTools()
{
    for( int i = 0; i < m_logic->GetNumberOfDevices(); ++i )
    {
        igtlio::DevicePointer dev = m_logic->GetDevice( i );
        if( !ModuleHasDevice( dev ) )
        {
            QString toolRealName = DeviceNameToToolName( dev->GetDeviceName().c_str() );
            int toolIndex = FindToolByName( toolRealName );
            if( toolIndex == -1 )  // There is no tool by this name already
            {
                TrackedSceneObject * obj = InstanciateSceneObjectFromDevice( dev );
                if( obj )
                {
                    Tool * newTool = new Tool;
                    newTool->sceneObject = obj;
                    toolIndex = m_tools.size();
                    m_tools.append( newTool );
                    GetSceneManager()->AddObject( m_tools[toolIndex]->sceneObject );
                }
            }
            if( toolIndex != -1 )
            {
                if( IsDeviceImage(dev) )
                {
                    m_tools[toolIndex]->imageDevice = dev;
                }
                else if( IsDeviceTransform(dev) )
                {
                    m_tools[toolIndex]->transformDevice = dev;
                }
                else if( IsDeviceVideo(dev) )
                {
                    m_tools[toolIndex]->imageDevice = dev;
                    CameraObject * cam = CameraObject::SafeDownCast( m_tools[toolIndex]->sceneObject );
                    if( cam )
                    {
                        igtlio::VideoDevice * videoDev = igtlio::VideoDevice::SafeDownCast( dev );
                        cam->SetVideoInputData( videoDev->GetContent().image );
                    }
                }
            }
        }
    }
}

void IbisHardwareIGSIO::FindRemovedTools()
{
    foreach( Tool * tool, m_tools )
    {
        if( tool->sceneObject && tool->sceneObject->IsObjectInScene() && tool->transformDevice && !IoHasDevice( tool->transformDevice ) )
        {
            GetSceneManager()->RemoveObject( tool->sceneObject );
            tool->transformDevice = igtlio::DevicePointer();
        }
    }
}

int IbisHardwareIGSIO::FindToolByName( QString name )
{
    for( int i = 0; i < m_tools.size(); ++i )
        if( m_tools[i]->sceneObject->GetName() == name )
            return i;
    return -1;
}

bool IbisHardwareIGSIO::ModuleHasDevice( igtlio::DevicePointer device )
{
    foreach( Tool * tool, m_tools )
    {
        if( ( tool->transformDevice && tool->transformDevice->GetDeviceName() == device->GetDeviceName()) ||
                ( tool->imageDevice && tool->imageDevice->GetDeviceName() == device->GetDeviceName() ) )
            return true;
    }
    return false;
}

bool IbisHardwareIGSIO::IoHasDevice( igtlio::DevicePointer device )
{
    for( int i = 0; i < m_logic->GetNumberOfDevices(); ++i )
        if( m_logic->GetDevice( i ) == device )
            return true;
    return false;
}

// TODO : for now, device of type Image are assumed to be US probes and devices of type Transform
// are assumed to be pointers. Find a more suitable scheme or associate with some sort of
// local config file.
TrackedSceneObject * IbisHardwareIGSIO::InstanciateSceneObjectFromDevice( igtlio::DevicePointer device )
{
    QString realName = DeviceNameToToolName( device->GetDeviceName().c_str() );
    TrackedSceneObject * res = 0;

    if( IsDeviceImage(device) )
    {
        igtlio::ImageDevice * imageDev = igtlio::ImageDevice::SafeDownCast( device );
        if( realName.startsWith("USProbe") )
        {
            UsProbeObject * probe = UsProbeObject::New();
            probe->SetVideoInputData( imageDev->GetContent().image );
            res = probe;
        }
        else if( realName.startsWith("Camera") )
        {
            CameraObject * camera = CameraObject::New();
            camera->SetVideoInputData( imageDev->GetContent().image );
            res = camera;
        }
    }
    else if( IsDeviceTransform(device) )
    {
        if( realName.startsWith("USProbe") )
        {
            res = UsProbeObject::New();
        }
        else if( realName.startsWith("Pointer") || realName.startsWith("Stylus") )
        {
            res = PointerObject::New();
        }
        else if( realName.startsWith("Camera") )
        {
            res = CameraObject::New();
        }
    }
    else if( IsDeviceVideo(device) )
    {
        igtlio::VideoDevice * videoDev = igtlio::VideoDevice::SafeDownCast( device );
        CameraObject * camera = CameraObject::New();
        camera->SetVideoInputData( videoDev->GetContent().image );
        res = camera;
    }

    if( res )
    {
        res->SetName( realName );
        res->SetObjectManagedBySystem( true );
        res->SetCanAppendChildren( false );
        res->SetObjectManagedByTracker(true);
        res->SetCanChangeParent( false );
        res->SetCanEditTransformManually( false );
        res->SetHardwareModule( this );
    }

    return res;
}

QString IbisHardwareIGSIO::DeviceNameToToolName( const QString & deviceName )
{
    QStringList nameParts = deviceName.split( "To" );
    return nameParts[0];
}

bool IbisHardwareIGSIO::IsDeviceImage( igtlio::DevicePointer device )
{
    return device->GetDeviceType() == igtlio::ImageConverter::GetIGTLTypeName();
}

bool IbisHardwareIGSIO::IsDeviceTransform( igtlio::DevicePointer device )
{
    return device->GetDeviceType() == igtlio::TransformConverter::GetIGTLTypeName();
}

bool IbisHardwareIGSIO::IsDeviceVideo( igtlio::DevicePointer device )
{
    return device->GetDeviceType() == igtlio::VideoConverter::GetIGTLTypeName();
}

void IbisHardwareIGSIO::AddToolObjectsToScene()
{
    foreach( Tool * tool, m_tools )
    {
        if( !tool->sceneObject->IsObjectInScene() && tool->transformDevice && IoHasDevice( tool->transformDevice ) )
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
        tool->transformDevice = igtlio::DevicePointer();
    }
}
