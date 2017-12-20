/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "vtkV4L2VideoSource2.h"
#include "vtkVideoBuffer.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"
#include "vtkCriticalSection.h"
#include "vtkDataArray.h"
#include "vtkMultiThreader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

vtkStandardNewMacro(vtkV4L2VideoSource2);


// Description:
// Utility class used to be able to change the way the
// buffer is unpacking images into vtkImageData. The way
// the image is stored in the buffer depends on the 
// frame grabber that is being used. The vtkUnpacker class is 
// overridden because the v4l2 framebuffer can use
// unusual pixel packing formats, such as XRGB XBRG BGRX BGR etc.
// For now, we assume input buffer is BGR if out buffer is VTK_RGB
class vtkV4L2Unpacker : public vtkUnpacker
{
    
 public:
     
    static vtkV4L2Unpacker * New() { return new vtkV4L2Unpacker; }
    virtual void UnpackRasterLine( char *outPtr, char *rowPtr, int start, int count, int numberOfScalarComponents, double opacity, int outputFormat )
    {
        char * inPtr = rowPtr + start * numberOfScalarComponents;
        if( outputFormat == VTK_LUMINANCE )
        {
            memcpy( outPtr, inPtr, count * numberOfScalarComponents );
        }
        else if( outputFormat == VTK_RGB )
        {
            char * outIt = outPtr + 2;
            for( int i = 0; i < count; ++i )
            {
                *outIt = *inPtr;
                --outIt;
                ++inPtr;
                *outIt = *inPtr;
                --outIt;
                ++inPtr;
                *outIt = *inPtr;
                outIt += 5;
                ++inPtr;
            }
        }
    }
    
 protected:
     
    vtkV4L2Unpacker() {}
    ~vtkV4L2Unpacker() {} 
};


//----------------------------------------------------------------------------
vtkV4L2VideoSource2::vtkV4L2VideoSource2()
{
    this->Initialized = 0;

    this->VideoDev  = -1;
    this->DevName = "/dev/video0";
    this->NbBuffers = 4;
    this->Buffers   = NULL;
    this->FrameRate = 15.0;
    this->FrameSize[0] = 320;
    this->FrameSize[1] = 240;

    this->InternalSetBuffer();

    // Implement generic params
    AddComboParam( "Video Device", &vtkV4L2VideoSource2::GetVideoDeviceName, &vtkV4L2VideoSource2::SetVideoDeviceName, &vtkV4L2VideoSource2::ListAvailableVideoDevice );
    AddComboParam( "Frame Size", &vtkV4L2VideoSource2::GetResolutionString, &vtkV4L2VideoSource2::SetResolutionString, &vtkV4L2VideoSource2::ListAvailableResolutions );
    AddComboParam( "Frame Rate", &vtkV4L2VideoSource2::GetFrameRateString, &vtkV4L2VideoSource2::SetFrameRateString, &vtkV4L2VideoSource2::ListAvailableFrameRates );
    AddComboParam( "Video Input", &vtkV4L2VideoSource2::GetVideoInput, &vtkV4L2VideoSource2::SetVideoInput, &vtkV4L2VideoSource2::ListAvailableVideoInput );

}


//----------------------------------------------------------------------------
vtkV4L2VideoSource2::~vtkV4L2VideoSource2()
{
    this->Cleanup();
}

//----------------------------------------------------------------------------
void vtkV4L2VideoSource2::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Video device name: " <<  this->DevName << endl;

    if( this->Initialized )
    {}
    else
    {
        os << indent<< indent << "Video source NOT initialized" << endl;
    }
}


//----------------------------------------------------------------------------
bool vtkV4L2VideoSource2::Initialize()
{
    if( this->Initialized )
    {
        this->Cleanup();
    }

    if( this->VideoDev == -1 )
        this->OpenDevice();

    if( this->VideoDev == -1 )
        return false;

    if( !this->QueryDeviceCap() )
        return false;

    if( !this->ListAvailableInput() )
        return false;

    if( !this->AssignVideoInput() )
        return false;

    this->ResetCroping();

    if( !this->ListAvailableStandard() )
        return false;

    if( !this->AssignVideoStandard() )
        return false;

    ListAvailableFormats();

    if( !this->SetPixelFormat() )
        return false;

    if( !this->CreateMMapedBuffers() )
        return false;

    // update frame buffer again to reflect any changes
    this->Buffer->Initialize();

    this->Initialized = 1;

    return true;
}

void vtkV4L2VideoSource2::Cleanup()
{
    this->ReleaseSystemResources();
    this->ClearAvailableInputs();
    this->ClearAvailableStandards();
}

//----------------------------------------------------------------------------
void vtkV4L2VideoSource2::ReleaseSystemResources()
{
    if( this->Recording )
    {
        this->Stop();
    }

    this->UnmapMemory();

    this->CloseDevice();

    this->Initialized = 0;
}
//----------------------------------------------------------------------------
void vtkV4L2VideoSource2::InternalGrab()
{
    // Wait for next frame to be ready
    int ready = 0;
    while( !ready )
    {
        // create the set of file descriptors to watch (only one, the video device )
        fd_set fds;
        FD_ZERO( &fds );
        FD_SET( this->VideoDev, &fds );

        // Timeout. Maximum time to wait for a
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 66667;  // if we have to wait more that 1/15th of a second, there is a problem.

        // Wait for a change on the device
        int r = select( this->VideoDev + 1, &fds, NULL, NULL, &tv );

        // Manage result
        if( r == -1 )
        {
            if ( errno != EINTR )
            {
                vtkErrorMacro(<< "Improper call to function select() while waiting for new frame" << endl );
                return;
            }
        }
        else if( r == 0 )
        {
            vtkWarningMacro(<< "Timer waiting for new frame timed out ( .25 seconds)" << endl );
            return;
        }
        else
        {
            ready = 1;
        }
     }

    // Dequeue the buffer containing the new frame
    v4l2_buffer buf;
    CLEAR (buf);
    buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory  = V4L2_MEMORY_MMAP;

    if (-1 == xioctl ( this->VideoDev, VIDIOC_DQBUF, &buf) )
    {
        switch (errno)
        {
        case EAGAIN:
            return;
            // TODO: process those errors correctly
        case EIO:
            // Could ignore EIO, see spec.
            break;
        default:
            vtkErrorMacro(<<"Error while dequeuing a grabbed buffer (VIDIOC_DQBUF) errno="<<errno);
            break;
        }
    }
    
    // Determine the timestamp for the current frame. 
    // this is the way vtkTimerLog::GetUniversalTime generated a double timestamp
    // using gettimeofday system function. This function is the one that is supposed
    // to be used by the video capture drivers to get the timeval for the beginning of 
    // the frame. So we convert the v4l2_buffer's timestamp using the same way vtkTimerLog
    // does.
    // On Ubuntu 14.04 we now use vtkTimerLog::GetUniversalTime
    
    this->Buffer->SetCurrentFrameData( this->Buffers[buf.index].start, vtkTimerLog::GetUniversalTime() );

    // TODO: process this call possible errors
    xioctl (this->VideoDev, VIDIOC_QBUF, &buf);
}


//----------------------------------------------------------------------------
// Circulate the buffer and grab a frame.
void vtkV4L2VideoSource2::Grab()
{
    if( !this->Initialized )
    {
        vtkErrorMacro( << "Need to call Initialize before Grab." );
        return;
    }

    if (!this->Recording)
    {
        this->StartCapturing( 1 );
        this->InternalGrab();
        this->StopCapturing();
    }
}


//----------------------------------------------------------------------------
// this function runs in an alternate thread to asyncronously grab frames
static void * vtkV4L2VideoSource2RecordThread( vtkMultiThreader::ThreadInfo * data )
{
    vtkV4L2VideoSource2 * self = (vtkV4L2VideoSource2 *)(data->UserData);

    self->StartCapturing( 0 );

    while( self->GetRecording() )
    {
        self->InternalGrab();
    }

    self->StopCapturing();

    return NULL;
}


//----------------------------------------------------------------------------
void vtkV4L2VideoSource2::Record()
{
    if( !this->Initialized )
    {
        vtkErrorMacro( << "Need to call Initialize before Record." );
        return;
    }

    if( !this->Recording )
    {
        this->Recording = 1;
        this->Modified();
        this->PlayerThreadId = this->PlayerThreader->SpawnThread( (vtkThreadFunctionType)&vtkV4L2VideoSource2RecordThread, this );
    }
}


//----------------------------------------------------------------------------
void vtkV4L2VideoSource2::Stop()
{
    if (!this->Recording)
    {
        return;
    }

    // stop recording and wait for player thread to terminate.
    this->Recording = 0;
    this->PlayerThreader->TerminateThread( this->PlayerThreadId );

    this->Modified();
}


//----------------------------------------------------------------------------
/*void vtkV4L2VideoSource2::SetFrameSize(int x, int y, int z)
{
    if (x < 1 || y < 1 || z != 1)
    {
        vtkErrorMacro(<< "SetFrameSize: Illegal frame size");
        return;
    }

    // better stop recording before anything else -- We can't stop inside the framebuffer mutex lock,
    // otherwise, we might wait forever for the player thread to stop.
    int recording = this->GetRecording();
    if( recording )
    {
        this->Stop();
    }

    this->FrameSize[0] = x;
    this->FrameSize[1] = y;
    this->Buffer->SetFrameSize( x, y, z );

    if( this->Initialized )
        this->Initialize();

    if( recording )
        this->Record();

    this->Modified();
}*/

static const std::string fps15( "15 fps" );
static const std::string fps30( "30 fps" );

// simtodo : rates are hardcoded, do something smarter.
void vtkV4L2VideoSource2::ListAvailableFrameRates( std::vector< std::string > & rates )
{
    //rates.push_back( fps15 );
    rates.push_back( fps30 );
}

void vtkV4L2VideoSource2::SetFrameRateString( const char * frameRate  )
{
    std::string fr( frameRate );
    if( fr == fps15 )
        SetFrameRate( 15.0 );
    else if( fr == fps30 )
        SetFrameRate( 30.0 );
}

const char * vtkV4L2VideoSource2::GetFrameRateString()
{
    if( this->Buffer->GetFrameRate() == 15.0 )
        return fps15.c_str();
    else if( this->Buffer->GetFrameRate() == 30.0 )
        return fps30.c_str();
    return 0;
}

static const std::string res320("320x240");
static const std::string res640("640x480");
static const std::string res720("720x480");

void vtkV4L2VideoSource2::ListAvailableResolutions( std::vector< std::string > & res )
{
    res.push_back( res320 );
    res.push_back( res640 );
    res.push_back( res720 );
}

void vtkV4L2VideoSource2::SetResolutionString( const char * res  )
{
    std::string r( res );
    if( res == res320 )
        SetFrameSize( 320, 240 );
    else if( res == res640 )
        SetFrameSize( 640, 480 );
    else if( res == res720 )
        SetFrameSize( 720, 480 );
}

const char * vtkV4L2VideoSource2::GetResolutionString()
{
    if( this->FrameSize[0] == 320 )
        return res320.c_str();
    else if( this->FrameSize[0] == 640 )
        return res640.c_str();
    else if( this->FrameSize[0] == 720 )
        return res720.c_str();
    return 0;
}

//----------------------------------------------------------------------------
void vtkV4L2VideoSource2::SetOutputFormat(int format)
{
    if( format != VTK_RGB && format != VTK_LUMINANCE )
    {
        vtkErrorMacro(<< "SetOutputFormat: Unrecognized color format or not supported by this class.");
        return;
    }

    vtkVideoSource2::SetOutputFormat( format );
}

#include <vtkGlobFileNames.h>
#include <vtkStringArray.h>

void vtkV4L2VideoSource2::ListAvailableVideoDevice( std::vector< std::string > & devs )
{
    vtkGlobFileNames * filenameSearch = vtkGlobFileNames::New();
    filenameSearch->AddFileNames( "/dev/video*" );
    vtkStringArray * fileNames = filenameSearch->GetFileNames();
    for( int i = 0; i < fileNames->GetNumberOfValues(); ++i )
        devs.push_back( fileNames->GetValue( i ) );
}

void vtkV4L2VideoSource2::SetVideoDeviceName( const char * name )
{
    int rec = this->GetRecording();
    this->DevName = name;
    if( this->Initialized ) // reinint if init already done
        this->Initialize();
    if( rec )
    {
        this->Record();
    }
}

void vtkV4L2VideoSource2::SetVideoInput( const char * inputName )
{
    int rec = this->GetRecording();
    this->VideoInputName = inputName;
    if( this->Initialized )
        this->Initialize();
    if( rec )
    {
        this->Record();
    }
}

const char * vtkV4L2VideoSource2::GetVideoInput()
{
    return this->VideoInputName.c_str();
}

void vtkV4L2VideoSource2::ListAvailableVideoInput( std::vector< std::string > & inputs )
{
    for( unsigned i = 0; i < this->AvailableInputs.size(); ++i )
    {
        inputs.push_back( std::string( this->AvailableInputs[i]->GetName() ) );
    }
}

void vtkV4L2VideoSource2::SetVideoStandard( const char * standardName )
{
    int rec = this->GetRecording();
    this->VideoStandard = standardName;
    if( this->Initialized )
        this->Initialize();
    if( rec )
    {
        this->Record();
    }
}

void vtkV4L2VideoSource2::ListAvailableVideoStandards( std::vector< std::string > & standards )
{
    standards.push_back( std::string( "NTSC" ) );
    standards.push_back( std::string( "PAL" ) );
    standards.push_back( std::string( "SECAM" ) );
}

int vtkV4L2VideoSource2::xioctl( int fd, int request, void * arg )
{
    int r;

    do
        r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}


void vtkV4L2VideoSource2::StopCapturing()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl ( this->VideoDev, VIDIOC_STREAMOFF, &type ) )
    {
        vtkErrorMacro(<< "Problem occured while stoping streaming (VIDIOC_STREAMOFF)" << endl );
    }
}


void vtkV4L2VideoSource2::StartCapturing( int singleBuffer )
{
    // Enqueue buffer in the driver for streaming
    int nbBuffers = singleBuffer ? 1 : this->NbBuffers;
    for ( int i = 0; i < nbBuffers; ++i )
    {
        v4l2_buffer buf;
        CLEAR (buf);
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        if (-1 == xioctl ( this->VideoDev, VIDIOC_QBUF, &buf ) )
        {
            vtkErrorMacro(<< "Problem occured while queuing video buffer for streaming (VIDIOC_QBUF)" << endl );
            return;
        }
    }
    
    // try to set the frame rate if the device supports it
    v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl ( this->VideoDev, VIDIOC_G_PARM, &parm ) )
    {
        vtkErrorMacro(<< "Problem occured while getting streaming parameters (VIDIOC_G_PARM)" << endl );
        return;
    }
    if( parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME )
    {
    
        double timePerFrame = parm.parm.capture.timeperframe.numerator / (double)(parm.parm.capture.timeperframe.denominator);
        double wantedTimePerFrame = 1 / this->GetFrameRate();
        if( wantedTimePerFrame > timePerFrame )
        {
            parm.parm.capture.timeperframe.numerator = 1;
            parm.parm.capture.timeperframe.denominator = (int)this->GetFrameRate();
            if (-1 == xioctl ( this->VideoDev, VIDIOC_S_PARM, &parm ) )
            {
                vtkErrorMacro(<< "Problem occured while setting streaming parameters (VIDIOC_S_PARM)" << endl );
                return;
            }
            if (-1 == xioctl ( this->VideoDev, VIDIOC_G_PARM, &parm ) )
            {
                vtkErrorMacro(<< "Problem occured while getting streaming parameters (VIDIOC_G_PARM)" << endl );
                return;
            }
            double newFrameRate = parm.parm.capture.timeperframe.denominator / (double)(parm.parm.capture.timeperframe.numerator);
            this->SetFrameRate( newFrameRate );
        }
    }
    
    // Start streaming
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl ( this->VideoDev, VIDIOC_STREAMON, &type))
    {
        vtkErrorMacro(<< "Problem occured while starting streaming (VIDIOC_STREAMON)" << endl );
        return;
    }
}


void vtkV4L2VideoSource2::InternalSetBuffer()
{
    this->Buffer->SetFlipFrames( 1 );

    vtkV4L2Unpacker * unpacker = vtkV4L2Unpacker::New();
    this->Buffer->SetUnpacker( unpacker );
    unpacker->Delete();
}


void vtkV4L2VideoSource2::OpenDevice()
{
    //==================================================================
    // Try to open the requested video device
    //==================================================================
    struct stat st;
    if ( stat ( this->DevName.c_str(), &st) == -1 )
    {
        vtkErrorMacro(<< "Cannot identify " << this->DevName << " : " <<  errno << " , " << strerror (errno) << endl );
        return;
    }

    if ( !S_ISCHR( st.st_mode ) )
    {
        vtkErrorMacro(<< this->DevName << " is not a character device." << endl );
        return;
    }

    this->VideoDev = open( this->DevName.c_str(), O_RDWR | O_NONBLOCK, 0 );

    if ( this->VideoDev == -1 )
    {
        vtkErrorMacro(<< "Cannot open " << this->DevName << " : " << errno << " , " << strerror (errno) << endl );
    }
}


int vtkV4L2VideoSource2::QueryDeviceCap()
{
    //==================================================================
    // Request video Driver capabilities
    //==================================================================
    v4l2_capability cap;

    if (-1 == xioctl( this->VideoDev, VIDIOC_QUERYCAP, &cap ))
    {
        if (EINVAL == errno)
        {
            vtkErrorMacro(<< this->DevName << " is no V4L2 device." << endl );
            return 0;
        }
        else
        {
            vtkErrorMacro(<< "VIDIOC_QUERYCAP error" << endl );
            return 0;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        vtkErrorMacro(<< this->DevName << " is no video capture device" << endl );
        return 0;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        vtkErrorMacro(<< this->DevName << " does not support streaming I/O" << endl );
        return 0;
    }

    return 1;
}


int vtkV4L2VideoSource2::ListAvailableInput()
{
    this->ClearAvailableInputs();

    v4l2_input input;
    CLEAR( input );
    int finished = 0;
    int index = 0;
    while( !finished )
    {
        if (-1 == xioctl( this->VideoDev, VIDIOC_ENUMINPUT, &input) )
        {
            if (EINVAL != errno)
            {
                cerr <<  "coudln't get input " << input.index << " properties." << endl;
                return 0;
            }
            else
            {
                finished = 1;
            }
        }
        else
        {
            std::ostringstream description;
            description << "Type: " << endl;
            if( input.type == V4L2_INPUT_TYPE_TUNER )
                description << "V4L2_INPUT_TYPE_TUNER";
            else
                description << "V4L2_INPUT_TYPE_CAMERA";
            description << endl;

            description << "Standard: " << endl;
            switch( input.std )
            {
                case V4L2_STD_PAL_B:
                    description << "V4L2_STD_PAL_B";
                    break;
                case V4L2_STD_PAL_B1:
                    description << "V4L2_STD_PAL_B1";
                    break;
                case V4L2_STD_PAL_G:
                    description << "V4L2_STD_PAL_G";
                    break;
                case V4L2_STD_PAL_H:
                    description << "V4L2_STD_PAL_H";
                    break;
                case V4L2_STD_PAL_I:
                    description << "V4L2_STD_PAL_I";
                    break;
                case V4L2_STD_PAL_D:
                    description << "V4L2_STD_PAL_D";
                    break;
                case V4L2_STD_PAL_D1:
                    description << "V4L2_STD_PAL_D1";
                    break;
                case V4L2_STD_PAL_K:
                    description << "V4L2_STD_PAL_K";
                    break;
                case V4L2_STD_PAL_M:
                    description << "V4L2_STD_PAL_M";
                    break;
                case V4L2_STD_PAL_N:
                    description << "V4L2_STD_PAL_N";
                    break;
                case V4L2_STD_PAL_Nc:
                    description << "V4L2_STD_PAL_Nc";
                    break;
                case V4L2_STD_PAL_60:
                    description << "V4L2_STD_PAL_60";
                    break;
                case V4L2_STD_NTSC_M:
                    description << "V4L2_STD_NTSC_M";
                    break;
                case V4L2_STD_NTSC_M_JP:
                    description << "V4L2_STD_NTSC_M_JP";
                    break;
                case V4L2_STD_SECAM_B:
                    description << "V4L2_STD_SECAM_B";
                    break;
                case V4L2_STD_SECAM_D:
                    description << "V4L2_STD_SECAM_D";
                    break;
                case V4L2_STD_SECAM_G:
                    description << "V4L2_STD_SECAM_G";
                    break;
                case V4L2_STD_SECAM_H:
                    description << "V4L2_STD_SECAM_H";
                    break;
                case V4L2_STD_SECAM_K:
                    description << "V4L2_STD_SECAM_K";
                    break;
                case V4L2_STD_SECAM_K1:
                    description << "V4L2_STD_SECAM_K1";
                    break;
                case V4L2_STD_SECAM_L:
                    description << "V4L2_STD_SECAM_L";
                    break;
                case V4L2_STD_ATSC_8_VSB:
                    description << "V4L2_STD_ATSC_8_VSB";
                    break;
                case V4L2_STD_ATSC_16_VSB:
                    description << "V4L2_STD_ATSC_16_VSB";
                    break;
                case V4L2_STD_PAL_BG:
                    description << "V4L2_STD_PAL_BG";
                    break;
                case V4L2_STD_PAL_DK:
                    description << "V4L2_STD_PAL_DK";
                    break;
                case V4L2_STD_PAL:
                    description << "V4L2_STD_PAL";
                    break;
                case V4L2_STD_NTSC:
                    description << "V4L2_STD_NTSC";
                    break;
                case V4L2_STD_SECAM:
                    description << "V4L2_STD_SECAM";
                    break;
                case V4L2_STD_525_60:
                    description << "V4L2_STD_525_60";
                    break;
                case V4L2_STD_625_50:
                    description << "V4L2_STD_625_50";
                    break;
                case V4L2_STD_UNKNOWN:
                    description << "V4L2_STD_UNKNOWN";
                    break;
                case V4L2_STD_ALL:
                    description << "V4L2_STD_ALL";
                    break;
            }

            description << ends;

            if( input.type != V4L2_INPUT_TYPE_TUNER )
            {
                vtkV4L2VideoInput * inputDesc = vtkV4L2VideoInput::New();
                inputDesc->SetName( (char*)(input.name) );
                inputDesc->SetDescription( description.str().data() );
                inputDesc->SetIndex( index );
                this->AvailableInputs.push_back( inputDesc );
            }

            index++;
            CLEAR( input );
            input.index = index;
        }
    }

    return 1;
}


int vtkV4L2VideoSource2::AssignVideoInput()
{
    //==================================================================
    // Set the video input to use
    //==================================================================
    if( this->AvailableInputs.size() < 1 )
    {
        vtkErrorMacro(<< "No input available!" << endl );
        return 0;
    }

    // Find index that matches the input name, if there is one
    int index = -1;
    for( unsigned i = 0; i < this->AvailableInputs.size(); ++i )
    {
        if( this->AvailableInputs[i]->GetName() == this->VideoInputName )
            index = i;
    }

    // if not valid, get the first one
    if( index == -1 )
    {
        index = 0;
        this->VideoInputName = this->AvailableInputs[0]->GetName();
    }

    v4l2_input input;
    CLEAR( input );
    input.index = this->AvailableInputs[ index ]->GetIndex();
    int res = xioctl (this->VideoDev, VIDIOC_ENUMINPUT, &input);
    if( res == -1 )
    {
        vtkErrorMacro(<< "Coudln't get input " << this->VideoInputName << " properties." << endl );
        return 0;
    }
    else
    {
        if( input.type != V4L2_INPUT_TYPE_CAMERA )
        {
            vtkErrorMacro(<< "Only V4L2_INPUT_TYPE_CAMERA video input is supported by this class."<< endl );
            return 0;
        }
    }

    // Set requested input now that we know it is supported
    res = xioctl( this->VideoDev, VIDIOC_S_INPUT, &(input.index) );
    if( res == -1 )
    {
        if( EINVAL != errno )
        {
            vtkErrorMacro(<< "coudln't set current input to " << this->VideoInputName << endl );
            return 0;
        }
    }

    return 1;
}


int vtkV4L2VideoSource2::ListAvailableStandard()
{
    v4l2_standard vidStandard;
    CLEAR(vidStandard);
    while( 0 == xioctl ( this->VideoDev, VIDIOC_ENUMSTD, &vidStandard ) )
    {
        std::stringstream description;
        description << "standard: ";
        if( vidStandard.id == V4L2_STD_PAL_B )
            description << "V4L2_STD_PAL_B";
        if( vidStandard.id == V4L2_STD_PAL_B1 )
            description << "V4L2_STD_PAL_B1";
        if( vidStandard.id == V4L2_STD_PAL_G )
            description << "V4L2_STD_PAL_G";
        if( vidStandard.id == V4L2_STD_PAL_H )
            description << "V4L2_STD_PAL_H";
        if( vidStandard.id == V4L2_STD_PAL_I )
            description << "V4L2_STD_PAL_I";
        if( vidStandard.id == V4L2_STD_PAL_D )
            description << "V4L2_STD_PAL_D";
        if( vidStandard.id == V4L2_STD_PAL_D1 )
            description << "V4L2_STD_PAL_D1";
        if( vidStandard.id == V4L2_STD_PAL_K )
            description << "V4L2_STD_PAL_K";
        if( vidStandard.id == V4L2_STD_PAL_M )
            description << "V4L2_STD_PAL_M";
        if( vidStandard.id == V4L2_STD_PAL_N )
            description << "V4L2_STD_PAL_N";
        if( vidStandard.id == V4L2_STD_PAL_Nc )
            description << "V4L2_STD_PAL_Nc";
        if( vidStandard.id == V4L2_STD_PAL_60 )
            description << "V4L2_STD_PAL_60";
        if( vidStandard.id == V4L2_STD_NTSC_M )
            description << "V4L2_STD_NTSC_M";
        if( vidStandard.id == V4L2_STD_NTSC_M_JP )
            description << "V4L2_STD_NTSC_M_JP";
        if( vidStandard.id == V4L2_STD_SECAM_B )
            description << "V4L2_STD_SECAM_B";
        if( vidStandard.id == V4L2_STD_SECAM_G )
            description << "V4L2_STD_SECAM_G";
        if( vidStandard.id == V4L2_STD_SECAM_H )
            description << "V4L2_STD_SECAM_H";
        if( vidStandard.id == V4L2_STD_SECAM_K )
            description << "V4L2_STD_SECAM_K";
        if( vidStandard.id == V4L2_STD_SECAM_K1 )
            description << "V4L2_STD_SECAM_K1";
        if( vidStandard.id == V4L2_STD_SECAM_L )
            description << "V4L2_STD_SECAM_L";
        if( vidStandard.id == V4L2_STD_ATSC_8_VSB )
            description << "V4L2_STD_ATSC_8_VSB";
        if( vidStandard.id == V4L2_STD_ATSC_16_VSB )
            description << "V4L2_STD_ATSC_16_VSB";
        if( vidStandard.id == V4L2_STD_PAL_BG )
            description << "V4L2_STD_PAL_BG";
        if( vidStandard.id == V4L2_STD_PAL_DK )
            description << "V4L2_STD_PAL_DK";
        if( vidStandard.id == V4L2_STD_PAL )
            description << "V4L2_STD_PAL";
        if( vidStandard.id == V4L2_STD_NTSC )
            description << "V4L2_STD_NTSC";
        if( vidStandard.id == V4L2_STD_525_60 )
            description << "V4L2_STD_525_60";
        if( vidStandard.id == V4L2_STD_625_50 )
            description << "V4L2_STD_625_50";
        if( vidStandard.id == V4L2_STD_UNKNOWN )
            description << "V4L2_STD_UNKNOWN";
        if( vidStandard.id == V4L2_STD_ALL )
            description << "V4L2_STD_ALL";
        description << endl;
        description << "frame period: " << vidStandard.frameperiod.numerator << " / " << vidStandard.frameperiod.denominator  << " sec" << endl;
        description << "framelines: " << vidStandard.framelines << ends;

        vtkV4L2VideoStandard * newStd = vtkV4L2VideoStandard::New();
        newStd->SetName( (char*)(vidStandard.name) );
        newStd->SetDescription( description.str().data() );
        newStd->Standard = vidStandard.id;

        this->AvailableStandards.push_back( newStd );

        vidStandard.index++;
    }

    if( errno != EINVAL )
    {
        vtkErrorMacro(<<  "Error Getting standard " << vidStandard.index << " properties." << endl );
        return 0;
    }

    return 1;
}

int vtkV4L2VideoSource2::FirstStandardThatMatches( v4l2_std_id stdId )
{
    for( unsigned i = 0; i < this->AvailableStandards.size(); ++i )
    {
        v4l2_std_id id = this->AvailableStandards[i]->Standard;
        if( id & stdId )
            return i;
    }
    return -1;
}

int vtkV4L2VideoSource2::AssignVideoStandard()
{
    int index = 0;
    if( this->VideoStandard == std::string("NTSC") )
        index = FirstStandardThatMatches( V4L2_STD_NTSC );
    else if( this->VideoStandard == std::string( "PAL" ) )
        index = FirstStandardThatMatches( V4L2_STD_PAL );
    else if( this->VideoStandard == std::string("SECAM") )
        index = FirstStandardThatMatches( V4L2_STD_SECAM );
    if( index == -1 )
    {
        vtkWarningMacro(<< "No valid video standard was found. Will take first standard provided by the v4l2 driver." << endl );
        index = 0;
    }

    // Assign the chosen standard
    int res = -1;
    if ( this->AvailableStandards.size() > 0 )
        res = xioctl (this->VideoDev, VIDIOC_S_STD, &(this->AvailableStandards[ index ]->Standard ) );
    if( res == -1 )
    {
        vtkErrorMacro(<< "Coudln't set video standard properties." << endl );
        return 0;
    }

    return 1;
}

// simtodo : do something with this information. For now, we only support a
// limited set of formats that are hardcoded and depend on the buffer type.
void vtkV4L2VideoSource2::ListAvailableFormats()
{
    v4l2_fmtdesc vidFormat;

    // Get video capture formats
    //std::cout << "Video capture formats:" << std::endl;
    int i = 0;
    int res = 0;
    while( res == 0 )
    {
        CLEAR( vidFormat );
        vidFormat.index = i;
        vidFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        res = xioctl( this->VideoDev, VIDIOC_ENUM_FMT, &vidFormat );
        //std::cout << ((const char*)vidFormat.description) << std::endl;
        ++i;
    }
}


void vtkV4L2VideoSource2::ResetCroping()
{
    //==================================================================
    // Reset cropping to default. This class doesn't care about cropping
    // for now, so we don't really care if there is an error here, we
    // just want to reset the cropping feature if one is available.
    //==================================================================
    v4l2_cropcap cropcap;
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl ( this->VideoDev, VIDIOC_CROPCAP, &cropcap) )
    {
        vtkErrorMacro(<< "Error getting crop caps (VIDIOC_CROPCAP)" << endl);
        return;
    }
    v4l2_crop crop;
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;
    if (-1 == xioctl( this->VideoDev, VIDIOC_S_CROP, &crop ) )
    {
        vtkErrorMacro(<< "Error setting crop (VIDIOC_S_CROP)" << endl);
        return;
    }
}


int vtkV4L2VideoSource2::SetPixelFormat()
{
    //==================================================================
    // Get the current format
    //==================================================================
    v4l2_format fmt;
    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int res = xioctl( this->VideoDev, VIDIOC_G_FMT, &fmt );
    if( res == -1 )
    {
        vtkErrorMacro(<< "Error getting default video format (VIDIOC_G_FMT)" << endl);
        return 0;
    }

    //==================================================================
    // Try to get the good pixel format
    //==================================================================
    if( this->Buffer->GetNumberOfScalarComponents() == 1 )
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    else if( this->Buffer->GetNumberOfScalarComponents() == 3 )
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    else if( this->Buffer->GetNumberOfScalarComponents() == 4 )
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;

    //==================================================================
    // Try to set image dimension
    //==================================================================
    fmt.fmt.pix.width = this->FrameSize[0];
    fmt.fmt.pix.height = this->FrameSize[1];

    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    // Do a set format anyways just to make sure
    res = xioctl( this->VideoDev, VIDIOC_S_FMT, &fmt );
    if( res == -1 )
    {
        vtkErrorMacro(<< "Error setting the pixel format." << endl);
        return 0;
    }

    return 1;
}


int vtkV4L2VideoSource2::CreateMMapedBuffers()
{
    //==================================================================
    // Allocate device buffer
    //==================================================================
    v4l2_requestbuffers req;
    CLEAR (req);
    req.count     = this->NbBuffers;
    req.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory    = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (this->VideoDev, VIDIOC_REQBUFS, &req) )
    {
        if (EINVAL == errno)
        {
            vtkErrorMacro(<< this->DevName << " does not support memory mapping" << endl);
        }
        else
        {
            vtkErrorMacro(<< "VIDIOC_REQBUFS error.");
        }
        return 0;
    }

    if( req.count < 2 )
    {
        vtkErrorMacro(<< "Insufficient buffer memory on " << this->DevName << endl );
        return 0;
    }


    // Request properties for every allocated buffer and map it to memory
    this->NbBuffers = req.count;
    this->Buffers = new buffer[this->NbBuffers];

    for ( int i = 0; i < this->NbBuffers; ++i )
    {
        CLEAR( this->Buffers[i] );

        // Query buffer length
        v4l2_buffer buf;
        CLEAR (buf);
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if( xioctl ( this->VideoDev, VIDIOC_QUERYBUF, &buf) == -1 )
        {
            vtkErrorMacro(<<"VIDIOC_QUERYBUF error" << endl);
            return 0;
        }

        // Queue the buffer for capturing
        this->Buffers[i].length = buf.length;
        this->Buffers[i].start  = mmap( NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->VideoDev, buf.m.offset );

        if( MAP_FAILED == this->Buffers[i].start )
        {
            vtkErrorMacro(<< "Failed to map " << this->DevName << " to memory" << endl );
            return 0;
        }
    }

    return 1;
}

void vtkV4L2VideoSource2::ClearAvailableInputs()
{
    std::vector< vtkV4L2VideoInput* >::iterator it = this->AvailableInputs.begin();
    std::vector< vtkV4L2VideoInput* >::iterator itEnd = this->AvailableInputs.end();
    while( it != itEnd )
    {
        (*it)->Delete();
        ++it;
    }
    this->AvailableInputs.clear();
}


void vtkV4L2VideoSource2::ClearAvailableStandards()
{
    std::vector< vtkV4L2VideoStandard* >::iterator it = this->AvailableStandards.begin();
    std::vector< vtkV4L2VideoStandard* >::iterator itEnd = this->AvailableStandards.end();
    while( it != itEnd )
    {
        (*it)->Delete();
        ++it;
    }
    this->AvailableStandards.clear();
}


int vtkV4L2VideoSource2::UnmapMemory()
{
    if( this->Buffers )
    {
        for ( int i = 0; i < this->NbBuffers; ++i )
        {
            munmap( this->Buffers[i].start, this->Buffers[i].length ) ;
        }
        delete [] this->Buffers;
        this->Buffers = NULL;
    }
    return 1;
}


int vtkV4L2VideoSource2::CloseDevice()
{
    // close video device
    if( this->VideoDev != -1 )
    {
        close( this->VideoDev );
        this->VideoDev = -1;
    }
    return 1;
}

