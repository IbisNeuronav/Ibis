#include "vtkPointGreyVideoSource2.h"
#include "vtkVideoBuffer.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"
#include "vtkVariant.h"
#include "flycapture/FlyCapture2.h"
#include <vector>
#include <sstream>

vtkStandardNewMacro(vtkPointGreyVideoSource2);

using namespace FlyCapture2;

struct FrameRateString
{
    FrameRate frEnum;
    const char * frString;
};

FrameRateString frameRateStrings[ NUM_FRAMERATES ] = {
    { FRAMERATE_1_875, "1.875" },
    { FRAMERATE_3_75, "3.75" },
    { FRAMERATE_7_5, "7.5" },
    { FRAMERATE_15, "15" },
    { FRAMERATE_30, "30" },
    { FRAMERATE_60, "60" },
    { FRAMERATE_120, "120" },
    { FRAMERATE_240, "240" },
    { FRAMERATE_FORMAT7, "Custom frame rate for Format7 functionality" }
};

struct VideoModeDescription
{
    VideoMode vmEnum;
    const char * vmString;
    int vmFrameWidth;
    int vmFrameHeight;
};

VideoModeDescription videoModeDescriptions[ NUM_VIDEOMODES ] = {
   { VIDEOMODE_160x120YUV444, "160x120 YUV444", 160, 120 },
   { VIDEOMODE_320x240YUV422, "320x240 YUV422", 320, 240 },
   { VIDEOMODE_640x480YUV411, "640x480 YUV411", 640, 480 },
   { VIDEOMODE_640x480YUV422, "640x480 YUV422", 640, 480  },
   { VIDEOMODE_640x480RGB, "640x480 24-bit RGB", 640, 480 },
   { VIDEOMODE_640x480Y8, "640x480 8-bit", 640, 480 },
   { VIDEOMODE_640x480Y16, "640x480 16-bit", 640, 480 },
   { VIDEOMODE_800x600YUV422, "800x600 YUV422", 800, 600 },
   { VIDEOMODE_800x600RGB, "800x600 RGB", 800, 600 },
   { VIDEOMODE_800x600Y8, "800x600 8-bit", 800, 600 },
   { VIDEOMODE_800x600Y16, "800x600 16-bit", 800, 600 },
   { VIDEOMODE_1024x768YUV422, "1024x768 YUV422", 1024, 768 },
   { VIDEOMODE_1024x768RGB, "1024x768 RGB", 1024, 768 },
   { VIDEOMODE_1024x768Y8, "1024x768 8-bit", 1024, 768 },
   { VIDEOMODE_1024x768Y16, "1024x768 16-bit", 1024, 768 },
   { VIDEOMODE_1280x960YUV422, "1280x960 YUV422", 1280, 960 },
   { VIDEOMODE_1280x960RGB, "1280x960 RGB", 1280, 960 },
   { VIDEOMODE_1280x960Y8, "1280x960 8-bit", 1280, 960 },
   { VIDEOMODE_1280x960Y16, "1280x960 16-bit", 1280, 960 },
   { VIDEOMODE_1600x1200YUV422, "1600x1200 YUV422", 1600, 1200 },
   { VIDEOMODE_1600x1200RGB, "1600x1200 RGB", 1600, 1200 },
   { VIDEOMODE_1600x1200Y8, "1600x1200 8-bit", 1600, 1200 },
   { VIDEOMODE_1600x1200Y16, "1600x1200 16-bit", 1600, 1200 },
   { VIDEOMODE_FORMAT7, "Custom video mode for Format7 functionality", 640, 480 }
};

static const VideoMode DefaultVideoMode = VIDEOMODE_640x480Y8;
static const FrameRate DefaultFrameRate = FRAMERATE_15;

//----------------------------------------------------------------------------
vtkPointGreyVideoSource2::vtkPointGreyVideoSource2()
{

    int videoMode = static_cast<int>(DefaultVideoMode);  // default video mode
    int frameRate = static_cast<int>(DefaultFrameRate);          // default frame rate
    m_videoModeAndFrameRate = VideoModeAndFrameRateToString( videoMode, frameRate );
    m_camera = 0;
    m_convertedFrame = new Image;
    this->InternalSetBuffer();

    // Define generic params
    AddComboParam( "Resolution + Frame Rate", &vtkPointGreyVideoSource2::GetVideoModeAndFrameRateString, &vtkPointGreyVideoSource2::SetVideoModeAndFrameRateString, &vtkPointGreyVideoSource2::GetVideoModeAndFrameRateStrings );
}

//----------------------------------------------------------------------------
vtkPointGreyVideoSource2::~vtkPointGreyVideoSource2()
{
    this->ReleaseSystemResources();
    delete m_convertedFrame;
}

//----------------------------------------------------------------------------
void vtkPointGreyVideoSource2::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf( os, indent );
    if( !this->Initialized )
    {
        os << indent << "Video source NOT initialized" << endl;
    }
}

//----------------------------------------------------------------------------
bool vtkPointGreyVideoSource2::Initialize()
{
    if( this->Initialized )
    {
        this->ReleaseSystemResources();
    }

    // See if there is at least one camera
    BusManager busMgr;
    unsigned int numCameras;
    Error error = busMgr.GetNumOfCameras(&numCameras);
    if( error != PGRERROR_OK )
    {
        vtkErrorMacro( << error.GetDescription() );
        return false;
    }

    if( numCameras < 1 )
    {
        vtkErrorMacro( << "No camera detected" );
        return false;
    }

    // Connect to the first camera
    PGRGuid guid;
    error = busMgr.GetCameraFromIndex( 0, &guid );
    if (error != PGRERROR_OK)
    {
        vtkErrorMacro( << error.GetDescription() );
        return false;
    }
    m_camera = new Camera;
    error = m_camera->Connect( &guid );
    if (error != PGRERROR_OK)
    {
        vtkErrorMacro( << error.GetDescription() );
        return false;
    }

    // Gather all possible video modes and frame rates
    GatherValidVideoModesAndFrameRates();

    // Set the VideoMode and FrameRate
    this->SetVideoModeAndFrameRate();

    this->Initialized = 1;
    return true;
}

//----------------------------------------------------------------------------
void vtkPointGreyVideoSource2::ReleaseSystemResources()
{
    if( this->Recording )
    {
        this->Stop();
    }

    if( m_camera )
    {
        m_camera->Disconnect();
        delete m_camera;
        m_camera = 0;
    }

    this->Initialized = 0;
}


//----------------------------------------------------------------------------
void vtkPointGreyVideoSource2::Grab()
{
    if( !this->Initialized )
    {
        vtkErrorMacro( << "vtkPointGreyVideoSource2::Grab() : must initialize object before calling this function." );
        return;
    }

    if (!this->Recording)
    {
        Error error = m_camera->StartCapture();
        if (error != PGRERROR_OK)
        {
            vtkErrorMacro( << error.GetDescription() );
            return;
        }

        // Retrieve an image
        Image rawImage;

        error = m_camera->RetrieveBuffer( &rawImage );
        if (error != PGRERROR_OK)
        {
            vtkErrorMacro( << error.GetDescription() );
        }
        else
        {
            this->OnImageGrabbed( &rawImage );
        }

        error = m_camera->StopCapture();
        if( error != PGRERROR_OK )
            vtkErrorMacro( << error.GetDescription() );
    }
}

//----------------------------------------------------------------------------
void vtkPointGreyVideoSource2::Record()
{
    if( !this->Initialized )
    {
        vtkErrorMacro( << "vtkPointGreyVideoSource2::Grab() : must initialize object before calling this function." );
        return;
    }

    if( !this->Recording )
    {
        this->Recording = 1;

        // Start capturing images
        Error error = m_camera->StartCapture( OnImageGrabbedStatic, (void*)this );
        if (error != PGRERROR_OK)
        {
            vtkErrorMacro( << error.GetDescription() );
            return;
        }

        this->Modified();
    }
}


//----------------------------------------------------------------------------
void vtkPointGreyVideoSource2::Stop()
{
    if( !this->Recording )
    {
        return;
    }

    // stop recording and wait for player thread to terminate.
    this->Recording = 0;

    // Stop camera
    Error error = m_camera->StopCapture();
    if( error != PGRERROR_OK )
    {
        vtkErrorMacro( << error.GetDescription() );
        return;
    }

    this->Modified();
}

void vtkPointGreyVideoSource2::SetVideoModeAndFrameRateString( const char * str )
{
    m_videoModeAndFrameRate = str;
    if( this->Initialized )
        SetVideoModeAndFrameRate();
}

void vtkPointGreyVideoSource2::GetVideoModeAndFrameRateStrings( std::vector< std::string > & modeStrings )
{
    for( int i = 0; i < m_availableVideoModeAndFrameRates.size(); ++i )
    {
        std::ostringstream os;
        std::string vms( videoModeDescriptions[ m_availableVideoModeAndFrameRates[i].vm ].vmString );
        std::string frs( frameRateStrings[ m_availableVideoModeAndFrameRates[i].fr ].frString );
        os << vms << "@" << frs << "fps";
        modeStrings.push_back( os.str().c_str() );
    }
}

void vtkPointGreyVideoSource2::InternalSetBuffer()
{
    vtkUnpacker * unpacker = vtkUnpacker::New();
    this->Buffer->SetUnpacker( unpacker );
    unpacker->Delete();
    this->Buffer->SetFlipFrames( 1 );
}

std::string vtkPointGreyVideoSource2::VideoModeAndFrameRateToString( int videoMode, int frameRate )
{
    std::ostringstream os;
    std::string vms( videoModeDescriptions[ videoMode ].vmString );
    std::string frs( frameRateStrings[ frameRate ].frString );
    os << vms << "@" << frs << "fps";
    return os.str();
}

bool vtkPointGreyVideoSource2::StringToModeAndFrameRate( std::string s, int & videoMode, int & frameRate )
{
    for( unsigned i = 0; i < m_availableVideoModeAndFrameRates.size(); ++i )
    {
        if( m_availableVideoModeAndFrameRates[i].ModeAndFrameRateString == s )
        {
            videoMode = m_availableVideoModeAndFrameRates[i].vm;
            frameRate = m_availableVideoModeAndFrameRates[i].fr;
            return true;
        }
    }
    return false;
}

void vtkPointGreyVideoSource2::GatherValidVideoModesAndFrameRates()
{
    for( int videoModeIndex = 0; videoModeIndex < NUM_VIDEOMODES; ++videoModeIndex )
    {
        for( int frameRateIndex = 0; frameRateIndex < NUM_FRAMERATES; ++frameRateIndex )
        {
            bool isSupported = false;
            m_camera->GetVideoModeAndFrameRateInfo( videoModeDescriptions[ videoModeIndex ].vmEnum, frameRateStrings[ frameRateIndex ].frEnum, &isSupported );
            if( isSupported )
            {
                ModeFrameRate mfr;
                mfr.vm = videoModeDescriptions[ videoModeIndex ].vmEnum;
                mfr.fr = frameRateStrings[ frameRateIndex ].frEnum;
                mfr.ModeAndFrameRateString = VideoModeAndFrameRateToString( mfr.vm, mfr.fr );
                m_availableVideoModeAndFrameRates.push_back( mfr );
            }
        }
    }
}

bool vtkPointGreyVideoSource2::SetVideoModeAndFrameRate()
{
    // Stop capturing if we were
    bool wasRecording = false;
    if( this->Recording )
    {
        wasRecording = true;
        Error error = m_camera->StopCapture();
        if( error != PGRERROR_OK )
        {
            vtkErrorMacro( << error.GetDescription() );
        }
    }

    // Change video mode and frame rate
    int vmInt = static_cast<int>(DefaultVideoMode);
    int frInt = static_cast<int>(DefaultFrameRate);
    if( !StringToModeAndFrameRate( m_videoModeAndFrameRate, vmInt, frInt ) )
    {
        vtkErrorMacro( << "Wanted video mode and frame rate not available" );
        return false;
    }
    VideoMode vm = static_cast<VideoMode>(vmInt);
    FlyCapture2::FrameRate fr = static_cast<FlyCapture2::FrameRate>(frInt);
    Error error = m_camera->SetVideoModeAndFrameRate( vm, fr );
    if( error != PGRERROR_OK )
    {
        vtkErrorMacro( << error.GetDescription() );
        return false;
    }

    // Make sure the buffer is able to handle the new video format
    int width = videoModeDescriptions[ vm ].vmFrameWidth;
    int height = videoModeDescriptions[ vm ].vmFrameHeight;
    this->FrameSize[ 0 ] = width;
    this->FrameSize[ 1 ] = height;
    this->Buffer->SetFrameSize( width, height, 1 );
    this->Buffer->Initialize();

    // Restart capture if needed
    if( wasRecording )
    {
        Error error = m_camera->StartCapture( OnImageGrabbedStatic, (void*)this );
        if (error != PGRERROR_OK)
        {
            vtkErrorMacro( << error.GetDescription() );
        }
    }

    return true;
}

void vtkPointGreyVideoSource2::OnImageGrabbedStatic( Image * image, const void * callbackData )
{
    vtkPointGreyVideoSource2 * grabber = (vtkPointGreyVideoSource2*)callbackData;
    grabber->OnImageGrabbed( image );
}

void vtkPointGreyVideoSource2::OnImageGrabbed( FlyCapture2::Image * image )
{
    // generate a time stamp  --- simtodo : check if getting the timestamp from Flycapture reduces the offset.
    double time = vtkTimerLog::GetUniversalTime();

    // Convert the raw image
    PixelFormat format = PIXEL_FORMAT_RGB8;
    if( this->Buffer->GetOutputFormat() == VTK_LUMINANCE )
        format = PIXEL_FORMAT_MONO8;
    Error error = image->Convert( format, m_convertedFrame );
    if( error != PGRERROR_OK )
    {
        vtkErrorMacro( << error.GetDescription() );
        return;
    }

    // Send frame to the buffer
    this->Buffer->SetCurrentFrameData( m_convertedFrame->GetData(), time );
}
