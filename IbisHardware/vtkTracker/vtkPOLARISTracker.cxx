/*=========================================================================
 
        Program:   Visualization Toolkit
        Module:    vtkPOLARISTracker.cxx
 
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

#include <limits.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#include "polaris.h"
#include "polaris_math.h"
#include "vtkMath.h"
#include "vtkTimerLog.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkCriticalSection.h"
#include "vtkPOLARISTracker.h"
#include "vtkTrackerTool.h"
#include "vtkFrameToTimeConverter.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPOLARISTracker* vtkPOLARISTracker::New()
{
    // First try to create the object from the vtkObjectFactory
    vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPOLARISTracker");
    if(ret)
    {
        return (vtkPOLARISTracker*)ret;
    }
    // If the factory was unable to create the object, then create it here.
    return new vtkPOLARISTracker;
}

//----------------------------------------------------------------------------
vtkPOLARISTracker::vtkPOLARISTracker()
{
    this->Polaris = 0;
    this->Version = NULL;
    this->CommandReply[0] = '\0';
    this->SendMatrix = vtkMatrix4x4::New();
    this->IsPOLARISTracking = 0;
    this->SerialPort = -1; // default is to probe
    this->BaudRate = 57600; //115200;
    this->SetNumberOfTools(VTK_POLARIS_NTOOLS);
    this->SetNumberOfActiveTools(VTK_POLARIS_NACTIVETOOLS);

    for (int i = 0; i < VTK_POLARIS_NTOOLS; i++)
    {
        this->PortEnabled[i] = 0;
        this->VirtualSROM[i] = 0;
    }

    // for accurate timing
    this->Timer = vtkFrameToTimeConverter::New();
    this->Timer->SetNominalFrequency(60.0);
}

//----------------------------------------------------------------------------
vtkPOLARISTracker::~vtkPOLARISTracker()
{
    if (this->Tracking)
    {
        this->StopTracking();
    }
    this->SendMatrix->Delete();
    for (int i = 0; i < VTK_POLARIS_NTOOLS; i++)
    {
        if (this->VirtualSROM[i] != 0)
        {
            delete [] this->VirtualSROM[i];
        }
    }
    if (this->Version)
    {
        delete [] this->Version;
    }
    if (this->Timer)
    {
        this->Timer->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPOLARISTracker::PrintSelf(ostream& os, vtkIndent indent)
{
    vtkTracker::PrintSelf(os,indent);

    os << indent << "SendMatrix: " << this->SendMatrix << "\n";
    this->SendMatrix->PrintSelf(os,indent.GetNextIndent());
}

//----------------------------------------------------------------------------
int vtkPOLARISTracker::Probe()
{
    int errnum = PL_OPEN_ERROR;
    ;

    if (this->IsPOLARISTracking)
    {
        return 1;
    }

    // if SerialPort is set to -1, then probe all serial ports
    if (this->SerialPort < 0)
    {
        for (int i = 0; i < NDI_NUMBER_OF_DEVICES; i++) /* Anka, 2008-05-01 for USB to Serial Keyspan */
        {
            char *devicename = plDeviceName(i);
            if (devicename)
            {
                errnum = plProbe(devicename);
                if (errnum == PL_OKAY)
                {
                    this->SerialPort = i+1;
                    break;
                }
            }
        }
    }
    else // otherwise probe the specified serial port only
    {
        char *devicename = plDeviceName(this->SerialPort-1);
        if (devicename)
        {
            errnum = plProbe(devicename);
        }
    }

    // if probe was okay, then send VER:0 to identify device
    if (errnum == PL_OKAY)
    {
        this->Polaris = plOpen(plDeviceName(this->SerialPort-1));
        if (this->Polaris)
        {
            this->SetVersion(plVER(this->Polaris,0));
            plClose(this->Polaris);
            this->Polaris = 0;
        }

        return 1;
    }

    return 0;
}

//----------------------------------------------------------------------------
// Send a raw command to the tracking unit.
// If communication has already been opened with the POLARIS,
// then lock the mutex to get exclusive access and then
// send the command.
// Otherwise, open communication with the unit, send the command,
// and close communication.
char *vtkPOLARISTracker::Command(const char *command)
{
    this->CommandReply[0] = '\0';

    if (this->Polaris)
    {
        this->RequestUpdateMutex->Lock();
        this->UpdateMutex->Lock();
        this->RequestUpdateMutex->Unlock();
        strncpy(this->CommandReply, plCommand(this->Polaris, command),
                VTK_POLARIS_REPLY_LEN-1);
        this->CommandReply[VTK_POLARIS_REPLY_LEN-1] = '\0';
        this->UpdateMutex->Unlock();
    }
    else
    {
        this->Polaris = plOpen(plDeviceName(this->SerialPort-1));
        if (this->Polaris == 0)
        {
            vtkErrorMacro(<< plErrorString(PL_OPEN_ERROR));
        }
        else
        {
            strncpy(this->CommandReply, plCommand(this->Polaris, command),
                    VTK_POLARIS_REPLY_LEN-1);
            this->CommandReply[VTK_POLARIS_REPLY_LEN-1] = '\0';
            plClose(this->Polaris);
        }
        this->Polaris = 0;
    }

    return this->CommandReply;
}

//----------------------------------------------------------------------------
int vtkPOLARISTracker::InternalStartTracking()
{
    int errnum, tool;
    int baud;

    if (this->IsPOLARISTracking)
    {
        return 1;
    }

    switch (this->BaudRate)
    {
    case 9600:
        baud = PL_9600;
        break;
    case 14400:
        baud = PL_14400;
        break;
    case 19200:
        baud = PL_19200;
        break;
    case 38400:
        baud = PL_38400;
        break;
    case 57600:
        baud = PL_57600;
        break;
    case 115200:
        baud = PL_115200;
        break;
    default:
        vtkErrorMacro(<< "Illegal baud rate");
        return 0;
    }

    this->Polaris = plOpen(plDeviceName(this->SerialPort-1));
    if (this->Polaris == 0)
    {
        vtkErrorMacro(<< plErrorString(PL_OPEN_ERROR));
        return 0;
    }
    // reset before initializing Polaris
    plRESET(this->Polaris);
    errnum = plGetError(this->Polaris);
    if (errnum == PL_OKAY)
    {
        // initialize Polaris
        plINIT(this->Polaris);
        errnum = plGetError(this->Polaris);
    }
    if (errnum)
    { // try again
        plRESET(this->Polaris);
        errnum = plGetError(this->Polaris);
        if (errnum)
        {
            vtkErrorMacro(<< plErrorString(errnum));
            plClose(this->Polaris);
            this->Polaris = 0;
            return 0;
        }
        plINIT(this->Polaris);
        if (errnum)
        {
            vtkErrorMacro(<< plErrorString(errnum));
            plClose(this->Polaris);
            this->Polaris = 0;
            return 0;
        }
    }

    this->SetVersion(plVER(this->Polaris,0));
    plCOMM(this->Polaris,baud,PL_8N1,PL_HANDSHAKE);
    errnum = plGetError(this->Polaris);
    if (errnum)
    {
        vtkErrorMacro(<< plErrorString(errnum));
        plClose(this->Polaris);
        this->Polaris = 0;
        return 0;
    }

    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        if (VirtualSROM[tool])
        {
            this->InternalLoadVirtualSROM(tool,VirtualSROM[tool]);
        }
    }

    
    this->EnableToolPorts();

    plTSTART(this->Polaris);
    errnum = plGetError(this->Polaris);
    if (errnum)
    {
        vtkErrorMacro(<< plErrorString(errnum));
        plClose(this->Polaris);
        this->Polaris = 0;
        return 0;
    }

    int passive = PL_PASSIVE|PL_PASSIVE_EXTRA;
    if (this->Version[0] == 'A') // if Aurora, no passive
    {
        passive = 0;
    }
    // prime the system by sending an initial GX command
    plGX(this->Polaris,PL_XFORMS_AND_STATUS|PL_FRAME_NUMBER|passive);

    // for accurate timing
    this->Timer->Initialize();

    this->IsPOLARISTracking = 1;

    return 1;
}

//----------------------------------------------------------------------------
int vtkPOLARISTracker::InternalStopTracking()
{
    if (this->Polaris == 0)
    {
        return 0;
    }

    int errnum, tool;

    plTSTOP(this->Polaris);
    errnum = plGetError(this->Polaris);
    if (errnum)
    {
        vtkErrorMacro(<< plErrorString(errnum));
    }
    this->IsPOLARISTracking = 0;

    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        if (VirtualSROM[tool])
        {
            this->ClearVirtualSROM(tool);
        }
    }

    this->DisableToolPorts();

    // return to default comm settings
    plCOMM(this->Polaris,PL_9600,PL_8N1,PL_NOHANDSHAKE);
    errnum = plGetError(this->Polaris);
    if (errnum)
    {
        vtkErrorMacro(<< plErrorString(errnum));
    }
    plRESET(this->Polaris);
    plClose(this->Polaris);
    this->Polaris = 0;

    return 1;
}

//----------------------------------------------------------------------------
// Important notes on the data collection rate of the POLARIS:
//
// The camera frame rate is 60Hz, and therefore the maximum data
// collection rate is also 60Hz.  The maximum data transfer rate
// to the computer is also 60Hz.
//
// Depending on the number of enabled tools, the data collection
// rate might be reduced.  Each of the active tools requires one
// camera frame, and all the passive tools (if any are enabled)
// collectively require one camera frame.
//
// Therefore if there are two enabled active tools, the data rate
// is reduced to 30Hz.  Ditto for an active tool and a passive tool.
// If all tools are passive, the data rate is 60Hz.  With 3 active
// tools and one or more passive tools, the data rate is 15Hz.
// With 3 active tools, or 2 active and one or more passive tools,
// the data rate is 20Hz.
//
// The data transfer rate to the computer is independent of the data
// collection rate, and there might be duplicated records.  The
// data tranfer rate is limited by the speed of the serial port
// and by the number of characters sent per data record.  If tools
// are marked as 'missing' then the number of characters that
// are sent will be reduced.

void vtkPOLARISTracker::InternalUpdate()
{
    int errnum, tool;
    int status[VTK_POLARIS_NTOOLS];
    int absent[VTK_POLARIS_NTOOLS];
    unsigned long frame[VTK_POLARIS_NTOOLS];
    double transform[VTK_POLARIS_NTOOLS][8];
    double *referenceTransform = 0;
    long flags;
    const unsigned long mflags = PL_TOOL_IN_PORT | PL_INITIALIZED | PL_ENABLED;

    if (!this->IsPOLARISTracking)
    {
        vtkWarningMacro( << "called Update() when POLARIS was not tracking");
        return;
    }

    // check to see if passive ports are being used
    int passive = 0;
    if (this->Version[0] != 'A') // only if not Aurora
    {
        for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
        {
            if (this->VirtualSROM[tool])
            {
                if (tool >= 3)
                {
                    passive |= PL_PASSIVE;
                }
                if (tool >= 6)
                {
                    passive |= PL_PASSIVE_EXTRA;
                }
            }
        }
    }

    
    // get the transforms for all tools from the POLARIS
    plGX(this->Polaris,PL_XFORMS_AND_STATUS|PL_FRAME_NUMBER|passive);
    errnum = plGetError(this->Polaris);

    if (errnum)
    {
        if (errnum == PL_BAD_CRC)  // CRC errors are common
        {
            vtkWarningMacro(<< plErrorString(errnum));
        }
        else
        {
            vtkErrorMacro(<< plErrorString(errnum));
        }
        return;
    }

    // default to incrementing frame count by one (in case there are
    // no transforms for any tools)
    unsigned long nextcount = 0;

    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        int port = ((tool < 3) ? ('1' + tool) : ('A' + tool - 3));
        absent[tool] = plGetGXTransform(this->Polaris, port, transform[tool]);
        status[tool] = plGetGXPortStatus(this->Polaris, port);
        frame[tool] = plGetGXFrame(this->Polaris, port);
        if (!absent[tool] && frame[tool] > nextcount)
        { // 'nextcount' is max frame number returned
            nextcount = frame[tool];
        }
    }
    

    // if no transforms were returned, advance frame count by 1
    // (assume the POLARIS will be returning the empty records at
    // its maximum reporting rate of 60Hz)
    if (nextcount == 0)
    {
        nextcount = this->Timer->GetLastFrame() + 1;
    }

    // the timestamp is always created using the frame number of
    // the most recent transformation
    this->Timer->SetLastFrame(nextcount);
    double timestamp = this->Timer->GetTimeStampForFrame(nextcount);

    // check to see if any tools have been plugged in
    int need_enable = 0;
    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        need_enable |= ((status[tool] & PL_TOOL_IN_PORT) &&
                        !this->PortEnabled[tool]);
    }

    if (need_enable)
    { // re-configure, a new tool has been plugged in
        this->EnableToolPorts();
    }
    else
    {
        for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
        {
            this->PortEnabled[tool] = ((status[tool] & mflags) == mflags);
        }
    }

    if (this->ReferenceTool >= 0)
    { // copy reference tool transform
        referenceTransform = transform[this->ReferenceTool];
    }

    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        // convert status flags from POLARIS to vtkTracker format
        int port_status = status[tool];
        flags = 0;
        if ((port_status & mflags) != mflags)
        {
            flags |= TR_MISSING;
        }
        else
        {
            if (absent[tool])
            {
                flags |= TR_OUT_OF_VIEW;
            }
            if (port_status & PL_OUT_OF_VOLUME)
            {
                flags |= TR_OUT_OF_VOLUME;
            }
            if (port_status & PL_SWITCH_1_ON)
            {
                flags |= TR_SWITCH1_IS_ON;
            }
            if (port_status & PL_SWITCH_2_ON)
            {
                flags |= TR_SWITCH2_IS_ON;
            }
            if (port_status & PL_SWITCH_3_ON)
            {
                flags |= TR_SWITCH3_IS_ON;
            }
        }

        // if tracking relative to another tool
        if (this->ReferenceTool >= 0 && tool != this->ReferenceTool)
        {
            if (!absent[tool])
            {
                if (absent[this->ReferenceTool])
                {
                    flags |= TR_OUT_OF_VIEW;
                }
                if (status[this->ReferenceTool] & PL_OUT_OF_VOLUME)
                {
                    flags |= TR_OUT_OF_VOLUME;
                }
            }
            // pre-multiply transform by inverse of relative tool transform
            plRelativeTransform(transform[tool],referenceTransform,transform[tool]);
        }
        plTransformToMatrixd(transform[tool],*this->SendMatrix->Element);
        this->SendMatrix->Transpose();

        // by default (if there is no camera frame number associated with
        // the tool transformation) the most recent timestamp is used.
        double tooltimestamp = timestamp;
        if (!absent[tool] && frame[tool])
        {
            // this will create a timestamp from the frame number
            tooltimestamp = this->Timer->GetTimeStampForFrame(frame[tool]);
        }
        // send the matrix and flags to the tool
        this->ToolUpdate(tool,this->SendMatrix,flags,tooltimestamp);
    }
}

//----------------------------------------------------------------------------
void vtkPOLARISTracker::LoadVirtualSROM(int tool, const char *filename)
{
    FILE *file = fopen(filename,"rb");
    if (file == NULL)
    {
        vtkErrorMacro("couldn't find srom file " << filename);
        return;
    }

    if (this->VirtualSROM[tool] == 0)
    {
        this->VirtualSROM[tool] = new unsigned char[1024];
    }

    memset(this->VirtualSROM[tool],0,1024);
    fread(this->VirtualSROM[tool],1,1024,file);

    fclose(file);

    if (this->Tracking)
    {
        this->RequestUpdateMutex->Lock();
        this->UpdateMutex->Lock();
        this->RequestUpdateMutex->Unlock();
        if (this->IsPOLARISTracking)
        {
            plTSTOP(this->Polaris);
        }
        this->InternalLoadVirtualSROM(tool,this->VirtualSROM[tool]);
        if (this->IsPOLARISTracking)
        {
            plTSTART(this->Polaris);
        }
        this->UpdateMutex->Unlock();
    }
}

//----------------------------------------------------------------------------
void vtkPOLARISTracker::ClearVirtualSROM(int tool)
{
    if (this->VirtualSROM[tool] != 0)
    {
        delete [] this->VirtualSROM[tool];
    }

    this->VirtualSROM[tool] = 0;

    if (this->Tracking)
    {
        this->RequestUpdateMutex->Lock(); 
        this->UpdateMutex->Lock();
        this->RequestUpdateMutex->Unlock();
        if (this->IsPOLARISTracking)
        {
            plTSTOP(this->Polaris);
        }
        this->InternalClearVirtualSROM(tool);
        if (this->IsPOLARISTracking)
        {
            plTSTART(this->Polaris);
        }
        this->UpdateMutex->Unlock();
    }
}

//----------------------------------------------------------------------------
int vtkPOLARISTracker::GetAvailablePassivePort()
{
    for( int i = this->NumberOfActiveTools; i < this->NumberOfTools; i++ )
    {
        if( this->VirtualSROM[i] == 0 )
        {
            return i;
        }
    }
    return -1;
}

//----------------------------------------------------------------------------
// Protected Methods

// helper method to strip whitespace
static char *vtkStripWhitespace(char *text)
{
    int n = strlen(text);
    // strip from right
    while (--n >= 0)
    {
        if (isspace(text[n]))
        {
            text[n] = '\0';
        }
        else
        {
            break;
        }
    }
    // strip from left
    while (isspace(*text))
    {
        text++;
    }
    return text;
}

//----------------------------------------------------------------------------
// Enable all tool ports that have tools plugged into them.
// The reference port is enabled with PL_STATIC.
void vtkPOLARISTracker::EnableToolPorts()
{
    int errnum = 0;
    int tool;
    int status;
    char identity[32];
    char partNumber[24];

    // reset our information about the tool ports
    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        this->PortEnabled[tool] = 0;
    }

    // stop tracking
    if (this->IsPOLARISTracking)
    {
        plTSTOP(this->Polaris);
        errnum = plGetError(this->Polaris);
        if (errnum)
        {
            vtkErrorMacro(<< plErrorString(errnum));
        }
    }

    // get status of all ports
    int passive = PL_PASSIVE|PL_PASSIVE_EXTRA;
    if (this->Version[0] == 'A') // if Aurora, no passive
    {
        passive = 0;
    }
    plPSTAT(this->Polaris,PL_BASIC|passive);
    errnum = plGetError(this->Polaris);
    if (errnum)
    {
        vtkErrorMacro(<< plErrorString(errnum));
        return;
    }

    // check to see if any new tools have been plugged in
    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        int port = ((tool < 3) ? ('1' + tool) : ('A' + tool - 3));
        status = plGetPSTATPortStatus(this->Polaris,port);

        // if a new tool has been plugged in, enable it
        if ((status & PL_TOOL_IN_PORT) && !(status & PL_ENABLED))
        {
            plPINIT(this->Polaris,port);
            errnum = plGetError(this->Polaris);
            if (errnum)
            {
                vtkErrorMacro(<< plErrorString(errnum));
            }
            plPSTAT(this->Polaris,PL_BASIC|passive);
            errnum = plGetError(this->Polaris);
            if (errnum)
            {
                vtkErrorMacro(<< plErrorString(errnum));
                return;
            }
            plGetPSTATToolInfo(this->Polaris, port, identity);
            // use static tracking for the reference tool
            int mode = ((tool == this->ReferenceTool) ? PL_STATIC : PL_DYNAMIC );
            // check whether tool is of type button-box
            if (identity[0] == '3')
            {
                mode = PL_BUTTON_BOX;
            }
            plPENA(this->Polaris,port,mode);
            errnum = plGetError(this->Polaris);
            if (errnum)
            {
                vtkErrorMacro(<< plErrorString(errnum));
            }

            // turn on all LEDs that the user has requested
            if (this->Tools[tool]->GetLED1())
            {
                this->InternalSetToolLED(tool,1,this->Tools[tool]->GetLED1());
            }
            if (this->Tools[tool]->GetLED2())
            {
                this->InternalSetToolLED(tool,2,this->Tools[tool]->GetLED2());
            }
            if (this->Tools[tool]->GetLED3())
            {
                this->InternalSetToolLED(tool,3,this->Tools[tool]->GetLED3());
            }
        }
    }

    // re-scan all of the ports
    plPSTAT(this->Polaris,PL_BASIC|PL_PART_NUMBER|passive);
    errnum = plGetError(this->Polaris);
    if (errnum)
    {
        vtkErrorMacro(<< plErrorString(errnum));
        return;
    }

    // update the tool information from all ports
    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        int port = ((tool < 3) ? ('1' + tool) : ('A' + tool - 3));

        status = plGetPSTATPortStatus(this->Polaris,port);
        this->PortEnabled[tool] = ((status & PL_ENABLED) != 0);

        // decompose identity string from end to front
        plGetPSTATToolInfo(this->Polaris, port, identity);
        identity[30] = '\0';
        this->Tools[tool]->SetToolSerialNumber(vtkStripWhitespace(&identity[22]));
        identity[22] = '\0';
        this->Tools[tool]->SetToolRevision(vtkStripWhitespace(&identity[19]));
        identity[19] = '\0';
        this->Tools[tool]->SetToolManufacturer(vtkStripWhitespace(&identity[7]));
        identity[7] = '\0';
        this->Tools[tool]->SetToolType(vtkStripWhitespace(&identity[0]));

        plGetPSTATPartNumber(this->Polaris, port, partNumber);
        partNumber[20] = '\0';
        this->Tools[tool]->SetToolPartNumber(vtkStripWhitespace(partNumber));
    }

    // re-start the tracking
    if (this->IsPOLARISTracking)
    {
        plTSTART(this->Polaris);
        errnum = plGetError(this->Polaris);
        if (errnum)
        {
            vtkErrorMacro(<< plErrorString(errnum));
        }
        // prime the system by sending an initial GX command
        plGX(this->Polaris,PL_XFORMS_AND_STATUS|PL_FRAME_NUMBER|passive);
    }
}

//----------------------------------------------------------------------------
// Disable all enabled tool ports.
void vtkPOLARISTracker::DisableToolPorts()
{
    int errnum = 0;
    int tool;
    int status;

    // stop tracking
    if (this->IsPOLARISTracking)
    {
        plTSTOP(this->Polaris);
        errnum = plGetError(this->Polaris);
        if (errnum)
        {
            vtkErrorMacro(<< plErrorString(errnum));
        }
    }

    int passive = PL_PASSIVE|PL_PASSIVE_EXTRA;
    if (this->Version[0] == 'A') // if Aurora, no passive
    {
        passive = 0;
    }
    // get status of all ports
    plPSTAT(this->Polaris,PL_BASIC|passive);
    errnum = plGetError(this->Polaris);
    if (errnum)
    {
        vtkErrorMacro(<< plErrorString(errnum));
        return;
    }

    // disable the enabled ports
    for (tool = 0; tool < VTK_POLARIS_NTOOLS; tool++)
    {
        int port = ((tool < 3) ? ('1' + tool) : ('A' + tool - 3));

        status = plGetPSTATPortStatus(this->Polaris,port);
        if ((status & PL_TOOL_IN_PORT) && (status & PL_ENABLED))
        {
            plPDIS(this->Polaris, port);
            errnum = plGetError(this->Polaris);
            if (errnum)
            {
                vtkErrorMacro(<< plErrorString(errnum));
            }
        }

        this->PortEnabled[tool] = 0;
    }

    // re-start the tracking
    if (this->IsPOLARISTracking)
    {
        plTSTART(this->Polaris);
        errnum = plGetError(this->Polaris);
        if (errnum)
        {
            vtkErrorMacro(<< plErrorString(errnum));
        }
    }
}

//----------------------------------------------------------------------------
// cause the POLARIS to beep
int vtkPOLARISTracker::InternalBeep(int n)
{
    int errnum;

    if (n > 9)
    {
        n = 9;
    }
    if (n < 0)
    {
        n = 0;
    }

    if (this->Tracking)
    {
        plBEEP(this->Polaris,n);
        errnum = plGetError(this->Polaris);
        /*
        if (errnum && errnum != PL_NO_TOOL)
          {
          vtkErrorMacro(<< plErrorString(errnum));
          return 0;
          }
        */
    }

    return 1;
}

//----------------------------------------------------------------------------
// change the state of an LED on the tool
int vtkPOLARISTracker::InternalSetToolLED(int tool, int led, int state)
{
    int plstate = PL_BLANK;
    int errnum;

    switch (state)
    {
    case 0:
        plstate = PL_BLANK;
        break;
    case 1:
        plstate = PL_SOLID;
        break;
    case 2:
        plstate = PL_FLASH;
        break;
    }

    if (this->Tracking && tool >= 0 && tool < 3 && led > 0 && led < 3)
    {
        int port = ((tool < 3) ? ('1' + tool) : ('A' + tool - 3));

        plLED(this->Polaris,port,led+1,plstate);
        errnum = plGetError(this->Polaris);
        /*
        if (errnum && errnum != PL_NO_TOOL)
          {
          vtkErrorMacro(<< plErrorString(errnum));
          return 0;
          }
        */
    }

    return 1;
}

//----------------------------------------------------------------------------
void vtkPOLARISTracker::InternalLoadVirtualSROM(int tool,
        const unsigned char data[1024])
{
    if (data == NULL)
    {
        return;
    }

    int port = ((tool < 3) ? ('1' + tool) : ('A' + tool - 3));
    char hexbuffer[128];
    for (int i = 0; i < 1024; i += 64)
    {
        plPVWR(this->Polaris, port, i, plHexEncode(hexbuffer, &data[i], 64));
    }
}

//----------------------------------------------------------------------------
void vtkPOLARISTracker::InternalClearVirtualSROM(int tool)
{
    int port = ((tool < 3) ? ('1' + tool) : ('A' + tool - 3));
    plPVCLR(this->Polaris, port);
}

