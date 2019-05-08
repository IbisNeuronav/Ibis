/*=========================================================================

  Program:   AtamaiTracking for VTK
  Module:    vtkTrackerTool.cxx
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

#include "vtkTrackerTool.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"
#include "vtkAmoebaMinimizer.h"
#include "vtkTrackerBuffer.h"
#include "vtkObjectFactory.h"
#include "vtkCriticalSection.h"

//----------------------------------------------------------------------------
vtkTrackerTool* vtkTrackerTool::New()
{
    // First try to create the object from the vtkObjectFactory
    vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTrackerTool");
    if(ret)
    {
        return (vtkTrackerTool*)ret;
    }
    // If the factory was unable to create the object, then create it here.
    return new vtkTrackerTool;
}

//----------------------------------------------------------------------------
vtkTrackerTool::vtkTrackerTool()
    : UpdateLocked( false )
{
    this->Tracker = 0;
    this->Transform = vtkTransform::New();
    this->Transform->Identity();
    this->Flags = TR_MISSING;
    this->TimeStamp = 0;
    this->CalibrationMatrix = vtkMatrix4x4::New();

    this->LED1 = 0;
    this->LED2 = 0;
    this->LED3 = 0;

    this->ToolType = 0;
    this->ToolRevision = 0;
    this->ToolSerialNumber = 0;
    this->ToolPartNumber = 0;
    this->ToolManufacturer = 0;

    this->SetToolType("");
    this->SetToolRevision("");
    this->SetToolSerialNumber("");
    this->SetToolPartNumber("");
    this->SetToolManufacturer("");

    this->TempMatrix = vtkMatrix4x4::New();
    this->RawMatrix = vtkMatrix4x4::New();

    this->Buffer = vtkTrackerBuffer::New();
    this->Buffer->SetToolCalibrationMatrix(this->CalibrationMatrix);
}

//----------------------------------------------------------------------------
vtkTrackerTool::~vtkTrackerTool()
{
    this->Transform->Delete();
    this->CalibrationMatrix->Delete();
    this->TempMatrix->Delete();
    this->RawMatrix->Delete();
    if (this->ToolType)
    {
        delete [] this->ToolType;
    }
    if (this->ToolRevision)
    {
        delete [] this->ToolRevision;
    }
    if (this->ToolSerialNumber)
    {
        delete [] this->ToolSerialNumber;
    }
    if (this->ToolManufacturer)
    {
        delete [] this->ToolManufacturer;
    }
    this->Buffer->Delete();
}

//----------------------------------------------------------------------------
void vtkTrackerTool::PrintSelf(ostream& os, vtkIndent indent)
{
    vtkObject::PrintSelf(os,indent);

    os << indent << "Tracker: " << this->Tracker << "\n";
    os << indent << "ToolPort: " << this->ToolPort << "\n";
    os << indent << "IsMissing: " << this->IsMissing() << "\n";
    os << indent << "IsOutOfView: " << this->IsOutOfView() << "\n";
    os << indent << "IsOutOfVolume: " << this->IsOutOfVolume() << "\n";
    os << indent << "IsSwitch1On: " << this->IsSwitch1On() << "\n";
    os << indent << "IsSwitch2On: " << this->IsSwitch2On() << "\n";
    os << indent << "IsSwitch3On: " << this->IsSwitch3On() << "\n";
    os << indent << "LED1: " << this->GetLED1() << "\n";
    os << indent << "LED2: " << this->GetLED2() << "\n";
    os << indent << "LED3: " << this->GetLED3() << "\n";
    os << indent << "ToolType: " << this->GetToolType() << "\n";
    os << indent << "ToolRevision: " << this->GetToolRevision() << "\n";
    os << indent << "ToolManufacturer: " << this->GetToolManufacturer() << "\n";
    os << indent << "ToolPartNumber: " << this->GetToolPartNumber() << "\n";
    os << indent << "ToolSerialNumber: " << this->GetToolSerialNumber() << "\n";
    os << indent << "Transform: " << this->Transform << "\n";
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    os << indent << "CalibrationMatrix: " << this->CalibrationMatrix << "\n";
    this->CalibrationMatrix->PrintSelf(os,indent.GetNextIndent());
    os << indent << "Buffer: " << this->Buffer << "\n";
    this->Buffer->PrintSelf(os,indent.GetNextIndent());
}

void vtkTrackerTool::LockUpdate()
{
    this->UpdateLocked = true;
}

void vtkTrackerTool::UnlockUpdate()
{
    this->UpdateLocked = false;
}

//----------------------------------------------------------------------------
// the update copies the latest matrix from the buffer
void vtkTrackerTool::Update()
{
    if( this->UpdateLocked )
        return;

    this->Buffer->Lock();

    this->Flags = this->Buffer->GetFlags(0);

    if ((this->Flags & (TR_MISSING | TR_OUT_OF_VIEW))  == 0)
    {
        this->Buffer->GetMatrix(this->TempMatrix, 0);
        this->Transform->SetMatrix(this->TempMatrix);
    }

    this->TimeStamp = this->Buffer->GetTimeStamp(0);

    this->Buffer->Unlock();
}


void vtkTrackerTool::NewMatrixAvailable()
{
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetCalibrationMatrix(vtkMatrix4x4 *vmat)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (this->CalibrationMatrix->GetElement(i,j) != vmat->GetElement(i,j))
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
        this->CalibrationMatrix->DeepCopy(vmat);
        this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *vtkTrackerTool::GetCalibrationMatrix()
{
    return this->CalibrationMatrix;
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetLED1(int state)
{
    this->Tracker->SetToolLED(this->ToolPort,1,state);
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetLED2(int state)
{
    this->Tracker->SetToolLED(this->ToolPort,2,state);
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetLED3(int state)
{
    this->Tracker->SetToolLED(this->ToolPort,3,state);
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetTracker(vtkTracker *tracker)
{
    if (tracker == this->Tracker)
    {
        return;
    }

    if (this->Tracker)
    {
        this->Buffer->SetWorldCalibrationMatrix(NULL);
        this->Tracker->Delete();
    }

    if (tracker)
    {
        tracker->Register(this);
        this->Tracker = tracker;
		this->Buffer->SetWorldCalibrationMatrix(tracker->GetWorldCalibrationMatrix());
    }
    else
    {
        this->Tracker = NULL;
    }

    this->Modified();
}

//----------------------------------------------------------------------------
// obsolete, replaced by the TrackerBuffer class, but left in for now
// just in case I want to go back to doing things this way.
void vtkTrackerTool::UpdateAndCalibrate(vtkMatrix4x4 *trans, long flags)
{
    if ((flags & (TR_MISSING | TR_OUT_OF_VIEW))  == 0)
    {
        // only copy in the new matrix if the transform is valid
        this->RawMatrix->DeepCopy(trans);
    }

    int i,j;
    vtkMatrix4x4 *matrix = this->Transform->GetMatrix();

    vtkMatrix4x4::Multiply4x4(this->RawMatrix,
                              this->CalibrationMatrix,
                              this->TempMatrix);
    vtkMatrix4x4::Multiply4x4(this->Tracker->GetWorldCalibrationMatrix(),
                              this->TempMatrix,
                              this->TempMatrix);

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (matrix->GetElement(i,j) != TempMatrix->GetElement(i,j))
            {
                break;
            }
        }
        if (j < 4)
        {
            break;
        }
    }

    if (i < 4 || j < 4 || flags != this->Flags) // the transform has changed
    {
        this->Transform->SetMatrix(TempMatrix);
        this->Flags = flags;
    }
}
