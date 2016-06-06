/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __HardwareModule_h_
#define __HardwareModule_h_

#include <QtPlugin>

class Application;
class SceneManager;
class QMenu;
class vtkImageData;
class vtkTransform;
class vtkMatrix4x4;
class CameraIntrinsicParams;

enum TrackerToolState{ Ok, Missing, OutOfVolume, OutOfView, Undefined };

class HardwareModule
{

public:

    HardwareModule() {}
    virtual ~HardwareModule() {}

    void SetApplication( Application * app ) { m_application = app; }
    Application * GetApplication() { return m_application; }
    SceneManager * GetSceneManager();

    virtual void AddSettingsMenuEntries( QMenu * menu ) {}
    virtual QWidget * CreateTrackerStatusDialog( QWidget * parent ) { return 0; }
    virtual bool Init( const char * filename = 0 ) = 0;
    virtual void Update() = 0;
    virtual bool ShutDown() = 0;
    virtual void WriteHardwareConfig( const char * filename, bool backupOnly ) = 0;

    virtual void AddObjectsToScene() = 0;
    virtual void RemoveObjectsFromScene() = 0;

    // Legacy methods : should be eliminated as we properly separate IbisLib and IbisHardware
    virtual bool CanCaptureTrackedVideo() = 0;
    virtual bool CanCaptureVideo() = 0;
    virtual int GetNavigationPointerObjectID() = 0;
    virtual vtkImageData * GetTrackedVideoOutput() = 0;
    virtual int GetVideoFrameWidth() = 0;
    virtual int GetVideoFrameHeight() = 0;
    virtual TrackerToolState GetVideoTrackerState() = 0;
    virtual vtkTransform * GetTrackedVideoTransform() = 0;
    virtual vtkTransform * GetTrackedVideoUncalibratedTransform() = 0;
    virtual vtkMatrix4x4 * GetVideoCalibrationMatrix() = 0;
    virtual void SetVideoCalibrationMatrix( vtkMatrix4x4 * mat ) = 0;
    virtual int GetNumberOfVideoCalibrationMatrices() = 0;
    virtual QString GetVideoCalibrationMatrixName( int index ) = 0;
    virtual void SetCurrentVideoCalibrationMatrixName( QString name ) = 0;
    virtual QString GetCurrentVideoCalibrationMatrixName() = 0;
    virtual bool IsVideoTransformFrozen() = 0;
    virtual void FreezeVideoTransform( int nbSamples ) = 0;
    virtual void UnFreezeVideoTransform() = 0;
    virtual const CameraIntrinsicParams & GetCameraIntrinsicParams() = 0;
    virtual void SetCameraIntrinsicParams( CameraIntrinsicParams & p ) = 0;
    virtual void AddTrackedVideoClient() = 0;
    virtual void RemoveTrackedVideoClient() = 0;
    virtual int GetReferenceToolIndex() = 0;
    virtual vtkTransform * GetTrackerToolTransform( int toolIndex ) = 0;
    virtual TrackerToolState GetTrackerToolState( int toolIndex ) = 0;

protected:

    Application * m_application;

};

Q_DECLARE_INTERFACE( HardwareModule, "ca.mcgill.mni.bic.Ibis.HardwareModule/1.0" );

#endif
