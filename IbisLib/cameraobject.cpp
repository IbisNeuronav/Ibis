/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "cameraobject.h"
#include "cameraobjectsettingswidget.h"
#include "vtkCamera.h"
#include "vtkImageActor.h"
#include "vtkPlanes.h"
#include "vtkAxesActor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkPolyDataMapper.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkMatrix4x4.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkSimpleProp3D.h"
#include "vtkIbisImagePlaneMapper.h"
#include "vtkXFMWriter.h"
#include "vtkXFMReader.h"
#include "application.h"
#include "hardwaremodule.h"
#include "scenemanager.h"
#include "pointerobject.h"
#include "vtkmatrix4x4utilities.h"
#include <QMessageBox>

ObjectSerializationMacro( CameraObject );

static int DefaultImageSize[2] = { 640, 480 };

//---------------------------------------------------------------------------------------
// CameraIntrinsicParams class implementation
//---------------------------------------------------------------------------------------

ObjectSerializationMacro( CameraIntrinsicParams );

CameraIntrinsicParams::CameraIntrinsicParams()
{
    m_center[0] = 319;
    m_center[1] = 239;
    m_focal[0] = 1000;
    m_focal[1] = 1000;
    m_distorsionK1 = 0.0;
    m_imageSize[0] = 640;
    m_imageSize[1] = 480;
    m_reprojectionError = 0.0;
}

CameraIntrinsicParams & CameraIntrinsicParams::operator=( const CameraIntrinsicParams & other )
{
    m_center[0] = other.m_center[0];
    m_center[1] = other.m_center[1];
    m_focal[0] = other.m_focal[0];
    m_focal[1] = other.m_focal[1];
    m_distorsionK1 = other.m_distorsionK1;
    m_imageSize[0] = other.m_imageSize[0];
    m_imageSize[1] = other.m_imageSize[1];
    m_reprojectionError = other.m_reprojectionError;
}

void CameraIntrinsicParams::Serialize( Serializer * ser )
{
    ::Serialize( ser, "Center", m_center, 2 );
    ::Serialize( ser, "Focal", m_focal, 2 );
    ::Serialize( ser, "DistortionK1", m_distorsionK1 );
    ::Serialize( ser, "ImageSize", m_imageSize, 2 );
    ::Serialize( ser, "CalibrationReprojectionError", m_reprojectionError );
}

#include "vtkMath.h"

double CameraIntrinsicParams::GetVerticalAngleDegrees() const
{
    return vtkMath::DegreesFromRadians( GetVerticalAngleRad() );
}

void CameraIntrinsicParams::SetVerticalAngleDegrees( double angle )
{
    double angleRad = vtkMath::RadiansFromDegrees( angle );
    SetVerticalAngleRad( angleRad );
}

double CameraIntrinsicParams::GetVerticalAngleRad() const
{
    return 2 * atan( .5 * m_imageSize[1] / m_focal[1] );
}

void CameraIntrinsicParams::SetVerticalAngleRad( double angle )
{
    double focal = 0.5 * m_imageSize[1] / tan( 0.5 * angle );
    m_focal[0] = focal;
    m_focal[1] = focal;
}

//---------------------------------------------------------------------------------------
// CameraObject class implementation
//---------------------------------------------------------------------------------------

CameraObject::CameraObject()
{
    m_uncalibratedWorldTransform = vtkTransform::New();
    m_uncalibratedWorldTransform->Identity();

    m_currentFrame = 0;
    m_uncalibratedTransform = vtkTransform::New();
    m_calibrationTransform = vtkTransform::New();

    m_trackable = false;
    m_trackingCamera = false;
    m_intrinsicEditable = false;
    m_extrinsicEditable = false;
    m_camera = vtkCamera::New();
    m_lensDisplacementTransform = vtkTransform::New();
    m_opticalCenterTransform = vtkTransform::New();
    m_opticalCenterTransform->Concatenate( this->GetWorldTransform() );
    m_opticalCenterTransform->Concatenate( m_lensDisplacementTransform );
    m_imageTransform = vtkTransform::New();
    m_imageTransform->SetInput( m_opticalCenterTransform );
    m_imageDistance = 100.0;
    m_globalOpacity = 0.7;
    m_saturation = 1.0;
    m_brightness = 1.0;
    m_useTransparency = false;
    m_useGradient = true;
    m_showMask = false;
    m_trackedTransparencyCenter = false;
    m_transparencyCenter[0] = (double)DefaultImageSize[0] * 0.5;
    m_transparencyCenter[1] = (double)DefaultImageSize[1] * 0.5;
    m_transparencyRadius[0] = 0.03 * DefaultImageSize[0];
    m_transparencyRadius[1] = 0.3 * DefaultImageSize[0];
    CreateCameraRepresentation();

    m_recordingCamera = 0;
    m_cachedImageSize[ 0 ] = DefaultImageSize[0];
    m_cachedImageSize[ 1 ] = DefaultImageSize[1];
}

CameraObject::~CameraObject()
{
    m_uncalibratedWorldTransform->Delete();

    for( unsigned i = 0; i < m_capturedFrames.size(); ++i )
    {
        m_capturedFrames[i]->Delete();
        m_uncalibratedMatrices[i]->Delete();
    }
    m_uncalibratedMatrices.clear();
    m_capturedFrames.clear();
    m_uncalibratedTransform->Delete();
    m_calibrationTransform->Delete();
    m_lensDisplacementTransform->Delete();
    m_opticalCenterTransform->Delete();

    m_camera->Delete();
    m_imageTransform->Delete();
    m_cameraPolyData->Delete();

    if( m_recordingCamera )
        m_recordingCamera->Delete();
}

#include <QDir>

void CameraObject::Serialize( Serializer * ser )
{
    SceneObject::Serialize(ser);

    ::Serialize( ser, "IntrinsicParams", m_intrinsicParams );
    ::Serialize( ser, "ImageDistance", m_imageDistance );
    ::Serialize( ser, "UseTransparency", m_useTransparency );
    ::Serialize( ser, "TransparencyCenter", m_transparencyCenter, 2 );
    ::Serialize( ser, "TransparencyRadius", m_transparencyRadius, 2 );

    vtkMatrix4x4 * calibrationMat = vtkMatrix4x4::New();
    m_calibrationTransform->GetMatrix( calibrationMat );
    ::Serialize( ser, "CalibrationMatrix", calibrationMat );

    ::Serialize( ser, "CurrentFrame", m_currentFrame );
    int numberOfImages = m_capturedFrames.size();
    ::Serialize( ser, "NumberOfImages", numberOfImages );

    // save tracking matrix for each frame
    if( ser->IsReader() )
    {
        m_uncalibratedMatrices.resize( numberOfImages );
        for( int i = 0; i < numberOfImages; ++i )
            m_uncalibratedMatrices[i] = vtkMatrix4x4::New();
    }
    ::Serialize( ser, "UncalibratedMatrices", m_uncalibratedMatrices, false );

    QString dataDirName = GetSceneDataDirectoryForThisObject( ser->GetCurrentDirectory() );
    if( !ser->IsReader() )
        WriteImages( dataDirName );
    else
        ReadImages( numberOfImages, dataDirName );

    if (ser->IsReader())
    {
        m_calibrationTransform->SetMatrix( calibrationMat );
        SetCurrentFrame( m_currentFrame );
        SetIntrinsicParams( m_intrinsicParams );
        emit Modified();
    }

    calibrationMat->Delete();
}

void CameraObject::Export()
{
    QString dirName = Application::GetInstance().GetExistingDirectory( tr("Export Camera"), QDir::homePath() );
    QFileInfo info( dirName );

    if( !info.isWritable() )
        return;

    QProgressDialog * progressDlg = Application::GetInstance().StartProgress( 100, "Exporting Camera..." );

    // Write images
    WriteImages( dirName.toUtf8().data(), progressDlg );

    // Write matrix for every frame
    WriteMatrices( m_uncalibratedMatrices, dirName );

    // Write calibration matrix
    QString calMatrixFilename = QString("%1/calibMat.xfm").arg( dirName );
    WriteMatrix( GetCalibrationMatrix(), calMatrixFilename );

    // Write intrinsic params
    SerializerWriter writer;
    QString intrinsicParamsFilename = QString("%1/intrinsic_params.xml").arg( dirName );
    writer.SetFilename( intrinsicParamsFilename.toUtf8().data() );
    writer.Start();
    ::Serialize( &writer, "IntrinsicParams", m_intrinsicParams );
    writer.Finish();

    Application::GetInstance().StopProgress( progressDlg );
}

 bool CameraObject::Import( QString & directory, QProgressDialog * progressDlg )
 {
     bool success = true;

     // check that dir exists and is readable
     QFileInfo dirInfo( directory );
     Q_ASSERT( dirInfo.exists() );
     Q_ASSERT( dirInfo.isReadable() );

     QString objectName = dirInfo.baseName();
     this->SetName( objectName );

     // read intrinsic params
     CameraIntrinsicParams params;
     SerializerReader reader;
     QString intrinsicParamsFilename = QString("%1/intrinsic_params.xml").arg( directory );
     QFileInfo intrinsicParamsInfo( intrinsicParamsFilename );
     if( !intrinsicParamsInfo.exists() || !intrinsicParamsInfo.isReadable() )
         Application::GetInstance().Warning( "Camera import", "Intrinsic calibration params (intrinsic_params.xml) is missing or unreadable. Intrinsic camera params will be the default ones." );
     else
     {
        reader.SetFilename( intrinsicParamsFilename.toUtf8().data() );
        reader.Start();
        ::Serialize( &reader, "IntrinsicParams", params );
        reader.Finish();
     }
     SetIntrinsicParams( params );

     // Read calibration matrix
     QString calMatrixFilename = QString("%1/calibMat.xfm").arg( directory );
     QFileInfo calMatrixInfo( calMatrixFilename );
     vtkMatrix4x4 * calMatrix = vtkMatrix4x4::New();
     if( !calMatrixInfo.exists() || !calMatrixInfo.isReadable() )
         Application::GetInstance().Warning( "Camera import", "Calibration matrix file (calibMat.xfm) is missing or unreadable. Calibration matrix will be identity." );
     else
        ReadMatrix( calMatrixFilename, calMatrix );
     SetCalibrationMatrix( calMatrix );
     calMatrix->Delete();

     // read uncal matrices
     ReadMatrices( m_uncalibratedMatrices, directory );

     // read video frames
     ReadImages( m_uncalibratedMatrices.size(), directory, progressDlg );

     if( m_uncalibratedMatrices.size() < 1 || m_capturedFrames.size() < 1 )
     {
         Application::GetInstance().Warning( "Import Camera", "Need at least one frame and corresponding matrix. Make sure directory contains at least uncalMat_0000.xfm and frame_0000.png." );
         success = false;
     }
     else if( m_uncalibratedMatrices.size() != m_capturedFrames.size() )
     {
         Application::GetInstance().Warning( "Import Camera", "Number of images is different than the number of tracking matrices. Make sure each uncalMat_xxxx.xfm has corresponding frame_xxxx.png." );
         success = false;
     }
     else
        SetCurrentFrame( 0 );

     // Update representation in view
     UpdateGeometricRepresentation();

     return success;
 }

vtkImageData * CameraObject::GetVideoOutput()
{   
    if( m_trackable )
        return Application::GetHardwareModule()->GetTrackedVideoOutput();
    else
    {
        Q_ASSERT( m_capturedFrames.size() > 0 && m_currentFrame >= 0 && m_currentFrame < m_capturedFrames.size() );
        return m_capturedFrames[ m_currentFrame ];
    }
}

int CameraObject::GetImageWidth()
{
    if( m_trackable )
        return Application::GetHardwareModule()->GetVideoFrameWidth();
    if( m_capturedFrames.size() > 0 )
        return m_capturedFrames[ 0 ]->GetDimensions()[0];
    return DefaultImageSize[0];
}

int CameraObject::GetImageHeight()
{
    if( m_trackable )
        return Application::GetHardwareModule()->GetVideoFrameHeight();
    if( m_capturedFrames.size() > 0 )
        return m_capturedFrames[ 0 ]->GetDimensions()[1];
    return DefaultImageSize[1];
}

TrackerToolState CameraObject::GetState()
{
    if( m_trackable )
        return Application::GetHardwareModule()->GetVideoTrackerState();
    return Missing;
}

vtkTransform * CameraObject::GetUncalibratedTransform()
{
    if( m_trackable )
        return Application::GetHardwareModule()->GetTrackedVideoUncalibratedTransform();
    return m_uncalibratedTransform;
}

void CameraObject::SetCalibrationMatrix( vtkMatrix4x4 * mat )
{
    if( m_trackable )
        Application::GetHardwareModule()->SetVideoCalibrationMatrix( mat );
    else
        m_calibrationTransform->SetMatrix( mat );
    emit Modified();
}

vtkMatrix4x4 * CameraObject::GetCalibrationMatrix()
{
    if( m_trackable )
        return Application::GetHardwareModule()->GetVideoCalibrationMatrix();
    return m_calibrationTransform->GetMatrix();
}

void CameraObject::AddClient()
{
    if( m_trackable )
        Application::GetHardwareModule()->AddTrackedVideoClient();
}

void CameraObject::RemoveClient()
{
    if( m_trackable )
        Application::GetHardwareModule()->RemoveTrackedVideoClient();
}

void CameraObject::SetMatrices( vtkMatrix4x4 * uncalibratedMatrix, vtkMatrix4x4 * calibrationMatrix )
{
    m_uncalibratedTransform->SetMatrix( uncalibratedMatrix );
    m_calibrationTransform->SetMatrix( calibrationMatrix );
    vtkTransform * localTransform = vtkTransform::New();
    localTransform->Concatenate( m_uncalibratedTransform );
    localTransform->Concatenate( m_calibrationTransform );
    SetLocalTransform( localTransform );
    localTransform->Delete();
    vtkTransform * uncalTrans = vtkTransform::New();
    uncalTrans->SetMatrix( uncalibratedMatrix );
    SetLocalUncalibratedTransform( uncalTrans );
    uncalTrans->Delete();
}

bool CameraObject::Setup( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        // turn on live video acquisition
        AddClient();

        PerViewElements perView;
        perView.cameraBackup = 0;

        vtkPolyDataMapper * camMapper = vtkPolyDataMapper::New();
        camMapper->SetInputData( m_cameraPolyData );
        perView.cameraActor = vtkActor::New();
        perView.cameraActor->SetMapper( camMapper );
        camMapper->Delete();
        perView.cameraActor->SetUserTransform( m_opticalCenterTransform );
        view->GetRenderer()->AddActor( perView.cameraActor );

        perView.cameraAxesActor = vtkAxesActor::New();
        perView.cameraAxesActor->SetTotalLength( 50, 50, 50 );
        perView.cameraAxesActor->AxisLabelsOff();
        perView.cameraAxesActor->SetUserTransform( m_opticalCenterTransform );
        view->GetRenderer()->AddActor( perView.cameraAxesActor );

        perView.cameraTrackerAxesActor = vtkAxesActor::New();
        perView.cameraTrackerAxesActor->SetTotalLength( 50, 50, 50 );
        perView.cameraTrackerAxesActor->AxisLabelsOff();
        perView.cameraTrackerAxesActor->SetUserTransform( this->m_uncalibratedWorldTransform );
        view->GetRenderer()->AddActor( perView.cameraTrackerAxesActor );

        perView.cameraImageActor = vtkSimpleProp3D::New();
        perView.cameraImageActor->SetUserTransform( m_imageTransform );

        perView.cameraImageMapper = vtkIbisImagePlaneMapper::New();
        perView.cameraImageMapper->SetGlobalOpacity( m_globalOpacity );
        perView.cameraImageMapper->SetImageCenter( m_intrinsicParams.m_center[0], m_intrinsicParams.m_center[1] );
        perView.cameraImageMapper->SetLensDistortion( m_intrinsicParams.m_distorsionK1 );
        perView.cameraImageMapper->SetUseTransparency( m_useTransparency );
        perView.cameraImageMapper->SetUseGradient( m_useGradient );
        perView.cameraImageMapper->SetShowMask( m_showMask );
        perView.cameraImageMapper->SetTransparencyPosition( m_transparencyCenter[0], m_transparencyCenter[1] );
        perView.cameraImageMapper->SetTransparencyRadius( m_transparencyRadius[0], m_transparencyRadius[1] );
        perView.cameraImageActor->SetMapper( perView.cameraImageMapper );
        if( m_trackable )
            perView.cameraImageMapper->SetInputDataObject( GetVideoOutput() );
        else if( m_capturedFrames.size() > 0 )
            perView.cameraImageMapper->SetInputDataObject( m_capturedFrames[m_currentFrame] );
        view->GetRenderer()->AddViewProp( perView.cameraImageActor );

        this->m_perViewElements[ view ] = perView;

        if( IsHidden() )
            Hide();

        connect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
    }
    return true;
}

bool CameraObject::Release( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {     
        PerViewElementCont::iterator it = m_perViewElements.find( view );
        if( it != m_perViewElements.end() )
        {
            PerViewElements & perView = (*it).second;
            view->GetRenderer()->RemoveViewProp( perView.cameraActor );
            perView.cameraActor->Delete();
            view->GetRenderer()->RemoveViewProp( perView.cameraAxesActor );
            perView.cameraAxesActor->Delete();
            view->GetRenderer()->RemoveViewProp( perView.cameraTrackerAxesActor );
            perView.cameraTrackerAxesActor->Delete();
            view->GetRenderer()->RemoveViewProp( perView.cameraImageActor );
            perView.cameraImageActor->Delete();
            if( perView.cameraBackup )
            {
                ReleaseControl( 0 );
            }
            ClearDrawingOneView( view, perView );
            m_perViewElements.erase( it );
            disconnect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );

            RemoveClient();
        }
    }
    return true;
}

void CameraObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets )
{
    CameraObjectSettingsWidget * dlg = new CameraObjectSettingsWidget;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->SetCamera(this);
    dlg->setObjectName("Properties");
    widgets->append(dlg);
}

void CameraObject::Hide()
{
    // turn off live video acquisition
    RemoveClient();

    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;

        elem.cameraActor->VisibilityOff();
        elem.cameraImageActor->VisibilityOff();
        elem.cameraAxesActor->VisibilityOff();
        elem.cameraTrackerAxesActor->VisibilityOff();

        ++it;
    }
}

void CameraObject::Show()
{
    // turn on live video acquisition
    AddClient();

    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;

        elem.cameraActor->VisibilityOn();
        elem.cameraImageActor->VisibilityOn();
        elem.cameraAxesActor->VisibilityOn();
        elem.cameraTrackerAxesActor->VisibilityOn();

        ++it;
    }
}

void CameraObject::SetIntrinsicParams( const CameraIntrinsicParams & params )
{
    m_intrinsicParams = params;
    InternalSetIntrinsicParams();
}

void CameraObject::SetVerticalAngleDegrees( double angle )
{
    m_intrinsicParams.SetVerticalAngleDegrees( angle );
    InternalSetIntrinsicParams();
}

void CameraObject::SetImageDistance( double distance )
{
    m_imageDistance = distance;
    UpdateGeometricRepresentation();
    emit Modified();
}

void CameraObject::SetGlobalOpacity( double opacity )
{
    Q_ASSERT( opacity >= 0.0 && opacity <= 1.0 );
    m_globalOpacity = opacity;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetGlobalOpacity( opacity );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetImageCenter( double x, double y )
{
    m_intrinsicParams.m_center[0] = x;
    m_intrinsicParams.m_center[1] = y;
    UpdateGeometricRepresentation();
}

void CameraObject::SetLensDistortion( double dist )
{
    m_intrinsicParams.m_distorsionK1 = dist;
    InternalSetIntrinsicParams();
}

double CameraObject::GetLensDisplacement()
{
    return m_lensDisplacementTransform->GetMatrix()->GetElement( 2, 3 );
}

void CameraObject::SetLensDisplacement( double d )
{
    m_lensDisplacementTransform->GetMatrix()->SetElement( 2, 3, d );
    m_lensDisplacementTransform->Modified();
    emit Modified();
}

void CameraObject::SetTransparencyCenterTracked( bool t )
{
    m_trackedTransparencyCenter = t;
    emit Modified();
}

void CameraObject::SetTransparencyCenter( double x, double y )
{
    m_transparencyCenter[0] = x;
    m_transparencyCenter[1] = y;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetTransparencyPosition( x, y );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetUseTransparency( bool use )
{
    m_useTransparency = use;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetUseTransparency( m_useTransparency );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetUseGradient( bool use )
{
    m_useGradient = use;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetUseGradient( m_useGradient );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetShowMask( bool show )
{
    m_showMask = show;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetShowMask( m_showMask );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetSaturation( double s )
{
    m_saturation = s;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetSaturation( s );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetBrightness( double b )
{
    m_brightness = b;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetBrightness( b );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetTransparencyRadius( double min, double max )
{
    m_transparencyRadius[0] = min;
    m_transparencyRadius[1] = max;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetTransparencyRadius( min, max );
        ++it;
    }
    emit Modified();
}

void CameraObject::SetCameraTrackable( bool t )
{
    m_trackable = t;
    if( m_trackable )
    {
        HardwareModule * hw = Application::GetHardwareModule();
        this->SetLocalTransform( hw->GetTrackedVideoTransform() );
        this->SetLocalUncalibratedTransform( hw->GetTrackedVideoUncalibratedTransform() );
        m_intrinsicParams = hw->GetCameraIntrinsicParams();
        UpdateGeometricRepresentation();
    }
}

void CameraObject::SetTrackCamera( bool t )
{
    if( t == m_trackingCamera )
        return;
    if( t )
    {
        m_trackingCamera = true;

        PerViewElementCont::iterator it = m_perViewElements.begin();
        while( it != m_perViewElements.end() )
        {
            View * v = (*it).first;
            PerViewElements & elem = (*it).second;

            // Tell the view we are taking control
            v->TakeControl( this );

            // Move image to overlay renderer and hide other parts of the cam
            elem.cameraActor->VisibilityOff();
            elem.cameraAxesActor->VisibilityOff();
            v->GetRenderer()->RemoveActor( elem.cameraImageActor );
            v->GetOverlayRenderer()->AddActor( elem.cameraImageActor );

            // Move anotations to overlay renderer
            for( int i = 0; i < elem.anotations.size(); ++i )
            {
                v->GetRenderer()->RemoveViewProp( elem.anotations[i] );
                v->GetOverlayRenderer()->AddViewProp( elem.anotations[i] );
            }

            // Set this camera to control the view
            elem.cameraBackup = v->GetRenderer()->GetActiveCamera();
            elem.cameraBackup->Register( this );
            v->GetRenderer()->SetActiveCamera( m_camera );
            v->GetOverlayRenderer()->SetActiveCamera( m_camera );
            v->GetOverlayRenderer2()->SetActiveCamera( m_camera );

            ++it;
        }
    }
    else
    {
        this->ReleaseControl( 0 );
    }
    emit Modified();
}

bool CameraObject::GetTrackCamera()
{
    return m_trackingCamera;
}

bool CameraObject::IsTransformFrozen()
{
    if( m_trackable )
        return Application::GetHardwareModule()->IsVideoTransformFrozen();
    return false;
}

void CameraObject::FreezeTransform()
{
    if( m_trackable )
        Application::GetHardwareModule()->FreezeVideoTransform( 10 );
    emit Modified();
}

void CameraObject::UnFreezeTransform()
{
    if( m_trackable )
        Application::GetHardwareModule()->UnFreezeVideoTransform();
    emit Modified();
}

void CameraObject::TakeSnapshot()
{
    Q_ASSERT( m_trackable );

    CameraObject * newCam = CameraObject::New();
    newCam->SetName( FindNextSnapshotName() );
    newCam->SetHidden( true );
    newCam->SetIntrinsicParams( m_intrinsicParams );
    newCam->SetImageDistance( m_imageDistance );
    newCam->SetTransparencyCenter( m_transparencyCenter[0], m_transparencyCenter[1] );
    newCam->SetTransparencyRadius( m_transparencyRadius[0], m_transparencyRadius[1] );
    newCam->SetCalibrationMatrix( GetCalibrationMatrix() );
    newCam->AddFrame( GetVideoOutput(), GetUncalibratedTransform()->GetMatrix() );
    newCam->SetCanEditTransformManually( false );

    Application::GetInstance().GetSceneManager()->AddObject( newCam );

    newCam->Delete();
}

void CameraObject::ToggleRecording()
{
    Q_ASSERT( m_trackable );

    if( !m_recordingCamera )
    {
        m_recordingCamera = CameraObject::New();
        m_recordingCamera->SetName( FindNextSnapshotName() );
        m_recordingCamera->SetIntrinsicParams( m_intrinsicParams );
        m_recordingCamera->SetImageDistance( m_imageDistance );
        m_recordingCamera->SetTransparencyCenter( m_transparencyCenter[0], m_transparencyCenter[1] );
        m_recordingCamera->SetTransparencyRadius( m_transparencyRadius[0], m_transparencyRadius[1] );
        m_recordingCamera->SetCalibrationMatrix( GetCalibrationMatrix() );
    }
    else
    {
        this->GetManager()->AddObject( m_recordingCamera );
        m_recordingCamera->Delete();
        m_recordingCamera = 0;
    }
}

bool CameraObject::IsRecording()
{
    return m_recordingCamera != 0;
}

void CameraObject::AddFrame( vtkImageData * image, vtkMatrix4x4 * uncalMat )
{
    vtkImageData * im = vtkImageData::New();
    im->DeepCopy( image );
    m_capturedFrames.push_back( im );
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    mat->DeepCopy( uncalMat );
    m_uncalibratedMatrices.push_back( mat );
    SetCurrentFrame( m_capturedFrames.size() - 1 );
    if( m_capturedFrames.size() == 1 )
        UpdateGeometricRepresentation();
}

void CameraObject::SetCurrentFrame( int frame )
{
    Q_ASSERT( frame < m_capturedFrames.size() && frame >= 0 );
    m_currentFrame = frame;

    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetInputDataObject( m_capturedFrames[ frame ] );
        ++it;
    }

    SetMatrices( m_uncalibratedMatrices[frame], GetCalibrationMatrix() );

    emit Modified();
}

void CameraObject::ReleaseControl( View * triggeredView )
{
    m_trackingCamera = false;
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        View * v = (*it).first;
        PerViewElements & elem = (*it).second;

        if( triggeredView != v )
            v->ReleaseControl( this );

        // Move back image from overlay renderer to main renderer
        v->GetOverlayRenderer()->RemoveActor( elem.cameraImageActor );
        v->GetRenderer()->AddActor( elem.cameraImageActor );
        elem.cameraActor->VisibilityOn();
        elem.cameraAxesActor->VisibilityOn();

        for( int i = 0; i < elem.anotations.size(); ++i )
        {
            v->GetOverlayRenderer()->RemoveViewProp( elem.anotations[i] );
            v->GetRenderer()->AddViewProp( elem.anotations[i] );
        }

        // Recover backup camera
        v->GetRenderer()->SetActiveCamera( elem.cameraBackup );
        v->GetOverlayRenderer()->SetActiveCamera( elem.cameraBackup );
        v->GetOverlayRenderer2()->SetActiveCamera( elem.cameraBackup );
        elem.cameraBackup->UnRegister( this );
        elem.cameraBackup = 0;

        ++it;
    }
}

void CameraObject::WorldToImage( double * worldPos, double & xIm, double & yIm )
{
    double p4[4];
    p4[0] = worldPos[0];
    p4[1] = worldPos[1];
    p4[2] = worldPos[2];
    p4[3] = 1.0;

    // todo: project the 3D position to the 2D image space coordinate
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    mat->DeepCopy( m_opticalCenterTransform->GetMatrix() );
    mat->Invert();
    double pCam[4] = { 0.0, 0.0, 0.0, 1.0 };
    mat->MultiplyPoint( p4, pCam );
    mat->Delete();

    xIm = m_intrinsicParams.m_center[0] + pCam[0] / pCam[2] * m_intrinsicParams.m_focal[0];
    yIm = m_intrinsicParams.m_center[1] + pCam[1] / pCam[2] * m_intrinsicParams.m_focal[1];
    xIm = m_intrinsicParams.m_imageSize[0] - xIm - 1.0;
    yIm = m_intrinsicParams.m_imageSize[1] - yIm - 1.0;
}

#include "simplepropcreator.h"
static double zMin = 0.01;

void CameraObject::DrawLine( double x1, double y1, double x2, double y2, double color[4] )
{
    double start[3];
    start[0] = x1;
    start[1] = y1;
    start[2] = zMin;
    double end[3];
    end[0] = x2;
    end[1] = y2;
    end[2] = zMin;

    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        View * v = (*it).first;
        if( v->GetType() == THREED_VIEW_TYPE )
        {
            vtkProp3D * line = SimplePropCreator::CreateLine( start, end, color );
            line->SetUserTransform( m_imageTransform );
            GetCurrentRenderer(v)->AddViewProp( line );
            PerViewElements & elem = (*it).second;
            elem.anotations.push_back( line );
        }
        ++it;
    }
}

void CameraObject::DrawPath( std::vector< Vec2 > & points, double color[4] )
{
    std::vector< Vec3 > p3d;
    p3d.reserve( points.size() );
    for( int i = 0; i < points.size(); ++i )
        p3d.push_back( Vec3( points[i], zMin ) );

    InternalDrawPath( p3d, color );
}

void CameraObject::DrawWorldPath( std::vector< Vec3 > & points, double color[4] )
{
    std::vector< Vec3 > p3d;
    p3d.reserve( points.size() );
    for( int i = 0; i < points.size(); ++i )
    {
        double x, y;
        WorldToImage( points[i].Ref(), x, y );
        p3d.push_back( Vec3( x, y, zMin ) );
    }

    InternalDrawPath( p3d, color );
}

void CameraObject::InternalDrawPath( std::vector< Vec3 > & p3d, double color[4] )
{
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        View * v = (*it).first;
        if( v->GetType() == THREED_VIEW_TYPE )
        {
            vtkProp3D * line = SimplePropCreator::CreatePath( p3d, color );
            line->SetUserTransform( m_imageTransform );
            GetCurrentRenderer(v)->AddViewProp( line );
            PerViewElements & elem = (*it).second;
            elem.anotations.push_back( line );
        }
        ++it;
    }
}

void CameraObject::DrawRect( double x, double y, double width, double height, double color[4] )
{
    DrawLine( x, y, x + width, y, color );
    DrawLine( x + width, y, x + width, y + height, color );
    DrawLine( x + width, y + height, x, y + height, color );
    DrawLine( x, y + height, x, y, color );
}

void CameraObject::ClearDrawing()
{
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        View * v = (*it).first;
        if( v->GetType() == THREED_VIEW_TYPE )
        {
            ClearDrawingOneView( v, (*it).second );
        }
        ++it;
    }
}

void CameraObject::VideoUpdatedSlot()
{
    Q_ASSERT( m_trackable );

    if( IsRecording() && GetState() == Ok )
    {
        m_recordingCamera->AddFrame( GetVideoOutput(), GetUncalibratedTransform()->GetMatrix() );
    }

    // Update representation if image size changed
    HardwareModule * hw = Application::GetHardwareModule();
    if( hw->GetVideoFrameWidth() != m_cachedImageSize[0] || hw->GetVideoFrameHeight() != m_cachedImageSize[1] )
        UpdateGeometricRepresentation();

    // Get the position of the transparency center from projection of the tracked pointer
    if( m_trackedTransparencyCenter )
    {
        Q_ASSERT( this->GetManager() );
        PointerObject * navPointer = this->GetManager()->GetNavigationPointerObject();
        if( navPointer )
        {
            // Get world space position of the tip
            double * tip = navPointer->GetTipPosition();
            double cx, cy;
            WorldToImage( tip, cx, cy );

            // Set new center for transparency
            SetTransparencyCenter( cx, cy );
        }
    }

    UpdateVtkCamera();

    emit VideoUpdatedSignal();
    emit Modified();
}

void CameraObject::ObjectAddedToScene()
{
    if( m_trackable )
        connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(VideoUpdatedSlot()) );
}

void CameraObject::ObjectAboutToBeRemovedFromScene()
{
    if( m_trackable )
        disconnect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(VideoUpdatedSlot()) );
    if( m_trackingCamera )
        SetTrackCamera( false );
}

void CameraObject::InternalWorldTransformChanged()
{
    UpdateVtkCamera();
}

void CameraObject::InternalSetIntrinsicParams()
{
    UpdateGeometricRepresentation();
    if( m_trackable )
        Application::GetHardwareModule()->SetCameraIntrinsicParams( m_intrinsicParams );
}

void CameraObject::UpdateGeometricRepresentation()
{
    PerViewElementCont::iterator it = m_perViewElements.begin();
    while( it != m_perViewElements.end() )
    {
        PerViewElements & elem = (*it).second;
        elem.cameraImageMapper->SetImageCenter( m_intrinsicParams.m_center[0] , m_intrinsicParams.m_center[1] );
        elem.cameraImageMapper->SetLensDistortion( m_intrinsicParams.m_distorsionK1 );
        ++it;
    }
    m_camera->SetViewAngle( m_intrinsicParams.GetVerticalAngleDegrees() );

    // Determine image size
    int width = GetImageWidth();
    int height = GetImageHeight();

    // Update transparency position and radius if image size has changed
    if( width != m_cachedImageSize[0] || height != m_cachedImageSize[1] )
    {
        m_transparencyCenter[0] = m_transparencyCenter[0] / m_cachedImageSize[0] * width;
        m_transparencyCenter[1] = m_transparencyCenter[1] / m_cachedImageSize[1] * height;
        m_transparencyRadius[0] = m_transparencyRadius[0] / m_cachedImageSize[0] * width;
        m_transparencyRadius[1] = m_transparencyRadius[1] / m_cachedImageSize[0] * width;
    }

    double angleRad = 0.5 * m_intrinsicParams.GetVerticalAngleRad();
    double offsetY = m_imageDistance * tan( angleRad );
    double offsetX = width / ((double)height) * offsetY;
    double scaleFactor = ( 2 * offsetY ) / height;

    vtkPoints * pts = m_cameraPolyData->GetPoints();
    pts->SetPoint( 1, -offsetX, -offsetY, -m_imageDistance );
    pts->SetPoint( 2, offsetX, -offsetY, -m_imageDistance );
    pts->SetPoint( 3, offsetX, offsetY, -m_imageDistance );
    pts->SetPoint( 4, -offsetX, offsetY, -m_imageDistance );
    m_cameraPolyData->Modified();

    m_imageTransform->Identity();
    m_imageTransform->Translate( -offsetX, -offsetY, -m_imageDistance );
    m_imageTransform->Scale( scaleFactor, scaleFactor, scaleFactor );

    m_cachedImageSize[ 0 ] = width;
    m_cachedImageSize[ 1 ] = height;

    emit Modified();
}

void CameraObject::SetLocalUncalibratedTransform( vtkTransform * t )
{
    if( m_uncalibratedTransform )
        m_uncalibratedTransform->UnRegister( this );

    m_uncalibratedTransform = t;

    if( m_uncalibratedTransform )
        m_uncalibratedTransform->Register( this );


    // reset the list of concatenated transforms
    this->m_uncalibratedWorldTransform->Identity();
    if( Parent )
        this->m_uncalibratedWorldTransform->Concatenate( Parent->GetWorldTransform() );
    if( m_uncalibratedTransform )
        this->m_uncalibratedWorldTransform->Concatenate( m_uncalibratedTransform );
}

void CameraObject::UpdateWorldTransform()
{
    // call the parent class
    SceneObject::UpdateWorldTransform();

    // Also update uncal world transform
    this->m_uncalibratedWorldTransform->Identity();
    if( Parent )
        this->m_uncalibratedWorldTransform->Concatenate( Parent->GetWorldTransform() );
    if( m_uncalibratedTransform )
        this->m_uncalibratedWorldTransform->Concatenate( m_uncalibratedTransform );
}

void CameraObject::UpdateVtkCamera()
{
    m_camera->SetPosition( 0.0, 0.0, 0.0 );
    m_camera->SetFocalPoint( 0.0, 0.0, -m_imageDistance );
    m_camera->SetViewUp( 0, 1, 0 );
    vtkTransform * wt = m_opticalCenterTransform;
    wt->Update();
    m_camera->ApplyTransform( wt );
}

vtkRenderer * CameraObject::GetCurrentRenderer( View * v )
{
    if( m_trackingCamera )
        return v->GetOverlayRenderer();
    else
        return v->GetRenderer();
}

void CameraObject::CreateCameraRepresentation()
{
    vtkPoints * pts = vtkPoints::New();
    pts->InsertNextPoint( 0.0, 0.0, 0.0 );
    pts->InsertNextPoint( -100.0, -100.0, -m_imageDistance );
    pts->InsertNextPoint( 100.0, -100.0, -m_imageDistance );
    pts->InsertNextPoint( 100.0, 100.0, -m_imageDistance );
    pts->InsertNextPoint( -100.0, 100.0, -m_imageDistance );

    static vtkIdType linesIndex[8][2]= { {0,1}, {0,2}, {0,3}, {0,4}, {1,2}, {2,3}, {3,4}, {4,1} };
    vtkCellArray * lines = vtkCellArray::New();
    for( int i = 0; i < 8; ++i )
        lines->InsertNextCell( 2, linesIndex[i] );

    m_cameraPolyData = vtkPolyData::New();
    m_cameraPolyData->SetPoints( pts );
    pts->Delete();
    m_cameraPolyData->SetLines( lines );
    lines->Delete();
}

QString CameraObject::FindNextSnapshotName()
{
    QList<CameraObject*> camObjects;
    Application::GetInstance().GetSceneManager()->GetAllCameraObjects( camObjects );
    int index = 0;
    while( 1 )
    {
        QString newName = QString("Cam snap %1").arg( index );
        bool found = false;
        for( int i = 0; i < camObjects.size(); ++i )
        {
            if( camObjects[i]->GetName() == newName )
            {
                found = true;
                break;
            }
        }
        if( !found )
            return newName;
        ++index;
    }
}

void CameraObject::ReadMatrix( QString filename, vtkMatrix4x4 * mat )
{
    vtkXFMReader * reader = vtkXFMReader::New();
    reader->SetFileName( filename.toUtf8().data() );
    reader->SetMatrix( mat );
    reader->Update();
    reader->Delete();
}

void CameraObject::ReadMatrices( std::vector< vtkMatrix4x4 * > & matrices, QString dirName )
{
    int index = 0;
    bool done = false;
    while( !done )
    {
        QString uncalMatrixFilename = QString("%1/uncalMat_%2.xfm").arg( dirName ).arg( index, 4, 10, QLatin1Char('0') );
        QFileInfo uncalMatInfo( uncalMatrixFilename );
        if( uncalMatInfo.exists() )
        {
            vtkMatrix4x4 * uncalMat = vtkMatrix4x4::New();
            ReadMatrix( uncalMatrixFilename, uncalMat );
            matrices.push_back( uncalMat );
        }
        else
            done = true;
        ++index;
    }
}

void CameraObject::WriteMatrix( vtkMatrix4x4 * mat, QString filename )
{
    vtkXFMWriter * writer = vtkXFMWriter::New();
    writer->SetFileName( filename.toUtf8().data() );
    writer->SetMatrix( mat );
    writer->Write();
}

void CameraObject::WriteMatrices( std::vector< vtkMatrix4x4 * > & matrices, QString dirName )
{
    for( int i = 0; i < matrices.size(); ++i )
    {
        QString uncalMatrixFilename = dirName + QString("/uncalMat_%1.xfm").arg( i, 4, 10, QLatin1Char('0') );
        WriteMatrix( m_uncalibratedMatrices[i], uncalMatrixFilename );
    }
}

#include "vtkPNGWriter.h"

void CameraObject::WriteImages( QString dirName, QProgressDialog * progressDlg )
{
    vtkPNGWriter * writer = vtkPNGWriter::New();
    for( int i = 0; i < m_capturedFrames.size(); ++i )
    {
        QString filename = dirName + QString("/frame_%1").arg( i, 4, 10, QLatin1Char('0') );
        writer->SetFileName( filename.toUtf8().data() );
        writer->SetInputData( m_capturedFrames[i] );
        writer->Write();

        if( progressDlg )
            Application::GetInstance().UpdateProgress( progressDlg, (int)round( (float)i / m_capturedFrames.size() * 100.0 ) );
    }
    writer->Delete();
}

#include "vtkPNGReader.h"

void CameraObject::ReadImages( int nbImages, QString dirName, QProgressDialog * progressDlg )
{
    vtkPNGReader * reader = vtkPNGReader::New();
    for( int i = 0; i < nbImages; ++i )
    {
        QString filename = dirName + QString("/frame_%1").arg( i, 4, 10, QLatin1Char('0') );
        reader->SetFileName( filename.toUtf8().data() );
        reader->Update();
        vtkImageData * image = vtkImageData::New();
        image->DeepCopy( reader->GetOutput() );
        m_capturedFrames.push_back( image );

        if( progressDlg )
            Application::GetInstance().UpdateProgress( progressDlg, (int)round( (float)i / nbImages * 100.0 ) );
    }
    reader->Delete();
}

void CameraObject::ClearDrawingOneView( View * v, PerViewElements & elem )
{
    for( int i = 0; i < elem.anotations.size(); ++i )
    {
        vtkProp3D * anotProp = elem.anotations[i];
        GetCurrentRenderer( v )->RemoveViewProp( anotProp );
        anotProp->Delete();
    }
    elem.anotations.clear();
}
