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

#ifndef __CameraCalibrator_h_
#define __CameraCalibrator_h_

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/calib3d/calib3d_c.h>
#include "cameraobject.h"

class vtkImageData;
class vtkMatrix4x4;
class vtkAmoebaMinimizer;
class CameraCalibrationPluginInterface;
class QProgressDialog;

struct CameraExtrinsicParams
{
    CameraExtrinsicParams();
    ~CameraExtrinsicParams();

    // In
    double translationOptimizationScale;
    double rotationOptimizationScale;

    // Out
    vtkMatrix4x4 * calibrationMatrix;
    vtkMatrix4x4 * gridMatrix;
    double extrinsicStdDev;
    double meanReprojError;
};

class CameraCalibrator
{

public:

    CameraCalibrator();
    ~CameraCalibrator();

    // Calibration Grid
    void SetGridProperties( int width, int height, double cellSize );
    bool DetectGrid( vtkImageData * im, std::vector<cv::Point2f> & imagePoints );
    int GetGridWidth() { return m_gridWidth; }
    int GetGridHeight() { return m_gridHeight; }
    double GetGridCellSize() { return m_gridCellSize; }

    // Do calibration and manage data
    bool GetOptimizeGridDetection();
    void SetOptimizeGridDetection( bool optimize );
    void ClearCalibrationData();
    void ExportCalibrationData( QString dirName, QProgressDialog * progressDlg );
    void ImportCalibrationData( QString dirName, QProgressDialog * progressDlg );
    int GetNumberOfViews();
    void AccumulateView( std::vector<cv::Point2f> & imagePoints, vtkMatrix4x4 * trackerMatrix );
    int GetNumberOfAccumulatedViews();
    void ClearAccumulatedViews();
    void AddAccumulatedViews( vtkImageData * im );
    void AddView( vtkImageData * im, std::vector<cv::Point2f> & imagePoints, vtkMatrix4x4 * trackerMatrix );
    void Calibrate( bool computeCenter, bool computeDistortion, CameraIntrinsicParams & params, CameraExtrinsicParams & extParams );

    // Used to show camera views that were used to compute intrinsic calibration
    bool IsViewEnabled( int index );
    void SetViewEnabled( int index, bool enabled );
    vtkImageData * GetCameraImage( int index );
    int GetCameraImageWidth();
    int GetCameraImageHeight();
    vtkMatrix4x4 * GetIntrinsicCameraToGridMatrix( int index );
    vtkMatrix4x4 * GetTrackingMatrix( int index );

    // Compute Cross-Validation - returns average reprojection error
    double ComputeCrossValidation( double translationScale, double rotationScale, double & stdDevReprojError, double & minDist, double & maxDist, QProgressDialog * progressDlg );

    double GetViewReprojectionError( int viewIndex );

    void SetPluginInterface( CameraCalibrationPluginInterface *ifc );

protected:

    void InitGridWorldPoints();
    void ClearMatrixArray( std::vector< vtkMatrix4x4 * > & matVec );
    double ComputeAvgReprojectionError( CameraIntrinsicParams & intParams, CameraExtrinsicParams & extParams );

    CameraCalibrationPluginInterface *m_pluginInterface;

    // Grid description
    int m_gridWidth;
    int m_gridHeight;
    double m_gridCellSize;

    // Calibration data
    bool m_optimizeGridDetection;
    std::vector<cv::Point3f>  m_objectPointsOneView;
    std::vector<bool> m_viewEnabled;
    std::vector< std::vector<cv::Point3f> > m_objectPoints;
    std::vector< std::vector<cv::Point2f> > m_imagePoints;
    std::vector< vtkImageData * > m_cameraImages;
    std::vector< vtkMatrix4x4 * > m_camToGridMatrices;
    std::vector< vtkMatrix4x4 * > m_gridToCamMatrices;
    std::vector< vtkMatrix4x4 * > m_trackingMatrices;

    // Accumulated calibration data
    std::vector< vtkMatrix4x4 * > m_accumulatedTrackingMatrices;
    std::vector< std::vector<cv::Point2f> > m_accumulatedImagePoints;

    // Extrinsic optimization
    void DoExtrinsicCalibration( CameraExtrinsicParams & params );
    static void ExtrinsicCalibrationCostFunction( void * userData );
    void ComputeMeanAndStdDevOfGridCorners( cv::Point3f & meanTL, cv::Point3f & meanTR, cv::Point3f & meanBL, cv::Point3f & meanBR, double & val );
    vtkAmoebaMinimizer * m_minimizer;

    // Results
    std::vector< double > m_perViewAvgReprojectionError;

};

#endif
