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

#include "cameracalibrationplugininterface.h"
#include "cameracalibrator.h"
#include "cameraobject.h"
#include "ibisapi.h"
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkPNGReader.h>
#include <vtkPNGWriter.h>
#include <vtkAmoebaMinimizer.h>
#include "vtkMatrix4x4Operators.h"
#include "vtkopencvbridge.h"
#include "vtkXFMReader.h"
#include "vtkXFMWriter.h"
#include <QtGlobal>
#include <QFileInfo>
#include <QProgressDialog>

using namespace std;
using namespace cv;

void ComputeReprojection( vector<Point2f> & imagePoints, vector<Point3f> & worldPoints,
                          vector<Point2f> & projectedPoints, vector<double> & reprojDistmm, vector<double> & pointDistmm,
                          int imageSize[2], double focal[2], double center[2],
                          double dist, vtkMatrix4x4 * trackingMatrix, CameraExtrinsicParams & extrinsic );

CameraExtrinsicParams::CameraExtrinsicParams()
    : translationOptimizationScale( 500.0 ),
      rotationOptimizationScale( 10.0 ),
      extrinsicStdDev(0.0),
      meanReprojError(0.0)
{
    calibrationMatrix = vtkMatrix4x4::New();
    gridMatrix = vtkMatrix4x4::New();
}

CameraExtrinsicParams::~CameraExtrinsicParams()
{
    calibrationMatrix->Delete();
    gridMatrix->Delete();
}

CameraCalibrator::CameraCalibrator()
{
    m_gridWidth = 1;
    m_gridHeight = 1;
    m_gridCellSize = 30.0;
    m_minimizer = vtkAmoebaMinimizer::New();
    m_pluginInterface = nullptr;
}

CameraCalibrator::~CameraCalibrator()
{
    ClearCalibrationData();
    m_minimizer->Delete();
}

void CameraCalibrator::SetGridProperties( int width, int height, double cellSize )
{
    m_gridWidth = width;
    m_gridHeight = height;
    m_gridCellSize = cellSize;
    InitGridWorldPoints();
    ClearCalibrationData();
}

bool CameraCalibrator::GetOptimizeGridDetection()
{
    return m_optimizeGridDetection;
}

void CameraCalibrator::SetOptimizeGridDetection( bool optimize )
{
    m_optimizeGridDetection = optimize;
}

void CameraCalibrator::ClearCalibrationData()
{
    m_objectPoints.clear();
    m_imagePoints.clear();
    for( unsigned i = 0; i < m_cameraImages.size(); ++i )
    {
        m_cameraImages[i]->Delete();
    }
    m_cameraImages.clear();
    ClearMatrixArray( m_camToGridMatrices );
    ClearMatrixArray( m_gridToCamMatrices );
    ClearMatrixArray( m_trackingMatrices );
    m_viewEnabled.clear();
}

void WriteMatrix( QString filename, vtkMatrix4x4 * mat )
{
    vtkXFMWriter * writer = vtkXFMWriter::New();
    writer->SetFileName( filename.toUtf8().data() );
    writer->SetMatrix( mat );
    writer->Write();
    writer->Delete();
}

void CameraCalibrator::ExportCalibrationData( QString dir, QProgressDialog * progressDlg )
{
    vtkPNGWriter * writer = vtkPNGWriter::New();
    for( int i = 0; i < GetNumberOfViews(); ++i )
    {
        // Write tracker matrix
        QString trackerMatrixFileName = dir + QString( "/trackerMatrix_%1.txt" ).arg( i, 4, 10, QLatin1Char('0') );
        WriteMatrix( trackerMatrixFileName, m_trackingMatrices[i] );

        // Write captured points
        QString calibrationPointsFileName = dir + QString( "/calibrationPoints_%1.txt" ).arg( i, 4, 10, QLatin1Char('0') );
        QFile calibrationPointsFile( calibrationPointsFileName );
        if( !calibrationPointsFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
            return;
        QTextStream pointsOut( &calibrationPointsFile );
        pointsOut << m_imagePoints[i].size() << " " << GetGridWidth() << " " << GetGridHeight() << " " << GetGridCellSize() << "\n";
        for( int pt = 0; pt < m_imagePoints[i].size(); ++pt )
        {
            cv::Point2f imgPoint = m_imagePoints[i][pt];
            pointsOut << imgPoint.x << " " << imgPoint.y << " ";
            cv::Point3f worldPoint = m_objectPoints[i][pt];
            pointsOut << worldPoint.x << " " << worldPoint.y << " " << worldPoint.z << "\n";
        }
        calibrationPointsFile.close();

        // write calibration images
        QString filename = dir + QString("/frame_%1").arg( i, 4, 10, QLatin1Char('0') );
        writer->SetFileName( filename.toUtf8().data() );
        writer->SetInputData( m_cameraImages[i] );
        writer->Write();

        if( progressDlg )
            m_pluginInterface->GetIbisAPI()->UpdateProgress( progressDlg, (int)round( (float)i / m_cameraImages.size() * 100.0 ) );
    }
    writer->Delete();
}

void ReadMatrix( QString filename, vtkMatrix4x4 * mat )
{
    vtkXFMReader * reader = vtkXFMReader::New();
    reader->SetFileName( filename.toUtf8().data() );
    reader->SetMatrix( mat );
    reader->Update();
    reader->Delete();
}

bool ExistsAndReadable( QString filename )
{
    QFileInfo fileInfo( filename );
    return fileInfo.exists() && fileInfo.isReadable();
}

void CameraCalibrator::ImportCalibrationData( QString dirName, QProgressDialog * progressDlg )
{
    // Count the number of views
    int numberOfViews = 0;
    while( 1 )
    {
        QString calibrationPointsFileName = dirName + QString( "/calibrationPoints_%1.txt" ).arg( numberOfViews, 4, 10, QLatin1Char('0') );
        if( !ExistsAndReadable( calibrationPointsFileName ) )
            break;

        QString trackerMatrixFileName = dirName + QString( "/trackerMatrix_%1.txt" ).arg( numberOfViews, 4, 10, QLatin1Char('0') );
        if( !ExistsAndReadable( trackerMatrixFileName ) )
            break;

        QString imageFileName = dirName + QString("/frame_%1").arg( numberOfViews, 4, 10, QLatin1Char('0') );
        if( !ExistsAndReadable( imageFileName ) )
            break;

        ++numberOfViews;
    }

    // Read the data
    vtkPNGReader * reader = vtkPNGReader::New();
    for( int i = 0; i < numberOfViews; ++i )
    {
        // import grid points for this view
        QString calibrationPointsFileName = dirName + QString( "/calibrationPoints_%1.txt" ).arg( i, 4, 10, QLatin1Char('0') );
        QFile calibPointsFile( calibrationPointsFileName );
        bool res = calibPointsFile.open( QIODevice::ReadOnly | QIODevice::Text );
        Q_ASSERT( res );
        QTextStream pointsIn( &calibPointsFile );
        int nbPoints = 0;
        int gridWidth = 6;
        int gridHeight = 8;
        double gridSpacing = 10.0;
        std::vector<cv::Point2f> imgPoints;
        std::vector<cv::Point3f> worldPoints;
        pointsIn >> nbPoints >> gridWidth >> gridHeight >> gridSpacing;
        if( i == 0 )
            SetGridProperties( gridWidth, gridHeight, gridSpacing );
        for( int pt = 0; pt < nbPoints; ++pt )
        {
            cv::Point2f imgPoint;
            pointsIn >> imgPoint.x >> imgPoint.y;
            imgPoints.push_back( imgPoint );
            cv::Point3f worldPoint;
            pointsIn >> worldPoint.x >> worldPoint.y >> worldPoint.z;
            worldPoints.push_back( worldPoint );
        }
        m_imagePoints.push_back( imgPoints );
        m_objectPoints.push_back( worldPoints );
        calibPointsFile.close();

        // Import tracker matrix
        QString trackerMatrixFileName = dirName + QString( "/trackerMatrix_%1.txt" ).arg( i, 4, 10, QLatin1Char('0') );
        vtkMatrix4x4 * trackerMat = vtkMatrix4x4::New();
        ReadMatrix( trackerMatrixFileName, trackerMat );
        m_trackingMatrices.push_back( trackerMat );

        // import image
        QString filename = dirName + QString("/frame_%1").arg( i, 4, 10, QLatin1Char('0') );
        reader->SetFileName( filename.toUtf8().data() );
        reader->Update();
        vtkImageData * image = vtkImageData::New();
        image->DeepCopy( reader->GetOutput() );
        m_cameraImages.push_back( image );

        m_viewEnabled.push_back( true );

        if( progressDlg )
            m_pluginInterface->GetIbisAPI()->UpdateProgress( progressDlg, (int)round( (float)i / numberOfViews * 100.0 ) );
    }
    reader->Delete();
}

bool CameraCalibrator::DetectGrid( vtkImageData * im, std::vector<cv::Point2f> & imagePoints )
{
    // Find the corners of the grid
    int dims[3];
    im->GetDimensions( dims );

    // Convert input VTK image to a greyscale OpenCV image
    cv::Mat image( dims[1], dims[0], CV_8UC3, im->GetScalarPointer() );
    cv::Mat imageFliped;
    cv::flip( image, imageFliped, 0 );
    cv::Mat imageGray;
    cv::cvtColor( imageFliped, imageGray, COLOR_RGB2GRAY);

    cv::Mat imageSmall;
    double factor = 1.0;
    if( m_optimizeGridDetection )
    {
        // Downsize image for faster initial detection (we aim for a 400px wide image)
        factor = 400.0 / dims[0];
        cv::resize(imageGray, imageSmall, Size(), factor, factor, INTER_AREA);
    }
    else
    {
        imageSmall = imageGray;
    }

    // Find the grid
    cv::Size patternSize( m_gridWidth, m_gridHeight );
    std::vector<cv::Point2f> pointsSmall;
    bool found = cv::findChessboardCorners( imageSmall, patternSize, pointsSmall, cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE + cv::CALIB_CB_FAST_CHECK );

    // if the grid is found, refine the positions
    if( found )
    {
        // scale up the points
        imagePoints.clear();
        for (cv::Point2f ps : pointsSmall)
        {
            imagePoints.push_back(cv::Point2f(ps.x / factor, ps.y / factor));
        }

        // Fine-tune corner detection
        cv::cornerSubPix( imageGray, imagePoints, cv::Size( 11, 11 ), cv::Size( -1, -1 ), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1 ) );
    }

    return found;
}

void CameraCalibrator::AddView( vtkImageData * im, std::vector<cv::Point2f> & imagePoints, vtkMatrix4x4 * trackerMatrix )
{
    // Check that dimensions of new image fit what is already in there
    int dims[3];
    im->GetDimensions( dims );
    int imageWidth = dims[0];
    int imageHeight = dims[1];
    if( m_cameraImages.size() > 0 )
    {
        int prevDims[3];
        m_cameraImages[0]->GetDimensions( prevDims );
        Q_ASSERT( prevDims[0] == imageWidth && prevDims[1] == imageHeight );
    }

    // Make a copy of the new image and store it
    vtkImageData * imCopy = vtkImageData::New();
    imCopy->DeepCopy( im );
    m_cameraImages.push_back( imCopy );

    // Store points and tracking matrix
    m_imagePoints.push_back( imagePoints );
    m_objectPoints.push_back( m_objectPointsOneView );
    vtkMatrix4x4 * trackingMatCopy = vtkMatrix4x4::New();
    trackingMatCopy->DeepCopy( trackerMatrix );
    m_trackingMatrices.push_back( trackingMatCopy );
    m_viewEnabled.push_back( true );

    // Just put in a value until it is computed
    m_perViewAvgReprojectionError.push_back( 0.0 );
}

void CameraCalibrator::Calibrate( bool computeCenter, bool computeDistortion, CameraIntrinsicParams & params, CameraExtrinsicParams & extParams )
{
    Q_ASSERT( GetNumberOfViews() > 0 );

    // Get image sizes
    int dims[3];
    m_cameraImages[0]->GetDimensions( dims );
    int imageWidth = dims[0];
    int imageHeight = dims[1];

    cv::Size imageSize( imageWidth, imageHeight );
    cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
    std::vector< cv::Mat > rvecs;
    std::vector< cv::Mat > tvecs;
    cv::Mat cameraMatrix = cv::Mat::eye( 3, 3, CV_64F );
    cameraMatrix.at<double>( 0, 0 ) = params.m_focal[0] * imageWidth;
    cameraMatrix.at<double>( 1, 1 ) = params.m_focal[1] * imageHeight;
    cameraMatrix.at<double>( 0, 2 ) = params.m_center[0] * imageWidth;
    cameraMatrix.at<double>( 1, 2 ) = params.m_center[1] * imageHeight;

    // Collect points only for enabled views
    std::vector< std::vector<cv::Point3f> > objectPoints;
    std::vector< std::vector<cv::Point2f> > imagePoints;
    for( int i = 0; i < m_imagePoints.size(); ++i )
    {
        if( m_viewEnabled[i] )
        {
            objectPoints.push_back( m_objectPoints[i] );
            imagePoints.push_back( m_imagePoints[i] );
        }
    }

    // Do the calibration - we always ignore tangential distortion and higher-order terms in the radial distortion. There is an option
    // to allow moving the center point and optimizing for the first radial distortion coefficient
    int flags = CV_CALIB_ZERO_TANGENT_DIST | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6;
    if( !computeCenter )
        flags |= CV_CALIB_FIX_PRINCIPAL_POINT;
    if( !computeDistortion )
        flags |= CV_CALIB_FIX_K1;
    params.m_reprojectionError = cv::calibrateCamera( objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, flags );

    // store results
    params.m_focal[0] = cameraMatrix.at<double>( 0, 0 ) / imageWidth;
    params.m_focal[1] = cameraMatrix.at<double>( 1, 1 ) / imageHeight;
    params.m_center[0] = cameraMatrix.at<double>( 0, 2 ) / imageWidth;
    params.m_center[1] = cameraMatrix.at<double>( 1, 2 ) / imageHeight;
    params.m_distorsionK1 = distCoeffs.at<double>( 0 );

    // Store cam to grid
    ClearMatrixArray( m_camToGridMatrices );
    ClearMatrixArray( m_gridToCamMatrices );
    for( int i = 0, ei = 0; i < m_cameraImages.size(); ++i )
    {
        vtkMatrix4x4 * mat = vtkMatrix4x4::New();
        if( m_viewEnabled[i] )
        {
            VtkOpenCvBridge::RvecTvecToMatrix4x4( tvecs[ei], rvecs[ei], mat );
            ++ei;
        }
        vtkMatrix4x4 * gridToCam = vtkMatrix4x4::New();
        gridToCam->DeepCopy( mat );
        gridToCam->Invert();
        m_camToGridMatrices.push_back( mat );
        m_gridToCamMatrices.push_back( gridToCam );
    }

    // Do the extrinsic calibration if enough views
    if( GetNumberOfViews() > 3 )
    {
        DoExtrinsicCalibration( extParams );
        extParams.meanReprojError = ComputeAvgReprojectionError( params, extParams );
    }
}

void CameraCalibrator::ComputeMeanAndStdDevOfGridCorners( cv::Point3f & meanTL, cv::Point3f & meanTR, cv::Point3f & meanBL, cv::Point3f & meanBR, double & val )
{
    // Get current param values
    double t[3] = { 0.0, 0.0, 0.0 };
    t[0] = m_minimizer->GetParameterValue("x");
    t[1] = m_minimizer->GetParameterValue("y");
    t[2] = m_minimizer->GetParameterValue("z");
    double r[3] = { 0.0, 0.0, 0.0 };
    r[0] = m_minimizer->GetParameterValue("rx");
    r[1] = m_minimizer->GetParameterValue("ry");
    r[2] = m_minimizer->GetParameterValue("rz");

    // Convert param values to a calibration matrix
    vtkMatrix4x4 * calMatrix = vtkMatrix4x4::New();
    vtkMatrix4x4Operators::TransRotToMatrix( t, r, calMatrix );

    double xGridMax = ( GetGridHeight() - 1 ) * GetGridCellSize();
    double yGridMax = ( GetGridWidth() - 1 ) * GetGridCellSize();

    double stl[3] = { 0.0, 0.0, 0.0 };
    double sstl[3] = { 0.0, 0.0, 0.0 };
    double str[3] = { 0.0, 0.0, 0.0 };
    double sstr[3] = { 0.0, 0.0, 0.0 };
    double sbl[3] = { 0.0, 0.0, 0.0 };
    double ssbl[3] = { 0.0, 0.0, 0.0 };
    double sbr[3] = { 0.0, 0.0, 0.0 };
    double ssbr[3] = { 0.0, 0.0, 0.0 };

    vtkMatrix4x4 * gridToWorld = vtkMatrix4x4::New();
    int n = 0;
    int nbViews = m_trackingMatrices.size();
    for( int i = 0; i < nbViews; ++i )
    {
        if( m_viewEnabled[i] )
        {
            // compute grid to world for view i
            gridToWorld->DeepCopy( m_trackingMatrices[i] );
            vtkMatrix4x4::Multiply4x4( gridToWorld, calMatrix, gridToWorld );
            vtkMatrix4x4::Multiply4x4( gridToWorld, m_gridToCamMatrices[i], gridToWorld );

            // transform 3 grid corners world
            double topLeft[ 4 ] = { 0.0, 0.0, 0.0, 1.0 };
            double topLeftWorld[4] = { 0.0, 0.0, 0.0, 1.0 };
            gridToWorld->MultiplyPoint( topLeft, topLeftWorld );

            double topRight[ 4 ] = { 0.0, yGridMax, 0.0, 1.0 };
            double topRightWorld[4] = { 0.0, 0.0, 0.0, 1.0 };
            gridToWorld->MultiplyPoint( topRight, topRightWorld );

            double bottomLeft[ 4 ] = { xGridMax, 0.0, 0.0, 1.0 };
            double bottomLeftWorld[4] = { 0.0, 0.0, 0.0, 1.0 };
            gridToWorld->MultiplyPoint( bottomLeft, bottomLeftWorld );

            double bottomRight[ 4 ] = { xGridMax, yGridMax, 0.0, 1.0 };
            double bottomrightWorld[4] = { 0.0, 0.0, 0.0, 1.0 };
            gridToWorld->MultiplyPoint( bottomRight, bottomrightWorld );

            for( int j = 0; j < 3; ++j )
            {
                stl[j] += topLeftWorld[j];
                sstl[j] += topLeftWorld[j] * topLeftWorld[j];
                str[j] += topRightWorld[j];
                sstr[j] += topRightWorld[j] * topRightWorld[j];
                sbl[j] += bottomLeftWorld[j];
                ssbl[j] += bottomLeftWorld[j] * bottomLeftWorld[j];
                sbr[j] += bottomrightWorld[j];
                ssbr[j] += bottomrightWorld[j] * bottomrightWorld[j];
            }
            ++n;
        }
    }

    calMatrix->Delete();
    gridToWorld->Delete();

    // Compute mean of corner position in world space
    meanTL = 1.0/n * cv::Point3f( stl[0], stl[1], stl[2] );
    meanTR = 1.0/n * cv::Point3f( str[0], str[1], str[2] );
    meanBL = 1.0/n * cv::Point3f( sbl[0], sbl[1], sbl[2] );
    meanBR = 1.0/n * cv::Point3f( sbr[0], sbr[1], sbr[2] );

    // Compute
    double valTopLeft = sqrt( (sstl[0] - stl[0]*stl[0]/n)/(n-1) + (sstl[1] - stl[1]*stl[1]/n)/(n-1) + (sstl[2] - stl[2]*stl[2]/n)/(n-1) );
    double valTopRight = sqrt( (sstr[0] - str[0]*str[0]/n)/(n-1) + (sstr[1] - str[1]*str[1]/n)/(n-1) + (sstr[2] - str[2]*str[2]/n)/(n-1) );
    double valbottomLeft = sqrt( (ssbl[0] - sbl[0]*sbl[0]/n)/(n-1) + (ssbl[1] - sbl[1]*sbl[1]/n)/(n-1) + (ssbl[2] - sbl[2]*sbl[2]/n)/(n-1) );
    double valbottomRight = sqrt( (ssbr[0] - sbr[0]*sbr[0]/n)/(n-1) + (ssbr[1] - sbr[1]*sbr[1]/n)/(n-1) + (ssbr[2] - sbr[2]*sbr[2]/n)/(n-1) );

    val = ( valTopLeft + valTopRight + valbottomLeft + valbottomRight ) / 4.0;
}

void CameraCalibrator::ExtrinsicCalibrationCostFunction( void * userData )
{
    CameraCalibrator * self = (CameraCalibrator *)userData;

    double val = 0.0;
    cv::Point3f mtl;
    cv::Point3f mtr;
    cv::Point3f mbl;
    cv::Point3f mbr;
    self->ComputeMeanAndStdDevOfGridCorners( mtl, mtr, mbl, mbr, val );

    self->m_minimizer->SetFunctionValue( val );
}

#include <vtkLandmarkTransform.h>
#include <vtkPoints.h>

void CameraCalibrator::DoExtrinsicCalibration( CameraExtrinsicParams & params )
{
    double calibrationTranslation[3];
    double calibrationRotation[3];
    vtkMatrix4x4Operators::MatrixToTransRot( params.calibrationMatrix, calibrationTranslation, calibrationRotation );

    m_minimizer->Initialize();
    m_minimizer->SetMaxIterations( 5000 );
    m_minimizer->SetFunction( ExtrinsicCalibrationCostFunction, this );
    m_minimizer->SetFunctionArgDelete(NULL);
    m_minimizer->SetParameterValue("x", calibrationTranslation[0] );
    m_minimizer->SetParameterScale("x", params.translationOptimizationScale );
    m_minimizer->SetParameterValue("y", calibrationTranslation[1] );
    m_minimizer->SetParameterScale("y", params.translationOptimizationScale );
    m_minimizer->SetParameterValue("z", calibrationTranslation[2] );
    m_minimizer->SetParameterScale("z", params.translationOptimizationScale );
    m_minimizer->SetParameterValue("rx", calibrationRotation[0] );
    m_minimizer->SetParameterScale("rx", params.rotationOptimizationScale );
    m_minimizer->SetParameterValue("ry", calibrationRotation[1] );
    m_minimizer->SetParameterScale("ry", params.rotationOptimizationScale );
    m_minimizer->SetParameterValue("rz", calibrationRotation[2] );
    m_minimizer->SetParameterScale("rz", params.rotationOptimizationScale );
    m_minimizer->Minimize();
    params.extrinsicStdDev = m_minimizer->GetFunctionValue();

    // Get back results
    calibrationTranslation[0] = m_minimizer->GetParameterValue("x");
    calibrationTranslation[1] = m_minimizer->GetParameterValue("y");
    calibrationTranslation[2] = m_minimizer->GetParameterValue("z");
    calibrationRotation[0] = m_minimizer->GetParameterValue("rx");
    calibrationRotation[1] = m_minimizer->GetParameterValue("ry");
    calibrationRotation[2] = m_minimizer->GetParameterValue("rz");

    // Convert param values to a calibration matrix
    vtkMatrix4x4Operators::TransRotToMatrix( calibrationTranslation, calibrationRotation, params.calibrationMatrix );

    // Compute the matrix of the grid

    // source points
    double xGridMax = ( GetGridHeight() - 1 ) * GetGridCellSize();
    double yGridMax = ( GetGridWidth() - 1 ) * GetGridCellSize();
    vtkPoints * srcPts = vtkPoints::New();
    srcPts->InsertNextPoint( 0.0, 0.0, 0.0 );
    srcPts->InsertNextPoint( 0.0, yGridMax, 0.0 );
    srcPts->InsertNextPoint( xGridMax, 0.0, 0.0 );
    srcPts->InsertNextPoint( xGridMax, yGridMax, 0.0 );

    // target points
    double val = 0.0;
    cv::Point3f mtl;
    cv::Point3f mtr;
    cv::Point3f mbl;
    cv::Point3f mbr;
    ComputeMeanAndStdDevOfGridCorners( mtl, mtr, mbl, mbr, val );
    vtkPoints * targetPts = vtkPoints::New();
    targetPts->InsertNextPoint( mtl.x, mtl.y, mtl.z );
    targetPts->InsertNextPoint( mtr.x, mtr.y, mtr.z );
    targetPts->InsertNextPoint( mbl.x, mbl.y, mbl.z );
    targetPts->InsertNextPoint( mbr.x, mbr.y, mbr.z );

    vtkLandmarkTransform * lt = vtkLandmarkTransform::New();
    lt->SetModeToRigidBody();
    lt->SetSourceLandmarks( srcPts );
    lt->SetTargetLandmarks( targetPts );
    lt->Update();
    params.gridMatrix->DeepCopy( lt->GetMatrix() );
    lt->Delete();
    srcPts->Delete();
    targetPts->Delete();
}

int CameraCalibrator::GetNumberOfViews()
{
    return m_objectPoints.size();
}

void CameraCalibrator::AccumulateView( std::vector<cv::Point2f> & imagePoints, vtkMatrix4x4 * trackerMatrix )
{
    vtkMatrix4x4 * matrixCopy = vtkMatrix4x4::New();
    matrixCopy->DeepCopy( trackerMatrix );
    m_accumulatedTrackingMatrices.push_back( matrixCopy );
    m_accumulatedImagePoints.push_back( imagePoints );
}

int CameraCalibrator::GetNumberOfAccumulatedViews()
{
    return m_accumulatedTrackingMatrices.size();
}

void CameraCalibrator::ClearAccumulatedViews()
{
    ClearMatrixArray( m_accumulatedTrackingMatrices );
    m_accumulatedImagePoints.clear();
}

void CameraCalibrator::AddAccumulatedViews( vtkImageData * im )
{
    Q_ASSERT( GetNumberOfAccumulatedViews() > 0 );

    // Concatenate accumulated matrices
    vtkMatrix4x4 * trackingMatrix = vtkMatrix4x4::New();
    vtkMatrix4x4Operators::MeanMatrix( m_accumulatedTrackingMatrices, trackingMatrix );

    // Concatenate image points
    std::vector<cv::Point2f> averagePoints;
    int nbPoints = m_accumulatedImagePoints[0].size();
    for( int pt = 0; pt < nbPoints; ++pt )
    {
        cv::Point2f avg( 0.0, 0.0 );
        for( int v = 0; v < m_accumulatedImagePoints.size(); ++v )
            avg += m_accumulatedImagePoints[v][pt];
        avg *= 1.0 / double( m_accumulatedImagePoints.size() );
        averagePoints.push_back( avg );
    }

    // Add view
    AddView( im, averagePoints, trackingMatrix );
    trackingMatrix->Delete();

    ClearAccumulatedViews();
}

bool CameraCalibrator::IsViewEnabled( int index )
{
    Q_ASSERT( index < m_cameraImages.size() );
    return m_viewEnabled[index];
}

void CameraCalibrator::SetViewEnabled( int index, bool enabled )
{
    Q_ASSERT( index < m_cameraImages.size() );
    m_viewEnabled[index] = enabled;
}

vtkImageData * CameraCalibrator::GetCameraImage( int index )
{
    Q_ASSERT( index < m_cameraImages.size() );
    return m_cameraImages[ index ];
}

int CameraCalibrator::GetCameraImageWidth()
{
    if( m_cameraImages.size() > 0 )
    {
        int dims[3];
        m_cameraImages[0]->GetDimensions( dims );
        return dims[0];
    }
    return 0;
}

int CameraCalibrator::GetCameraImageHeight()
{
    if( m_cameraImages.size() > 0 )
    {
        int dims[3];
        m_cameraImages[0]->GetDimensions( dims );
        return dims[1];
    }
    return 0;
}

vtkMatrix4x4 * CameraCalibrator::GetIntrinsicCameraToGridMatrix( int index )
{
    Q_ASSERT( index < m_camToGridMatrices.size() );
    return m_camToGridMatrices[ index ];
}

vtkMatrix4x4 * CameraCalibrator::GetTrackingMatrix( int index )
{
    Q_ASSERT( index < m_camToGridMatrices.size() );
    return m_trackingMatrices[ index ];
}

void ComputeReprojection( vector<Point2f> & imagePoints, vector<Point3f> & worldPoints,
                          vector<Point2f> & projectedPoints, vector<double> & reprojDistmm, vector<double> & pointDistmm,
                          int imageSize[2], double focal[2], double center[2],
                          double dist, vtkMatrix4x4 * trackingMatrix, CameraExtrinsicParams & extrinsic )
{
    // Compute world to camera matrix
    vtkMatrix4x4 * worldToCamMatrix = vtkMatrix4x4::New();
    vtkMatrix4x4::Multiply4x4( trackingMatrix, extrinsic.calibrationMatrix, worldToCamMatrix );
    worldToCamMatrix->Invert();
    vtkMatrix4x4::Multiply4x4( worldToCamMatrix, extrinsic.gridMatrix, worldToCamMatrix );

    // Compute coordinate of world point in camera space
    double cpa[4];
    double wpa[4];
    vector<Point3f> cameraPoints;
    for( unsigned i = 0; i < imagePoints.size(); ++i )
    {
        // transform point in camera space
        Point3f & wp = worldPoints[i];
        wpa[0] = wp.x; wpa[1] = wp.y; wpa[2] = wp.z; wpa[3] = 1.0;
        worldToCamMatrix->MultiplyPoint( wpa, cpa );
        cameraPoints.push_back( Point3f( cpa[0], cpa[1], cpa[2] ) );
    }

    // compute reprojection of image points on the plane of the corresponding 3D point
    for( unsigned i = 0; i < imagePoints.size(); ++i )
    {
        double dz = -cameraPoints[i].z;  // distance between camera and 3D point plane

        double xim = imagePoints[i].x / imageSize[0];
        double cx = center[0];
        double fx = focal[0];
        double xw = dz * ( xim - cx ) / fx;

        double yim = (((double)imageSize[1]) - imagePoints[i].y - 1.0) / imageSize[1];
        double cy = center[1];
        double fy = focal[1];
        double yw = dz * ( yim - cy ) / fy;

        double oxw = cameraPoints[i].x;
        double oyw = cameraPoints[i].y;
        double xdist = xw - oxw;
        double ydist = yw - oyw;
        double reprojDist = sqrt( xdist * xdist + ydist * ydist );
        reprojDistmm.push_back( reprojDist );
        pointDistmm.push_back( dz );
    }

    // Create camera matrix and distortion coeficient vector
    cv::Mat distCoeffs( 8, 1, cv::DataType<double>::type, dist );
    cv::Mat cameraMatrix = cv::Mat::eye( 3, 3, cv::DataType<double>::type );
    cameraMatrix.at<double>( 0, 0 ) = focal[0];
    cameraMatrix.at<double>( 1, 1 ) = focal[1];
    cameraMatrix.at<double>( 0, 2 ) = center[0];
    cameraMatrix.at<double>( 1, 2 ) = center[1];

    // Project points
    cv::Mat rVec = cv::Mat::zeros( 3, 1, cv::DataType<double>::type );
    cv::Mat tVec = cv::Mat::zeros( 3, 1, cv::DataType<double>::type );
    projectPoints( cameraPoints, rVec, tVec, cameraMatrix, distCoeffs, projectedPoints );

    // Flip projection point's x-axis : Has to do with the way OpenCV is projecting that is different from
    // what is done in vtk. This is also affecting the way OpenCV's rotation and translations are converted in vtkMatrix4x4
    for( unsigned i = 0; i < projectedPoints.size(); ++i )
        projectedPoints[i].x = 1. - projectedPoints[i].x;
}

double CameraCalibrator::ComputeCrossValidation( double translationScale, double rotationScale, double & stdDevReprojError, double & minDist, double & maxDist, QProgressDialog * progressDlg )
{
    // Compute reprojection error obtained for each of the views when taking all
    // other views to calibrate.
    int numberOfViews = this->GetNumberOfViews();
    int nbEnabledViews = 0;

    vector< double > allReprojErrors;
    minDist = DBL_MAX;
    maxDist = 0.0;

    for( int xvalIndex = 0; xvalIndex < numberOfViews; ++xvalIndex )
    {
        if( m_viewEnabled[xvalIndex] )
        {
            // Create a new camera calibrator with all views except the one at xvalIndex
            CameraCalibrator * cal = new CameraCalibrator;
            cal->SetGridProperties( m_gridWidth, m_gridHeight, m_gridCellSize );
            for( int i = 0; i < numberOfViews; ++i )
            {
                if( i != xvalIndex && m_viewEnabled[i] )
                    cal->AddView( m_cameraImages[i], m_imagePoints[i], m_trackingMatrices[i] );
            }

            // Run the calibration
            CameraIntrinsicParams intrinsicParams;
            intrinsicParams.m_center[0] = cal->GetCameraImageWidth() * 0.5;
            intrinsicParams.m_center[1] = cal->GetCameraImageHeight() * 0.5;
            CameraExtrinsicParams extrinsicParams;
            extrinsicParams.translationOptimizationScale = translationScale;
            extrinsicParams.rotationOptimizationScale = rotationScale;
            cal->Calibrate( false, false, intrinsicParams, extrinsicParams );

            // Compute reprojection error on the view at xvalIndex
            vector<Point2f> projectedPoints;
            vector<double> reprojDistmm;
            vector<double> pointDistmm;
            int imageSize[2];
            imageSize[0] = GetCameraImageWidth();
            imageSize[1] = GetCameraImageHeight();
            ComputeReprojection( m_imagePoints[xvalIndex], m_objectPoints[xvalIndex],
                                 projectedPoints, reprojDistmm, pointDistmm,
                                 imageSize, intrinsicParams.m_focal, intrinsicParams.m_center,
                                 intrinsicParams.m_distorsionK1, m_trackingMatrices[xvalIndex],
                                 extrinsicParams );

            allReprojErrors.insert( allReprojErrors.end(), reprojDistmm.begin(), reprojDistmm.end() );
            for( int k = 0; k < reprojDistmm.size(); ++k )
            {
                if( pointDistmm[k] > maxDist )
                    maxDist = pointDistmm[k];
                if( pointDistmm[k] < minDist )
                    minDist = pointDistmm[k];
            }

            delete cal;

            ++nbEnabledViews;
        }

        if( progressDlg )
            m_pluginInterface->GetIbisAPI()->UpdateProgress( progressDlg, (int)round( (float)xvalIndex / numberOfViews * 100.0 ) );
    }

    // Compute mean reprojection error
    int nbPoints = nbEnabledViews * m_gridWidth * m_gridHeight;
    double sumReprojErrormm = 0.0;
    for( int i = 0; i < allReprojErrors.size(); ++i )
        sumReprojErrormm += allReprojErrors[i];
    double avgReprojectionErrormm = sumReprojErrormm / ( (double) nbPoints );

    // Compute std dev of reprojection error
    double sumSqrDev = 0.0;
    for( int i = 0; i < allReprojErrors.size(); ++i )
    {
        double diffError = allReprojErrors[i] - avgReprojectionErrormm;
        sumSqrDev += diffError * diffError;
    }
    double var = sumSqrDev / ((double)nbPoints);
    stdDevReprojError = sqrt( var );

    return avgReprojectionErrormm;
}

double CameraCalibrator::GetViewReprojectionError( int viewIndex )
{
    Q_ASSERT( viewIndex < m_perViewAvgReprojectionError.size() && viewIndex >= 0 );
    return m_perViewAvgReprojectionError[ viewIndex ];
}

double CameraCalibrator::ComputeAvgReprojectionError( CameraIntrinsicParams & intParams, CameraExtrinsicParams & extParams )
{
    double sumReprojErrormm = 0.0;
    int nbPoints = m_gridWidth * m_gridHeight;
    m_perViewAvgReprojectionError.clear();
    int nbEnabledViews = 0;
    for( int v = 0; v < GetNumberOfViews(); ++v )
    {
        if( m_viewEnabled[v] )
        {
            vector<Point2f> projectedPoints;
            vector<double> reprojDistmm;
            vector<double> pointDistmm;
            int imageSize[2];
            imageSize[0] = GetCameraImageWidth();
            imageSize[1] = GetCameraImageHeight();
            ComputeReprojection( m_imagePoints[v], m_objectPoints[v],
                                 projectedPoints, reprojDistmm, pointDistmm,
                                 imageSize, intParams.m_focal, intParams.m_center,
                                 intParams.m_distorsionK1, m_trackingMatrices[v],
                                 extParams );

            double sumReprojView = 0.0;
            for( int k = 0; k < reprojDistmm.size(); ++k )
            {
                sumReprojErrormm += reprojDistmm[k];
                sumReprojView += reprojDistmm[k];
            }

            double viewReprojError = sumReprojView / (double)nbPoints;
            m_perViewAvgReprojectionError.push_back( viewReprojError );
            ++nbEnabledViews;
        }
        else
            m_perViewAvgReprojectionError.push_back( 0.0 );
    }

    double avgReprojectionErrormm = sumReprojErrormm / (double)( nbEnabledViews * nbPoints );

    return avgReprojectionErrormm;
}

void CameraCalibrator::InitGridWorldPoints()
{
    m_objectPointsOneView.clear();
    for( int x = 0; x < m_gridHeight; ++x )
    {
        for( int y = 0; y < m_gridWidth; ++y )
        {
            double pOrig[3];
            pOrig[0] = x * m_gridCellSize;
            pOrig[1] = y * m_gridCellSize;
            pOrig[2] = 0.0;
            cv::Point3f p( pOrig[0], pOrig[1], pOrig[2] );
            m_objectPointsOneView.push_back( p );
        }
    }
}

void CameraCalibrator::ClearMatrixArray( std::vector< vtkMatrix4x4 * > & matVec )
{
    for( unsigned i = 0; i < matVec.size(); ++i )
        matVec[i]->Delete();
    matVec.clear();
}

void CameraCalibrator::SetPluginInterface( CameraCalibrationPluginInterface *ifc )
{
    m_pluginInterface = ifc;
}
