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

#include "vtkTrackedVideoBuffer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCriticalSection.h"
#include "vtkMatrix4x4.h"
#include "vtkMatrix4x4Operators.h"
#include "vtkObjectFactory.h"
#include "vtkTrackerTool.h"
#include "vtkTrackerBuffer.h"
#include "vtkTransform.h"

vtkTrackerToolTransform::vtkTrackerToolTransform()
{
    this->UncalibratedMatrix = vtkMatrix4x4::New();
    this->Flags = -1;
}

vtkTrackerToolTransform::vtkTrackerToolTransform( const vtkTrackerToolTransform & toCopy )
{
    this->UncalibratedMatrix = vtkMatrix4x4::New();
    this->UncalibratedMatrix->DeepCopy( toCopy.UncalibratedMatrix );
    this->Flags = toCopy.Flags;
}

vtkTrackerToolTransform::~vtkTrackerToolTransform()
{
    this->UncalibratedMatrix->Delete();
}

void vtkTrackerToolTransform::Reset()
{
    this->UncalibratedMatrix->Identity();
    this->Flags = -1;
}

vtkStandardNewMacro(vtkTrackedVideoBuffer);

vtkTrackedVideoBuffer::vtkTrackedVideoBuffer()
{
    this->TrackerTool   = 0;
    this->TimeShift     = 0;

    this->TransformFrozen = false;
    this->TotalFreezeMatrixCount = 0;
    this->FreezeMatrixCount = 0;
    this->FreezeMatrixSum = vtkMatrix4x4::New();
    this->FreezeMatrix = vtkMatrix4x4::New();

    this->UncalibratedOutputTransform = vtkTransform::New();
    this->CalibrationTransform = vtkTransform::New();
    this->OutputTransform = vtkTransform::New();
    this->OutputTransform->Concatenate( this->UncalibratedOutputTransform );
    this->OutputTransform->Concatenate( this->CalibrationTransform );

    this->Flags           = -1;
    this->VideoBufferFull = false;
    this->SetNumberOfInputPorts(0);
}

vtkTrackedVideoBuffer::~vtkTrackedVideoBuffer()
{
    if( this->TrackerTool )
    {
        this->TrackerTool->UnRegister( 0 );
    }
    this->CalibrationTransform->Delete();
    this->UncalibratedOutputTransform->Delete();
    this->OutputTransform->Delete();
}

void vtkTrackedVideoBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
    vtkVideoBuffer::PrintSelf( os, indent );
}

void vtkTrackedVideoBuffer::SetFrameBufferSize(int FrameBufferSize)
{
    vtkVideoBuffer::SetFrameBufferSize( FrameBufferSize );
    if( this->FrameBufferSize != (int)this->TransformBuffer.size() )
    {
         this->TransformBuffer.resize( this->FrameBufferSize );
    }
}

void vtkTrackedVideoBuffer::FillCurrentFrameWithNoise( double timestamp )
{
    double trackerTimeStamp = timestamp - TimeShift;
    if( !this->TrackerTool || this->TrackerTool->GetBuffer()->GetTimeStamp( 0 ) > trackerTimeStamp )
    {
        this->CaptureMutex->Lock();
        this->InternalFillCurrentFrameWithNoise( timestamp );
        this->CaptureMutex->Unlock();
        this->Modified();
    }
    else
    {
        // simtodo : implement this correctly
        vtkErrorMacro( << "Shouldn't be going here.")
    }
}

void vtkTrackedVideoBuffer::SetCurrentFrameData( void * imageData, double timestamp )
{
    if( this->IsBufferFull() )
        return;

    this->CaptureMutex->Lock();

    bool isTracking = this->TrackerTool != 0 && this->TrackerTool->GetTracker() != 0 && this->TrackerTool->GetTracker()->IsTracking();

    // Store the video frame if the buffer is not full or if we are using a loop buffer
    if( this->LoopRecording || !this->VideoBufferFull )
    {
        this->FrameBufferTimeStamps[this->CurrentCaptureIndex] = timestamp;
        int imageSize = this->FrameSize[0] * this->FrameSize[1] * this->NumberOfScalarComponents;
        void * ptr = ((reinterpret_cast<vtkDataArray *>( this->FrameBuffer[this->CurrentCaptureIndex]))->GetVoidPointer(0));
        memcpy( ptr, imageData, imageSize );
        AdvanceCaptureFrame();

        // If there is no tracking, report frame as not valid
        if( !isTracking )
        {
            this->TransformBuffer[ this->CurrentUpdateIndex ].Flags = TR_MISSING;
            IncrementFrameCount();
            AdvanceUpdateFrame();
        }
    }

    // Capture the transform if we have tracking, otherwise, report missing
    if( isTracking )
    {
        // See if we can store a couple of the pending transforms
        int index = this->CurrentUpdateIndex;
        double lastToolTimeStamp = this->TrackerTool->GetBuffer()->GetTimeStamp( 0 );       // timestamp of the last sample in the tracker buffer
        bool done = false;
        while( !done )
        {
            AdvanceFrame( index );
            double frameTimeStampInTrackerTime = this->FrameBufferTimeStamps[ index ] - this->TimeShift;
            if( index != this->CurrentCaptureIndex && frameTimeStampInTrackerTime < lastToolTimeStamp )
            {
                vtkMatrix4x4 * matPtr = vtkMatrix4x4::New();
                vtkMatrix4x4 * uncalMatPtr = this->TransformBuffer[index].UncalibratedMatrix;
                this->TransformBuffer[ index ].Flags = this->TrackerTool->GetBuffer()->GetFlagsAndMatrixFromTime( matPtr, uncalMatPtr, frameTimeStampInTrackerTime );
                if( this->TransformFrozen )
                    ManageFrozenTransform( index );
                matPtr->Delete();
                AdvanceUpdateFrame();
                IncrementFrameCount();
            }
            else
                done = true;
        }
    }
    this->CaptureMutex->Unlock();
    this->Modified();
}

void vtkTrackedVideoBuffer::ManageFrozenTransform( int index )
{
    if( this->FreezeMatrixCount < this->TotalFreezeMatrixCount && AreFlagsValid( this->TransformBuffer[index].Flags ) )
    {
        vtkMatrix4x4Operators::AddMatrix( this->FreezeMatrixSum, this->TransformBuffer[index].UncalibratedMatrix );
        this->FreezeMatrixCount++;
        vtkMatrix4x4Operators::MatrixMultScalar( this->FreezeMatrixSum, this->FreezeMatrix, 1.0 / this->FreezeMatrixCount );
    }
    this->TransformBuffer[ index ].UncalibratedMatrix->DeepCopy( this->FreezeMatrix );
}

void vtkTrackedVideoBuffer::SetTrackerTool( vtkTrackerTool * tool )
{
    if( tool == this->TrackerTool )
    {
        return;
    }

    if( this->TrackerTool != 0 )
    {
        this->TrackerTool->UnRegister( this );
    }

    this->TrackerTool = tool;

    if( this->TrackerTool )
    {
        this->TrackerTool->Register( this );
    }
}

void vtkTrackedVideoBuffer::SetCalibrationMatrix( vtkMatrix4x4 * mat )
{
    vtkMatrix4x4 * calMatrix = this->CalibrationTransform->GetMatrix();
    calMatrix->DeepCopy( mat );
    this->CalibrationTransform->Modified();
}

void vtkTrackedVideoBuffer::FreezeTransform( int numberOfFrames )
{
    this->TotalFreezeMatrixCount = numberOfFrames;
    this->FreezeMatrixCount = 0;
    this->FreezeMatrixSum->Zero();
    this->FreezeMatrix->Identity();
    this->TransformFrozen = true;
}

void vtkTrackedVideoBuffer::UnFreezeTransform()
{
    this->TransformFrozen = false;
}

bool vtkTrackedVideoBuffer::IsCurrentFrameValid()
{
    return AreFlagsValid( this->Flags );
}

bool vtkTrackedVideoBuffer::AreFlagsValid( int flags )
{
    if( ( flags & (TR_MISSING | TR_OUT_OF_VIEW | TR_OUT_OF_VOLUME) )  == 0 )
        return true;
    return false;
}

void vtkTrackedVideoBuffer::UpdateFrameBuffer()
{
    // Update parent's frame buffer
    vtkVideoBuffer::UpdateFrameBuffer();

    this->ResetAllTransforms();
    this->VideoBufferFull = false;
}

void vtkTrackedVideoBuffer::ResetAllTransforms()
{
    TransformVec::iterator it = this->TransformBuffer.begin();
    for( ; it != this->TransformBuffer.end(); ++it )
    {
        (*it).Reset();
    }
}

// This function should only be called by ExecuteData in the parent class
// and assumes it is called when the framebuffer mutex has been locked.
void vtkTrackedVideoBuffer::InternalRequestData( vtkDataObject * data )
{
    vtkTrackerToolTransform * currentTransform = &(this->TransformBuffer[ this->CurrentUpdateIndex ]);
    if (!currentTransform)
        return;
    this->Flags = currentTransform->Flags;

    if( (this->Flags & (TR_MISSING | TR_OUT_OF_VIEW | TR_OUT_OF_VOLUME) )  == 0 )
    {
        this->UncalibratedOutputTransform->SetMatrix( currentTransform->UncalibratedMatrix );
    }
}

#include "vtkImageData.h"

void vtkTrackedVideoBuffer::SetFullFrameData( vtkImageData * data, double timestamp, int flags, vtkMatrix4x4 * uncalibratedMatrix )
{
    if( this->VideoBufferFull )
        return;

    this->CaptureMutex->Lock();
    this->FrameBufferTimeStamps[ this->CurrentCaptureIndex ] = timestamp;
    vtkTrackerToolTransform * currentTransform = &(this->TransformBuffer[ this->CurrentCaptureIndex ]);
    currentTransform->UncalibratedMatrix->DeepCopy( uncalibratedMatrix );
    this->TransformBuffer[ this->CurrentCaptureIndex ].Flags = flags;
    int imageSize = this->FrameSize[0] * this->FrameSize[1] * this->NumberOfScalarComponents;
    void * ptr = ((reinterpret_cast<vtkDataArray *>( this->FrameBuffer[this->CurrentCaptureIndex]))->GetVoidPointer(0));
    memcpy( ptr, data->GetScalarPointer(), imageSize );
    AdvanceCaptureFrame();
    AdvanceUpdateFrame();
    IncrementFrameCount();
    this->CaptureMutex->Unlock();
    this->Modified();
}

void vtkTrackedVideoBuffer::AdvanceCaptureFrame()
{
    bool loop = AdvanceFrame( this->CurrentCaptureIndex );
    if( loop && !this->LoopRecording )
    {
        this->VideoBufferFull = true;
        this->InvokeEvent( vtkBufferFullEvent );
    }
    if( this->CurrentCaptureIndex == this->CurrentUpdateIndex )
    {
        vtkErrorMacro( << "Capture frame should not be = to Update frame index" );
    }
}

void vtkTrackedVideoBuffer::AdvanceUpdateFrame()
{
    AdvanceFrame( this->CurrentUpdateIndex );
    if( this->CurrentCaptureIndex == this->CurrentUpdateIndex )
    {
        vtkErrorMacro( << "Capture frame should not be = to Update frame index" );
    }
}

// Returns true if it loops
bool vtkTrackedVideoBuffer::AdvanceFrame( int & frameIndex )
{
    frameIndex++;
    if( frameIndex >= this->FrameBufferSize )
    {
        frameIndex = 0;
        return true;
    }
    return false;
}

