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
#include "igtlioLogic.h"
#include "qIGTLIOLogicController.h"
#include "qIGTLIOClientWidget.h"
#include "igtlioTransformDevice.h"
#include "igtlioImageDevice.h"
#include "vtkImageData.h"

#include "igtlioImageConverter.h"
#include "igtlioTransformConverter.h"

IbisHardwareIGSIO::IbisHardwareIGSIO()
{
    m_logicController = 0;
    m_logic = 0;
    m_clientWidget = 0;
}

IbisHardwareIGSIO::~IbisHardwareIGSIO()
{
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
        if( !tool->ioDevice )
            continue;
        if( tool->ioDevice->GetDeviceType() == igtlio::ImageConverter::GetIGTLTypeName() )
        {
            igtlio::ImageDevice * imageDevice = igtlio::ImageDevice::SafeDownCast( tool->ioDevice );
            tool->sceneObject->SetInputMatrix( imageDevice->GetContent().transform );
            tool->sceneObject->SetState( Undefined ); // todo : set state properly when available
        }
        else if( tool->ioDevice->GetDeviceType() == igtlio::TransformConverter::GetIGTLTypeName() )
        {
            igtlio::TransformDevice * transformDevice = igtlio::TransformDevice::SafeDownCast( tool->ioDevice );
            tool->sceneObject->SetInputMatrix( transformDevice->GetContent().transform );
            tool->sceneObject->SetState( Undefined ); // todo : set state properly when available
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
            int toolIndex = FindToolByName( QString( dev->GetDeviceName().c_str() ) );
            if( toolIndex == -1 )
            {
                TrackedSceneObject * obj = InstanciateSceneObjectFromDevice( dev );
                if( obj )
                {
                    Tool * newTool = new Tool;
                    newTool->ioDevice = dev;
                    newTool->sceneObject = obj;
                    toolIndex = m_tools.size();
                    m_tools.append( newTool );
                }
            }
            if( toolIndex != -1 )
                GetSceneManager()->AddObject( m_tools[toolIndex]->sceneObject );
        }
    }
}

void IbisHardwareIGSIO::FindRemovedTools()
{
    foreach( Tool * tool, m_tools )
    {
        if( tool->sceneObject && tool->sceneObject->IsObjectInScene() && tool->ioDevice && !IoHasDevice( tool->ioDevice ) )
        {
            GetSceneManager()->RemoveObject( tool->sceneObject );
            tool->ioDevice = igtlio::DevicePointer();
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
        if( tool->ioDevice->GetDeviceName() == device->GetDeviceName() )
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
    QString devName( device->GetDeviceName().c_str() );
    TrackedSceneObject * res = 0;

    if( device->GetDeviceType() == igtlio::ImageConverter::GetIGTLTypeName() )
    {
        igtlio::ImageDevice * imageDev = igtlio::ImageDevice::SafeDownCast( device );
        UsProbeObject * probe = UsProbeObject::New();
        probe->SetVideoInputData( imageDev->GetContent().image );
        res = probe;
    }
    else if( device->GetDeviceType() == igtlio::TransformConverter::GetIGTLTypeName() )
    {
        res = PointerObject::New();
    }

    if( res )
    {
        res->SetName( devName );
        res->SetObjectManagedBySystem( true );
        res->SetCanAppendChildren( false );
        res->SetObjectManagedByTracker(true);
        res->SetCanChangeParent( false );
        res->SetCanEditTransformManually( false );
        res->SetHardwareModule( this );
    }

    return res;
}

void IbisHardwareIGSIO::AddToolObjectsToScene()
{
    foreach( Tool * tool, m_tools )
    {
        if( !tool->sceneObject->IsObjectInScene() && tool->ioDevice && IoHasDevice( tool->ioDevice ) )
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
        tool->ioDevice = igtlio::DevicePointer();
    }
}
