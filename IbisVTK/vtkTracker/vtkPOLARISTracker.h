/*=========================================================================

        Program:   Visualization Toolkit
        Module:    vtkPOLARISTracker.h

    David Gobbi 
    Imaging Research Laboratories
        John P. Robarts Research Institute
        Box 5015, 100 Perth Drive
    London, Ontario

        dgobbi@atamai.com
    (519)663-5777x34213

==========================================================================

Copyright 2000,2001 Atamai, Inc. 

=========================================================================*/
// .NAME vtkPOLARISTracker - VTK interface for Northern Digital's POLARIS
// .SECTION Description
// The vtkPOLARISTracker class provides an  interface to the POLARIS
// (Northern Digital Inc., Waterloo, Canada) optical tracking system.
// It also works with the AURORA magnetic tracking system, using the
// POLARIS API.
// .SECTION Caveats
// This class refers to ports 1,2,3,A,B,C as ports 0,1,2,3,4,5
// .SECTION see also
// vtkTrackerTool vtkFlockTracker


#ifndef __vtkPOLARISTracker_h
#define __vtkPOLARISTracker_h

#include "vtkTracker.h"
#include "polaris.h"

class vtkFrameToTimeConverter;

// the number of tools the polaris can handle
#define VTK_POLARIS_NTOOLS 12
#define VTK_POLARIS_NACTIVETOOLS 3
#define VTK_POLARIS_REPLY_LEN 512

class VTK_EXPORT vtkPOLARISTracker : public vtkTracker
{
public:

  static vtkPOLARISTracker *New();
  vtkTypeMacro(vtkPOLARISTracker,vtkTracker);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Probe to see if the tracking system is present on the
  // specified serial port.  If the SerialPort is set to -1,
  // then all serial ports will be checked.
  int Probe();

  // Description:
  // Send a command to the POLARIS in the format INIT: or VER:0 (the
  // command should include a colon).  Commands can only be done after
  // either Probe() or StartTracking() has been called.
  // The text reply from the POLARIS is returned, without the CRC or
  // final carriage return.
  char *Command(const char *command);

  // Description:
  // Get the a string (perhaps a long one) describing the type and version
  // of the device.
  vtkGetStringMacro(Version);

  // Description:
  // Set which serial port to use, 1 through 4.
  vtkSetMacro(SerialPort, int);
  vtkGetMacro(SerialPort, int);

  // Description:
  // Set the desired baud rate.  Default: 115200.  If CRC errors are
  // common, reduce to 57600.
  vtkSetMacro(BaudRate, int);
  vtkGetMacro(BaudRate, int);

  // Description:
  // Enable a passive tool by uploading a virtual SROM for that
  // tool, where 'tool' is a number between 0 and 5.
  void LoadVirtualSROM(int tool, const char *filename);
  void ClearVirtualSROM(int tool);
  
  // Description:
  // Implementation of virtual func. see parent class description.
  int GetAvailablePassivePort();

  // Description:
  // Get an update from the tracking system and push the new transforms
  // to the tools.  This should only be used within vtkTracker.cxx.
  void InternalUpdate();

protected:
  vtkPOLARISTracker();
  ~vtkPOLARISTracker();

  // Description:
  // Set the version information.
  vtkSetStringMacro(Version);

  // Description:
  // Start the tracking system.  The tracking system is brought from
  // its ground state into full tracking mode.  The POLARIS will
  // only be reset if communication cannot be established without
  // a reset.
  int InternalStartTracking();

  // Description:
  // Stop the tracking system and bring it back to its ground state:
  // Initialized, not tracking, at 9600 Baud.
  int InternalStopTracking();

  // Description:
  // Cause the POLARIS to beep the specified number of times.
  int InternalBeep(int n);

  // Description:
  // Set the specified tool LED to the specified state.
  int InternalSetToolLED(int tool, int led, int state);

  // Description:
  // This is a low-level method for loading a virtual SROM.
  // You must halt the tracking thread and take the POLARIS
  // out of tracking mode before you use it.
  void InternalLoadVirtualSROM(int tool, const unsigned char data[1024]);
  void InternalClearVirtualSROM(int tool);

  // Description:
  // Methods for detecting which ports have tools in them, and
  // auto-enabling those tools.
  void EnableToolPorts();
  void DisableToolPorts();

  // Description:
  // Class for updating the virtual clock that accurately times the
  // arrival of each transform, more accurately than is possible with
  // the system clock alone because the virtual clock averages out the
  // jitter.
  vtkFrameToTimeConverter *Timer;

  polaris *Polaris;
  char *Version;

  vtkMatrix4x4 *SendMatrix;
  int SerialPort; 
  int BaudRate;
  int IsPOLARISTracking;

  int PortEnabled[VTK_POLARIS_NTOOLS];
  unsigned char *VirtualSROM[VTK_POLARIS_NTOOLS];

  char CommandReply[VTK_POLARIS_REPLY_LEN];

private:
  vtkPOLARISTracker(const vtkPOLARISTracker&);
  void operator=(const vtkPOLARISTracker&);  
};

#endif





