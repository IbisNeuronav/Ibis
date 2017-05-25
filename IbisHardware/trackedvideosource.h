/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __TrackedVideoSource_h_
#define __TrackedVideoSource_h_

#include <stdio.h>
#include "vtkObject.h"
#include <QObject>
#include "serializer.h"
#include "vtkGenericParam.h"
#include "vtkObjectCallback.h"
#include "ibistypes.h"

class Tracker;
class vtkMatrix4x4;
class vtkTransform;
class vtkImageData;
class vtkTrackedVideoBuffer;
class vtkVideoSource2;
class vtkVideoSource2Factory;
class vtkTrackerTool;

struct VideoSourceSettings
{
    VideoSourceSettings();
    virtual void Serialize( Serializer * serializer );

    QString VideoDeviceTypeName;
    int OutputFormat;
    typedef std::map< std::string, std::vector< vtkGenericParamValues > > SourceTypeSpecificSettingsCont;
    SourceTypeSpecificSettingsCont SourceTypeSpecificSettings;  // settings for different source types (v4l2, PointGrey, etc.)
};

// Description:
// This class is used to synchronize a tracker and a video source. When
// it gets the signal that there is a new video frame available, it looks
// at the timestamp of the frame. If its between the timestamps of 2
// already existing transformations, the transformations are interpolated
// to get the good one for the frame. Otherwise, we wait until next
// transformation arrives. This class also emits an event when a new
// frame and its transformation are available.
class TrackedVideoSource : public QObject, public vtkObject
{
    
Q_OBJECT

public:

    static TrackedVideoSource * New() { return new TrackedVideoSource; }

    vtkTypeMacro(TrackedVideoSource,vtkObject);

    TrackedVideoSource();
    ~TrackedVideoSource();
    
    virtual bool Serialize( Serializer * serializer );

    void SetSyncedTrackerTool( vtkTrackerTool * t );

    bool InitializeVideo();
    void ClearVideo();

    // Description:
    // Set/Get the TimeShift. TimeShift is the difference in timestamp of
    // tracking and video events that would have occured at the same time
    // in the real world. To get the tracking timestamp corresponding to
    // the video timestamp, you have to do: VideoTimeStamp - TimeShift.
    void SetTimeShift( double shift );
    double GetTimeShift();

    // Description:
    // Get the transformation of the US image and the image itself 
    vtkTransform * GetTransform();
    TrackerToolState GetState();
    vtkImageData * GetVideoOutput();
    vtkVideoSource2 * GetSource() { return m_source; }

    // Description:
    // Freeze tracker tool transform by averaging a couple of them
    bool IsTransformFrozen();
    void FreezeTransform( int nbFrames );
    void UnFreezeTransform();
    
    // Allows for more control over when the tracker and the video source are updated
    // This is useful in order to make sure that update is called only once per iteration
    // of the event loop
    void LockUpdates();
    void UnlockUpdates();

    void Update();


    //-------------------------------------------------------------------------------------
    // VideoSource settings
    int GetFrameWidth();
    int GetFrameHeight();

    double GetFrameRate();

    void AddClient();
    void RemoveClient();
    bool IsCapturing() { return m_numberOfClients > 0; }

    int GetNumberOfDeviceTypes();
    QString GetDeviceTypeName( int index );
    int GetCurrentDeviceTypeIndex();
    QString GetCurrentDeviceTypeName();
    void SetCurrentDeviceType( QString deviceType );

    int OutputFormatIsLuminance();
    int OutputFormatIsRGB();
    int GetOutputFormat();
    void SetOutputFormatToLuminance();
    void SetOutputFormatToRGB();

    QWidget * CreateVideoSettingsDialog( QWidget * parent );
    QWidget * CreateVideoViewDialog( QWidget * parent );
    //-------------------------------------------------------------------------------------

signals:

    void OutputUpdatedSignal();

private slots:

    void OutputUpdatedSlot();
    
protected:
    
    bool SerializeWrite( Serializer * Serializer );
    bool SerializeRead( Serializer * Serializer );

    void BackupSourceTypeSpecificSettings();
    vtkTrackedVideoBuffer * GetCurrentBuffer();
    
    int m_numberOfClients;
    VideoSourceSettings m_videoSettings;
    QString VideoDeviceTypeName;
    vtkVideoSource2 * m_source;
    vtkVideoSource2Factory * m_sourceFactory;
    vtkObjectCallback<TrackedVideoSource> * m_newFrameCallback;

    vtkTrackedVideoBuffer * m_liveVideoBuffer;     // Buffer where we capture when there is no acquisition (just a couple of frames)

    // shift between tracker and video timestamps
    double m_timeShift;
};

ObjectSerializationHeaderMacro( TrackedVideoSource );
ObjectSerializationHeaderMacro( VideoSourceSettings );

#endif
