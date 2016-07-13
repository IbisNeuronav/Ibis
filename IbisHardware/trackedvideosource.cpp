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
#include "videosettingsdialog.h"
#include "videoviewdialog.h"
#include "vtkTransform.h"
#include "trackerflags.h"
#include "serializerhelper.h"
#include "vtkTrackerTool.h"
#include "vtkTrackerBuffer.h"
#include "vtkTrackedVideoBuffer.h"
#include "vtkVideoSource2Factory.h"
#include "vtkVideoSource2.h"

ObjectSerializationMacro( TrackedVideoSource );
ObjectSerializationMacro( VideoSourceSettings );

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
}

TrackedVideoSource::~TrackedVideoSource()
{
    if( m_source )
        m_source->Delete();
    m_sourceFactory->Delete();
    m_newFrameCallback->Delete();
    m_liveVideoBuffer->Delete();
}


bool TrackedVideoSource::SerializeRead( Serializer * serializer )
{
    // ----------------------------------------------------------
    // video sour serializsaton
    m_videoSettings.Serialize( serializer );
    VideoDeviceTypeName = m_videoSettings.VideoDeviceTypeName;
    // ----------------------------------------------------------

    ::Serialize( serializer, "TimeShift", m_timeShift );

    return true;
}

bool TrackedVideoSource::SerializeWrite( Serializer * serializer )
{
    m_videoSettings.VideoDeviceTypeName = VideoDeviceTypeName;
    m_videoSettings.OutputFormat = m_source ? m_source->GetOutputFormat() : VTK_LUMINANCE;
    this->BackupSourceTypeSpecificSettings();
    m_videoSettings.Serialize( serializer );

    ::Serialize( serializer, "TimeShift", m_timeShift );

    return true;
}

bool TrackedVideoSource::Serialize( Serializer * serializer )
{
    if( serializer->IsReader() )
        return SerializeRead( serializer );
    return SerializeWrite( serializer );
}

void TrackedVideoSource::SetSyncedTrackerTool( vtkTrackerTool * t )
{
    m_liveVideoBuffer->SetTrackerTool( t );
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
    
vtkTransform * TrackedVideoSource::GetTransform()
{
    return this->GetCurrentBuffer()->GetOutputTransform();
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
    m_liveVideoBuffer->LockUpdate();
}

void TrackedVideoSource::UnlockUpdates()
{
    m_liveVideoBuffer->UnlockUpdate();
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

void TrackedVideoSource::OutputUpdatedSlot()
{
    emit OutputUpdatedSignal();
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
