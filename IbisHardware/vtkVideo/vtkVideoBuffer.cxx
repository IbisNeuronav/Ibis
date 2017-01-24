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

#include "vtkVideoBuffer.h"
#include "vtkCriticalSection.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include <ctype.h>
#include <time.h>

//---------------------------------------------------------------
// Important FrameBufferMutex rules:
//
// The frame grabs are generally done asynchronously, and it is necessary
// to ensure that when the frame buffer is valid when it is being written
// to or read from
//
// The following information can only be changed within a mutex lock,
// and the lock must not be released until the frame buffer agrees with the
// information.
//
// FrameBuffer
// FrameBufferTimeStamps
// FrameBufferSize
// FrameBufferIndex
// FrameBufferExtent
// FrameBufferBitsPerPixel
// FrameBufferRowAlignment
//
// After one of the above has been changed, and before the mutex is released,
// the following must be called to update the frame buffer:
//
// UpdateFrameBuffer()
//
// Likewise, the following function must only be called from within a
// mutex lock because it modifies FrameBufferIndex:
//
// AdvanceFrameBuffer()
//
// Any methods which might be called asynchronously must lock the
// mutex before reading the above information, and you must be very
// careful when accessing any information except for the above.
// These methods include the following:
//
// InternalGrab()
//
// Finally, when Execute() is reading from the FrameBuffer it must do
// so from within a mutex lock.  Otherwise tearing artifacts might result.

vtkStandardNewMacro(vtkVideoBuffer);

#if ( _MSC_VER >= 1300 ) // Visual studio .NET
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4312 )
#endif


void vtkUnpacker::UnpackRasterLine( char *outPtr, char *rowPtr, int start, int count, int numberOfScalarComponents, double opacity, int outputFormat )
{
    char * inPtr = rowPtr + start * numberOfScalarComponents;
    memcpy( outPtr, inPtr, count * numberOfScalarComponents );
    if( outputFormat == VTK_RGBA )
    { // RGBA image: need to copy in the opacity
        unsigned char alpha = (unsigned char)( opacity * 255 );
        int k;
        outPtr += 3;
        for (k = 0; k < count; k++)
        {
            outPtr[4*k] = alpha;
        }
    }
}


//----------------------------------------------------------------------------
vtkVideoBuffer::vtkVideoBuffer()
{
    int i;

    this->Initialized = 0;

    this->LoopRecording = true;

    this->FrameSize[0] = 320;
    this->FrameSize[1] = 240;
    this->FrameSize[2] = 1;

    for (i = 0; i < 6; i++)
    {
        this->FrameBufferExtent[i] = 0;
    }

    this->Playing = 0;

    this->FrameRate = 30;

    this->FrameCount = 0;

    this->FrameTimeStamp = 0;

    this->UpdateLocked = false;

    this->OutputNeedsInitialization = 1;

    this->OutputFormat = VTK_LUMINANCE;
    this->NumberOfScalarComponents = 1;

    this->NumberOfOutputFrames = 1;

    this->Opacity = 1.0;

    for (i = 0; i < 3; i++)
    {
        this->ClipRegion[i*2] = 0;
        this->ClipRegion[i*2+1] = VTK_INT_MAX;
        this->OutputWholeExtent[i*2] = 0;
        this->OutputWholeExtent[i*2+1] = -1;
        this->DataSpacing[i] = 1.0;
        this->DataOrigin[i] = 0.0;
    }

    for (i = 0; i < 6; i++)
    {
        this->LastOutputExtent[i] = 0;
    }
    this->LastNumberOfScalarComponents = 0;

    this->FlipFrames = 0;

    this->PlayerThreader = vtkMultiThreader::New();
    this->PlayerThreadId = -1;

    this->UpdateMutex = vtkCriticalSection::New();
    this->CaptureMutex = vtkCriticalSection::New();

    this->FrameBufferSize = 0;
    this->FrameBuffer = NULL;
    this->FrameBufferTimeStamps = NULL;

    // Default buffer is of size 2 and starts capturing on frame 0 and updating frame 1 (last)
    this->CurrentCaptureIndex = 0;
    this->CurrentUpdateIndex = 1;
    this->SetFrameBufferSize(2);

    this->FrameBufferBitsPerPixel = 8;
    this->FrameBufferRowAlignment = 1;

    this->Unpacker = vtkUnpacker::New();
    this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkVideoBuffer::~vtkVideoBuffer()
{
    if (this->Playing)
    {
        this->Stop();
    }

    if( this->Unpacker )
        this->Unpacker->UnRegister(this);

    this->SetFrameBufferSize(0);
    this->UpdateMutex->Delete();
    this->CaptureMutex->Delete();
    this->PlayerThreader->Delete();
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
    int idx;

    this->Superclass::PrintSelf(os,indent);

    os << indent << "FrameSize: (" << this->FrameSize[0] << ", "
    << this->FrameSize[1] << ", " << this->FrameSize[2] << ")\n";

    os << indent << "ClipRegion: (" << this->ClipRegion[0];
    for (idx = 1; idx < 6; ++idx)
    {
        os << ", " << this->ClipRegion[idx];
    }
    os << ")\n";

    os << indent << "DataSpacing: (" << this->DataSpacing[0];
    for (idx = 1; idx < 3; ++idx)
    {
        os << ", " << this->DataSpacing[idx];
    }
    os << ")\n";

    os << indent << "DataOrigin: (" << this->DataOrigin[0];
    for (idx = 1; idx < 3; ++idx)
    {
        os << ", " << this->DataOrigin[idx];
    }
    os << ")\n";

    os << indent << "OutputFormat: " <<
    (this->OutputFormat == VTK_RGBA ? "RGBA" :
     (this->OutputFormat == VTK_RGB ? "RGB" :
      (this->OutputFormat == VTK_LUMINANCE_ALPHA ? "LuminanceAlpha" :
       (this->OutputFormat == VTK_LUMINANCE ? "Luminance" : "Unknown"))))
    << "\n";

    os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0];
    for (idx = 1; idx < 6; ++idx)
    {
        os << ", " << this->OutputWholeExtent[idx];
    }
    os << ")\n";

    os << indent << "FrameRate: " << this->FrameRate << "\n";
    os << indent << "FrameCount: " << this->FrameCount << "\n";
    os << indent << "CurrentCaptureIndex: " << this->CurrentCaptureIndex << "\n";
    os << indent << "CurrentUpdateIndex: " << this->CurrentUpdateIndex << "\n";
    os << indent << "Playing: " << (this->Playing ? "On\n" : "Off\n");
    os << indent << "NumberOfOutputFrames: " << this->NumberOfOutputFrames << "\n";
    os << indent << "Opacity: " << this->Opacity << "\n";
    os << indent << "FlipFrames: " << this->FlipFrames << "\n";
    os << indent << "FrameBufferBitsPerPixel: " << this->FrameBufferBitsPerPixel << "\n";
    os << indent << "FrameBufferRowAlignment: " << this->FrameBufferRowAlignment << "\n";
}

//----------------------------------------------------------------------------
// Update the FrameBuffers according to any changes in the FrameBuffer*
// information.
// This function should always be called from within a FrameBufferMutex lock
// and should never be called asynchronously.
// It sets up the FrameBufferExtent
void vtkVideoBuffer::UpdateFrameBuffer()
{
    int i, oldExt;
    int ext[3];
    vtkDataArray *buffer;

    // clip the ClipRegion with the FrameSize
    for (i = 0; i < 3; i++)
    {
        oldExt = this->FrameBufferExtent[2*i+1] - this->FrameBufferExtent[2*i] + 1;
        this->FrameBufferExtent[2*i] = ((this->ClipRegion[2*i] > 0)
                                        ? this->ClipRegion[2*i] : 0);
        this->FrameBufferExtent[2*i+1] = ((this->ClipRegion[2*i+1] <
                                           this->FrameSize[i]-1)
                                          ? this->ClipRegion[2*i+1] : this->FrameSize[i]-1);

        ext[i] = this->FrameBufferExtent[2*i+1] - this->FrameBufferExtent[2*i] + 1;
        if (ext[i] < 0)
        {
            this->FrameBufferExtent[2*i] = 0;
            this->FrameBufferExtent[2*i+1] = -1;
            ext[i] = 0;
        }

        if (oldExt > ext[i])
        { // dimensions of framebuffer changed
            this->OutputNeedsInitialization = 1;
        }
    }

    // total number of bytes required for the framebuffer
    int bytesPerRow = ( ext[0] * this->FrameBufferBitsPerPixel + 7 ) / 8;
    bytesPerRow = (( bytesPerRow + this->FrameBufferRowAlignment - 1) / this->FrameBufferRowAlignment)*this->FrameBufferRowAlignment;
    int totalSize = bytesPerRow * ext[1] * ext[2];

    i = this->FrameBufferSize;

    while (--i >= 0)
    {
        buffer = reinterpret_cast<vtkDataArray *>(this->FrameBuffer[i]);
        if (buffer->GetDataType() != VTK_UNSIGNED_CHAR ||
                buffer->GetNumberOfComponents() != 1 ||
                buffer->GetNumberOfTuples() != totalSize)
        {
            buffer->Delete();
            buffer = vtkUnsignedCharArray::New();
            this->FrameBuffer[i] = buffer;
            buffer->SetNumberOfComponents(1);
            buffer->SetNumberOfTuples(totalSize);

            // clear the buffer
            buffer->FillComponent( 0, 0.0 );
        }
    }

    this->FrameCount = 0;
    this->CurrentCaptureIndex = 1;
    this->CurrentUpdateIndex = 0;
}

void vtkVideoBuffer::Initialize()
{
    this->LockBufferForModif();
    this->UpdateFrameBuffer();
    this->Initialized = 1;
    this->UnlockBufferForModif();
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::SetFrameSize(int x, int y, int z)
{
    if (x == this->FrameSize[0] &&
            y == this->FrameSize[1] &&
            z == this->FrameSize[2])
    {
        return;
    }

    if (x < 1 || y < 1 || z < 1)
    {
        vtkErrorMacro(<< "SetFrameSize: Illegal frame size");
        return;
    }

    if (this->Initialized)
    {
        this->LockBufferForModif();
        this->FrameSize[0] = x;
        this->FrameSize[1] = y;
        this->FrameSize[2] = z;
        this->UpdateFrameBuffer();
        this->UnlockBufferForModif();
    }
    else
    {
        this->FrameSize[0] = x;
        this->FrameSize[1] = y;
        this->FrameSize[2] = z;
    }

    this->Modified();
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::SetFrameRate(float rate)
{
    if (this->FrameRate == rate)
    {
        return;
    }

    this->FrameRate = rate;
    this->Modified();
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::SetClipRegion(int x0, int x1, int y0, int y1,int z0, int z1)
{
    if (this->ClipRegion[0] != x0 || this->ClipRegion[1] != x1 ||
            this->ClipRegion[2] != y0 || this->ClipRegion[3] != y1 ||
            this->ClipRegion[4] != z0 || this->ClipRegion[5] != z1)
    {
        this->Modified();
        if (this->Initialized)
        { // modify the FrameBufferExtent
            this->LockBufferForModif();
            this->ClipRegion[0] = x0;
            this->ClipRegion[1] = x1;
            this->ClipRegion[2] = y0;
            this->ClipRegion[3] = y1;
            this->ClipRegion[4] = z0;
            this->ClipRegion[5] = z1;
            this->UpdateFrameBuffer();
            this->UnlockBufferForModif();
        }
        else
        {
            this->ClipRegion[0] = x0;
            this->ClipRegion[1] = x1;
            this->ClipRegion[2] = y0;
            this->ClipRegion[3] = y1;
            this->ClipRegion[4] = z0;
            this->ClipRegion[5] = z1;
        }
    }
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
// this function runs in an alternate thread to 'play the tape' at the
// specified frame rate.
static void *vtkVideoBufferPlayThread(vtkMultiThreader::ThreadInfo *data)
{  
    vtkVideoBuffer *self = (vtkVideoBuffer *)(data->UserData);

    double startTime = vtkTimerLog::GetUniversalTime();
    double rate = self->GetFrameRate();
    int frame = 0;

    do
    {
        self->Seek(1);
        frame++;
    }
    while (vtkThreadSleep(data, startTime + frame/rate) && frame < self->GetFrameBufferSize()-1);

    return NULL;
}

//----------------------------------------------------------------------------
// Set the source to play back recorded frames.
// You should override this as appropriate for your device.
void vtkVideoBuffer::Play()
{
    if (!this->Playing)
    {
        this->Playing = 1;
        this->Modified();
        this->PlayerThreadId = this->PlayerThreader->SpawnThread( (vtkThreadFunctionType)&vtkVideoBufferPlayThread, this );
    }
}

//----------------------------------------------------------------------------
// Stop continuous grabbing or playback.  You will have to override this
// if your class overrides Play() and Record()
void vtkVideoBuffer::Stop()
{
    if( this->Playing )
    {
        this->PlayerThreader->TerminateThread(this->PlayerThreadId);
        this->PlayerThreadId = -1;
        this->Playing = 0;
        this->Modified();
    }
}

void vtkVideoBuffer::Seek( int n )
{
    int index = this->CurrentUpdateIndex + n;
    if( index >= this->FrameBufferSize )
        index = this->FrameBufferSize - 1;
    if( index < 0 )
        index = 0;
    this->GoToFrame( index );
}

void vtkVideoBuffer::GoToFrame( int index )
{
    this->UpdateMutex->Lock();

    if( index >= this->FrameBufferSize )
    {
        vtkErrorMacro(<<"Current frame index can't be > last frame in the buffer");
        index = this->FrameBufferSize - 1;
    }
    if( index < 0 )
    {
        vtkErrorMacro(<<"Current frame index can't be < 0");
        index = 0;
    }
    int indexCapture = index + 1;
    if( indexCapture >= this->FrameBufferSize )
        indexCapture = 0;
    this->CurrentCaptureIndex = indexCapture;
    this->CurrentUpdateIndex = index;
    this->UpdateMutex->Unlock();
    this->Modified();
}

void vtkVideoBuffer::GoToLastRecordedFrame()
{
    this->UpdateMutex->Lock();
    int updateIndex = this->FrameCount - 1;
    if( updateIndex < 0 )
        updateIndex = this->FrameBufferSize - 1;
    int captureIndex = updateIndex + 1;
    if( captureIndex >= this->FrameBufferSize )
        captureIndex = 0;
    this->CurrentCaptureIndex = captureIndex;
    this->CurrentUpdateIndex = updateIndex;
    this->UpdateMutex->Unlock();
    this->Modified();
}

//----------------------------------------------------------------------------
// Override this and provide checks to ensure an appropriate number
// of components was asked for (i.e. 1 for greyscale, 3 for RGB,
// or 4 for RGBA)
void vtkVideoBuffer::SetOutputFormat(int format)
{
    if (format == this->OutputFormat)
    {
        return;
    }

    this->OutputFormat = format;

    // convert color format to number of scalar components
    int numComponents = 1;

    switch (this->OutputFormat)
    {
    case VTK_RGBA:
        numComponents = 4;
        break;
    case VTK_RGB:
        numComponents = 3;
        break;
    case VTK_LUMINANCE_ALPHA:
        numComponents = 2;
        break;
    case VTK_LUMINANCE:
        numComponents = 1;
        break;
    default:
        vtkErrorMacro(<< "SetOutputFormat: Unrecognized color format.");
        break;
    }
    this->NumberOfScalarComponents = numComponents;

    if (this->FrameBufferBitsPerPixel != numComponents*8)
    {
        this->LockBufferForModif();
        this->FrameBufferBitsPerPixel = numComponents*8;
        if (this->Initialized)
        {
            this->UpdateFrameBuffer();
        }
        this->UnlockBufferForModif();
    }

    this->Modified();
}

//----------------------------------------------------------------------------
// set or change the circular buffer size
// you will have to override this if you want the buffers
// to be device-specific (i.e. something other than vtkDataArray)
void vtkVideoBuffer::SetFrameBufferSize( int bufsize )
{
    if( bufsize == this->FrameBufferSize )
        return;

    this->LockBufferForModif();

    if( this->FrameBuffer == 0 )
    {
        if (bufsize > 0)
        {
            this->FrameBuffer = new void *[bufsize];
            this->FrameBufferTimeStamps = new double[bufsize];
            for( int i = 0; i < bufsize; ++i )
            {
                this->FrameBuffer[i] = vtkUnsignedCharArray::New();
                this->FrameBufferTimeStamps[i] = 0.0;
            }
            this->FrameBufferSize = bufsize;
            this->Modified();
        }
    }
    else
    {
        void **framebuffer = 0;
        double *timestamps = 0;

        if (bufsize > 0)
        {
            framebuffer = new void *[ bufsize ];
            timestamps = new double[ bufsize ];

            // copy over old image buffers
            for( int i = 0; i < this->FrameBufferSize; ++i )
            {
                framebuffer[i] = this->FrameBuffer[i];
            }
            // create new image buffers if necessary
            for( int i = this->FrameBufferSize; i < bufsize; ++i )
            {
                framebuffer[i] = vtkUnsignedCharArray::New();
                timestamps[i] = 0.0;
            }
        }
        // delete image buffers we no longer need
        for( int i = bufsize; i < this->FrameBufferSize; ++i)
        {
            reinterpret_cast<vtkDataArray *>(this->FrameBuffer[i])->Delete();
        }

        // delete old arrays and reference new ones
        delete [] this->FrameBuffer;
        this->FrameBuffer = framebuffer;
        delete [] this->FrameBufferTimeStamps;
        this->FrameBufferTimeStamps = timestamps;

        this->FrameBufferSize = bufsize;
        this->Modified();
    }

    if (this->Initialized)
    {
        this->UpdateFrameBuffer();
    }

    this->UnlockBufferForModif();
}

//----------------------------------------------------------------------------
// This function only be called within the capture thread
void vtkVideoBuffer::AdvanceFrameBuffer()
{
    this->CurrentCaptureIndex++;
    if( this->CurrentCaptureIndex >= this->FrameBufferSize )
        this->CurrentCaptureIndex = 0;
    this->CurrentUpdateIndex = this->CurrentCaptureIndex - 1;

}

//----------------------------------------------------------------------------
double vtkVideoBuffer::GetFrameTimeStamp(int frame)
{
    if( frame > this->FrameBufferSize || frame < 0 )
    {
        vtkErrorMacro(<<"frame out of bounds");
        return 0.0;
    }

    double timeStamp;
    this->UpdateMutex->Lock();
    timeStamp = this->FrameBufferTimeStamps[frame];
    this->UpdateMutex->Unlock();

    return timeStamp;
}


void vtkVideoBuffer::FillCurrentFrameWithNoise( double timestamp )
{
    // get a thread lock on the frame buffer
    this->CaptureMutex->Lock();
    this->InternalFillCurrentFrameWithNoise( timestamp );
    this->CaptureMutex->Unlock();
    this->Modified();
}

void vtkVideoBuffer::SetCurrentFrameData( void * outPtr, double timestamp )
{
    this->CaptureMutex->Lock();
    this->InternalSetCurrentFrameData( outPtr, timestamp );
    this->CaptureMutex->Unlock();
    this->Modified();
}


void vtkVideoBuffer::InternalFillCurrentFrameWithNoise( double timestamp )
{
    if( IsBufferFull() )
        return;

    int i,index;
    static int randsave = 0;
    int randNum;
    unsigned char *ptr;
    int *lptr;

    index = this->CurrentCaptureIndex;

    int bytesPerRow = ((this->FrameBufferExtent[1]-this->FrameBufferExtent[0]+1)*
                       this->FrameBufferBitsPerPixel + 7)/8;
    bytesPerRow = ((bytesPerRow + this->FrameBufferRowAlignment - 1) /
                   this->FrameBufferRowAlignment)*this->FrameBufferRowAlignment;
    int totalSize = bytesPerRow *
                    (this->FrameBufferExtent[3]-this->FrameBufferExtent[2]+1) *
                    (this->FrameBufferExtent[5]-this->FrameBufferExtent[4]+1);

    randNum = randsave;

    // copy 'noise' into the frame buffer
    ptr = reinterpret_cast<vtkUnsignedCharArray *>(this->FrameBuffer[index])->GetPointer(0);

    // Somebody should check this:
    lptr = (int *)(((((long)ptr) + 3)/4)*4);
    i = totalSize/4;

    while (--i >= 0)
    {
        randNum = 1664525*randNum + 1013904223;
        *lptr++ = randNum;
    }
    unsigned char *ptr1 = ptr + 4;
    i = (totalSize-4)/16;
    while (--i >= 0)
    {
        randNum = 1664525*randNum + 1013904223;
        *ptr1 = randNum;
        ptr1 += 16;
    }
    randsave = randNum;

    this->FrameBufferTimeStamps[index] = timestamp;

    this->AdvanceFrameBuffer();
    this->IncrementFrameCount();
}


void vtkVideoBuffer::InternalSetCurrentFrameData( void * imageData, double timestamp )
{
    if( IsBufferFull() )
        return;

    this->FrameBufferTimeStamps[this->CurrentCaptureIndex] = timestamp;

    int imageSize = this->FrameSize[0] * this->FrameSize[1] * this->NumberOfScalarComponents;
    void * ptr = ((reinterpret_cast<vtkDataArray *>( this->FrameBuffer[this->CurrentCaptureIndex]))->GetVoidPointer(0));
    memcpy( ptr, imageData, imageSize );

    this->AdvanceFrameBuffer();
    this->IncrementFrameCount();
}

void vtkVideoBuffer::LockBufferForModif()
{
    this->UpdateMutex->Lock();
    this->CaptureMutex->Lock();
}

void vtkVideoBuffer::UnlockBufferForModif()
{
    this->CaptureMutex->Unlock();
    this->UpdateMutex->Unlock();
}

//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
//void vtkVideoBuffer::ExecuteInformation()
int vtkVideoBuffer::RequestInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
    // get the info objects
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    int i;
    int extent[6];

    for (i = 0; i < 3; i++)
    {
        // initially set extent to the OutputWholeExtent
        extent[2*i] = this->OutputWholeExtent[2*i];
        extent[2*i+1] = this->OutputWholeExtent[2*i+1];
        // if 'flag' is set in output extent, use the FrameBufferExtent instead
        if (extent[2*i+1] < extent[2*i])
        {
            extent[2*i] = 0;
            extent[2*i+1] = this->FrameBufferExtent[2*i+1] - this->FrameBufferExtent[2*i];
        }
        this->FrameOutputExtent[2*i] = extent[2*i];
        this->FrameOutputExtent[2*i+1] = extent[2*i+1];
    }

    int numFrames = this->NumberOfOutputFrames;
    if (numFrames < 1)
    {
        numFrames = 1;
    }
    if (numFrames > this->FrameBufferSize)
    {
        numFrames = this->FrameBufferSize;
    }

    // multiply Z extent by number of frames to output
    extent[5] = extent[4] + (extent[5]-extent[4]+1) * numFrames - 1;

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);

    // set the spacing
    outInfo->Set(vtkDataObject::SPACING(),this->DataSpacing,3);

    // set the origin.
    outInfo->Set(vtkDataObject::ORIGIN(),this->DataOrigin,3);

    // set default data type (8 bit greyscale)
    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR,
      this->NumberOfScalarComponents);
    return 1;
}

bool vtkVideoBuffer::IsBufferFull()
{
    if( this->LoopRecording )
        return false;
    if( this->FrameCount >= this->FrameBufferSize )
        return true;
    return false;
}

void vtkVideoBuffer::IncrementFrameCount()
{
    this->FrameCount++;
}

//----------------------------------------------------------------------------
// The UnpackRasterLine method should be overridden if the framebuffer uses
// unusual pixel packing formats, such as XRGB XBRG BGRX BGR etc.
// The version below assumes that the packing of the framebuffer is
// identical to that of the output.
void vtkVideoBuffer::UnpackRasterLine(char *outPtr, char *rowPtr, int start, int count)
{
    this->Unpacker->UnpackRasterLine( outPtr, rowPtr, start, count, this->NumberOfScalarComponents, this->Opacity, this->OutputFormat );
}

//----------------------------------------------------------------------------
// The Execute method is fairly complex, so I would not recommend overriding
// it unless you have to.  Override the UnpackRasterLine() method instead.
// You should only have to override it if you are using something other
// than 8-bit vtkUnsignedCharArray for the frame buffer.
//void vtkVideoBuffer::ExecuteData(vtkDataObject *output)
int vtkVideoBuffer::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkImageData *data = this->AllocateOutputData( vtkImageData::GetData(outInfo), outInfo );
    // Make sure we should update
    if( this->UpdateLocked )
        return 1;

    int outputExtent[6];     // will later be clipped in Z to a single frame
    int saveOutputExtent[6]; // will possibly contain multiple frames
    data->GetExtent(outputExtent);
    for( int i = 0; i < 6; i++)
    {
        saveOutputExtent[i] = outputExtent[i];
    }
    // clip to extent to the Z size of one frame
    outputExtent[4] = this->FrameOutputExtent[4];
    outputExtent[5] = this->FrameOutputExtent[5];

    int frameExtentX = this->FrameBufferExtent[1]-this->FrameBufferExtent[0]+1;
    int frameExtentY = this->FrameBufferExtent[3]-this->FrameBufferExtent[2]+1;
    int frameExtentZ = this->FrameBufferExtent[5]-this->FrameBufferExtent[4]+1;

    int extentX = outputExtent[1]-outputExtent[0]+1;
    int extentY = outputExtent[3]-outputExtent[2]+1;
    int extentZ = outputExtent[5]-outputExtent[4]+1;

    // if the output is more than a single frame,
    // then the output will cover a partial or full first frame,
    // several full frames, and a partial or full last frame

    // index and Z size of the first frame in the output extent
    int firstFrame = (saveOutputExtent[4]-outputExtent[4])/extentZ;
    int firstOutputExtent4 = saveOutputExtent[4] - extentZ*firstFrame;

    // index and Z size of the final frame in the output extent
    int finalFrame = (saveOutputExtent[5]-outputExtent[4])/extentZ;
    int finalOutputExtent5 = saveOutputExtent[5] - extentZ*finalFrame;

    char *outPtr = (char *)data->GetScalarPointer();
    char *outPtrTmp;

    int inIncY = (frameExtentX*this->FrameBufferBitsPerPixel + 7)/8;
    inIncY = ((inIncY + this->FrameBufferRowAlignment - 1) / this->FrameBufferRowAlignment ) * this->FrameBufferRowAlignment;
    int inIncZ = inIncY*frameExtentY;

    int outIncX = this->NumberOfScalarComponents;
    int outIncY = outIncX*extentX;
    int outIncZ = outIncY*extentY;

    int inPadX = 0;
    int inPadY = 0;
    int inPadZ; // do inPadZ later

    int outPadX = -outputExtent[0];
    int outPadY = -outputExtent[2];
    int outPadZ;  // do outPadZ later

    if (outPadX < 0)
    {
        inPadX -= outPadX;
        outPadX = 0;
    }

    if (outPadY < 0)
    {
        inPadY -= outPadY;
        outPadY = 0;
    }

    int outX = frameExtentX - inPadX;
    int outY = frameExtentY - inPadY;
    int outZ; // do outZ later

    if (outX > extentX - outPadX)
    {
        outX = extentX - outPadX;
    }
    if (outY > extentY - outPadY)
    {
        outY = extentY - outPadY;
    }

    // if output extent has changed, need to initialize output to black
    for( int i = 0; i < 3; i++ )
    {
        if (saveOutputExtent[i] != this->LastOutputExtent[i])
        {
            this->LastOutputExtent[i] = saveOutputExtent[i];
            this->OutputNeedsInitialization = 1;
        }
    }

    // ditto for number of scalar components
    if (data->GetNumberOfScalarComponents() != this->LastNumberOfScalarComponents)
    {
        this->LastNumberOfScalarComponents = data->GetNumberOfScalarComponents();
        this->OutputNeedsInitialization = 1;
    }

    // initialize output to zero only when necessary
    if (this->OutputNeedsInitialization)
    {
        memset(outPtr,0,
               (saveOutputExtent[1]-saveOutputExtent[0]+1)*
               (saveOutputExtent[3]-saveOutputExtent[2]+1)*
               (saveOutputExtent[5]-saveOutputExtent[4]+1)*outIncX);
        this->OutputNeedsInitialization = 0;
    }

    // we have to modify the outputExtent of the first frame,
    // because it might be complete (it will be restored after
    // the first frame has been copied to the output)
    int saveOutputExtent4 = outputExtent[4];
    outputExtent[4] = firstOutputExtent4;

    this->UpdateMutex->Lock();

    this->FrameTimeStamp = this->FrameBufferTimeStamps[ this->CurrentUpdateIndex ];

    for( int frame = firstFrame; frame <= finalFrame; frame++)
    {
        if (frame == finalFrame)
        {
            outputExtent[5] = finalOutputExtent5;
        }

        // Fix the index. simtodo : this is not the right way to go, but we never use multiple output frames
        int index = this->CurrentUpdateIndex + frame;
        if( index < 0 )
            index = 0;
        if( index >= this->FrameBufferSize )
            index = this->FrameBufferSize - 1;
        vtkDataArray *frameBuffer = reinterpret_cast<vtkDataArray *>(this->FrameBuffer[index]);

        char *inPtr = reinterpret_cast<char*>(frameBuffer->GetVoidPointer(0));
        char *inPtrTmp ;

        extentZ = outputExtent[5]-outputExtent[4]+1;
        inPadZ = 0;
        outPadZ = -outputExtent[4];

        if (outPadZ < 0)
        {
            inPadZ -= outPadZ;
            outPadZ = 0;
        }

        outZ = frameExtentZ - inPadZ;

        if (outZ > extentZ - outPadZ)
        {
            outZ = extentZ - outPadZ;
        }

        if (this->FlipFrames)
        { // apply a vertical flip while copying to output
            outPtr += outIncZ*outPadZ+outIncY*outPadY+outIncX*outPadX;
            inPtr += inIncZ*inPadZ+inIncY*(frameExtentY-inPadY-outY);

            for( int i = 0; i < outZ; i++ )
            {
                inPtrTmp = inPtr;
                outPtrTmp = outPtr + outIncY*outY;
                for( int j = 0; j < outY; j++ )
                {
                    outPtrTmp -= outIncY;
                    if (outX > 0)
                    {
                        this->UnpackRasterLine(outPtrTmp,inPtrTmp,inPadX,outX);
                    }
                    inPtrTmp += inIncY;
                }
                outPtr += outIncZ;
                inPtr += inIncZ;
            }
        }
        else
        { // don't apply a vertical flip
            outPtr += outIncZ*outPadZ+outIncY*outPadY+outIncX*outPadX;
            inPtr += inIncZ*inPadZ+inIncY*inPadY;

            for( int i = 0; i < outZ; i++ )
            {
                inPtrTmp = inPtr;
                outPtrTmp = outPtr;
                for( int j = 0; j < outY; j++ )
                {
                    if (outX > 0)
                    {
                        this->UnpackRasterLine(outPtrTmp,inPtrTmp,inPadX,outX);
                    }
                    outPtrTmp += outIncY;
                    inPtrTmp += inIncY;
                }
                outPtr += outIncZ;
                inPtr += inIncZ;
            }
        }
        // restore the output extent once the first frame is done
        outputExtent[4] = saveOutputExtent4;
    }
    
    this->InternalRequestData( vtkImageData::GetData(outInfo) ); //output );

    this->UpdateMutex->Unlock();

    // Tell clients the output has been updated
    this->InvokeEvent( vtkCommand::UpdateEvent );
    return 1;
}




