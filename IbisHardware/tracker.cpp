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
#include "vtkPOLARISTracker.h"
#include "vtkTrackerTool.h"
#include "trackersettingsdialog.h"
#include "trackerstatusdialog.h"
#include "vtkMatrix4x4.h"
#include "vtkmatrix4x4utilities.h"
#include "vtkXFMWriter.h"

#include <QList>
#include <QFileInfo>
#include <QMessageBox>

ObjectSerializationMacro( Tracker );
ObjectSerializationMacro( ToolDescription );


ToolDescription::ToolDescription() : type( Passive ), use( NoUse ), active(0), toolPort(-1), name(""), romFileName("")
{
    this->cachedCalibrationMatrix = vtkMatrix4x4::New();
    this->cachedCalibrationMatrix->Identity();
    this->cachedCalibrationRMS = 0.0;
    this->cachedSerialNumber = "";
    this->toolObjectId = -1;
}

ToolDescription::ToolDescription( const ToolDescription & orig )
{
    this->use = orig.use;
    this->type = orig.type;
    this->active = orig.active;
    this->toolPort = orig.toolPort;
    this->name = orig.name;
    this->romFileName = orig.romFileName;
    this->cachedCalibrationMatrix = vtkMatrix4x4::New();
    this->cachedCalibrationMatrix->DeepCopy( orig.cachedCalibrationMatrix );
    this->cachedCalibrationRMS = orig.cachedCalibrationRMS;
    this->cachedSerialNumber = orig.cachedSerialNumber;
    this->toolObjectId = orig.toolObjectId;
}

void ToolDescription::Serialize( Serializer * serial )
{
    ::Serialize( serial, "Name", name );
    ::Serialize( serial, "Type", (int&)type );
    ::Serialize( serial, "Use", (int&)use );
    ::Serialize( serial, "Active", active );
    ::Serialize( serial, "RomFileName", romFileName );
    ::Serialize( serial, "CachedCalibrationMatrix", cachedCalibrationMatrix );
    ::Serialize( serial, "CachedCalibrationRMS", cachedCalibrationRMS );
    ::Serialize( serial, "CachedSerialNumber", cachedSerialNumber );
    if( serial->IsReader() )
    {
        if( this->type == Active )
        {
            this->active = 0;
        }
    }
}

Tracker::Tracker()
{
    m_tracker = vtkPOLARISTracker::New();
    m_currentTool = -1;
    m_referenceTool = -1;
    m_navigationPointer = -1;
    m_cachedWorldCalibrationMatrix = vtkMatrix4x4::New();
    m_cachedWorldCalibrationMatrix->Identity();
    m_baudRate = DEFAULT_BAUD_RATE;
}

Tracker::~Tracker()
{
    m_cachedWorldCalibrationMatrix->Delete();
    m_tracker->Destroy();
}

void Tracker::Initialize()
{
    if( !this->IsInitialized() )
    {        
        m_tracker->SetBaudRate( m_baudRate );
        if( m_tracker->Probe() )
        {
            if (m_tracker->GetVersion())// patch for errors when getting version in vtkPolarisTracker::InternalStartTracking
                m_trackerVersion = m_tracker->GetVersion(); 

            this->ActivateAllPassiveTools();

            m_tracker->StartTracking();
            if(m_trackerVersion.isEmpty() && m_tracker->GetVersion()) // patch for errors when getting version in vtkPOLARISTracker::InternalStartTracking
                m_trackerVersion = m_tracker->GetVersion(); 
            m_navigationPointer = this->FindFirstActivePointer();
            m_tracker->Update();
            emit TrackerInitialized();
        }
    }
}
    
void Tracker::StopTracking()
{
    if( this->IsInitialized() )
    {
        this->DeactivateAllPassiveTools();
        m_tracker->StopTracking();
        emit TrackerStopped();
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
        emit Updated();
    }
}


double Tracker::GetUpdateRate()
{
    return m_tracker->GetInternalUpdateRate();
}


vtkTrackerTool * Tracker::GetTool( int toolIndex )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        if( m_toolVec[ toolIndex ].toolPort != -1 )
        {
            return m_tracker->GetTool( m_toolVec[ toolIndex ].toolPort );
        }
    }
    return 0;
}


vtkTrackerTool * Tracker::GetTool( QString & toolName )
{
    int index = this->GetToolIndex( toolName );
    return this->GetTool( index );    
}

int Tracker::GetUSProbeToolIndex( )
{ // There may be only one in the current version - 2007-06-04
    ToolDescriptionVec::iterator it = m_toolVec.begin();
    int index = 0;
    for( ; it != m_toolVec.end(); ++it )
    {
        if( (*it).use == UsProbe )
        {
            return index;
        }
        index++;
    }
    return -1;
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


int Tracker::SetReferenceToolIndex( int toolIndex )
{
    if( m_referenceTool != toolIndex )
    {
        int numberOfTools = m_toolVec.size();
        if( toolIndex >= -1 && toolIndex < numberOfTools )
        {
            if (this->GetToolUse(toolIndex) != Reference)
            {
                QMessageBox::warning( 0, "Warning!", "Selected tool is not a reference tool: " + this->GetToolName(toolIndex), 1, 0 );
                return 0;
            }
            else if (!this->IsToolActive(m_referenceTool))
            {
                QMessageBox::warning( 0, "Warning!", "Selected reference tool is not active: " + this->GetToolName(m_referenceTool), 1, 0 );
            }
            if( m_toolVec[ toolIndex ].active )
            {
                m_tracker->SetReferenceTool( m_toolVec[ toolIndex ].toolPort );
            }
            m_referenceTool = toolIndex;
        }
    }
    if (this->GetToolUse(m_referenceTool) == Reference && this->IsToolActive(m_referenceTool))
        emit ReferenceToolChanged(true); 
    else
        emit ReferenceToolChanged(false); 
    return 1;
}

int Tracker::GetNavigationPointerIndex()
{
    return m_navigationPointer;
}

int Tracker::SetNavigationPointerIndex( int toolIndex )
{
    if( m_navigationPointer != toolIndex )
    {
        int numberOfTools = m_toolVec.size();
        if( toolIndex >= -1 && toolIndex < numberOfTools )
        {
            if (this->GetToolUse(toolIndex) != Pointer)
            {
                QMessageBox::warning( 0, "Warning!", "Selected tool is not a pointer: " + this->GetToolName(toolIndex), 1, 0 );
                return 0;
            }
            else if (!this->IsToolActive(toolIndex))
            {
                QMessageBox::warning( 0, "Warning!", "Selected pointer is not active: " + this->GetToolName(toolIndex), 1, 0 );
                return 0;
            }
        }
    }
    m_navigationPointer = toolIndex;
    emit NavigationPointerChangedSignal();
    return 1;
}

int Tracker::GetCurrentToolIndex()
{
    return m_currentTool;
}


void Tracker::SetCurrentToolIndex( int toolIndex )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
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
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        return m_toolVec[ toolIndex ].name;
    }
    else
    {
        return QString("");
    }           
}


void Tracker::SetToolName( int toolIndex, QString toolName )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        if( !this->ToolNameExists( toolName ) )
        {
            QString prevName = m_toolVec[ toolIndex ].name;
            m_toolVec[ toolIndex ].name = toolName;
            emit ToolNameChanged( toolIndex, prevName );
        }
    }
}

int Tracker::GetToolObjectId(int toolIndex)
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
        return m_toolVec[ toolIndex ].toolObjectId;
    else
        return -1;
}

void Tracker::SetToolObjectId(int toolIndex, int id)
{
    m_toolVec[ toolIndex ].toolObjectId = id;
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

void Tracker::GetActiveToolTypeAndRom(ToolUse use, QString &type, QString &romFile)
{
    int index = 0;
    std::vector< ToolDescription >::iterator it = m_toolVec.begin();
    while( it != m_toolVec.end() )
    {
        if( (*it).active && (*it).use == use)
        {
            type = this->GetToolDescription(index);
            QFileInfo info( (*it).romFileName );
            romFile = info.fileName();
        }
        ++it;
        ++index;
    }
}

void Tracker::StartToolTipCalibration( int toolIndex )
{
    vtkTrackerTool * tool = this->GetTool( toolIndex );
    if( tool )
    {
        tool->StartTipCalibration();
    }
}


double Tracker::GetToolLastCalibrationRMS( int toolIndex )
{
    vtkTrackerTool * tool = this->GetTool( toolIndex );
    if( tool )
    {
        return tool->GetLastCalibrationRMS();
    }
    return 0;
}


void Tracker::SetToolLastCalibrationRMS( int toolIndex, double rms )
{
    vtkTrackerTool * tool = this->GetTool( toolIndex );
    if( tool )
    {
        tool->SetLastCalibrationRMS( rms );
    }
}


void Tracker::StopToolTipCalibration( int toolIndex )
{
    vtkTrackerTool * tool = this->GetTool( toolIndex );
    if( tool )
    {
        tool->StopTipCalibration();
    }   
}


int Tracker::IsToolCalibrating( int toolIndex )
{
    vtkTrackerTool * tool = this->GetTool( toolIndex );
    if( tool )
    {
        return tool->GetCalibrating();
    }   
    return 0;
}


ToolType Tracker::GetToolType( int toolIndex )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        return m_toolVec[ toolIndex ].type;
    }
    return None;
}

void Tracker::SetToolUse( int toolIndex, ToolUse use )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        m_toolVec[ toolIndex ].use = use;
        emit ToolUseChanged( toolIndex );
    }
}


ToolUse Tracker::GetToolUse( int toolIndex )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        return m_toolVec[ toolIndex ].use;
    }
    return NoUse;
}


QString Tracker::GetRomFileName( int toolIndex )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        return m_toolVec[ toolIndex ].romFileName;
    }
    return "";
}


void Tracker::SetRomFileName( int toolIndex, QString & name )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        m_toolVec[ toolIndex ].romFileName = name;
    }
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
        m_toolVec.push_back( desc );
        return m_toolVec.size() - 1;
    }
    return -1;
}
    

void Tracker::RemoveTool( int toolIndex )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        if( m_toolVec.size() == 1 )
        {
            m_currentTool = -1;
        }
        else
        {
            m_currentTool--;
        }
        if( m_toolVec[ toolIndex ].active )
        {
            this->ActivateTool( toolIndex, 0 );
        }
        else
        {
            emit ToolDeactivated( toolIndex );           
        }

        m_toolVec.erase( m_toolVec.begin() + toolIndex );
        
        if( m_referenceTool == toolIndex )
        {
            m_referenceTool = -1;
        }
        else if( m_referenceTool > toolIndex )
        {
            m_referenceTool--;
        }
        emit ToolRemoved ( toolIndex );
    }
}


void Tracker::ActivateTool( int toolIndex, int activate )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
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
}


int Tracker::IsToolActive( int toolIndex )
{
    if( this->IsInitialized() && toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        return m_toolVec[ toolIndex ].active;
    }
    return 0;
}


TrackerToolState Tracker::GetCurrentToolState( int toolIndex )
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


vtkMatrix4x4 * Tracker::GetToolCalibrationMatrix( int toolIndex )
{
    if( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() )
    {
        vtkTrackerTool * tool = this->GetTool( toolIndex );
        if( tool )
        {
            return tool->GetCalibrationMatrix();
        }
        else
        {
            return m_toolVec[ toolIndex ].cachedCalibrationMatrix;
        }               
    }
    return 0;
}


vtkMatrix4x4 * Tracker::GetWorldCalibrationMatrix()
{
    if( this->IsInitialized() )
    {
        return m_tracker->GetWorldCalibrationMatrix();
    }
    return m_cachedWorldCalibrationMatrix;
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


QWidget * Tracker::CreateStatusDialog( QWidget * parent )
{
    TrackerStatusDialog * res = new TrackerStatusDialog( parent );
	res->setAttribute( Qt::WA_DeleteOnClose, true );
    res->SetTracker( this );
    connect( this, SIGNAL( NavigationPointerChangedSignal() ), res, SLOT( UpdateToolList() ) );
    return res;
}


void Tracker::Serialize( Serializer * serial )
{
    bool wasTracking = this->IsInitialized();
    if( !serial->IsReader() )
    {
        this->BackupCalibrationMatrices();
    }
    else
    {
        this->StopTracking();
    }
    ::Serialize( serial, "TrackerBaudeRate", m_baudRate );
    ::Serialize( serial, "CurrentTool", m_currentTool );
    ::Serialize( serial, "ReferenceTool", m_referenceTool );
    ::Serialize( serial, "CachedWorldCalibrationMatrix", m_cachedWorldCalibrationMatrix );
    ::Serialize( serial, "Tools", m_toolVec );

    if( serial->IsReader() )
    {
        if( wasTracking )
            this->Initialize();
    }
}

void Tracker::ActivateAllPassiveTools()
{
    QList <QString> filesNotFound;

    for( unsigned int i = 0; i < m_toolVec.size(); i++ )
    {
        if( m_toolVec[i].type == Passive && m_toolVec[i].active)
        {
            if( !m_toolVec[i].romFileName.isEmpty() )
            {
                if (QFile::exists(m_toolVec[i].romFileName))
                    this->ReallyActivatePassiveTool( i );
                else
                {
                    filesNotFound.append(m_toolVec[i].romFileName);
                    m_toolVec[i].active = 0;
                }
            }
            else
            {
                QString use("undefined usage");
                if (m_toolVec[i].use == 0)
                    use = "reference";
                else if (m_toolVec[i].use == 1)
                        use = "pointer";
                else if (m_toolVec[i].use == 2)
                        use = "USprobe";
                else if (m_toolVec[i].use == 3)
                        use = "camera";
                QString  accessError = "Missing ROM file for \"" + m_toolVec[i].name + "\" used as " + use +".";
                QMessageBox::warning(0, "Error: ", accessError, 1, 0);
                m_toolVec[i].active = 0;
            }
        }
    }
    if (!filesNotFound.isEmpty())
    {
        QString  accessError("ROM Files not found\n");
        accessError.append(filesNotFound.at(0));
        for (int i = 1; i < filesNotFound.count(); i++)
        {
            accessError.append(",\n");
            accessError.append(filesNotFound.at(i));
        }
        QMessageBox::warning(0, "Error: ", accessError, 1, 0);
    }
}

void Tracker::DeactivateAllPassiveTools()
{
    vtkPOLARISTracker * polaris = vtkPOLARISTracker::SafeDownCast( m_tracker );
    for( unsigned int i = 0; i < m_toolVec.size(); i++ )
    {
        if( m_toolVec[i].type == Passive )
        {
            this->ReallyDeactivatePassiveTool( i );
        }
    }
}


void Tracker::ReallyActivatePassiveTool( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size()  );  // index is within range
    if (m_toolVec[toolIndex].toolPort == -1 )                  // if port has not been assigned already, the code below will assign it
    {
        vtkPOLARISTracker * polaris = vtkPOLARISTracker::SafeDownCast( m_tracker );
        if( polaris )
        {
            // Find a tool port available
            int toolPort = polaris->GetAvailablePassivePort();

            if( toolPort != -1 )
            {
                if( QFile::exists( m_toolVec[ toolIndex ].romFileName ) )
                {
                    polaris->LoadVirtualSROM( toolPort, m_toolVec[ toolIndex ].romFileName.toUtf8().data() );
                    // TODO: implement a mechanism to make sure that if the calibration matrix is modified
                    // by someone else than the Tracker class, the cached matrix is updated before the application
                    // is closed.
                    polaris->GetTool( toolPort )->SetCalibrationMatrix( m_toolVec[ toolIndex ].cachedCalibrationMatrix );
                    if( m_referenceTool == toolIndex )
                    {
                        polaris->SetReferenceTool( toolPort );
                        emit ReferenceToolChanged(true);
                    }
                    m_toolVec[ toolIndex ].toolPort = toolPort;
                    m_toolVec[ toolIndex ].active = 1;

                    emit ToolActivated( toolIndex );
                }
            }
        }
    }
}

void Tracker::ReallyDeactivatePassiveTool( int toolIndex )
{
    Q_ASSERT( toolIndex >= 0 && toolIndex < (int)m_toolVec.size() );
    if( m_toolVec[toolIndex].toolPort == -1 )
        return;

    vtkPOLARISTracker * polaris = vtkPOLARISTracker::SafeDownCast( m_tracker );
    if( polaris )
    {
        // backup calibration matrix for the tool
        vtkTrackerTool * tool = this->GetTool( toolIndex );
        if( tool )
        {
            m_toolVec[ toolIndex ].cachedCalibrationMatrix->DeepCopy( tool->GetCalibrationMatrix() );
        }

        polaris->ClearVirtualSROM( m_toolVec[ toolIndex ].toolPort );
        if( m_referenceTool == toolIndex )
        {
            polaris->SetReferenceTool( -1 );
            emit ReferenceToolChanged( false );
        }
        m_toolVec[ toolIndex ].toolPort = -1;
        m_toolVec[ toolIndex ].active = 0;

        emit ToolDeactivated( toolIndex );
    }
}


void Tracker::BackupCalibrationMatrices()
{
    for( unsigned int toolIndex = 0; toolIndex < m_toolVec.size(); toolIndex++ )
    {
        vtkTrackerTool * tool = this->GetTool( toolIndex );
        if( tool )
        {
            m_toolVec[ toolIndex ].cachedCalibrationMatrix->DeepCopy( tool->GetCalibrationMatrix() );
            m_toolVec[ toolIndex ].cachedCalibrationRMS = tool->GetLastCalibrationRMS();
        }
    }
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
                tool->SetCalibrationMatrix( m_toolVec[ toolIndex ].cachedCalibrationMatrix );
                tool->SetLastCalibrationRMS( m_toolVec[ toolIndex ].cachedCalibrationRMS );
                if( m_referenceTool == toolIndex )
                {
                    emit ReferenceToolChanged(true);
                }
                emit ToolActivated( toolIndex );
            }
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
                    (*it).cachedCalibrationMatrix->DeepCopy( tool->GetCalibrationMatrix() );
                    (*it).cachedCalibrationRMS = tool->GetLastCalibrationRMS();
                    emit ToolDeactivated( toolIndex );
                    if( m_referenceTool == toolIndex )
                    {
                       emit ReferenceToolChanged( false );
                    } 
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

void Tracker::WriteActivePointerCalibrationMatrix(QString & filename)
{
    if (m_navigationPointer > -1)
    {
        vtkMatrix4x4 *calibrationMatrix = this->GetToolCalibrationMatrix(m_navigationPointer);
        vtkXFMWriter *writer = vtkXFMWriter::New();
        writer->SetFileName( filename.toUtf8().data() );
        writer->SetMatrix(calibrationMatrix);
        writer->Write();
        writer->Delete();
    }
}

int Tracker::FindFirstActivePointer()
{
    for (int i = 0; i < this->GetNumberOfTools(); i++)
    {
        if (this->GetToolUse(i) == Pointer && this->IsToolActive(i))
            return i;
    }
    return -1;
}

void Tracker::RenameTool(QString oldName, QString newName)
{
    int index = this->GetToolIndex(oldName);
    if (index >= 0)
        this->SetToolName(index, newName);
}

bool Tracker::ValidateReference()
{
    if (m_referenceTool >= 0 && this->IsToolActive(m_referenceTool))
    {
        return true;
    }
    else
    {
        return false;
    }
}
