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
// .NAME vtkTrackedVideoBuffer - Video buffer with tracking data
// .SECTION Description
// Video buffer that also listens to the changes to a vtkTrackerBuffer.
// An update on this buffer doesn't output a new video frame until
// a transform matrix is available for it. It implements that by having
// read and write buffer index that are different.
// .SECTION See Also
// vtkVideoBuffer vtkVideoSource2 vtkV4L2VideoSource2

#ifndef __vtkTrackedVideoBuffer_h
#define __vtkTrackedVideoBuffer_h

#include <stdio.h>
#include "vtkVideoBuffer.h"
#include <vector>

class vtkMatrix4x4;
class vtkTrackerTool;
class vtkCommand;
class vtkUnsignedCharArray;
class vtkCriticalSection;
class vtkTransform;

class vtkTrackerToolTransform
{

public:

    vtkTrackerToolTransform();
    vtkTrackerToolTransform( const vtkTrackerToolTransform & toCopy );
    ~vtkTrackerToolTransform();
    void Reset();

    vtkMatrix4x4 * UncalibratedMatrix;
    int Flags;
};

class vtkTrackedVideoBuffer : public vtkVideoBuffer
{

public:

  static vtkTrackedVideoBuffer * New();
  vtkTypeMacro(vtkTrackedVideoBuffer,vtkVideoBuffer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set size of the frame buffer, i.e. the number of frames that
  // the 'tape' can store.
  virtual void SetFrameBufferSize(int FrameBufferSize);

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

  // Description:
  // Set all data relative to the current capture frame. This is used in the case where
  // a client application want to fill the buffer with frame data as oposed to SetCurrentFrameData,
  // which is used when recording live video with tracking data
  void SetFullFrameData( vtkImageData * data, double timestamp, int flags, vtkMatrix4x4 * uncalibratedMatrix );

  vtkGetObjectMacro( TrackerTool, vtkTrackerTool );
  void SetTrackerTool( vtkTrackerTool * tool );

  // Description:
  // Set/Get the TimeShift. TimeShift is the difference in timestamp of
  // tracking and video events that would have occured at the same time
  // in the real world. To get the tracking timestamp corresponding to
  // the video timestamp, you have to do: VideoTimeStamp - TimeShift.
  vtkSetMacro( TimeShift, double );
  vtkGetMacro( TimeShift, double );

  void SetCalibrationMatrix( vtkMatrix4x4 * mat );

  // Description:
  // Freezing the transform is used when the tracker tool is not moving
  // (ex.: camera attached to tripod). This class will then accumulate
  // numberOfFrames tracker matrices that are averaged together to
  // be used as each frame's matrix as long as the transform is frozen.
  void FreezeTransform( int numberOfFrames );
  void UnFreezeTransform();
  bool IsTransformFrozen() { return this->TransformFrozen; }
  
  // Description:
  // Get the transform of the current frame and the 
  // associated flags. These data are updated on
  // every pipeline execution.
  vtkGetObjectMacro( OutputTransform, vtkTransform );
  vtkGetObjectMacro( UncalibratedOutputTransform, vtkTransform );
  vtkGetObjectMacro( CalibrationTransform, vtkTransform );
  vtkGetMacro( Flags, int );
  bool IsCurrentFrameValid();
  bool AreFlagsValid( int flags );

protected:

  void AdvanceCaptureFrame();
  void AdvanceUpdateFrame();
  bool AdvanceFrame( int & frameIndex );

  vtkTrackedVideoBuffer();
  ~vtkTrackedVideoBuffer();

  // Description:
  // These methods can be overridden in subclasses
  virtual void UpdateFrameBuffer();

  // Description:
  // Reset all transforms in the buffer
  void ResetAllTransforms();
 
  // Description:
  // Update the OutputTransform during the parent's mutex
  // lock of the framebuffer. That way, we make sure the
  // frame index of the output is the same for the video
  // frame and the OutputTransform.
  virtual void InternalRequestData( vtkDataObject * data );

  // Description:
  // This is the tracker tool from which transform it taken
  vtkTrackerTool * TrackerTool;

  // Description:
  // TimeShift between the video and tracking
  double TimeShift;

  // Description:
  // Vars used to control transform when it is frozen
  bool TransformFrozen;
  int TotalFreezeMatrixCount;
  int FreezeMatrixCount;
  vtkMatrix4x4 * FreezeMatrixSum;
  vtkMatrix4x4 * FreezeMatrix;
  void ManageFrozenTransform( int index );

  // Description:
  // transforms associated with every video frame. The size of
  // this vector should be the same as the size of the video
  // buffer.
  typedef std::vector<vtkTrackerToolTransform> TransformVec;
  TransformVec TransformBuffer;
  bool VideoBufferFull;  // this is true when video buffer is full, but transforms might be missing
  
  // Description:
  // This is the transform that is outputed by the class. It
  // is updated automatically when Update is called to reflect
  // the current frame's real transform.
  vtkTransform * OutputTransform;
  vtkTransform * UncalibratedOutputTransform;
  vtkTransform * CalibrationTransform;
  
  // Flags of the current frame's transform. 
  int Flags;

private:

  vtkTrackedVideoBuffer(const vtkTrackedVideoBuffer&);  // Not implemented.
  void operator=(const vtkTrackedVideoBuffer&);  // Not implemented.
};

#endif





