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

// .NAME vtkV4L2VideoSource2 - Video for linux 2 frame grabber for vtk
// .SECTION Description
// .SECTION See Also
// vtkVideoSource

#ifndef __vtkV4L2VideoSource2_h
#define __vtkV4L2VideoSource2_h

#include "vtkVideoSource2.h"
#include <asm/types.h>          // for videodev2.h
#include <linux/videodev2.h>
#include <vector>

// Video formats:
#define VTK_V4L2_STD_NTSC  0
#define VTK_V4L2_STD_PAL   1
#define VTK_V4L2_STD_SECAM 2


class vtkV4L2VideoInput : public vtkObject
{
    public:
        static vtkV4L2VideoInput * New() { return new vtkV4L2VideoInput; }
        vtkV4L2VideoInput() { Name = NULL; Description = NULL; Index = -1; }
        ~vtkV4L2VideoInput() { delete Name; delete Description; }

        vtkGetStringMacro(Name);
        vtkGetStringMacro(Description);
        vtkSetStringMacro(Name);
        vtkSetStringMacro(Description);
        vtkSetMacro(Index,int);
        vtkGetMacro(Index,int);

    protected:

        char * Name;
        char * Description;
        int Index;
};


class vtkV4L2VideoStandard : public vtkObject
{
    public:
        static vtkV4L2VideoStandard * New() { return new vtkV4L2VideoStandard; }
        vtkV4L2VideoStandard() { Name = NULL; Description = NULL; Standard = V4L2_STD_PAL_B; }
        ~vtkV4L2VideoStandard() { delete Name; delete Description; }

        vtkGetStringMacro(Name);
        vtkGetStringMacro(Description);
        vtkSetStringMacro(Name);
        vtkSetStringMacro(Description);

        v4l2_std_id Standard;

    protected:

        char * Name;
        char * Description;
};

// Description:
// This class implements the vtkVideoSource interface using the Video For
// Linux 2 (v4l2) api.
class vtkV4L2VideoSource2 : public vtkVideoSource2
{

public:

    static vtkV4L2VideoSource2 * New();
    vtkTypeMacro(vtkV4L2VideoSource2,vtkVideoSource2);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Standard VCR functionality: Record incoming video.
    virtual void Record();

    // Description:
    // Standard VCR functionality: Stop recording or playing.
    virtual void Stop();

    // Description:
    // Grab a single video frame.
    virtual void Grab();

    // Description:
    // Request a particular frame size (set the third value to 1).
    //virtual void SetFrameSize(int x, int y, int z);
    //virtual void SetFrameSize( int size[3] ) { this->SetFrameSize( size[0], size[1], size[2] ); }
    
    // Description:
    // Request a particular frame rate: default is 30. If Frame rate is different
    // Than the one specified by the video standard used, then it should be a
    // diviser of it. For example, for NTSC, 30 and 15 fps may be used. The 
    // reason for this is that frame rate control is implemented by dropping
    // Frames. V4L2 has a mechanism to change the frame rate, but it is not
    // supported by all drivers, so we decided to implement frame rate control
    // on the client side instead of asking the driver to do it.
    void ListAvailableFrameRates( std::vector< std::string > & rates );
    void SetFrameRateString( const char * rate );
    const char * GetFrameRateString();

    // Description:
    // A set of valid resolution is defined by the driver (ideally, but most of the time we use hardcoded
    // values that are known to work accross different drivers). These functions allow to choose the
    // resolution from the list.
    void ListAvailableResolutions( std::vector< std::string > & res );
    void SetResolutionString( const char * res  );
    const char * GetResolutionString();
    
    // Description:
    // Request a particular output format (default: VTK_RGB).
    virtual void SetOutputFormat(int format);

    // Description:
    // Initialize the driver (this is called automatically when the
    // first grab is done).
    virtual bool Initialize();
    void Cleanup();

    // Description:
    // Free the driver (this is called automatically inside the
    // destructor).
    virtual void ReleaseSystemResources();

    // These methods should not be used from outside the class
    virtual void InternalGrab();
    void StopCapturing();
    void StartCapturing( int singleBuffer );

    void ListAvailableVideoDevice( std::vector< std::string > & devs );
    void SetVideoDeviceName( const char * dev );
    const char * GetVideoDeviceName() { return this->DevName.c_str(); }

    void SetVideoInput( const char * inputName );
    const char * GetVideoInput();
    void ListAvailableVideoInput( std::vector< std::string > & inputs );

    void SetVideoStandard( const char * standardName );
    const char * GetVideoStandard() { return this->VideoStandard.c_str(); }
    void ListAvailableVideoStandards( std::vector< std::string > & standards );

protected:

    vtkV4L2VideoSource2();
    ~vtkV4L2VideoSource2();
    
    // Description:
    // Give the chance to sub-classes to react to a buffer change
    // by setting the new buffer's unpacker for example.
    virtual void InternalSetBuffer();

    // initializer helpers
    void OpenDevice();
    int QueryDeviceCap();
    int ListAvailableInput();
    int AssignVideoInput();
    int ListAvailableStandard();
    int FirstStandardThatMatches( v4l2_std_id stdId );
    int AssignVideoStandard();
    void ListAvailableFormats();
    void ResetCroping();
    int SetPixelFormat();
    int CreateMMapedBuffers();

    // De-init helpers
    void ClearAvailableInputs();
    void ClearAvailableStandards();
    int UnmapMemory();
    int CloseDevice();

    struct buffer
    {
        void   * start;
        size_t   length;
    };

    // Params (can be set before init)
    std::string DevName;
    std::string VideoInputName;
    std::string VideoStandard;

    int       VideoDev;
    int       NbBuffers;
    buffer  * Buffers;

    std::vector<vtkV4L2VideoInput *> AvailableInputs;
    std::vector<vtkV4L2VideoStandard *> AvailableStandards;

private:

    static int xioctl( int fd, int request, void * arg );

    vtkV4L2VideoSource2(const vtkV4L2VideoSource2&);  // Not implemented.
    void operator=(const vtkV4L2VideoSource2&);  // Not implemented.

};

#endif
