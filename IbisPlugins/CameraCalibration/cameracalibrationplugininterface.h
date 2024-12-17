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

#ifndef CAMERACALIBRATIONPLUGININTERFACE_H
#define CAMERACALIBRATIONPLUGININTERFACE_H

#include "SVL.h"
#include "opencv2/opencv.hpp"
#include "toolplugininterface.h"

class CameraCalibrationWidget;
class CameraManualCalibrationWidget;
class PolyDataObject;
class CameraCalibrator;
class TrackedVideoSource;
class CameraObject;
class PointerObject;
class vtkMatrix4x4;
class vtkImageData;
class vtkProp3D;

class CameraCalibrationPluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.CameraCalibrationPluginInterface" )

public:
    CameraCalibrationPluginInterface();
    ~CameraCalibrationPluginInterface();
    virtual QString GetPluginName() override { return QString( "CameraCalibration" ); }
    virtual QString GetPluginDescription() override;
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString( "Camera Calibration" ); }

    // Tab widget
    virtual QWidget * CreateTab() override;
    virtual bool WidgetAboutToClose() override;

    // Plugin Settings
    virtual void LoadSettings( QSettings & s ) override;
    virtual void SaveSettings( QSettings & s ) override;

    // Calibration widget
    void StartCalibrationWidget( bool on );
    bool IsCalibrationWidgetOn();

    CameraCalibrator * GetCameraCalibrator() { return m_cameraCalibrator; }

    // Manage the camera to calibrate
    void GetAllValidCamerasFromScene( QList<CameraObject *> & all );
    int GetCurrentCameraObjectId() { return m_currentCameraObjectId; }
    void SetCurrentCameraObjectId( int id );
    bool HasValidCamera();
    CameraObject * GetCurrentCameraObject();

    // Calibration params
    void SetComputeCenter( bool c );
    bool GetComputeCenter() { return m_computeCenter; }
    void SetComputeDistorsion( bool c );
    bool GetComputeDistortion() { return m_computeK1; }
    void SetComputeIntrinsic( bool c );
    bool GetComputeIntrinsic() { return m_computeIntrinsic; }
    void SetComputeExtrinsic( bool c );
    bool GetComputeExtrinsic() { return m_computeExtrinsic; }
    void SetExtrinsicTranslationScale( double s );
    double GetExtrinsicTranslationScale() { return m_extrinsicTranslationScale; }
    void SetExtrinsicRotationScale( double s );
    double GetExtrinsicRotationScale() { return m_extrinsicRotationScale; }
    double GetMeanRoprojectionError() { return m_meanReprojectionErrormm; }
    void SetUseAccumulation( bool a ) { m_accumulate = a; }
    bool IsUsingAccumulation() { return m_accumulate; }
    bool IsAccumulating() { return m_isAccumulating; }
    void StartAccumulating();
    void AccumulateView( vtkImageData * imageVtk, std::vector<cv::Point2f> imagePoints, vtkMatrix4x4 * trackerMatrix );
    size_t GetNumberOfAccumulatedViews();
    size_t GetNumberOfViewsToAccumulate();
    void CancelAccumulation();
    bool GetOptimizeGridDetection();
    void SetOptimizeGridDetection( bool optimize );

    // Do Calibration
    void ImportCalibrationData( QString dir );
    void ClearCalibrationData();
    void CaptureCalibrationView();
    void UpdateCameraViewsObjects();
    void DoCalibration();

    // Manage grid params
    int GetCalibrationGridWidth() { return m_calibrationGridWidth; }
    void SetCalibrationGridWidth( int width );
    int GetCalibrationGridHeight() { return m_calibrationGridHeight; }
    void SetCalibrationGridHeight( int height );
    double GetCalibrationGridCellSize() { return m_calibrationGridCellSize; }
    void SetCalibrationGridSquareSize( double size );

    // Manage display of calibration objects in 3D view
    void ShowAllCapturedViews();
    void HideAllCapturedViews();
    void ShowGrid( bool show );
    bool IsShowingGrid();

public slots:

    void OnObjectAddedOrRemoved();
    void OnCameraCalibrationWidgetClosed();

signals:

    void CameraCalibrationWidgetClosedSignal();

protected:
    void ValidateCurrentCamera();
    void InitializeCameraCalibrator();
    void BuildCalibrationGridRepresentation();
    void ClearCameraViews();

    int m_calibrationGridWidth;  // number of cells (squares) in the grid
    int m_calibrationGridHeight;
    double m_calibrationGridCellSize;  // size of each cell (square) in mm
    PolyDataObject * m_calibrationGridObject;

    // Popup Calibration widget
    CameraCalibrationWidget * m_cameraCalibrationWidget;

    std::vector<int> m_cameraViewsObjectIds;
    std::vector<int> m_cameraCalibratedViewsObjectIds;
    CameraCalibrator * m_cameraCalibrator;
    int m_currentCameraObjectId;

    // Calibration params
    bool m_computeCenter;
    bool m_computeK1;
    bool m_computeIntrinsic;
    bool m_computeExtrinsic;
    double m_extrinsicTranslationScale;
    double m_extrinsicRotationScale;

    // Accumulation of views for capture
    int m_numberOfViewsToAccumulate;
    bool m_accumulate;
    bool m_isAccumulating;

    // Result
    double m_meanReprojectionErrormm;
};

#endif
