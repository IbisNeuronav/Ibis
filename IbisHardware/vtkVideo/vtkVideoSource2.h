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

// .NAME vtkVideoSource2 - Superclass of video input devices for VTK.
// .SECTION Description
// vtkVideoSource2 is a superclass for video input interfaces for VTK.
// The functionality and specification is the same as vtkVideoSource,
// but the vtkVideoSource2 as the advantage of having the buffer 
// separated from the video source.
// .SECTION Caveats
// You must call the ReleaseSystemResources() method before the application
// exits.  Otherwise the application might hang while trying to exit.
// .SECTION See Also
// vtkV4L2VideoSource2 vtkVideoSource

#ifndef __vtkVideoSource2_h
#define __vtkVideoSource2_h

#include "vtkImageAlgorithm.h"
#include "vtkGenericParam.h"
#include <vector>

class vtkTimerLog;
class vtkCriticalSection;
class vtkMultiThreader;
class vtkVideoBuffer;

class vtkVideoSource2 : public vtkObject, public vtkGenericParamInterface
{
    
public:
    
  static vtkVideoSource2 * New();
  vtkTypeMacro(vtkVideoSource2,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Record incoming video at the specified FrameRate.  The recording
  // continues indefinitely until Stop() is called. 
  virtual void Record();

  // Description:
  // Stop recording or playing.
  virtual void Stop();

  // Description:
  // Grab a single video frame.
  virtual void Grab();

  // Description:
  // Are we in record mode? (record mode and play mode are mutually
  // exclusive).
  vtkGetMacro(Recording,int);
  vtkGetMacro(FrameRate,double);
  vtkGetVector2Macro(FrameSize,int);

  // Description:
  // Set the output format.  This must be appropriate for device,
  // usually only VTK_LUMINANCE, VTK_RGB, and VTK_RGBA are supported.
  vtkGetMacro( OutputFormat, int );
  virtual void SetOutputFormat(int format);
  void SetOutputFormatToLuminance() { this->SetOutputFormat(VTK_LUMINANCE); };
  void SetOutputFormatToRGB() { this->SetOutputFormat(VTK_RGB); };
  void SetOutputFormatToRGBA() { this->SetOutputFormat(VTK_RGBA); };

  // Description:
  // Initialize the hardware.  This is called automatically
  // on the first Update or Grab.
  virtual bool Initialize();
  virtual int GetInitialized() { return this->Initialized; };

  // Description:
  // Release the video driver.  This method must be called before
  // application exit, or else the application might hang during
  // exit.  
  virtual void ReleaseSystemResources();

  // Description:
  // The internal function which actually does the grab.  You will
  // definitely want to override this if you develop a vtkVideoSource2
  // subclass. 
  virtual void InternalGrab();
  
  // Description:
  // Get/Set the buffer to be used to store video
  vtkGetObjectMacro(Buffer,vtkVideoBuffer);
  void SetBuffer( vtkVideoBuffer * buffer );
  bool IsBufferCompatible( vtkVideoBuffer * buf );

protected:
    
  vtkVideoSource2();
  ~vtkVideoSource2();

  void SetFrameRate( double rate );
  void SetFrameSize( int x, int y );
  void SetFrameSize( int size[2] ) { SetFrameSize( size[0], size[1] ); }
  
  // Description:
  // Give the chance to sub-classes to react to a buffer change
  // by setting the new buffer's unpacker for example.
  virtual void InternalSetBuffer() {}

  int Initialized;
  int Recording;

  // An example of asynchrony
  vtkMultiThreader * PlayerThreader;
  int PlayerThreadId;
  
  // Buffer containing the captured frames.
  int FrameSize[2];
  double FrameRate;
  int OutputFormat;
  vtkVideoBuffer * Buffer;

  // Dummy string we return for all unimplemented features of this generic class
  static const char * UnimplementedString;

private:
    
  vtkVideoSource2(const vtkVideoSource2&);  // Not implemented.
  void operator=(const vtkVideoSource2&);  // Not implemented.
};

#endif





