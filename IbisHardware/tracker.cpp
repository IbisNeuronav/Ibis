/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "tracker.h"
#include "ibishardwaremodule.h"
#include "trackedvideosource.h"
#include "vtkPOLARISTracker.h"
#include "vtkTrackerTool.h"
#include "trackersettingsdialog.h"
#include "trackerstatusdialog.h"
#include "vtkMatrix4x4.h"
#include "vtkXFMWriter.h"
#include "vtkTransform.h"
#include "vtkVideoSource2.h"
#include "serializerhelper.h"

#include "scenemanager.h"
#include "trackedsceneobject.h"
#include "pointerobject.h"
#include "usprobeobject.h"
#include "cameraobject.h"

#include <QList>
#include <QFileInfo>
#include <QMessageBox>

ObjectSerializationMacro( Tracker );
ObjectSerializationMacro( ToolDescription );

ToolDescription::ToolDescription() : type( Passive ), use( Generic ), active(0), toolPort(-1), name(""), romFileName("")
{
    this->videoSourceIndex = 0;
    this->cachedSerialNumber = "";
    this->sceneObject = 0;
}

ToolDescription::ToolDescription( const ToolDescription & orig )
{
    this->use = orig.use;
    this->type = orig.type;
    this->active = orig.active;
    this->toolPort = orig.toolPort;
    this->name = orig.name;
    this->romFileName = orig.romFileName;
    this->cachedSerialNumber = orig.cachedSerialNumber;
    this->sceneObject = orig.sceneObject;
}

void ToolDescription::Serialize( Serializer * serial )
{
    ::Serialize( serial, "Name", name );
    ::Serialize( serial, "Type", (int&)type );
    ::Serialize( serial, "Use", (int&)use );
    ::Serialize( serial, "Active", active );
    ::Serialize( serial, "RomFileName", romFileName );
    ::Serialize( serial, "CachedSerialNumber", cachedSerialNumber );

    // Serialize sceneObject
    if( serial->IsReader() )
        InstanciateSceneObject();
    serial->BeginSection( "ToolSceneObject" );
    sceneObject->SerializeTracked( serial );
    serial->EndSection();

    if( serial->IsReader() )
    {
        if( this->type == Active )
        {
            this->active = 0;
        }
    }
}

void ToolDescription::InstanciateSceneObject()
{
    Q_ASSERT( !sceneObject );
    if( use == Generic )
        sceneObject = TrackedSceneObject::New();
    else if( use == Pointer )
        sceneObject = PointerObject::New();
    else if( use == UsProbe )
        sceneObject = UsProbeObject::New();
    else
        sceneObject = CameraObject::New();
    sceneObject->SetName( this->name );
    sceneObject->SetObjectManagedBySystem( true );
    sceneObject->SetCanAppendChildren( false );
    sceneObject->SetObjectManagedByTracker(true);
    sceneObject->SetCanChangeParent( false );
    sceneObject->SetCanEditTransformManually( false );
}

void ToolDescription::ClearSceneObject()
{
    if( sceneObject )
    {
        sceneObject->Delete();
        sceneObject = 0;
    }
}

Tracker::Tracker()
{
    m_hardwareModule = 0;
    m_sceneManager = 0;
    m_tracker = vtkPOLARISTracker::New();
    m_currentTool = -1;
    m_referenceTool = -1;
    m_baudRate = DEFAULT_BAUD_RATE;
    m_referenceTransform = vtkTransform::New();
}

Tracker::~Tracker()
{
    m_tracker->Destroy();
}

void Tracker::Initialize()
{
    Q_ASSERT( m_hardwareModule );
    Q_ASSERT( m_sceneManager );

    if( !this->IsInitialized() )
    {        
        m_tracker->SetBaudRate( m_baudRate );
        if( m_tracker->Probe() )
        {
            if (m_tracker->GetVersion())// patch for errors when getting version in vtkNDITracker::InternalStartTracking
                m_trackerVersion = m_tracker->GetVersion(); 

            this->ActivateAllPassiveTools();

            m_tracker->StartTracking();
            if(m_trackerVersion.isEmpty() && m_tracker->GetVersion()) // patch for errors when getting version in vtkPOLARISTracker::InternalStartTracking
                m_trackerVersion = m_tracker->GetVersion(); 
            m_tracker->Update();
        }
    }
}
    
void Tracker::StopTracking()
{
    if( this->IsInitialized() )
    {
        this->DeactivateAllPassiveTools();
        m_tracker->StopTracking();
    }
}

int Tracker::IsInitialized()
{
    return m_tracker->IsTracking();
}

QString Tracker::GetTrackerVersion()
{
    return m_trackerVersion; //m_tracker->GetVersion();
}

void Tracker::LockUpdate()
{
    m_tracker->LockUpdate();
}

void Tracker::UnlockUpdate()
{
    m_tracker->UnlockUpdate();
}

void Tracker::Update()
{
    if( this->IsInitialized() )
    {
        m_tracker->Update();
        this->LookForNewActiveTools();
        this->LookForDeactivatedActiveTools();
        this->PushTrackerStateToSceneObjects();
        emit Updated();
    }
}

double Tracker::GetUpdateRate()
{
    return m_tracker->GetInternalUpdateRate();
}

vtkTrackerTool * Tracker::GetTool( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() );
    if( m_toolVec[ toolIndex ].toolPort != -1 )
        return m_tracker->GetTool( m_toolVec[ toolIndex ].toolPort );
    return 0;
}

vtkTrackerTool * Tracker::GetTool( QString & toolName )
{
    int index = this->GetToolIndex( toolName );
    return this->GetTool( index );    
}

vtkTrackerTool * Tracker::GetTool( TrackedSceneObject * obj )
{
    for( int i = 0; i < m_toolVec.size(); ++i )
    {
        if( m_toolVec[i].sceneObject == obj )
            return GetTool( i );
    }
    return 0;
}

TrackedVideoSource * Tracker::GetVideoSource( TrackedSceneObject * obj )
{
    int index = GetToolIndex( obj );
    Q_ASSERT( index != -1 );
    Q_ASSERT( m_toolVec[index].use == UsProbe || m_toolVec[index].use == Camera );
    return GetVideoSource( m_toolVec[index].videoSourceIndex );
}

QString Tracker::GetToolDescription( int toolIndex )
{
    QString ret;
    vtkTrackerTool * tool = this->GetTool( toolIndex );
    if( tool )
    {
		ret = QString("Type: ") + QString( tool->GetToolType() ) + QString("\n");
		ret += QString("Revision: ") + QString(tool->GetToolRevision()) + QString("\n");
		ret += QString("Manufacturer: ") + QString(tool->GetToolManufacturer()) + QString("\n");
		ret += QString("PartNumber: ") + QString(tool->GetToolPartNumber()) + QString("\n");
		ret += QString("SerialNumber: ") + QString(tool->GetToolSerialNumber());
    }
    else
    {
        ret = QString( "Tool has not been activated. No description yet\n" );
    }
    return ret;        
}

int Tracker::ToolNameExists( QString toolName )
{
    ToolDescriptionVec::iterator it = m_toolVec.begin();
    for( ; it != m_toolVec.end(); ++it )
    {
        if( (*it).name == toolName )
        {
            return 1;
        }
    }
    return 0;
}

int Tracker::GetReferenceToolIndex()
{
    return m_referenceTool;
}

void Tracker::SetReferenceToolIndex( int toolIndex )
{
    if( m_referenceTool == toolIndex )
        return;

    Q_ASSERT( toolIndex >= -1 && toolIndex < GetNumberOfTools() );
    Q_ASSERT( this->IsToolActive( toolIndex ) );  // interface should not let user set an inactive tool as reference

    if( m_toolVec[ toolIndex ].toolPort != -1 )
    {
        m_tracker->SetReferenceTool( m_toolVec[ toolIndex ].toolPort );
        m_referenceTransform->SetInput( GetTool(toolIndex)->GetTransform() );
    }
    m_referenceTool = toolIndex;
}

int Tracker::GetCurrentToolIndex()
{
    return m_currentTool;
}

void Tracker::SetCurrentToolIndex( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() );
    m_currentTool = toolIndex;
}

int Tracker::GetNumberOfTools()
{
    return m_toolVec.size();    
}

int Tracker::GetNumberOfActiveTools()
{
    int nbTools = this->GetNumberOfTools();
    int count = 0;
    for( int i = 0; i < nbTools; i++ )
    {
        if( this->IsToolActive( i ) )
            count++;
    }       
    return count;
}

QString Tracker::GetToolName( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    return m_toolVec[ toolIndex ].name;
}

void Tracker::SetToolName( int toolIndex, QString toolName )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    if( !this->ToolNameExists( toolName ) )
    {
        QString prevName = m_toolVec[ toolIndex ].name;
        m_toolVec[ toolIndex ].name = toolName;
        m_toolVec[ toolIndex ].sceneObject->SetName( toolName );
    }
}

int Tracker::GetToolIndex( QString toolName )
{
    int index = 0;
    std::vector< ToolDescription >::iterator it = m_toolVec.begin();
    while( it != m_toolVec.end() )
    {
        if( (*it).name == toolName )
        {
            return index;
        }
        ++it;
        ++index;
    }
    return -1;
}

int Tracker::GetToolIndex( TrackedSceneObject * obj )
{
    for( int i = 0; i < m_toolVec.size(); ++i )
        if( m_toolVec[i].sceneObject == obj )
        {
            return i;
        }
    return -1;
}

bool Tracker::ToolHasVideo( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    if( m_toolVec[ toolIndex ].use == UsProbe || m_toolVec[ toolIndex ].use == Camera )
        return true;
    return false;
}

ToolType Tracker::GetToolType( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    return m_toolVec[ toolIndex ].type;
}

void Tracker::SetToolUse( int toolIndex, ToolUse use )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    if( m_toolVec[ toolIndex ].use != use )
    {
        m_toolVec[ toolIndex ].use = use;
        if( m_toolVec[ toolIndex ].sceneObject &&  m_sceneManager )
            m_sceneManager->RemoveObject( m_toolVec[ toolIndex ].sceneObject );
        m_toolVec[ toolIndex ].ClearSceneObject();
        m_toolVec[ toolIndex ].InstanciateSceneObject();
        if( m_sceneManager && m_toolVec[ toolIndex ].toolPort != -1 )
            m_sceneManager->AddObject( m_toolVec[ toolIndex ].sceneObject );
    }
}

ToolUse Tracker::GetToolUse( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    return m_toolVec[ toolIndex ].use;
}

QString Tracker::GetRomFileName( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    return m_toolVec[ toolIndex ].romFileName;
}

void Tracker::SetRomFileName( int toolIndex, QString & name )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    Q_ASSERT( m_toolVec[ toolIndex ].toolPort == -1 );  // make sure tool is not active
    m_toolVec[ toolIndex ].romFileName = name;
}

int Tracker::AddNewTool( ToolType type, QString & name )
{
    if( !this->ToolNameExists( name ) )
    {
        ToolDescription desc;
        desc.type = type;
        desc.active = 0;
        desc.name = name;
        desc.romFileName = "";
        desc.InstanciateSceneObject();
        m_toolVec.push_back( desc );
        m_currentTool = m_toolVec.size() - 1;
        return m_currentTool;
    }
    return -1;
}

void Tracker::RemoveTool( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );

    // Deal with current tool
    if( m_currentTool == toolIndex )
    {
        if( GetNumberOfTools() > 1 )
            SetCurrentToolIndex( 0 );
        else
            SetCurrentToolIndex( -1 );
    }
    else if( m_currentTool > toolIndex )
        SetCurrentToolIndex( m_currentTool - 1 );

    // Deal with reference tool
    if( m_referenceTool == toolIndex )
    {
        if( GetNumberOfTools() > 1 )
            SetReferenceToolIndex( 0 );
        else
            SetReferenceToolIndex( -1 );
    }
    else if( m_referenceTool > toolIndex )
        SetReferenceToolIndex( m_referenceTool - 1 );

    // Deactivate tool
    if( m_toolVec[ toolIndex ].active )
        this->ActivateTool( toolIndex, 0 );

    m_toolVec.erase( m_toolVec.begin() + toolIndex );
}

void Tracker::ActivateTool( int toolIndex, int activate )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );

    if( this->IsInitialized() )
    {
        if( activate )
            this->ReallyActivatePassiveTool( toolIndex );
        else
            this->ReallyDeactivatePassiveTool( toolIndex );
    }
    else
    {
        m_toolVec[ toolIndex ].active = activate;
    }
}

// This is returning whether the tool is marked as active, but it might
// not be tracking if tracker is not running.
int Tracker::IsToolActive( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < GetNumberOfTools() );
    return m_toolVec[ toolIndex ].active;
}

TrackerToolState Tracker::GetToolState( int toolIndex )
{
    if( !this->IsInitialized() )
    {
        return Undefined;
    }
    
    TrackerToolState state = Ok;
    vtkTrackerTool * tool = this->GetTool( toolIndex );
    if( tool )
    {
        if( tool->IsMissing() )
            state = Missing;
        else if( tool->IsOutOfView() )
            state = OutOfView;
        else if( tool->IsOutOfVolume() )
            state = OutOfVolume;
    }
    else
        state = Missing;
    return state;
}   

QWidget * Tracker::CreateSettingsDialog( QWidget * parent )
{
    TrackerSettingsDialog * res = NULL;
    if( m_tracker->IsA( "vtkPOLARISTracker" ) )
    {
		 res = new TrackerSettingsDialog( parent, "Tracker Settings" );
		 res->setAttribute( Qt::WA_DeleteOnClose, true );
         res->SetTracker( this );   
    }
    return res;
}

void Tracker::Serialize( Serializer * serial )
{
    bool wasTracking = this->IsInitialized();

    if( serial->IsReader() )
        this->StopTracking();

    ::Serialize( serial, "TrackerBaudeRate", m_baudRate );
    ::Serialize( serial, "CurrentTool", m_currentTool );
    ::Serialize( serial, "ReferenceTool", m_referenceTool );
    ::Serialize( serial, "Tools", m_toolVec );

    if( serial->IsReader() )
    {
        if( wasTracking )
            this->Initialize();
    }
}

void Tracker::AddAllToolsToScene()
{
    for( int i = 0; i < m_toolVec.size(); ++i )
        if( m_toolVec[i].toolPort != -1 )
            AddToolToScene( i );
}

void Tracker::RemoveAllToolsFromScene()
{
    for( int i = 0; i < m_toolVec.size(); ++i )
        if( m_toolVec[i].toolPort == -1 )
            RemoveToolFromScene( i );
}

TrackedVideoSource * Tracker::GetVideoSource( int sourceIndex )
{
    return m_hardwareModule->GetVideoSource( sourceIndex );
}

bool Tracker::ActivateAllPassiveTools()
{
    bool success = true;
    for( unsigned int i = 0; i < m_toolVec.size(); i++ )
    {
        if( m_toolVec[i].type == Passive && m_toolVec[i].active)
            success |= this->ReallyActivatePassiveTool( i );
    }
    return success;
}

void Tracker::DeactivateAllPassiveTools()
{
    for( unsigned int i = 0; i < m_toolVec.size(); i++ )
    {
        if( m_toolVec[i].type == Passive && m_toolVec[i].toolPort != -1 )
            this->ReallyDeactivatePassiveTool( i );
    }
}

bool Tracker::ReallyActivatePassiveTool( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size()  );
    Q_ASSERT( m_toolVec[toolIndex].toolPort == -1 );
    vtkPOLARISTracker * polaris = vtkPOLARISTracker::SafeDownCast( m_tracker );
    Q_ASSERT( polaris );

    // Find a tool port available
    int toolPort = polaris->GetAvailablePassivePort();
    if( toolPort == -1 )
    {
        m_trackerError += QString("Cannot activate tool %1. No more passive tool port available.\n").arg( m_toolVec[toolIndex].name );
        m_toolVec[toolIndex].active = 0;
        return false;
    }

    // Make sure the rom file exists
    if( !QFile::exists( m_toolVec[ toolIndex ].romFileName ) )
    {
        m_trackerError += QString("Cannot activate tool %1. Rom file %2 not found.").arg( m_toolVec[toolIndex].name ).arg( m_toolVec[toolIndex].romFileName );
        m_toolVec[toolIndex].active = 0;
        return false;
    }

    polaris->LoadVirtualSROM( toolPort, m_toolVec[ toolIndex ].romFileName.toUtf8().data() );
    if( m_referenceTool == toolIndex )
    {
        polaris->SetReferenceTool( toolPort );
    }
    m_toolVec[ toolIndex ].toolPort = toolPort;
    m_toolVec[ toolIndex ].active = 1;

    // Assign tool to proper video source for camera and usprobe.
    if( ToolHasVideo( toolIndex ) )
    {
        TrackedVideoSource * v = GetVideoSource( m_toolVec[ toolIndex ].videoSourceIndex );
        v->SetSyncedTrackerTool( GetTool( toolIndex ) );
    }

    AddToolToScene( toolIndex );

    return true;
}

void Tracker::ReallyDeactivatePassiveTool( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() );
    Q_ASSERT( m_toolVec[toolIndex].toolPort != -1 );

    vtkPOLARISTracker * polaris = vtkPOLARISTracker::SafeDownCast( m_tracker );
    Q_ASSERT( polaris );

    polaris->ClearVirtualSROM( m_toolVec[ toolIndex ].toolPort );
    if( m_referenceTool == toolIndex )
    {
        polaris->SetReferenceTool( -1 );
    }
    m_toolVec[ toolIndex ].toolPort = -1;
    m_toolVec[ toolIndex ].active = 0;

    RemoveToolFromScene( toolIndex );
}

void Tracker::AddToolToScene( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() );
    Q_ASSERT( m_toolVec[toolIndex].toolPort != -1 );

    if( m_toolVec[toolIndex].use == Camera )
    {
        CameraObject * cam = CameraObject::SafeDownCast( m_toolVec[toolIndex].sceneObject );
        TrackedVideoSource * videoSource = GetVideoSource( m_toolVec[toolIndex].videoSourceIndex );
        cam->SetInputTransform( videoSource->GetTransform() );
        cam->SetVideoInputData( videoSource->GetVideoOutput() );
    }
    else if( m_toolVec[toolIndex].use == UsProbe )
    {
        UsProbeObject * probe = UsProbeObject::SafeDownCast( m_toolVec[toolIndex].sceneObject );
        TrackedVideoSource * videoSource = GetVideoSource( m_toolVec[toolIndex].videoSourceIndex );
        probe->SetInputTransform( videoSource->GetTransform() );
        probe->SetVideoInputData( videoSource->GetVideoOutput() );
    }
    else
    {
        m_toolVec[toolIndex].sceneObject->SetInputTransform( GetTool(toolIndex)->GetTransform() );
    }

    m_toolVec[toolIndex].sceneObject->SetHardwareModule( m_hardwareModule );
    m_sceneManager->AddObject( m_toolVec[toolIndex].sceneObject );
}

void Tracker::RemoveToolFromScene( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() );
    Q_ASSERT( m_toolVec[toolIndex].toolPort == -1 );

    m_sceneManager->RemoveObject( m_toolVec[ toolIndex ].sceneObject );
}

void Tracker::LookForNewActiveTools()
{
    int numberOfActivePorts = m_tracker->GetNumberOfActiveTools();
    for( int i = 0; i < numberOfActivePorts; i++ )
    {
        vtkTrackerTool * tool = m_tracker->GetTool( i );
        if( !tool->IsMissing() )
        {
            // means this tool is active, lets see if we already know it
            std::vector<ToolDescription>::iterator it = m_toolVec.begin();
            while( it != m_toolVec.end() )
            {
                if( (*it).toolPort == i )
                {
                    break;
                }
                ++it;
            }            
            if( it == m_toolVec.end() )
            {
                // Look for an unactivated tool with same cached serial number
                QString toolSerial = tool->GetToolSerialNumber();
                it = m_toolVec.begin();
                int toolIndex = -1;
                int index = 0;
                while( it != m_toolVec.end() )
                {
                    if( (*it).cachedSerialNumber == toolSerial )
                    {
                        toolIndex = index;
                        break;
                    }
                    ++it;
                    ++index;
                }
                
                // Create a new tool with a generic name
                if( toolIndex == -1 )
                {
                    QString name("Active tool");
                    toolIndex = this->AddNewTool( Active, name );
                }
                
                // Initialize the newly activated tool
                m_toolVec[ toolIndex ].toolPort = i;
                m_toolVec[ toolIndex ].active = 1;
                m_toolVec[ toolIndex ].cachedSerialNumber = toolSerial;
                AddToolToScene( toolIndex );

                emit ToolListChanged();
            }
        }
    }
}

void Tracker::PushTrackerStateToSceneObjects()
{
    for( int i = 0; i < m_toolVec.size(); ++i )
    {
        if( m_toolVec[i].toolPort != -1 )
        {
            if( m_toolVec[i].use == UsProbe || m_toolVec[i].use == Camera )
            {
                TrackedVideoSource * source = GetVideoSource( m_toolVec[i].videoSourceIndex );
                m_toolVec[i].sceneObject->SetState( source->GetState() );
            }
            else
                m_toolVec[i].sceneObject->SetState( GetToolState( i ) );
        }
    }
}

void Tracker::LookForDeactivatedActiveTools()
{
    int numberOfActivePorts = m_tracker->GetNumberOfActiveTools();
    for( int i = 0; i < numberOfActivePorts; i++ )
    {
        vtkTrackerTool * tool = m_tracker->GetTool( i );
        if( tool->IsMissing() )
        {
            // tool is missing, make sure we don't think it is active
            std::vector< ToolDescription >::iterator it = m_toolVec.begin();
            int toolIndex = 0;
            while( it != m_toolVec.end() )
            {
                if( (*it).toolPort == i )
                {
                    (*it).toolPort = -1;
                    (*it).active = 0;
                    RemoveToolFromScene( toolIndex );
                    emit ToolListChanged();
                    break;
                }
                ++it;
                ++toolIndex;
            }
        }
    }
}

void Tracker::SetBaudRate(int rate)
{
    m_tracker->SetBaudRate(rate);
    m_baudRate = rate;
}

int Tracker::GetBaudRate()
{
    return m_tracker->GetBaudRate();
}

void Tracker::RenameTool(QString oldName, QString newName)
{
    int index = this->GetToolIndex(oldName);
    if (index >= 0)
        this->SetToolName( index, newName );
}

