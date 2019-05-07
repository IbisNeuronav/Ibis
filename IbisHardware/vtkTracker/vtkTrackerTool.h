/*=========================================================================

  Program:   AtamaiTracking for VTK
  Module:    vtkTrackerTool.h
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
// .NAME vtkTrackerTool - interfaces VTK to a handheld 3D positioning tool
// .SECTION Description
// The vtkTrackerTool provides an interface between a tracked object in
// the real world and a virtual object.
// .SECTION see also
// vtkTracker vtkPOLARISTracker vtkFlockTracker

#ifndef __vtkTrackerTool_h
#define __vtkTrackerTool_h

#include "vtkObject.h"
#include "vtkTracker.h"

class vtkMatrix4x4;
class vtkTransform;
class vtkDoubleArray;
class vtkAmoebaMinimizer;
class vtkTrackerBuffer;

class VTK_EXPORT vtkTrackerTool : public vtkObject
{
public:

  static vtkTrackerTool *New();
  vtkTypeMacro(vtkTrackerTool,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get a reference to the transform associated with this tool.  The
  // transform will automatically update when Update() is called
  // on the tracking system.  You can connect this transform or its
  // matrix to a vtkActor.
  vtkGetObjectMacro(Transform,vtkTransform);

  // Description:
  // Get a running list of all the transforms received for this
  // tool.  See the vtkTrackerBuffer class for more information.
  vtkGetObjectMacro(Buffer,vtkTrackerBuffer);

  // Description:
  // Get the tracker which owns this tool. 
  vtkGetObjectMacro(Tracker,vtkTracker);

  // Description:
  // Get a numeric identifier for this tool i.e. 0, 1, 2.
  vtkGetMacro(ToolPort,int);

  // Description:
  // Set a calibration matrix.  The raw tool transform will be pre-multiplied
  // by this to provide this->Transform.  This allows you to define a
  // custom coordinate system for the tool which is different from the
  // manufacturer's tool coordinate system.
  // Warning: the calibration matrix is copied, not referenced.
  void SetCalibrationMatrix(vtkMatrix4x4* vmat);
  vtkMatrix4x4 *GetCalibrationMatrix();

  // Description:
  // Get additional information about the transform for the latest update:
  // <p>Missing:     there is no tool plugged into this port.
  // <p>OutOfView:   tracker is temporarily unable to supply a transform.
  // <p>OutOfVolume: tool is outside of its calibrated volume.
  // <p>SwitchNOn:   button N on the tool is being held down.
  int GetFlags()   { return this->Flags; };
  int IsMissing()   {return ((this->Flags & TR_MISSING) != 0); };
  int IsOutOfView() {return ((this->Flags & TR_OUT_OF_VIEW) != 0); };
  int IsOutOfVolume() {return ((this->Flags & TR_OUT_OF_VOLUME) != 0); };
  int IsSwitch1On() {return ((this->Flags & TR_SWITCH1_IS_ON) != 0); };
  int IsSwitch2On() {return ((this->Flags & TR_SWITCH2_IS_ON) != 0); };
  int IsSwitch3On() {return ((this->Flags & TR_SWITCH3_IS_ON) != 0); };

  // Description:
  // Get the timestamp (in seconds since 1970) for the last update to
  // the tool Transform.
  double GetTimeStamp() { return this->TimeStamp; };

  // Description:
  // Set the states of the LEDs on the tool.  If the tracking system
  // is not in tracking mode, the state will not be realized until
  // the system enters tracking mode.  The states are 0 (off), 1 (on),
  // and 2 (flashing).  Not all tracking systems support LEDs.
  void SetLED1(int state);
  void SetLED2(int state);
  void SetLED3(int state);
  void LED2Off() { this->SetLED2(TR_LED_OFF); };
  void LED3Off() { this->SetLED3(TR_LED_OFF); };
  void LED1Off() { this->SetLED1(TR_LED_OFF); };
  void LED1On() { this->SetLED1(TR_LED_ON); };
  void LED2On() { this->SetLED2(TR_LED_ON); };
  void LED3On() { this->SetLED3(TR_LED_ON); };
  void LED1Flash() { this->SetLED1(TR_LED_FLASH); };
  void LED2Flash() { this->SetLED2(TR_LED_FLASH); };
  void LED3Flash() { this->SetLED3(TR_LED_FLASH); };
  vtkGetMacro(LED1,int);
  vtkGetMacro(LED2,int);
  vtkGetMacro(LED3,int);

  // Description:
  // Get miscellaneous information about the tool.  Most
  // tracking systems only support a subset of these (or none at all).
  vtkGetStringMacro(ToolType);
  vtkGetStringMacro(ToolRevision);
  vtkGetStringMacro(ToolManufacturer);
  vtkGetStringMacro(ToolPartNumber);
  vtkGetStringMacro(ToolSerialNumber);

  //BTX
  // Description:
  // To be used only by the vtkTracker class
  void SetTracker(vtkTracker *tracker);
  vtkSetMacro(ToolPort, int);
  vtkSetStringMacro(ToolType);
  vtkSetStringMacro(ToolRevision);
  vtkSetStringMacro(ToolManufacturer);
  vtkSetStringMacro(ToolPartNumber);
  vtkSetStringMacro(ToolSerialNumber);

  void LockUpdate();
  void UnlockUpdate();
  void Update();
  void NewMatrixAvailable();
  //ETX

  //BTX
  // Description:
  // This is obsolete since threading was added, but I'm
  // leaving it in just in case I change my mind.
  void UpdateAndCalibrate(vtkMatrix4x4 *matrix, long flags);
  //ETX

protected:
  vtkTrackerTool();
  ~vtkTrackerTool();

  vtkTracker *Tracker;
  int ToolPort;
  vtkTransform *Transform;
  vtkMatrix4x4 *CalibrationMatrix;
  bool UpdateLocked;

  int Flags;

  double TimeStamp;

  int LED1;
  int LED2;
  int LED3;

  char *ToolType;
  char *ToolRevision;
  char *ToolSerialNumber;
  char *ToolPartNumber;
  char *ToolManufacturer;

  vtkTrackerBuffer *Buffer;

private:
  vtkTrackerTool(const vtkTrackerTool&);
  void operator=(const vtkTrackerTool&);  

  vtkMatrix4x4 *TempMatrix;
  vtkMatrix4x4 *RawMatrix;
};

#endif







