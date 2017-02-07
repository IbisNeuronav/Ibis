/*=========================================================================

  Program:   AtamaiTracking for VTK
  Module:    vtkTracker.cxx
  Creator:   David Gobbi <dgobbi@atamai.com>
  Language:  C++

==========================================================================

Copyright (c) 2000-2005 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.  

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) The name of the Atamai Inc., nor of its principals or owners,
   past or present, may be used to ensorse or promote products derived
   from this software without specific prior written permission.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include <limits.h>
#include <float.h>
#include <math.h>
#include "vtkTracker.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkTimerLog.h"
#include "vtkTrackerTool.h"
#include "vtkTrackerBuffer.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkTracker* vtkTracker::New()
{
    // First try to create the object from the vtkObjectFactory
    vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTracker");
    if(ret)
    {
        return (vtkTracker*)ret;
    }
    // If the factory was unable to create the object, then create it here.
    return new vtkTracker;
}

//----------------------------------------------------------------------------
vtkTracker::vtkTracker()
{
    this->Tracking = 0;
    this->WorldCalibrationMatrix = vtkMatrix4x4::New();
    this->NumberOfTools = 0;
    this->NumberOfActiveTools = 0;
    this->ReferenceTool = -1;
    this->UpdateTimeStamp = 0;
    this->Tools = 0;
    this->LastUpdateTime = 0;
    this->InternalUpdateRate = 0;

  // for threaded capture of transformations
  this->Threader = vtkMultiThreader::New();
  this->ThreadId = -1;
  this->UpdateMutex = vtkCriticalSection::New();
  this->RequestUpdateMutex = vtkCriticalSection::New();
}

//----------------------------------------------------------------------------
vtkTracker::~vtkTracker()
{
    // The thread should have been stopped before the
    // subclass destructor was called, but just in case
    // se stop it here.
    if (this->ThreadId != -1)
    {
        this->Threader->TerminateThread(this->ThreadId);
        this->ThreadId = -1;
    }

    for (int i = 0; i < this->NumberOfTools; i++)
    {
        this->Tools[i]->SetTracker(NULL);
        this->Tools[i]->Delete();
    }
    if (this->Tools)
    {
        delete [] this->Tools;
    }

    this->WorldCalibrationMatrix->Delete();

    this->Threader->Delete();
    this->UpdateMutex->Delete();
    this->RequestUpdateMutex->Delete();
}


//----------------------------------------------------------------------------
void vtkTracker::Destroy()
{
    if( this->IsTracking() )
    {
        this->StopTracking();    
    }
    
    if( this->Tools )
    {
        for (int i = 0; i < this->NumberOfTools; i++)
        {
            this->Tools[i]->SetTracker(NULL);
            this->Tools[i]->Delete();
        }
        delete [] this->Tools;
        this->Tools = 0;
        this->NumberOfTools = 0;
    }
    
    this->Delete();
}

//----------------------------------------------------------------------------
void vtkTracker::PrintSelf(ostream& os, vtkIndent indent)
{
    vtkObject::PrintSelf(os,indent);

  os << indent << "WorldCalibrationMatrix: " << this->WorldCalibrationMatrix << "\n";
  this->WorldCalibrationMatrix->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Tracking: " << this->Tracking << "\n";
  os << indent << "ReferenceTool: " << this->ReferenceTool << "\n";
  os << indent << "NumberOfTools: " << this->NumberOfTools << "\n";
}
  
//----------------------------------------------------------------------------
// allocates a vtkTrackerTool object for each of the tools.
void vtkTracker::SetNumberOfTools(int numtools)
{
    int i;

    if (this->NumberOfTools > 0)
    {
        vtkErrorMacro( << "SetNumberOfTools() can only be called once");
    }
    this->NumberOfTools = numtools;

    this->Tools = new vtkTrackerTool *[numtools];

    for (i = 0; i < numtools; i++)
    {
        this->Tools[i] = vtkTrackerTool::New();
        this->Tools[i]->SetTracker(this);
        this->Tools[i]->SetToolPort(i);
    }
}


void vtkTracker::SetNumberOfActiveTools( int numtools )
{
    if( numtools <= this->NumberOfTools )
        this->NumberOfActiveTools = numtools;
}


//----------------------------------------------------------------------------
vtkTrackerTool *vtkTracker::GetTool(int tool)
{
    if (tool < 0 || tool > this->NumberOfTools)
    {
        vtkErrorMacro( << "GetTool(" << tool << "): only " << this->NumberOfTools << " are available");
        return 0;
    }
    return this->Tools[tool];
}


int vtkTracker::GetAvailablePassivePort()
{
    return -1;
}


//----------------------------------------------------------------------------
// this thread is run whenever the tracker is tracking
static void *vtkTrackerThread(vtkMultiThreader::ThreadInfo *data)
{
    vtkTracker *self = (vtkTracker *)(data->UserData);

    double currtime[10];

  // loop until cancelled
  for (int i = 0;; i++)
    {
        // get current tracking rate over last 10 updates
        double newtime = vtkTimerLog::GetUniversalTime();
        double difftime = newtime - currtime[i%10];
        currtime[i%10] = newtime;
        if (i > 10 && difftime != 0)
        {
            self->InternalUpdateRate = (10.0/difftime);
        }

    // query the hardware tracker
    self->UpdateMutex->Lock();
    self->InternalUpdate();
    self->UpdateTime.Modified();
    self->UpdateMutex->Unlock();

    // check to see if main thread wants to lock the UpdateMutex
    self->RequestUpdateMutex->Lock();
    self->RequestUpdateMutex->Unlock();
    
    // check to see if we are being told to quit 
    data->ActiveFlagLock->Lock();
    int activeFlag = *(data->ActiveFlag);
    data->ActiveFlagLock->Unlock();

        if (activeFlag == 0)
        {
            return NULL;
        }
    }
}

//----------------------------------------------------------------------------
int vtkTracker::Probe()
{
    this->UpdateMutex->Lock();

    if (this->InternalStartTracking() == 0)
    {
        this->UpdateMutex->Unlock();
        return 0;
    }

    this->Tracking = 1;

    if (this->InternalStopTracking() == 0)
    {
        this->Tracking = 0;
        this->UpdateMutex->Unlock();
        return 0;
    }

    this->Tracking = 0;
    this->UpdateMutex->Unlock();
    return 1;
}

//----------------------------------------------------------------------------
void vtkTracker::StartTracking()
{
    int tracking = this->Tracking;

    this->Tracking = this->InternalStartTracking();

    // start the tracking thread
    if (!(this->Tracking && !tracking && this->ThreadId == -1))
    {
        return;
    }

  // this will block the tracking thread until we're ready
    this->UpdateMutex->Lock();
    
    // start the tracking thread
    this->ThreadId = this->Threader->SpawnThread((vtkThreadFunctionType)\
                                                  &vtkTrackerThread,this);
    this->LastUpdateTime = this->UpdateTime.GetMTime();

  // allow the tracking thread to proceed
    this->UpdateMutex->Unlock();
    
// wait until the first update has occurred before returning
    int timechanged = 0;
    while (!timechanged)
    {
        this->RequestUpdateMutex->Lock();
        this->UpdateMutex->Lock();
        this->RequestUpdateMutex->Unlock();
        timechanged = (this->LastUpdateTime != this->UpdateTime.GetMTime());
        this->UpdateMutex->Unlock();
#ifdef _WIN32
        Sleep((int)(100));
#elif defined(__FreeBSD__) || defined(__linux__) || defined(sgi) || defined(__APPLE__)
        struct timespec sleep_time, dummy;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 100000000;
        nanosleep(&sleep_time,&dummy);
#endif
    }
}

//----------------------------------------------------------------------------
void vtkTracker::StopTracking()
{
  if (this->Tracking && this->ThreadId != -1)
    {
    this->Threader->TerminateThread(this->ThreadId);
    this->ThreadId = -1;
    }

  this->InternalStopTracking();
  this->Tracking = 0;
}

void vtkTracker::LockUpdate()
{
    for (int tool = 0; tool < this->NumberOfTools; tool++)
    {
        vtkTrackerTool * trackerTool = this->Tools[tool];
        trackerTool->LockUpdate();
    }
}

void vtkTracker::UnlockUpdate()
{
    for (int tool = 0; tool < this->NumberOfTools; tool++)
    {
        vtkTrackerTool * trackerTool = this->Tools[tool];
        trackerTool->UnlockUpdate();
    }
}

//----------------------------------------------------------------------------
void vtkTracker::Update()
{
    if (!this->Tracking)
    {
        return;
    }

    for (int tool = 0; tool < this->NumberOfTools; tool++)
    {
        vtkTrackerTool *trackerTool = this->Tools[tool];

        trackerTool->Update();

        this->UpdateTimeStamp = trackerTool->GetTimeStamp();
    }

    this->LastUpdateTime = this->UpdateTime.GetMTime();
}

//----------------------------------------------------------------------------
void vtkTracker::SetWorldCalibrationMatrix(vtkMatrix4x4 *vmat)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (this->WorldCalibrationMatrix->GetElement(i,j)
                    != vmat->GetElement(i,j))
            {
                break;
            }
        }
        if (j < 4)
        {
            break;
        }
    }

    if (i < 4 || j < 4) // the matrix is different
    {
        this->WorldCalibrationMatrix->DeepCopy(vmat);
        this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *vtkTracker::GetWorldCalibrationMatrix()
{
    return this->WorldCalibrationMatrix;
}

//----------------------------------------------------------------------------
void vtkTracker::ToolUpdate(int tool, vtkMatrix4x4 *matrix, long flags,
                            double timestamp)
{
    vtkTrackerBuffer *buffer = this->Tools[tool]->GetBuffer();

    buffer->Lock();
    buffer->AddItem(matrix, flags, timestamp);
    buffer->Unlock();
    
    this->Tools[tool]->NewMatrixAvailable();
}
  
//----------------------------------------------------------------------------
void vtkTracker::Beep(int n)
{
  this->RequestUpdateMutex->Lock();
  this->UpdateMutex->Lock();
  this->RequestUpdateMutex->Unlock();
  this->InternalBeep(n);
  this->UpdateMutex->Unlock();
}

//----------------------------------------------------------------------------
void vtkTracker::SetToolLED(int tool, int led, int state)
{
  this->RequestUpdateMutex->Lock();
  this->UpdateMutex->Lock();
  this->RequestUpdateMutex->Unlock();
  this->InternalSetToolLED(tool, led, state);
  this->UpdateMutex->Unlock();
}






