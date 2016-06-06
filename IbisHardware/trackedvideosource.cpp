/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "trackedvideosource.h"
#include "usmask.h"
#include "videosettingsdialog.h"
#include "videoviewdialog.h"
#include "vtkTransform.h"
#include "trackerflags.h"
#include "vtkmatrix4x4utilities.h"
#include "vtkTrackerTool.h"
#include "vtkTrackerBuffer.h"
#include "vtkTrackedVideoBuffer.h"
#include "vtkVideoSource2Factory.h"
#include "vtkVideoSource2.h"

ObjectSerializationMacro( TrackedVideoSource );
ObjectSerializationMacro( TrackedVideoSource::ScaleFactorInfo );
ObjectSerializationMacro( VideoSourceSettings );

//---------------------------------------------------------------------------------------
// ScaleFactorInfo class implementation
//---------------------------------------------------------------------------------------

TrackedVideoSource::ScaleFactorInfo::ScaleFactorInfo()
{
    name = QString( "None" );
    matrix = vtkMatrix4x4::New();
}

TrackedVideoSource::ScaleFactorInfo::ScaleFactorInfo( const ScaleFactorInfo & other )
{
    name = other.name;
    matrix = vtkMatrix4x4::New();
    matrix->DeepCopy( other.matrix );
}

TrackedVideoSource::ScaleFactorInfo::~ScaleFactorInfo()
{
    matrix->Delete();
}

bool TrackedVideoSource::ScaleFactorInfo::Serialize( Serializer * serializer )
{
    bool res = true;
    res &= ::Serialize( serializer, "Name", name );
    res &= ::Serialize( serializer, "Matrix", matrix );
    return res;
}

//---------------------------------------------------------------------------------------
// VideoSourceSettings class implementation
//---------------------------------------------------------------------------------------

VideoSourceSettings::VideoSourceSettings() : VideoDeviceTypeName( GenericVideoDeviceTypeName )
{
    OutputFormat = VTK_LUMINANCE;
}

#include "serializerhelper.h"

void VideoSourceSettings::Serialize( Serializer * serializer )
{
    ::Serialize( serializer, "VideoDeviceTypeName", VideoDeviceTypeName );
    ::Serialize( serializer, "OutputFormat", OutputFormat );
    ::Serialize( serializer, "SourceTypeSpecificSettings", SourceTypeSpecificSettings );
}

//---------------------------------------------------------------------------------------
// TrackedVideoSource class implementation
//---------------------------------------------------------------------------------------

TrackedVideoSource::TrackedVideoSource()
{
    m_numberOfClients = 0;
    VideoDeviceTypeName = GenericVideoDeviceTypeName;
    m_source = 0;
    m_timeShift = 0.0;
    m_sourceFactory = vtkVideoSource2Factory::New();
    m_newFrameCallback = vtkObjectCallback<TrackedVideoSource>::New();
    m_newFrameCallback->SetCallback( this, &TrackedVideoSource::OutputUpdatedSlot );

    m_liveVideoBuffer = vtkTrackedVideoBuffer::New();
    m_liveVideoBuffer->SetFrameSize( 640, 480, 1 );
    m_liveVideoBuffer->SetFrameBufferSize( 10 );
    m_liveVideoBuffer->SetTimeShift( m_timeShift );
    m_liveVideoBuffer->Initialize();
    m_liveVideoBuffer->AddObserver( vtkCommand::UpdateEvent, m_newFrameCallback );

    m_defaultMask = USMask::New();

    this->Track = 0;

    CheckCalibrationValidity();
}

TrackedVideoSource::~TrackedVideoSource()
{
    if( m_source )
        m_source->Delete();
    if( this->Track )
        this->Track->UnRegister( this );
    m_sourceFactory->Delete();
    m_newFrameCallback->Delete();
    m_liveVideoBuffer->Delete();
    m_defaultMask->Delete();

    //this->SetTracker( 0 );  // simtodo : causes a crash : need to eventually fix this.
    m_calibrationMatrices.clear();
}

// simtodo : remove code to read old format
bool TrackedVideoSource::SerializeRead( Serializer * serializer )
{
    // ----------------------------------------------------------
    // video sour serializsaton
    m_videoSettings.Serialize( serializer );
    VideoDeviceTypeName = m_videoSettings.VideoDeviceTypeName;
    // ----------------------------------------------------------

    ::Serialize( serializer, "TrackerToolName", TrackerToolName );
    this->m_calibrationMatrices.clear();
    ::Serialize( serializer, "CalibrationMatrices", m_calibrationMatrices );
    ::Serialize( serializer, "CurrentCalibrationMatrixName", m_currentCalibrationMatrixName );
    ::Serialize( serializer, "TimeShift", m_timeShift );
    ::Serialize( serializer, "CameraIntrinsicParams", m_cameraIntrinsicParams );
    ::Serialize( serializer, "DefaultMask", m_defaultMask );

    // Updates
    CheckCalibrationValidity();
    if( this->Track )
        AssignTool();
    m_liveVideoBuffer->SetCalibrationMatrix( GetCurrentCalibrationMatrix() );
    m_defaultMask->SetAsDefault(); // this sets default params
    m_defaultMask->ResetToDefault(); // this will set mask params to deafult values and build the mask
    return true;
}

bool TrackedVideoSource::SerializeWrite( Serializer * serializer )
{
    m_videoSettings.VideoDeviceTypeName = VideoDeviceTypeName;
    m_videoSettings.OutputFormat = m_source ? m_source->GetOutputFormat() : VTK_LUMINANCE;
    this->BackupSourceTypeSpecificSettings();
    m_videoSettings.Serialize( serializer );
    // ----------------------------------------------------------

    ::Serialize( serializer, "TrackerToolName", TrackerToolName );
    ::Serialize( serializer, "CalibrationMatrices", m_calibrationMatrices );
    ::Serialize( serializer, "CurrentCalibrationMatrixName", m_currentCalibrationMatrixName );
    ::Serialize( serializer, "TimeShift", m_timeShift );
    ::Serialize( serializer, "CameraIntrinsicParams", m_cameraIntrinsicParams );
    ::Serialize( serializer, "DefaultMask", m_defaultMask );

    return true;
}

bool TrackedVideoSource::Serialize( Serializer * serializer )
{
    if( serializer->IsReader() )
        return SerializeRead( serializer );
    return SerializeWrite( serializer );
}

void TrackedVideoSource::SetTracker( Tracker * track )
{
    if( this->Track == track )
    {
        return;
    }
    
    if( this->Track )
    {
        this->Track->UnRegister( this );
        this->Track->disconnect( this );
    }
    
    this->Track = track;
    
    if( this->Track )
    {
        this->Track->Register( this );
        this->AssignTool();
        connect( this->Track, SIGNAL( ToolUseChanged(int) ), this, SLOT( ToolUseChanged( int ) ) );
    }
}

void TrackedVideoSource::SetTrackerToolName( QString name )
{
    if( this->TrackerToolName == name )
    {
        return;
    }
    
    this->TrackerToolName = name;
    
    this->AssignTool();
}

void TrackedVideoSource::InitializeVideo()
{
    this->ClearVideo();

    // If no type has been defined, take the first one available
    if( VideoDeviceTypeName == QString( GenericVideoDeviceTypeName ) && m_sourceFactory->GetNumberOfTypes() > 0 )
        VideoDeviceTypeName = QString( m_sourceFactory->GetTypeName( 0 ) );

    m_source = m_sourceFactory->CreateInstance( VideoDeviceTypeName.toUtf8().data() );
    m_source->SetOutputFormat( m_videoSettings.OutputFormat );
    m_source->SetBuffer( m_liveVideoBuffer );

    // Apply generic params
    VideoSourceSettings::SourceTypeSpecificSettingsCont::iterator it = m_videoSettings.SourceTypeSpecificSettings.find( std::string( VideoDeviceTypeName.toUtf8().data() ) );
    if( it != m_videoSettings.SourceTypeSpecificSettings.end() )
    {
        m_source->SetAllParamsValues( (*it).second );
    }

    // Initialize with params applied
    m_source->Initialize();

    // by default, current frame is -1. We want 0.
    m_source->GetBuffer()->Seek( 1 );

    if( m_numberOfClients > 0 )
        m_source->Record();
}

void TrackedVideoSource::ClearVideo()
{
    if( m_source )
    {
        m_source->Delete();
        m_source = 0;
    }
}

void TrackedVideoSource::SetTimeShift( double shift )
{
    m_timeShift = shift;
    m_liveVideoBuffer->SetTimeShift( shift );
}

double TrackedVideoSource::GetTimeShift()
{
    return m_timeShift;
}

void TrackedVideoSource::AddCalibrationMatrix( QString name )
{
    int newIndex = this->GetCalibrationMatrixIndex( name );
    if(  newIndex == -1 )  // don't allow duplicate names
    {
        ScaleFactorInfo info;
        info.name = name;
        info.matrix = vtkMatrix4x4::New();
        this->m_calibrationMatrices.push_back( info );
    }
}

void TrackedVideoSource::RemoveCalibrationMatrix( QString name )
{
    int newIndex = this->GetCalibrationMatrixIndex( name );
    if( newIndex != -1 )
    {
        ScaleFactorMatricesContainer::iterator it = this->m_calibrationMatrices.begin() + newIndex;
        this->m_calibrationMatrices.erase( it );
    }
}

int TrackedVideoSource::GetNumberOfCalibrationMatrices()
{
    return this->m_calibrationMatrices.size();
}

QString TrackedVideoSource::GetCalibrationMatrixName( int index )
{
    if( index >= 0 && index < (int)this->m_calibrationMatrices.size() )
    {
        return this->m_calibrationMatrices[ index ].name;
    }
    return QString("");
}

vtkMatrix4x4 * TrackedVideoSource::GetCalibrationMatrix( int index )
{
    if( index >= 0 && index < (int)this->m_calibrationMatrices.size() )
    {
        return this->m_calibrationMatrices[ index ].matrix;
    }
    return 0;
}

void TrackedVideoSource::SetCurrentCalibrationMatrixName( QString factorName )
{
    if( factorName == this->m_currentCalibrationMatrixName )
        return;
    
    this->m_currentCalibrationMatrixName = factorName;
    
    int scaleFactorIndex = this->GetCalibrationMatrixIndex( this->m_currentCalibrationMatrixName );
    vtkMatrix4x4 * newCalMatrix = this->m_calibrationMatrices[ scaleFactorIndex ].matrix;
    int toolIndex = this->Track->GetToolIndex( this->TrackerToolName );
    Q_ASSERT( toolIndex != -1 );
    vtkMatrix4x4 * toolCalMatrix = this->Track->GetToolCalibrationMatrix( toolIndex );
    toolCalMatrix->DeepCopy( newCalMatrix );

    // need to set the matrix in the buffer too. Should it be automatic?
    m_liveVideoBuffer->SetCalibrationMatrix( toolCalMatrix );
}

QString TrackedVideoSource::GetCurrentCalibrationMatrixName()
{
    if (!this->m_currentCalibrationMatrixName.isEmpty())
        return this->m_currentCalibrationMatrixName;
    return QString("Probe not present.");
}

vtkMatrix4x4 * TrackedVideoSource::GetCurrentCalibrationMatrix()
{
    int scaleFactorIndex = GetCalibrationMatrixIndex( this->m_currentCalibrationMatrixName );
    Q_ASSERT( scaleFactorIndex != -1 && scaleFactorIndex < m_calibrationMatrices.size() );  // there should always be a valid calibration matrix
    return m_calibrationMatrices[ scaleFactorIndex ].matrix;
}

void TrackedVideoSource::SetCurrentCalibrationMatrix( vtkMatrix4x4 * mat )
{
    // Copy the matrix here
    vtkMatrix4x4 * calMat = this->GetCurrentCalibrationMatrix();
    calMat->DeepCopy( mat );

    // Assign cal matrix to the tool if tool available
    if( this->Track )
    {
        vtkTrackerTool * tool = this->Track->GetTool( this->TrackerToolName );
        if( tool )
            tool->SetCalibrationMatrix( mat );
    }

    m_liveVideoBuffer->SetCalibrationMatrix( mat );
}
    
vtkTransform * TrackedVideoSource::GetTransform()
{
    return this->GetCurrentBuffer()->GetOutputTransform();
}

vtkTransform * TrackedVideoSource::GetUncalibratedTransform()
{
    return this->GetCurrentBuffer()->GetUncalibratedOutputTransform();
}

TrackerToolState TrackedVideoSource::GetState()
{
    int flags = this->GetCurrentBuffer()->GetFlags();
    if( flags & TR_MISSING ) return Missing;
    if( flags & TR_OUT_OF_VIEW ) return OutOfView;
    if( flags & TR_OUT_OF_VOLUME ) return OutOfVolume;
    if( flags == -1 ) return Undefined;
    return Ok;
}
    
vtkImageData * TrackedVideoSource::GetVideoOutput()
{
    return this->GetCurrentBuffer()->GetOutput();
}

bool TrackedVideoSource::IsTransformFrozen()
{
    return this->GetCurrentBuffer()->IsTransformFrozen();
}

void TrackedVideoSource::FreezeTransform( int nbFrames )
{
    this->GetCurrentBuffer()->FreezeTransform( nbFrames );
}

void TrackedVideoSource::UnFreezeTransform()
{
    this->GetCurrentBuffer()->UnFreezeTransform();
}

void TrackedVideoSource::LockUpdates()
{
    this->Track->LockUpdate();
    m_liveVideoBuffer->LockUpdate();
}

void TrackedVideoSource::UnlockUpdates()
{
    m_liveVideoBuffer->UnlockUpdate();
    this->Track->UnlockUpdate();
}

void TrackedVideoSource::Update()
{
    this->GetCurrentBuffer()->Update();
}

int TrackedVideoSource::GetFrameWidth( )
{
    return m_source->GetFrameSize()[0];
}

int TrackedVideoSource::GetFrameHeight()
{
    return m_source->GetFrameSize()[1];
}

double TrackedVideoSource::GetFrameRate()
{
    return m_source->GetFrameRate();
}

void TrackedVideoSource::AddClient()
{
    if( m_numberOfClients == 0 )
        m_source->Record();
    m_numberOfClients++;
}

void TrackedVideoSource::RemoveClient()
{
    if (m_numberOfClients > 0)
    {
        m_numberOfClients--;
        if( m_numberOfClients == 0)
            m_source->Stop();
    }
}

int TrackedVideoSource::GetNumberOfDeviceTypes()
{
    return m_sourceFactory->GetNumberOfTypes();
}

QString TrackedVideoSource::GetDeviceTypeName( int index )
{
    return QString( m_sourceFactory->GetTypeName( index ) );
}

int TrackedVideoSource::GetCurrentDeviceTypeIndex()
{
    int res = -1;
    for( int i = 0; i < m_sourceFactory->GetNumberOfTypes(); ++i )
    {
        if( VideoDeviceTypeName == QString( m_sourceFactory->GetTypeName( i ) ) )
        {
            res = i;
            break;
        }
    }
    return res;
}

QString TrackedVideoSource::GetCurrentDeviceTypeName()
{
    return VideoDeviceTypeName;
}

void TrackedVideoSource::SetCurrentDeviceType( QString deviceType )
{
    if( deviceType != this->VideoDeviceTypeName )
    {
        this->VideoDeviceTypeName = deviceType;
        this->InitializeVideo();
    }
}

void TrackedVideoSource::SetOutputFormatToLuminance()
{
    m_source->SetOutputFormatToLuminance();
}

int TrackedVideoSource::OutputFormatIsLuminance()
{
    return ( m_source->GetBuffer()->GetOutputFormat() == VTK_LUMINANCE );
}

int TrackedVideoSource::OutputFormatIsRGB()
{
    return ( m_source->GetBuffer()->GetOutputFormat() == VTK_RGB );
}

int TrackedVideoSource::GetOutputFormat()
{
    return m_source->GetBuffer()->GetOutputFormat();
}

void TrackedVideoSource::SetOutputFormatToRGB()
{
    m_source->SetOutputFormatToRGB();
}

QWidget * TrackedVideoSource::CreateVideoSettingsDialog( QWidget * parent )
{
    VideoSettingsDialog * res  = new VideoSettingsDialog( parent );
    res->setAttribute( Qt::WA_DeleteOnClose, true );
    res->SetSource( this );
    return res;
}

QWidget * TrackedVideoSource::CreateVideoViewDialog( QWidget * parent )
{
    VideoViewDialog * res = new VideoViewDialog( parent );
    res->setAttribute( Qt::WA_DeleteOnClose, true );
    res->SetSource( this );
    return res;
}

void TrackedVideoSource::ToolUseChanged( int toolIndex )
{
    if( this->Track->GetToolName( toolIndex ) == this->TrackerToolName )
    {
        ToolUse use = this->Track->GetToolUse( toolIndex );
        if( use != UsProbe && use != Camera )
        {
            this->TrackerToolName = "";
            this->AssignTool();
        }
    }
}

void TrackedVideoSource::OutputUpdatedSlot()
{
    emit OutputUpdatedSignal();
}

int TrackedVideoSource::FindValidTrackerTool()
{
    for( int i = 0; i < this->Track->GetNumberOfTools(); i++)
    {
        ToolUse use =  this->Track->GetToolUse(i);
        if( use == UsProbe || use == Camera )
        {
            if( this->Track->IsToolActive(i) )
                return i;
        }
    }
    return -1;
}

void TrackedVideoSource::AssignTool()
{
    Q_ASSERT( this->Track );

    vtkTrackerTool * tool = this->Track->GetTool( this->TrackerToolName );
    if( !tool )
    {
        int otherValidToolIndex = FindValidTrackerTool();
        if( otherValidToolIndex != -1 )
        {
            tool = this->Track->GetTool( otherValidToolIndex );
            this->TrackerToolName = this->Track->GetToolName( otherValidToolIndex );
        }
    }

    m_liveVideoBuffer->SetTrackerTool( tool );

    if( tool )
        tool->SetCalibrationMatrix( this->GetCurrentCalibrationMatrix() );
    
    emit AssignedToolChanged();
}

void TrackedVideoSource::CheckCalibrationValidity()
{
    // make sure there is at least one scale factor
    if( m_calibrationMatrices.size() == 0 )
    {
        ScaleFactorInfo info;
        info.name = QString("Default");
        this->m_calibrationMatrices.push_back( info );
        this->m_currentCalibrationMatrixName = info.name;
    }
    // Make sure m_currentCalibrationMatrixName is valid, otherwise, change it to the first element
    if( GetCalibrationMatrixIndex( this->m_currentCalibrationMatrixName ) == -1 )
    {
        this->m_currentCalibrationMatrixName = m_calibrationMatrices[0].name;
    }
}

void TrackedVideoSource::BackupSourceTypeSpecificSettings()
{
    if( m_source )
    {
        std::vector< vtkGenericParamValues > genericParams;
        m_source->GetAllParamsValues( genericParams );
        m_videoSettings.SourceTypeSpecificSettings[ std::string(VideoDeviceTypeName.toUtf8().data()) ] = genericParams;
    }
}

vtkTrackedVideoBuffer * TrackedVideoSource::GetCurrentBuffer()
{
    return m_liveVideoBuffer;
}

int TrackedVideoSource::GetCalibrationMatrixIndex( QString name )
{
    int i = 0;
    ScaleFactorMatricesContainer::iterator it = this->m_calibrationMatrices.begin();
    for( ; it != this->m_calibrationMatrices.end(); ++it, ++i )
    {
        if( (*it).name == name )
            return i;
    }
    return -1;
}

void TrackedVideoSource::ComputeScaleCalibrationMatrices( std::vector< std::pair<QString,double> > & scaleFactors, vtkMatrix4x4 * unscaledCalibrationMatrix, double scaleCenter[2] )
{
    for( unsigned i = 0; i < scaleFactors.size(); ++i )
    {
        vtkMatrix4x4 * newCal = vtkMatrix4x4::New();

        // create a matrix that represents the image origin and its invers
        vtkMatrix4x4 * origin = vtkMatrix4x4::New();
        origin->SetElement( 0, 3, scaleCenter[0] );
        origin->SetElement( 1, 3, scaleCenter[1] );
        vtkMatrix4x4 * originInverse = vtkMatrix4x4::New();
        originInverse->SetElement( 0, 3, -scaleCenter[0] );
        originInverse->SetElement( 1, 3, -scaleCenter[1] );

        // create a matrix that represents the scale factor
        vtkMatrix4x4 * scaleFactor = vtkMatrix4x4::New();
        double currentFactor = scaleFactors[ i ].second;
        for( int j = 0; j < 3; j++ )
            scaleFactor->SetElement( j, j, currentFactor );

        // Generate the scaled calibration matrix
        vtkMatrix4x4::Multiply4x4( unscaledCalibrationMatrix, origin, newCal );
        vtkMatrix4x4::Multiply4x4( newCal, scaleFactor, newCal );
        vtkMatrix4x4::Multiply4x4( newCal, originInverse, newCal );

        ScaleFactorInfo info;
        info.name = scaleFactors[i].first;
        info.matrix->DeepCopy( newCal );
        m_calibrationMatrices.push_back( info );

        origin->Delete();
        originInverse->Delete();
        scaleFactor->Delete();
        newCal->Delete();
    }
}
