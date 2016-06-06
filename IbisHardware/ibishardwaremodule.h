/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __IbisHardwareModule_h_
#define __IbisHardwareModule_h_

#include "hardwaremodule.h"

class Tracker;
class TrackedVideoSource;
class TrackerToolDisplayManager;
class QMenu;

class IbisHardwareModule : public QObject, public HardwareModule
{
    
    Q_OBJECT
    Q_INTERFACES( HardwareModule )
    Q_PLUGIN_METADATA(IID "Ibis.IbisHardwareModule" )
    
public:

    static IbisHardwareModule * New() { return new IbisHardwareModule; }
    IbisHardwareModule();
    ~IbisHardwareModule();

    // Implementation of the HardwareModule interface
    virtual void AddSettingsMenuEntries( QMenu * menu );
    virtual QWidget * CreateTrackerStatusDialog( QWidget * parent );
    virtual bool Init( const char * filename = 0 );
    virtual void Update();
    virtual bool ShutDown();
    virtual void WriteHardwareConfig( const char * filename, bool backupOnly = false );

    virtual void AddObjectsToScene();
    virtual void RemoveObjectsFromScene();

    // Legacy methods : should be eliminated as we properly separate IbisLib and IbisHardware
    virtual bool CanCaptureTrackedVideo();
    virtual bool CanCaptureVideo();
    virtual int GetNavigationPointerObjectID();
    virtual vtkImageData * GetTrackedVideoOutput();
    virtual int GetVideoFrameWidth();
    virtual int GetVideoFrameHeight();
    virtual TrackerToolState GetVideoTrackerState();
    virtual vtkTransform * GetTrackedVideoTransform();
    virtual vtkTransform * GetTrackedVideoUncalibratedTransform();
    virtual vtkMatrix4x4 * GetVideoCalibrationMatrix();
    virtual void SetVideoCalibrationMatrix( vtkMatrix4x4 * mat );
    virtual int GetNumberOfVideoCalibrationMatrices();
    virtual QString GetVideoCalibrationMatrixName( int index );
    virtual void SetCurrentVideoCalibrationMatrixName( QString name );
    virtual QString GetCurrentVideoCalibrationMatrixName();
    virtual bool IsVideoTransformFrozen();
    virtual void FreezeVideoTransform( int nbSamples );
    virtual void UnFreezeVideoTransform();
    virtual const CameraIntrinsicParams & GetCameraIntrinsicParams();
    virtual void SetCameraIntrinsicParams( CameraIntrinsicParams & p );
    virtual void AddTrackedVideoClient();
    virtual void RemoveTrackedVideoClient();
    virtual int GetReferenceToolIndex();
    virtual vtkTransform * GetTrackerToolTransform( int toolIndex );
    virtual TrackerToolState GetTrackerToolState( int toolIndex );

private slots:

    void OpenVideoSettingsDialog();
    void OpenTrackerSettingsDialog();

protected:

    void ReadHardwareConfig( const char * filename );
    void InternalWriteHardwareConfig( QString filename );

    TrackedVideoSource          * m_trackedVideoSource;
    Tracker                     * m_tracker;
    TrackerToolDisplayManager   * m_toolDisplayManager;

};

#endif
