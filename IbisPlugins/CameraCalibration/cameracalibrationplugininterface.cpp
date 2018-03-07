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
#include "cameracalibrationwidget.h"
#include "cameracalibrationsidepanelwidget.h"
#include "cameracalibrator.h"
#include "ibisapi.h"
#include "sceneobject.h"
#include "polydataobject.h"
#include "cameraobject.h"
#include "pointsobject.h"
#include "vtkImageData.h"
#include "vtkTransform.h"
#include <QtPlugin>
#include <QSettings>
#include <QTime>
#include <vector>
#include "opencv2/opencv.hpp"
#include "simplepropcreator.h"
#include "vtkRenderer.h"


CameraCalibrationPluginInterface::CameraCalibrationPluginInterface()
{
    m_cameraCalibrationWidget = 0;
    m_calibrationGridWidth = 6;   // This is the number of crossings being considered for calibration and not the number of squares
    m_calibrationGridHeight = 8;
    m_calibrationGridCellSize = 30.0;
    m_cameraCalibrator = new CameraCalibrator;
    m_cameraCalibrator->SetPluginInterface( this );
    m_currentCameraObjectId = IbisAPI::InvalidId;
    m_calibrationGridObject = 0;

    m_computeCenter = false;
    m_computeK1 = false;
    m_computeExtrinsic = true;
    m_extrinsicTranslationScale = 500.0;
    m_extrinsicRotationScale = 10.0;

    m_accumulate = true;
    m_isAccumulating = false;
    m_numberOfViewsToAccumulate = 10;

    m_meanReprojectionErrormm = 0.0;
}

CameraCalibrationPluginInterface::~CameraCalibrationPluginInterface()
{
    if( m_calibrationGridObject )
        m_calibrationGridObject->Delete();
    delete m_cameraCalibrator;
}

QString CameraCalibrationPluginInterface::GetPluginDescription()
{
    return QString("<html><h1>Camera Calibration plugin</h1><p>Author: Simon Drouin</p><p>Description</p><p>This plugin is used to calibrate a tracked camera to produce an augmented reality image</p></html>");
}

bool CameraCalibrationPluginInterface::CanRun()
{
    return true;
}

QWidget * CameraCalibrationPluginInterface::CreateTab()
{   
    ValidateCurrentCamera();
    InitializeCameraCalibrator();

    BuildCalibrationGridRepresentation();
    GetIbisAPI()->AddObject( m_calibrationGridObject );

    UpdateCameraViewsObjects();

    connect( GetIbisAPI(), SIGNAL(ObjectAdded(int)), this, SLOT(OnObjectAddedOrRemoved()) );
    connect( GetIbisAPI(), SIGNAL(ObjectRemoved(int)), this, SLOT(OnObjectAddedOrRemoved()) );

    CameraCalibrationSidePanelWidget * widget = new CameraCalibrationSidePanelWidget;
    widget->SetPluginInterface( this );

    return widget;
}

bool CameraCalibrationPluginInterface::WidgetAboutToClose()
{
    if( IsCalibrationWidgetOn() )
        m_cameraCalibrationWidget->close();

    ClearCameraViews();
    GetIbisAPI()->RemoveObject( m_calibrationGridObject );

    disconnect( GetIbisAPI(), SIGNAL(ObjectAdded(int)), this, SLOT(OnObjectAddedOrRemoved()) );
    disconnect( GetIbisAPI(), SIGNAL(ObjectRemoved(int)), this, SLOT(OnObjectAddedOrRemoved()) );

    return true;
}

void CameraCalibrationPluginInterface::LoadSettings( QSettings & s )
{
    m_calibrationGridWidth = s.value( "CalibrationGridWidth", 6 ).toInt();
    m_calibrationGridHeight = s.value( "CalibrationGridHeight", 8 ).toInt();
    m_calibrationGridCellSize = s.value( "CalibrationGridCellSize", 30.0 ).toDouble();
}

void CameraCalibrationPluginInterface::SaveSettings( QSettings & s )
{
    s.setValue( "CalibrationGridWidth", m_calibrationGridWidth );
    s.setValue( "CalibrationGridHeight", m_calibrationGridHeight );
    s.setValue( "CalibrationGridCellSize", m_calibrationGridCellSize );
}

void CameraCalibrationPluginInterface::StartCalibrationWidget( bool on )
{
    Q_ASSERT( IsCalibrationWidgetOn() != on );
    if( on )
    {
        m_cameraCalibrationWidget = new CameraCalibrationWidget;
        m_cameraCalibrationWidget->setAttribute( Qt::WA_DeleteOnClose );
        m_cameraCalibrationWidget->SetPluginInterface( this );
        connect( m_cameraCalibrationWidget, SIGNAL(destroyed()), this, SLOT(OnCameraCalibrationWidgetClosed()) );
        m_cameraCalibrationWidget->show();
    }
    else
        m_cameraCalibrationWidget->close();
}

bool CameraCalibrationPluginInterface::IsCalibrationWidgetOn()
{
    return (m_cameraCalibrationWidget != 0);
}

void CameraCalibrationPluginInterface::OnCameraCalibrationWidgetClosed()
{
    m_cameraCalibrationWidget = 0;
    emit CameraCalibrationWidgetClosedSignal();
}

void CameraCalibrationPluginInterface::OnObjectAddedOrRemoved()
{
    this->ValidateCurrentCamera();
}

void CameraCalibrationPluginInterface::GetAllValidCamerasFromScene( QList<CameraObject*> & all )
{
    QList<CameraObject*> allCams;
    GetIbisAPI()->GetAllCameraObjects( allCams );
    int nbValidCams = 0;
    bool needTrackableCamera = !GetIbisAPI()->IsViewerOnly();
    for( int i = 0; i < allCams.size(); ++i )
    {
        if( !needTrackableCamera || allCams[i]->IsDrivenByHardware() )
            all.push_back( allCams[i] );
    }
}

void CameraCalibrationPluginInterface::SetCurrentCameraObjectId( int id )
{
    if( id != m_currentCameraObjectId )
    {
        m_currentCameraObjectId = id;
        m_cameraCalibrator->ClearCalibrationData();
    }
    emit PluginModified();
}

bool CameraCalibrationPluginInterface::HasValidCamera()
{
    return (GetCurrentCameraObject() != 0);
}

CameraObject * CameraCalibrationPluginInterface::GetCurrentCameraObject()
{
    return CameraObject::SafeDownCast( GetIbisAPI()->GetObjectByID( m_currentCameraObjectId ) );
}

void CameraCalibrationPluginInterface::SetComputeCenter( bool c )
{
    m_computeCenter = c;
    DoCalibration();
}

void CameraCalibrationPluginInterface::SetComputeDistorsion( bool c )
{
    m_computeK1 = c;
    DoCalibration();
}

void CameraCalibrationPluginInterface::SetComputeExtrinsic( bool c )
{
    m_computeExtrinsic = c;
    DoCalibration();
}

void CameraCalibrationPluginInterface::SetExtrinsicTranslationScale( double s )
{
    m_extrinsicTranslationScale = s;
}

void CameraCalibrationPluginInterface::SetExtrinsicRotationScale( double s )
{
    m_extrinsicRotationScale = s;
}

void CameraCalibrationPluginInterface::StartAccumulating()
{
    Q_ASSERT( !m_isAccumulating );
    m_isAccumulating = true;
}

void CameraCalibrationPluginInterface::AccumulateView( vtkImageData * imageVtk, std::vector<cv::Point2f> imagePoints, vtkMatrix4x4 * trackerMatrix )
{
    m_cameraCalibrator->AccumulateView( imagePoints, trackerMatrix );
    if( m_cameraCalibrator->GetNumberOfAccumulatedViews() >= m_numberOfViewsToAccumulate )
    {
        m_cameraCalibrator->AddAccumulatedViews( imageVtk );
        m_isAccumulating = false;
        DoCalibration();
    }
}

void CameraCalibrationPluginInterface::CancelAccumulation()
{
    m_isAccumulating = false;
    m_cameraCalibrator->ClearAccumulatedViews();
}

void CameraCalibrationPluginInterface::ImportCalibrationData( QString dir )
{
    QProgressDialog * dlg = GetIbisAPI()->StartProgress( 100, "Importing calibration data..." );
    m_cameraCalibrator->ImportCalibrationData( dir, dlg );
    GetIbisAPI()->StopProgress( dlg );

    // Update grid representation as it might have changed because of imported data
    m_calibrationGridWidth = m_cameraCalibrator->GetGridWidth();
    m_calibrationGridHeight = m_cameraCalibrator->GetGridHeight();
    m_calibrationGridCellSize = m_cameraCalibrator->GetGridCellSize();
    BuildCalibrationGridRepresentation();

    // Recalibrate
    DoCalibration();
}

void CameraCalibrationPluginInterface::ClearCalibrationData()
{
    m_cameraCalibrator->ClearCalibrationData();
}

void CameraCalibrationPluginInterface::CaptureCalibrationView()
{
    CameraObject * currentCamera = GetCurrentCameraObject();
    Q_ASSERT( currentCamera );

    // Get the latest frame
    vtkImageData * imageVtk = currentCamera->GetVideoOutput();

    // Compute the corners of the grid
    std::vector<cv::Point2f> currentImagePoints;
    bool found = m_cameraCalibrator->DetectGrid( imageVtk, currentImagePoints );

    if( found )
    {
        vtkMatrix4x4 * trackerMatrix = currentCamera->GetUncalibratedTransform()->GetMatrix();
        m_cameraCalibrator->AddView( imageVtk, currentImagePoints, trackerMatrix );

        DoCalibration();
    }
}

void CameraCalibrationPluginInterface::DoCalibration()
{
    CameraObject * currentCamera = GetCurrentCameraObject();
    if( currentCamera && m_cameraCalibrator->GetNumberOfViews() > 0 )
    {
        // Run the calibration
        CameraIntrinsicParams intParams( currentCamera->GetIntrinsicParams() );
        CameraExtrinsicParams extParams;
        extParams.translationOptimizationScale = m_extrinsicTranslationScale;
        extParams.rotationOptimizationScale = m_extrinsicRotationScale;
        extParams.calibrationMatrix->DeepCopy( currentCamera->GetCalibrationMatrix() );
        m_cameraCalibrator->Calibrate( m_computeCenter, m_computeK1, intParams, extParams );

        m_meanReprojectionErrormm = extParams.meanReprojError;

        // Update scene with new calibration
        currentCamera->SetIntrinsicParams( intParams );
        if( m_computeExtrinsic )
        {
            currentCamera->SetCalibrationMatrix( extParams.calibrationMatrix );
            vtkTransform * gridTransform = vtkTransform::SafeDownCast( m_calibrationGridObject->GetLocalTransform() );
            if( gridTransform )
                gridTransform->SetMatrix( extParams.gridMatrix );
        }
        UpdateCameraViewsObjects();

        emit PluginModified();
    }
}

void CameraCalibrationPluginInterface::UpdateCameraViewsObjects()
{
    ClearCameraViews();

    // Add updated objects
    CameraObject * currentCam = GetCurrentCameraObject();
    if( currentCam )
    {
        int nbViews = m_cameraCalibrator->GetNumberOfViews();
        for( int i = 0; i < nbViews; ++i )
        {
            if( m_cameraCalibrator->IsViewEnabled(i) )
            {
                CameraObject * newCam = CameraObject::New();
                newCam->SetName( QString("Intrinsic view %1").arg( i ) );
                newCam->SetIntrinsicParams( currentCam->GetIntrinsicParams() );
                newCam->SetCanEditTransformManually( false );
                vtkMatrix4x4 * calibMat = vtkMatrix4x4::New();
                calibMat->Identity();
                newCam->SetCalibrationMatrix( calibMat );
                calibMat->Delete();
                newCam->AddFrame( m_cameraCalibrator->GetCameraImage( i ), m_cameraCalibrator->GetIntrinsicCameraToGridMatrix( i ) );

                GetIbisAPI()->AddObject( newCam, m_calibrationGridObject );
                m_cameraViewsObjectIds.push_back( newCam->GetObjectID() );
                newCam->Delete();

                // Now same view, but using tracking matrix and calibration matrix instead
                CameraObject * calCam = CameraObject::New();
                calCam->SetName( QString("Extrinsic view %1").arg( i ) );
                calCam->SetIntrinsicParams( currentCam->GetIntrinsicParams() );
                calCam->SetCanEditTransformManually( false );
                vtkMatrix4x4 * calMat2 = vtkMatrix4x4::New();
                calMat2->DeepCopy( currentCam->GetCalibrationMatrix() );
                calCam->SetCalibrationMatrix( calMat2 );
                calMat2->Delete();
                calCam->AddFrame( m_cameraCalibrator->GetCameraImage(i), m_cameraCalibrator->GetTrackingMatrix( i ) );

                GetIbisAPI()->AddObject( calCam );
                m_cameraCalibratedViewsObjectIds.push_back( calCam->GetObjectID() );
                calCam->Delete();
            }
        }
    }
}

void CameraCalibrationPluginInterface::SetCalibrationGridWidth( int width )
{
    m_calibrationGridWidth = width;
    BuildCalibrationGridRepresentation();
    InitializeCameraCalibrator();
    emit PluginModified();
}

void CameraCalibrationPluginInterface::SetCalibrationGridHeight( int height )
{
    m_calibrationGridHeight = height;
    BuildCalibrationGridRepresentation();
    InitializeCameraCalibrator();
    emit PluginModified();
}

void CameraCalibrationPluginInterface::SetCalibrationGridSquareSize( double size )
{
    m_calibrationGridCellSize = size;
    BuildCalibrationGridRepresentation();
    InitializeCameraCalibrator();
    emit PluginModified();
}

void CameraCalibrationPluginInterface::ShowAllCapturedViews()
{
    for( int i = 0; i < m_cameraViewsObjectIds.size(); ++i )
    {
        SceneObject * obj = GetIbisAPI()->GetObjectByID( m_cameraViewsObjectIds[i] );
        obj->SetHidden( false );
        SceneObject * calObj = GetIbisAPI()->GetObjectByID( m_cameraCalibratedViewsObjectIds[i] );
        calObj->SetHidden( false );
    }
}

void CameraCalibrationPluginInterface::HideAllCapturedViews()
{
    for( int i = 0; i < m_cameraViewsObjectIds.size(); ++i )
    {
        SceneObject * obj = GetIbisAPI()->GetObjectByID( m_cameraViewsObjectIds[i] );
        obj->SetHidden( true );
        SceneObject * calObj = GetIbisAPI()->GetObjectByID( m_cameraCalibratedViewsObjectIds[i] );
        calObj->SetHidden( true );
    }
}

void CameraCalibrationPluginInterface::ShowGrid( bool show )
{
    Q_ASSERT( m_calibrationGridObject );
    m_calibrationGridObject->SetHidden( !show );
}

bool CameraCalibrationPluginInterface::IsShowingGrid()
{
    Q_ASSERT( m_calibrationGridObject );
    return !m_calibrationGridObject->IsHidden();
}

void CameraCalibrationPluginInterface::ValidateCurrentCamera()
{
    bool needTrackableCamera = !GetIbisAPI()->IsViewerOnly();

    // if we have an id, check object is still valid
    int newCameraId = m_currentCameraObjectId;
    if( m_currentCameraObjectId != IbisAPI::InvalidId )
    {
        SceneObject * obj = GetIbisAPI()->GetObjectByID( m_currentCameraObjectId );
        CameraObject * camObj = CameraObject::SafeDownCast( obj );
        if( !camObj || ( needTrackableCamera && !camObj->IsDrivenByHardware() ) )
            newCameraId = IbisAPI::InvalidId;
    }

    // If we don't have an id, try to find a valid camera
    if( newCameraId == IbisAPI::InvalidId )
    {
        QList< CameraObject* > cams;
        GetIbisAPI()->GetAllCameraObjects( cams );
        for( int i = 0; i < cams.size(); ++i )
        {
            if( !needTrackableCamera || cams[i]->IsDrivenByHardware() )
            {
                newCameraId = cams[i]->GetObjectID();
                break;
            }
        }
    }

    // Still no valid camera, create one
    if( newCameraId == IbisAPI::InvalidId && !needTrackableCamera )
    {
        CameraObject * cam = CameraObject::New();
        cam->SetName( "Default Camera" );
        cam->SetCanEditTransformManually( false );
        GetIbisAPI()->AddObject( cam );
        newCameraId = cam->GetObjectID();
        cam->Delete();
    }

    SetCurrentCameraObjectId( newCameraId );
    emit PluginModified();
}

void CameraCalibrationPluginInterface::InitializeCameraCalibrator()
{
    m_cameraCalibrator->SetGridProperties( m_calibrationGridWidth, m_calibrationGridHeight, m_calibrationGridCellSize );
}

#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"

void CameraCalibrationPluginInterface::BuildCalibrationGridRepresentation()
{
    vtkPoints * pts = vtkPoints::New();
    vtkUnsignedCharArray * colors = vtkUnsignedCharArray::New();
    colors->SetNumberOfComponents(3);
    colors->SetName("Colors");
    vtkCellArray * quads = vtkCellArray::New();
    unsigned char white[3] = { 255, 255, 255 };
    unsigned char black[3] = { 0, 0, 0 };
    for( int row = 0; row <= m_calibrationGridHeight; ++row )
    {
        for( int col = 0; col <= m_calibrationGridWidth; ++col )
        {
            unsigned char * cellColor = ( col % 2 == row % 2 ) ? black : white;
            double y = col * m_calibrationGridCellSize;
            double x = row * m_calibrationGridCellSize;
            pts->InsertNextPoint( x, y, 0.0 );
            pts->InsertNextPoint( x - m_calibrationGridCellSize, y, 0.0 );
            pts->InsertNextPoint( x - m_calibrationGridCellSize, y - m_calibrationGridCellSize, 0.0 );
            pts->InsertNextPoint( x, y - m_calibrationGridCellSize, 0.0 );
            colors->InsertNextTypedTuple( cellColor );
            colors->InsertNextTypedTuple( cellColor );
            colors->InsertNextTypedTuple( cellColor );
            colors->InsertNextTypedTuple( cellColor );
            vtkIdType cellQuad[4];
            vtkIdType current = row * ( m_calibrationGridWidth + 1 ) + col;
            cellQuad[0] = current * 4;
            cellQuad[1] = current * 4 + 1;
            cellQuad[2] = current * 4 + 2;
            cellQuad[3] = current * 4 + 3;
            quads->InsertNextCell( 4, cellQuad );
        }
    }

    vtkPolyData * poly = vtkPolyData::New();
    poly->SetPoints( pts );
    poly->SetPolys( quads );
    poly->GetPointData()->SetScalars( colors );

    if( !m_calibrationGridObject )
    {
        m_calibrationGridObject = PolyDataObject::New();
        m_calibrationGridObject->SetName( "CameraCalibrationGrid" );
        m_calibrationGridObject->SetScalarsVisible( 1 );
        m_calibrationGridObject->SetLutIndex( -1 );
        m_calibrationGridObject->SetCanEditTransformManually( false );
        m_calibrationGridObject->SetObjectDeletable( false );
        m_calibrationGridObject->SetHidable( false );
        //m_calibrationGridObject->SetListable( false );
    }
    m_calibrationGridObject->SetPolyData( poly );

    // cleanup
    pts->Delete();
    quads->Delete();
    colors->Delete();
    poly->Delete();
}

void CameraCalibrationPluginInterface::ClearCameraViews()
{
    // Clear all camera views objects
    for( unsigned i = 0; i < m_cameraViewsObjectIds.size(); ++i )
    {
        SceneObject * cam = GetIbisAPI()->GetObjectByID( m_cameraViewsObjectIds[i] );
        if( cam )
            GetIbisAPI()->RemoveObject( cam );
    }
    m_cameraViewsObjectIds.clear();

    // Clear all calibrated camera views objects
    for( unsigned i = 0; i < m_cameraCalibratedViewsObjectIds.size(); ++i )
    {
        SceneObject * cam = GetIbisAPI()->GetObjectByID( m_cameraCalibratedViewsObjectIds[i] );
        if( cam )
            GetIbisAPI()->RemoveObject( cam );
    }
    m_cameraCalibratedViewsObjectIds.clear();
}
