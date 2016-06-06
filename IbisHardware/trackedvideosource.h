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
#include <qobject.h>
#include "serializer.h"
#include <string>
#include "tracker.h"
#include <QVector>
#include "vtkGenericParam.h"
#include "vtkObjectCallback.h"
#include "cameraobject.h"

class Tracker;
class vtkMatrix4x4;
class vtkTransform;
class vtkImageData;
class vtkTrackedVideoBuffer;
class vtkVideoSource2;
class vtkVideoSource2Factory;
class USMask;


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

    TrackedVideoSource();
    ~TrackedVideoSource();
    
    virtual bool Serialize( Serializer * serializer );

    // Description:
    // Set the tracker tool to synchronize to video source. This must be done
    // before calling Initialize.
    void SetTracker( Tracker * track );
    Tracker *GetTracker() { return Track; }

    void InitializeVideo();
    void ClearVideo();

    // Description:
    // Get the name of the tracker tool that is attached to the us probe
    QString GetTrackerToolName() { return this->TrackerToolName; }
    void SetTrackerToolName( QString name );

    // Description:
    // Set/Get the TimeShift. TimeShift is the difference in timestamp of
    // tracking and video events that would have occured at the same time
    // in the real world. To get the tracking timestamp corresponding to
    // the video timestamp, you have to do: VideoTimeStamp - TimeShift.
    void SetTimeShift( double shift );
    double GetTimeShift();
    
    // Description:
    // Modify, Get and Set the scale factors that can be applied
    // to the calibration matrix of the ultrasound probe. 
    void AddCalibrationMatrix( QString name );
    void RemoveCalibrationMatrix( QString name );
    int GetNumberOfCalibrationMatrices();
    QString GetCalibrationMatrixName( int index );
    vtkMatrix4x4 * GetCalibrationMatrix( int index );
    int GetCalibrationMatrixIndex( QString name );
    
    // Description:
    // Set/Get the scale factor that is currently being used by the
    // assigned tool.
    void SetCurrentCalibrationMatrixName( QString factorName );
    QString GetCurrentCalibrationMatrixName();
    vtkMatrix4x4 * GetCurrentCalibrationMatrix();
    void SetCurrentCalibrationMatrix( vtkMatrix4x4 * mat );

    void SetCameraIntrinsicParams( const CameraIntrinsicParams & params ) { m_cameraIntrinsicParams = params; }
    const CameraIntrinsicParams & GetCameraIntrinsicParams() { return m_cameraIntrinsicParams; }

    // Description:
    // Get the transformation of the US image and the image itself 
    vtkTransform * GetTransform();
    vtkTransform * GetUncalibratedTransform();
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

    struct ScaleFactorInfo
    {
        ScaleFactorInfo();
        ScaleFactorInfo( const ScaleFactorInfo & other );
        ~ScaleFactorInfo();
        bool Serialize( Serializer * ser );
        QString name;
        vtkMatrix4x4 * matrix;
    };


    // mask
    USMask *GetDefaultMask() {return m_defaultMask;}

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

    void AssignedToolChanged();
    void OutputUpdatedSignal();
    
public slots:
    
    void ToolUseChanged( int );

private slots:

    void OutputUpdatedSlot();
    
protected:
    
    bool SerializeWrite( Serializer * Serializer );
    bool SerializeRead( Serializer * Serializer );

    int FindValidTrackerTool();
    void AssignTool();
    void CheckCalibrationValidity();
    void BackupSourceTypeSpecificSettings();
    vtkTrackedVideoBuffer * GetCurrentBuffer();
    
    // Description:
    // Compute the real calibration matrices of the ultrasound
    // probe by using the scale factor and scale center. This is to support legacy settings
    void ComputeScaleCalibrationMatrices( std::vector< std::pair<QString,double> > & scaleFactors, vtkMatrix4x4 * unscaledCalibrationMatrix, double scaleCenter[2] );

    int m_numberOfClients;
    VideoSourceSettings m_videoSettings;
    QString VideoDeviceTypeName;
    vtkVideoSource2 * m_source;
    vtkVideoSource2Factory * m_sourceFactory;
    vtkObjectCallback<TrackedVideoSource> * m_newFrameCallback;

    vtkTrackedVideoBuffer * m_liveVideoBuffer;     // Buffer where we capture when there is no acquisition (just a couple of frames)
    Tracker * Track;
    QString TrackerToolName;

    // Calibration matrices for different scale factors of the probe
    typedef std::vector< ScaleFactorInfo > ScaleFactorMatricesContainer;
    ScaleFactorMatricesContainer m_calibrationMatrices;
    QString m_currentCalibrationMatrixName;

    // camera intrinsic parameters (not used for US)
    CameraIntrinsicParams m_cameraIntrinsicParams;

    // shift between tracker and video timestamps
    double m_timeShift;

    // default mask used for this video source
    USMask *m_defaultMask;
};

ObjectSerializationHeaderMacro( TrackedVideoSource );
ObjectSerializationHeaderMacro( TrackedVideoSource::ScaleFactorInfo );
ObjectSerializationHeaderMacro( VideoSourceSettings );

#endif
