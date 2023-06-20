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

#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <QElapsedTimer>
#include <QTimer>
#include "cameracalibrationplugininterface.h"
#include "cameracalibrationwidget.h"
#include "cameracalibrator.h"
#include "cameraobject.h"
#include "ui_cameracalibrationwidget.h"
#include "vtkCircleWithCrossSource.h"
#include "vtkMatrix4x4Operators.h"

CameraCalibrationWidget::CameraCalibrationWidget( QWidget * parent )
    : QWidget( parent ), ui( new Ui::CameraCalibrationWidget )
{
    m_pluginInterface = nullptr;

    ui->setupUi( this );

    setWindowTitle( "Camera calibration" );

    m_imageActor = vtkImageActor::New();
    m_imageActor->InterpolateOff();

    m_renderer = vtkRenderer::New();
    m_renderer->AddViewProp( m_imageActor );

    GetRenderWindow()->AddRenderer( m_renderer );

    vtkInteractorStyleImage * interactorStyle = vtkInteractorStyleImage::New();
    GetInteractor()->SetInteractorStyle( interactorStyle );
    interactorStyle->Delete();

    m_accumulationTime = new QElapsedTimer;
}

CameraCalibrationWidget::~CameraCalibrationWidget()
{
    if( m_pluginInterface && m_pluginInterface->IsAccumulating() ) m_pluginInterface->CancelAccumulation();

    this->DeleteGrid();

    GetRenderWindow()->RemoveRenderer( m_renderer );
    m_renderer->RemoveViewProp( m_imageActor );
    m_renderer->Delete();
    m_imageActor->Delete();

    delete m_accumulationTime;

    delete ui;
}

vtkRenderWindow * CameraCalibrationWidget::GetRenderWindow() { return ui->videoWidget->GetRenderWindow(); }

vtkRenderWindowInteractor * CameraCalibrationWidget::GetInteractor() { return ui->videoWidget->GetInteractor(); }

void CameraCalibrationWidget::SetPluginInterface( CameraCalibrationPluginInterface * pluginInterface )
{
    m_pluginInterface = pluginInterface;

    this->CreateGrid();

    CameraObject * currentCamera = m_pluginInterface->GetCurrentCameraObject();
    if( currentCamera )
    {
        m_imageActor->SetInputData( currentCamera->GetVideoOutput() );
        connect( currentCamera, SIGNAL( VideoUpdatedSignal() ), this, SLOT( UpdateDisplay() ) );
        currentCamera->AddClient();
        this->RenderFirst();
    }

    UpdateUi();
}

void CameraCalibrationWidget::UpdateDisplay()
{
    if( m_pluginInterface->IsAccumulating() && m_accumulationTime->elapsed() > 4000 )
    {
        m_pluginInterface->CancelAccumulation();
    }

    CameraObject * currentCam = m_pluginInterface->GetCurrentCameraObject();
    if( currentCam )
    {
        // Get the latest frame
        vtkImageData * imageVtk = currentCam->GetVideoOutput();
        int dims[3];
        imageVtk->GetDimensions( dims );

        // Compute the corners of the grid
        std::vector<cv::Point2f> currentImagePoints;
        bool found = m_pluginInterface->GetCameraCalibrator()->DetectGrid( imageVtk, currentImagePoints );

        // Show/hide the grid depending on weather it is found or not
        ShowGrid( found );

        if( found )
        {
            // Accumulate view if needed
            if( m_pluginInterface->IsAccumulating() )
            {
                vtkMatrix4x4 * trackerMatrix = currentCam->GetUncalibratedTransform()->GetMatrix();
                m_pluginInterface->AccumulateView( imageVtk, currentImagePoints, trackerMatrix );
            }

            // update the position of markers
            std::vector<cv::Point2f>::iterator itPoints = currentImagePoints.begin();
            std::vector<vtkActor *>::iterator itActors  = m_markerActors.begin();
            while( itPoints != currentImagePoints.end() && itActors != m_markerActors.end() )
            {
                vtkActor * a        = *itActors;
                cv::Point2f & point = *itPoints;
                double y            = dims[1] - point.y - 1.0;
                a->SetPosition( point.x, y, 0.0 );
                ++itPoints;
                ++itActors;
            }
        }
    }

    this->UpdateUi();
    this->GetRenderWindow()->Render();
}

void CameraCalibrationWidget::CreateGrid()
{
    vtkCircleWithCrossSource * source = vtkCircleWithCrossSource::New();
    source->SetRadius( 5 );
    source->SetResolution( 5 );
    double basePos[2] = {50, 50};
    int patternWidth  = m_pluginInterface->GetCalibrationGridWidth();
    int patternHeight = m_pluginInterface->GetCalibrationGridHeight();
    double offset     = m_pluginInterface->GetCalibrationGridCellSize();

    double hueRowOffset = 1.0 / patternHeight;
    double hueColOffset = hueRowOffset / ( 2 * patternWidth );
    double hueRow       = 0.0;
    double hue          = 0.0;

    for( int j = 0; j < patternHeight; ++j )
    {
        hue = hueRow;
        for( int i = 0; i < patternWidth; ++i )
        {
            double color[3];
            if( i == 0 && j == 0 )
            {
                color[0] = 0.0;
                color[1] = 0.0;
                color[2] = 0.0;
            }
            else if( i == patternWidth - 1 && j == patternHeight - 1 )
            {
                color[0] = 1.0;
                color[1] = 1.0;
                color[2] = 1.0;
            }
            else
            {
                vtkMath::HSVToRGB( hue, 1.0, 1.0, &color[0], &color[1], &color[2] );
            }
            vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
            mapper->SetInputConnection( source->GetOutputPort() );
            vtkActor * actor = vtkActor::New();
            actor->SetMapper( mapper );
            actor->SetPosition( basePos[0] + i * offset, basePos[1] + j * offset, 0.0 );
            actor->GetProperty()->SetColor( color );
            m_renderer->AddActor( actor );
            mapper->Delete();
            m_markerActors.push_back( actor );
            hue += hueColOffset;
        }

        hueRow += hueRowOffset;
    }
}

void CameraCalibrationWidget::DeleteGrid()
{
    std::vector<vtkActor *>::iterator it = m_markerActors.begin();
    while( it != m_markerActors.end() )
    {
        vtkActor * actor = *it;
        m_renderer->RemoveActor( actor );
        actor->Delete();
        ++it;
    }
    m_markerActors.clear();
}

void CameraCalibrationWidget::ShowGrid( bool show )
{
    std::vector<vtkActor *>::iterator it = m_markerActors.begin();
    while( it != m_markerActors.end() )
    {
        vtkActor * actor = *it;
        if( show )
            actor->VisibilityOn();
        else
            actor->VisibilityOff();
        ++it;
    }
}

void CameraCalibrationWidget::RenderFirst()
{
    int * extent  = m_imageActor->GetInput()->GetExtent();
    int diffx     = extent[1] - extent[0] + 1;
    double scalex = static_cast<double>( diffx ) / 2.0;
    int diffy     = extent[3] - extent[2] + 1;
    double scaley = static_cast<double>( diffy ) / 2.0;

    vtkCamera * cam = m_renderer->GetActiveCamera();
    cam->ParallelProjectionOn();
    cam->SetParallelScale( scaley );
    double * prevPos   = cam->GetPosition();
    double * prevFocal = cam->GetFocalPoint();
    cam->SetPosition( scalex, scaley, prevPos[2] );
    cam->SetFocalPoint( scalex, scaley, prevFocal[2] );

    GetRenderWindow()->Render();
}

void CameraCalibrationWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );

    bool isAccum = m_pluginInterface->IsAccumulating();

    // Calibration group
    ui->calibrateGroupBox->setEnabled( !isAccum );

    // capturing views can have different requirements depending on the mode
    // so once views have been capture in one mode, we don't allow changing
    bool hasViews = m_pluginInterface->GetCameraCalibrator()->GetNumberOfViews() > 0;
    ui->calibrateGroupBox->setEnabled( !hasViews && !isAccum );

    ui->intrinsicRadioButton->blockSignals( true );
    ui->extrinsicRadioButton->blockSignals( true );
    ui->bothRadioButton->blockSignals( true );
    bool intrinsic = m_pluginInterface->GetComputeIntrinsic();
    bool extrinsic = m_pluginInterface->GetComputeExtrinsic();
    ui->intrinsicRadioButton->setChecked( intrinsic && !extrinsic );
    ui->extrinsicRadioButton->setEnabled(
        false );  // For now it is not possible to do only extrinsic calib, this is work in progress
    ui->extrinsicRadioButton->setChecked( extrinsic && !intrinsic );
    ui->bothRadioButton->setChecked( intrinsic && extrinsic );
    ui->intrinsicRadioButton->blockSignals( false );
    ui->extrinsicRadioButton->blockSignals( false );
    ui->bothRadioButton->blockSignals( false );

    // Intrinsic group
    ui->intrinsicGroupBox->setEnabled( !isAccum );

    ui->computeCenterCheckBox->blockSignals( true );
    ui->computeCenterCheckBox->setChecked( m_pluginInterface->GetComputeCenter() );
    ui->computeCenterCheckBox->blockSignals( false );

    ui->computeDistortionCheckBox->blockSignals( true );
    ui->computeDistortionCheckBox->setChecked( m_pluginInterface->GetComputeDistortion() );
    ui->computeDistortionCheckBox->blockSignals( false );

    // Views group
    ui->viewsGroupBox->setEnabled( !isAccum );

    ui->accumulateCheckBox->blockSignals( true );
    ui->accumulateCheckBox->setChecked( m_pluginInterface->IsUsingAccumulation() );
    ui->accumulateCheckBox->blockSignals( false );

    if( isAccum )
    {
        QString bt = QString( "%1 of %2" )
                         .arg( m_pluginInterface->GetNumberOfAccumulatedViews() )
                         .arg( m_pluginInterface->GetNumberOfViewsToAccumulate() );
        ui->captureViewButton->setText( bt );
    }
    else
    {
        ui->captureViewButton->setText( "Capture" );
    }

    // Calibration results
    CameraObject * currentCamera = m_pluginInterface->GetCurrentCameraObject();
    if( currentCamera )
    {
        QString calibResults = QString( "===== Intrinsic results =====\n\n" );
        calibResults +=
            QString( "Number of views: %1\n" ).arg( m_pluginInterface->GetCameraCalibrator()->GetNumberOfViews() );
        const CameraIntrinsicParams & params = currentCamera->GetIntrinsicParams();
        calibResults += QString( "Intrinsic reprojection error: %1\n" ).arg( params.m_reprojectionError );
        calibResults += QString( "Focal: ( %1, %2 )\n" ).arg( params.m_focal[0] ).arg( params.m_focal[1] );
        calibResults += QString( "Center: ( %1, %2 )\n" ).arg( params.m_center[0] ).arg( params.m_center[1] );
        calibResults += QString( "Distortion: %1\n" ).arg( params.m_distorsionK1 );
        calibResults += QString( "Vertical angle: %1\n" ).arg( params.GetVerticalAngleDegrees() );
        calibResults += QString( "\n===== Extrinsic results =====\n\n" );
        vtkMatrix4x4 * calibrationMat = currentCamera->GetCalibrationMatrix();
        double translation[3]         = {0.0, 0.0, 0.0};
        double rotation[3]            = {0.0, 0.0, 0.0};
        vtkMatrix4x4Operators::MatrixToTransRot( calibrationMat, translation, rotation );
        calibResults += QString( "Translation: ( %1, %2, %3 )\n" )
                            .arg( translation[0] )
                            .arg( translation[1] )
                            .arg( translation[2] );
        calibResults +=
            QString( "Rotation: ( %1, %2, %3 )\n" ).arg( rotation[0] ).arg( rotation[1] ).arg( rotation[2] );
        ui->calibrationResultTextEdit->setPlainText( calibResults );
    }
    else
        ui->calibrationResultTextEdit->setPlainText( QString( "No current camera selected" ) );
}

void CameraCalibrationWidget::on_clearCalibrationViewsButton_clicked()
{
    m_pluginInterface->ClearCalibrationData();
    UpdateUi();
}

void CameraCalibrationWidget::on_computeCenterCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetComputeCenter( checked );
    UpdateUi();
}

void CameraCalibrationWidget::on_computeDistortionCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetComputeDistorsion( checked );
    UpdateUi();
}

void CameraCalibrationWidget::on_accumulateCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetUseAccumulation( checked );
    UpdateUi();
}

void CameraCalibrationWidget::on_captureViewButton_clicked()
{
    Q_ASSERT( m_pluginInterface );

    if( m_pluginInterface->IsUsingAccumulation() )  // start accumulating
    {
        m_pluginInterface->StartAccumulating();
        m_accumulationTime->restart();
    }
    else  // capture
    {
        m_pluginInterface->CaptureCalibrationView();
    }
}

void CameraCalibrationWidget::on_intrinsicRadioButton_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    if( checked )
    {
        m_pluginInterface->SetComputeExtrinsic( false );
        m_pluginInterface->SetComputeIntrinsic( true );
    }
}

void CameraCalibrationWidget::on_extrinsicRadioButton_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    if( checked )
    {
        m_pluginInterface->SetComputeExtrinsic( true );
        m_pluginInterface->SetComputeIntrinsic( false );
    }
}

void CameraCalibrationWidget::on_bothRadioButton_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    if( checked )
    {
        m_pluginInterface->SetComputeExtrinsic( true );
        m_pluginInterface->SetComputeIntrinsic( true );
    }
}

void CameraCalibrationWidget::on_optimizeGridDetectCheckBox_toggled( bool checked )
{
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->SetOptimizeGridDetection( checked );
}
