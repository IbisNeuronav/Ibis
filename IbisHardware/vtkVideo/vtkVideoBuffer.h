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

// .NAME vtkVideoBuffer - SuperClass for video buffers in vtk.
// .SECTION Description
// vtkVideoBuffer is a superclass for video buffers for VTK. Any stream
// of 2D images can be contained in that buffer. This buffer is also
// used to hold images grabbed by vtkVideoSource2 and its subclasses.
// .SECTION See Also
// vtkVideoSource2 vtkV4L2VideoSource2

#ifndef __vtkVideoBuffer_h
#define __vtkVideoBuffer_h

#include "vtkImageAlgorithm.h"
#include "vtkCommand.h"

class vtkTimerLog;
class vtkCriticalSection;
class vtkMultiThreader;
class vtkInformation;
class vtkInformationVector;

#define vtkBufferFullEvent ((vtkCommand::EventIds)(vtkCommand::UserEvent + 1))

// Description:
// Utility class used to be able to change the way the
// buffer is unpacking images into vtkImageData. The way
// the image is stored in the buffer depends on the
// frame grabber that is being used.
class vtkUnpacker : public vtkObject
{

 public:

    static vtkUnpacker * New() { return new vtkUnpacker; }
    virtual void UnpackRasterLine( char *outPtr, char *rowPtr, int start, int count, int numberOfScalarComponents, double opacity, int outputFormat );

 protected:

    vtkUnpacker() {}
    ~vtkUnpacker() {}
};


class vtkVideoBuffer : public vtkImageAlgorithm
{

public:

  static vtkVideoBuffer * New();
  vtkTypeMacro(vtkVideoBuffer,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Play through the 'tape' sequentially at the specified frame rate.
  // If you have just finished Recoding, you should call Rewind() first.
  virtual void Play();

  // Description:
  // Stop recording or playing.
  virtual void Stop();

  // Description:
  // Seek forwards or backwards by the specified number of frames
  // (positive is forward, negative is backward).
  virtual void Seek(int n);

  // Description:
  // Seek forwards or backwards to the specified frame number
  // (positive is forward, negative is backward).
  virtual void GoToFrame(int n);

  void GoToLastRecordedFrame();

  // Description:
  // Are we in play mode?
  vtkGetMacro(Playing,int);

  // Description:
  // Set the full-frame size.  This must be an allowed size for the device,
  // the device may either refuse a request for an illegal frame size or
  // automatically choose a new frame size.
  // The default is usually 320x240x1, but can be device specific.
  // The 'depth' should always be 1 (unless you have a device that
  // can handle 3D acquisition).
  virtual void SetFrameSize(int x, int y, int z);
  virtual void SetFrameSize(int dim[3]) { this->SetFrameSize(dim[0], dim[1], dim[2]); };
  vtkGetVector3Macro(FrameSize,int);

  // Description:
  // Request a particular frame rate (default 30 frames per second).
  virtual void SetFrameRate(float rate);
  vtkGetMacro(FrameRate,float);

  // Description:
  // Set the output format.  This must be appropriate for device,
  // usually only VTK_LUMINANCE, VTK_RGB, and VTK_RGBA are supported.
  virtual void SetOutputFormat(int format);
  void SetOutputFormatToLuminance() { this->SetOutputFormat(VTK_LUMINANCE); };
  void SetOutputFormatToRGB() { this->SetOutputFormat(VTK_RGB); };
  void SetOutputFormatToRGBA() { this->SetOutputFormat(VTK_RGBA); };
  vtkGetMacro(OutputFormat,int);

  // Description:
  // Set size of the frame buffer, i.e. the number of frames that
  // the 'tape' can store.
  virtual void SetFrameBufferSize(int FrameBufferSize);
  vtkGetMacro(FrameBufferSize,int);

  // Description:
  // Set the number of frames to copy to the output on each execute.
  // The frames will be concatenated along the Z dimension, with the
  // most recent frame first.
  // Default: 1
  vtkSetMacro(NumberOfOutputFrames,int);
  vtkGetMacro(NumberOfOutputFrames,int);

  // Description:
  // Get/Set the WholeExtent of the output.  This can be used to either
  // clip or pad the video frame.  This clipping/padding is done when
  // the frame is copied to the output, and does not change the contents
  // of the framebuffer.  This is useful e.g. for expanding
  // the output size to a power of two for texture mapping.  The
  // default is (0,-1,0,-1,0,-1) which causes the entire frame to be
  // copied to the output.
  vtkSetVector6Macro(OutputWholeExtent,int);
  vtkGetVector6Macro(OutputWholeExtent,int);

  // Description:
  // Set/Get the pixel spacing.
  // Default: (1.0,1.0,1.0)
  vtkSetVector3Macro(DataSpacing,double);
  vtkGetVector3Macro(DataSpacing,double);

  // Description:
  // Set/Get the coordinates of the lower, left corner of the frame.
  // Default: (0.0,0.0,0.0)
  vtkSetVector3Macro(DataOrigin,double);
  vtkGetVector3Macro(DataOrigin,double);

  void SetClipRegion(int x0, int x1, int y0, int y1,int z0, int z1);

  // Description:
  // For RGBA output only (4 scalar components), set the opacity.  This
  // will not modify the existing contents of the framebuffer, only
  // subsequently grabbed frames.
  vtkSetMacro(Opacity,float);
  vtkGetMacro(Opacity,float);

  // Description:
  // This value is incremented each time a frame is grabbed.
  // reset it to zero (or any other value) at any time.
  vtkGetMacro(FrameCount, int);
  vtkSetMacro(FrameCount, int);

  // Description:
  // Get the frame index relative to the 'beginning of the tape'.  This
  // value wraps back to zero if it increases past the FrameBufferSize.
  int GetCurrentFrameIndex() { return CurrentUpdateIndex; }

  // Description:
  // Get a time stamp in seconds (resolution of milliseconds) for
  // a video frame.   Time began on Jan 1, 1970.  You can specify
  // a number (negative or positive) to specify the position of the
  // video frame relative to the current frame.
  virtual double GetFrameTimeStamp(int frame);

  // Description:
  // Get a time stamp in seconds (resolution of milliseconds) for
  // the Output.  Time began on Jan 1, 1970.  This timestamp is only
  // valid after the Output has been Updated.
  double GetFrameTimeStamp() { return this->FrameTimeStamp; };

  // Description:
  // Initialize the hardware.  This is called automatically
  // on the first Update or Grab.
  virtual void Initialize();

  // Description:
  // Fill current buffer frame with noise. This function
  // is used by the generic vtkVideoSource2.
  virtual void FillCurrentFrameWithNoise( double timestamp );

  // Description:
  // This function is mainly used by the video grabbing classes to put
  // image data in the buffer. advance specifies if the buffer's current
  // frame should be advanced before copying the data. timestamp is the
  // timestamp of the frame who's data is set.
  virtual void SetCurrentFrameData( void * imageData, double timestamp );

  vtkSetObjectMacro(Unpacker,vtkUnpacker);

  vtkSetMacro(FlipFrames,int);
  vtkGetMacro(NumberOfScalarComponents,int);
  vtkSetMacro(LoopRecording,bool);

  void LockUpdate() { this->UpdateLocked = true; }
  void UnlockUpdate() { this->UpdateLocked = false; }

  bool IsBufferFull();

protected:

  vtkVideoBuffer();
  ~vtkVideoBuffer();
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void IncrementFrameCount();

  // Description:
  // The next 2 methods do the real job of FillCurrentFrameWithNoise and
  // SetCurrentFrameData but don't lock the framebuffer mutex. FrameBufferMutex
  // should be locked when they are called. The lock has been separated from
  // the real job so that derived classes can call they even if they have
  // already locked the framebuffer mutex.
  void InternalFillCurrentFrameWithNoise( double timestamp );
  void InternalSetCurrentFrameData( void * imageData, double timestamp );

  void LockBufferForModif();
  void UnlockBufferForModif();

  int Initialized;

  bool LoopRecording;
  int FrameSize[3];
  int ClipRegion[6];
  int OutputWholeExtent[6];
  double DataSpacing[3];
  double DataOrigin[3];
  int OutputFormat;
  // set according to the OutputFormat
  int NumberOfScalarComponents;
  // The FrameOutputExtent is the WholeExtent for a single output frame.
  // It is initialized in ExecuteInformation.
  int FrameOutputExtent[6];

  // save this information from the output so that we can see if the
  // output scalars have changed
  int LastNumberOfScalarComponents;
  int LastOutputExtent[6];

  int Playing;
  float FrameRate;
  int FrameCount;
  int CurrentCaptureIndex;
  int CurrentUpdateIndex;
  double FrameTimeStamp;
  bool UpdateLocked;

  int NumberOfOutputFrames;

  float Opacity;

  // true if Execute() must apply a vertical flip to each frame
  int FlipFrames;

  // set if output needs to be cleared to be cleared before being written
  int OutputNeedsInitialization;

  // Object that unpack buffer lines into vtkImageData
  vtkUnpacker * Unpacker;

  // An example of asynchrony
  vtkMultiThreader * PlayerThreader;
  int PlayerThreadId;

  // Mutex to protect the framebuffer. To modify the params of the buffer (not its content),
  // one needs to lock both. Typically, if you read from the buffer, you should lock the update mutex
  // and if you write to it, the capture mutex.
  vtkCriticalSection * UpdateMutex;
  vtkCriticalSection * CaptureMutex;

  // set according to the needs of the hardware:
  // number of bits per framebuffer pixel
  int FrameBufferBitsPerPixel;
  // byte alignment of each row in the framebuffer
  int FrameBufferRowAlignment;
  // FrameBufferExtent is the extent of frame after it has been clipped
  // with ClipRegion.  It is initialized in CheckBuffer(). AK - no function CheckBuffer() found
  int FrameBufferExtent[6];

  int FrameBufferSize;
  void **FrameBuffer;
  double *FrameBufferTimeStamps;

  // Description:
  // These methods can be overridden in subclasses
  virtual void UpdateFrameBuffer();
  virtual void AdvanceFrameBuffer();
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // This function is called ONLY within the framebuffer mutex
  // lock in Execute data. It should be used by subclasses that
  // which to update their output during the framebuffer mutex
  // lock but don't want to reimplement ExecuteData.
  virtual void InternalRequestData( vtkDataObject * data ) {};

  // if some component conversion is required, it is done here:
  void UnpackRasterLine(char *outPtr, char *rowPtr, int start, int count);

private:

  vtkVideoBuffer(const vtkVideoBuffer&);  // Not implemented.
  void operator=(const vtkVideoBuffer&);  // Not implemented.
};

#endif





