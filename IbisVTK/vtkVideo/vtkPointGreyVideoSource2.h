// .NAME vtkPointGreyVideoSource2 - Frame grabber Point Grey camera
// .SECTION Description
// .SECTION See Also
// vtkVideoSource2

#ifndef __vtkPointGreyVideoSource2_h
#define __vtkPointGreyVideoSource2_h

#include "vtkVideoSource2.h"
#include <vector>
#include <utility>

namespace FlyCapture2
{
    class Camera;
    class Image;
};

class vtkPointGreyVideoSource2 : public vtkVideoSource2
{

public:

    static vtkPointGreyVideoSource2 * New();
    vtkTypeMacro( vtkPointGreyVideoSource2, vtkVideoSource2 );
    void PrintSelf( ostream & os, vtkIndent indent );

    // Description:
    // Standard VCR functionality: Record incoming video.
    virtual void Record();

    // Description:
    // Standard VCR functionality: Stop recording.
    virtual void Stop();

    // Description:
    // Grab a single video frame.
    virtual void Grab();

    // Description:
    // Initialize the driver (this is called automatically when the
    // first grab is done).
    virtual bool Initialize();

    // Description:
    // Free the driver (this is called automatically inside the
    // destructor).
    virtual void ReleaseSystemResources();

    // Description:
    // Set the video mode/frame rate combination
    void SetVideoModeAndFrameRateString( const char * );
    const char * GetVideoModeAndFrameRateString() { return m_videoModeAndFrameRate.c_str(); }
    void GetVideoModeAndFrameRateStrings( std::vector< std::string > & modeStrings );

protected:

    bool SetVideoModeAndFrameRate();

    // callbacks for when there is a new frame
    static void OnImageGrabbedStatic( FlyCapture2::Image * image, const void * callbackData );
    void OnImageGrabbed( FlyCapture2::Image * image );

    vtkPointGreyVideoSource2();
    ~vtkPointGreyVideoSource2();
    
    // Description:
    // Give the chance to sub-classes to react to a buffer change
    // by setting the new buffer's unpacker for example.
    virtual void InternalSetBuffer();

    std::string VideoModeAndFrameRateToString( int videoMode, int frameRate );
    bool StringToModeAndFrameRate( std::string s, int & videoMode, int & frameRate );
    void GatherValidVideoModesAndFrameRates();

    std::string m_videoModeAndFrameRate;
    FlyCapture2::Camera * m_camera;
    FlyCapture2::Image * m_convertedFrame;

    // All available video modes and frame rates gathered for the current camera.
    // Note: we use int instead of VideoMode and FrameRate enum to avoid poluting
    // the rest of the code with Flycapture includes. First int is VideoMode and Second is
    // FrameRate.
    struct ModeFrameRate
    {
        int vm;
        int fr;
        std::string ModeAndFrameRateString;
    };
    std::vector< ModeFrameRate > m_availableVideoModeAndFrameRates;

private:

    vtkPointGreyVideoSource2(const vtkPointGreyVideoSource2&);  // Not implemented.
    void operator=(const vtkPointGreyVideoSource2&);  // Not implemented.

};

#endif
