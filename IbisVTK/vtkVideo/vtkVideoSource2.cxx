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

#include "vtkVideoSource2.h"
#include "vtkVideoBuffer.h"
#include "vtkCriticalSection.h"
#include "vtkMutexLock.h"
#include "vtkImageData.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

#include <ctype.h>
#include <time.h>


vtkStandardNewMacro(vtkVideoSource2);

#if ( _MSC_VER >= 1300 ) // Visual studio .NET
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4312 )
#endif

const char * vtkVideoSource2::UnimplementedString = "Feature not implemented";

//----------------------------------------------------------------------------
vtkVideoSource2::vtkVideoSource2()
{
    this->FrameSize[0] = 320;
    this->FrameSize[1] = 240;
    this->FrameRate = 30.0;
    this->Initialized = 0;
    this->Recording = 0;
    this->PlayerThreader = vtkMultiThreader::New();
    this->PlayerThreadId = -1;
    this->Buffer = vtkVideoBuffer::New();
}

//----------------------------------------------------------------------------
vtkVideoSource2::~vtkVideoSource2()
{
    if( this->Recording )
    {
        this->Stop();
    }

    this->PlayerThreader->Delete();
    this->Buffer->Delete();
}

//----------------------------------------------------------------------------
void vtkVideoSource2::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);
    os << indent << "Buffer: " << endl;
    this->Buffer->PrintSelf( os, indent );
    os << indent << "Recording: " << (this->Recording ? "On\n" : "Off\n");
}


//----------------------------------------------------------------------------
// Initialize() should be overridden to initialize the hardware frame grabber
bool vtkVideoSource2::Initialize()
{
    if (this->Initialized)
    {
        return true;
    }
    this->Initialized = 1;

    this->Buffer->Initialize();

    return true;
}

//----------------------------------------------------------------------------
// ReleaseSystemResources() should be overridden to release the hardware
void vtkVideoSource2::ReleaseSystemResources()
{
    if( this->Recording )
    {
        this->Stop();
    }

    this->Initialized = 0;
}

//----------------------------------------------------------------------------
// Copy pseudo-random noise into the frames.  This function may be called
// asynchronously.
void vtkVideoSource2::InternalGrab()
{
    double timestamp = vtkTimerLog::GetUniversalTime();
    this->Buffer->FillCurrentFrameWithNoise( timestamp );
}

//----------------------------------------------------------------------------
// platform-independent sleep function
static inline void vtkSleep(double duration)
{
    duration = duration; // avoid warnings
    // sleep according to OS preference
#ifdef _WIN32

    Sleep((int)(1000*duration));
#elif defined(__FreeBSD__) || defined(__linux__) || defined(sgi)

    struct timespec sleep_time, dummy;
    sleep_time.tv_sec = (int)duration;
    sleep_time.tv_nsec = (int)(1000000000*(duration-sleep_time.tv_sec));
    nanosleep(&sleep_time,&dummy);
#endif
}

//----------------------------------------------------------------------------
// Sleep until the specified absolute time has arrived.
// You must pass a handle to the current thread.
// If '0' is returned, then the thread was aborted before or during the wait.
static int vtkThreadSleep(vtkMultiThreader::ThreadInfo *data, double time)
{
    for (int i = 0;; i++)
    {
        double remaining = time - vtkTimerLog::GetUniversalTime();

        // check to see if we have reached the specified time
        if (remaining <= 0)
        {
            if (i == 0)
            {
                vtkGenericWarningMacro("Dropped a video frame.");
            }
            return 1;
        }
        // check the ActiveFlag at least every 0.1 seconds
        if (remaining > 0.1)
        {
            remaining = 0.1;
        }

        // check to see if we are being told to quit
        data->ActiveFlagLock->Lock();
        int activeFlag = *(data->ActiveFlag);
        data->ActiveFlagLock->Unlock();

        if (activeFlag == 0)
        {
            return 0;
        }

        vtkSleep(remaining);
    }
}

//----------------------------------------------------------------------------
// this function runs in an alternate thread to asyncronously grab frames
static void *vtkVideoSource2RecordThread(vtkMultiThreader::ThreadInfo *data)
{
    vtkVideoSource2 *self = (vtkVideoSource2 *)(data->UserData);

    double startTime = vtkTimerLog::GetUniversalTime();
    double rate = self->GetFrameRate();
    int frame = 0;

    do
    {
        self->InternalGrab();
        frame++;
    }
    while (vtkThreadSleep(data, startTime + frame/rate));

    return NULL;
}

//----------------------------------------------------------------------------
// Set the source to grab frames continuously.
// You should override this as appropriate for your device.
void vtkVideoSource2::Record()
{
    if (this->Buffer->GetPlaying())
    {
        this->Buffer->Stop();
    }

    if (!this->Recording)
    {
        this->Initialize();

        this->Recording = 1;
        this->Modified();
        this->PlayerThreadId = this->PlayerThreader->SpawnThread((vtkThreadFunctionType)&vtkVideoSource2RecordThread,this);
    }
}


//----------------------------------------------------------------------------
// Stop continuous grabbing or playback.  You will have to override this
// if your class overrides Play() and Record()
void vtkVideoSource2::Stop()
{
    if( this->Recording )
    {
        this->PlayerThreader->TerminateThread(this->PlayerThreadId);
        this->PlayerThreadId = -1;
        this->Recording = 0;
        this->Modified();
    }
}


//----------------------------------------------------------------------------
// The grab function, which should (of course) be overridden to do
// the appropriate hardware stuff.  This function should never be
// called asynchronously.
void vtkVideoSource2::Grab()
{
    // ensure that the hardware is initialized.
    this->Initialize();

    this->InternalGrab();
}

void vtkVideoSource2::SetOutputFormat(int format)
{
    if( format == this->OutputFormat )
        return;
    this->OutputFormat = format;

    // better stop recording before anything else -- We can't stop inside the framebuffer mutex lock,
    // otherwise, we might wait forever for the player thread to stop.
    int recording = this->GetRecording();
    if( recording )
    {
        this->Stop();
    }

    this->Buffer->SetOutputFormat( format );

    if( this->Initialized )
        this->Initialize();

    if( recording )
        this->Record();

    this->Modified();
}

void vtkVideoSource2::SetBuffer( vtkVideoBuffer * buffer )
{
    if( buffer == this->Buffer )
    {
        return;
    }
    
    if( this->Buffer )
    {
        this->Buffer->UnRegister( this );
    }
    
    this->Buffer = buffer;
    
    if( this->Buffer )
    {
        this->Buffer->Register( this );
        if( !this->IsBufferCompatible( this->Buffer ) )
        {
            this->Buffer->SetFrameSize( this->FrameSize[0], this->FrameSize[1], 1 );
            this->Buffer->SetOutputFormat( this->OutputFormat );
            this->Buffer->Initialize();
        }
        this->InternalSetBuffer();
    }
}

bool vtkVideoSource2::IsBufferCompatible( vtkVideoBuffer * buf )
{
    // check if we have the same frame sizes
    int frameSize[3];
    buf->GetFrameSize( frameSize );
    if( frameSize[0] != this->FrameSize[0] || frameSize[1] != this->FrameSize[1] )
        return false;

    // check if we have the same output format
    if( this->OutputFormat != buf->GetOutputFormat() )
        return false;

    return true;
}

void vtkVideoSource2::SetFrameRate( double rate )
{
    this->FrameRate = rate;
    this->Buffer->SetFrameRate( rate );
}

void vtkVideoSource2::SetFrameSize( int x, int y )
{
    // better stop recording before anything else -- We can't stop inside the framebuffer mutex lock,
    // otherwise, we might wait forever for the player thread to stop.
    int recording = this->GetRecording();
    if( recording )
    {
        this->Stop();
    }

    this->FrameSize[0] = x;
    this->FrameSize[1] = y;
    this->Buffer->SetFrameSize( x, y, 1 );

    if( this->Initialized )
        this->Initialize();

    if( recording )
        this->Record();

    this->Modified();
}
