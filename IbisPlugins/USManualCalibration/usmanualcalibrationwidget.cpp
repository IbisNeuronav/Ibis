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

#include "usmanualcalibrationwidget.h"
#include "usmanualcalibrationplugininterface.h"
#include "ui_usmanualcalibrationwidget.h"
#include <vtkImageActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkInteractorStyleImage.h>
#include <vtkImageData.h>
#include <vtkCamera.h>
#include <vtkTransform.h>
#include <vtkEventQtSlotConnect.h>
#include <QTimer>
#include "vtkNShapeCalibrationWidget.h"
#include "ibisapi.h"

USManualCalibrationWidget::USManualCalibrationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::USManualCalibrationWidget)
{
    m_pluginInterface = 0;

    m_frozenImage = vtkImageData::New();
    m_frozenMatrix = vtkMatrix4x4::New();
    m_imageFrozen = false;
    m_frozenState = Undefined;

    ui->setupUi(this);

    setWindowTitle( "US Manual Calibration" );

    m_imageActor = vtkImageActor::New();
    m_imageActor->InterpolateOff();

    m_renderer = vtkRenderer::New();
    m_renderer->AddViewProp( m_imageActor );

    GetRenderWindow()->AddRenderer( m_renderer );

    vtkInteractorStyleImage * interactorStyle = vtkInteractorStyleImage::New();
    GetInteractor()->SetInteractorStyle( interactorStyle );
    interactorStyle->Delete();

    for( int i = 0; i < 4; ++i )
        m_manipulators[ i ] = 0;
    m_manipulatorsCallbacks = 0;
}

USManualCalibrationWidget::~USManualCalibrationWidget()
{
    EnableManipulators( false );

    GetRenderWindow()->RemoveRenderer( m_renderer );
    m_renderer->RemoveViewProp( m_imageActor );
    m_renderer->Delete();
    m_imageActor->Delete();

    delete ui;
}

vtkRenderWindow * USManualCalibrationWidget::GetRenderWindow()
{
    return ui->videoWidget->GetRenderWindow();
}

vtkRenderWindowInteractor * USManualCalibrationWidget::GetInteractor()
{
    return ui->videoWidget->GetInteractor();
}

void USManualCalibrationWidget::SetPluginInterface( USManualCalibrationPluginInterface * pluginInterface )
{
    m_pluginInterface = pluginInterface;

    UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
    if( probe )
    {
        m_imageActor->SetInputData( probe->GetVideoOutput() );
        connect( m_pluginInterface->GetIbisAPI(), SIGNAL(IbisClockTick()), this, SLOT(NewFrameSlot()) );
        probe->AddClient();
        this->RenderFirst();
    }

    EnableManipulators( true );

    UpdateUi();
}

void USManualCalibrationWidget::NewFrameSlot()
{
    UpdateDisplay();
}

void USManualCalibrationWidget::UpdateDisplay()
{
    if( !m_imageFrozen )
        UpdateManipulators();
    this->UpdateUSProbeStatus();
    this->GetRenderWindow()->Render();
}

void USManualCalibrationWidget::RenderFirst()
{
    int * extent = m_imageActor->GetInput()->GetExtent();
    int diffx = extent[1] - extent[0] + 1;
    double scalex = (double)diffx / 2.0;
    int diffy = extent[3] - extent[2] + 1;
    double scaley = (double)diffy / 2.0;

    vtkCamera * cam = m_renderer->GetActiveCamera();
    cam->ParallelProjectionOn();
    cam->SetParallelScale( scaley );
    double * prevPos = cam->GetPosition();
    double * prevFocal = cam->GetFocalPoint();
    cam->SetPosition( scalex, scaley, prevPos[2] );
    cam->SetFocalPoint( scalex, scaley, prevFocal[2] );

    GetRenderWindow()->Render();
}

void USManualCalibrationWidget::UpdateUi()
{
    //CameraCalibrator * calib = m_pluginInterface->GetCameraCalibrator();
    //QString calibResults = QString( "Number of views: %1\n" ).arg( calib->GetNumberOfViews() );
    //calibResults += QString("Reprojection error: %1\n").arg( calib->GetReprojectionError() );
    //ui->calibrationResultTextEdit->setPlainText( calibResults );
    ui->resetButton->setEnabled( m_imageFrozen );
    ui->depthComboBox->setEnabled( !m_imageFrozen );
}

void USManualCalibrationWidget::EnableManipulators( bool on )
{
    if( on )
    {
        Q_ASSERT( m_manipulatorsCallbacks == 0 );
        m_manipulatorsCallbacks = vtkEventQtSlotConnect::New();

        for( int i = 0; i < 4; ++i )
        {
            m_manipulators[i] = vtkNShapeCalibrationWidget::New();
            m_manipulators[i]->SetPoint1( 0.0, 0.0, 0.0 );   // default position, doesn't really matter
            m_manipulators[i]->SetPoint2( 200.0, 0.0, 0.0 );
            m_manipulators[i]->SetHandlesSize( 2.5 );
            m_manipulators[i]->SetInteractor( GetInteractor() );
            m_manipulators[i]->SetPriority( 1.0 );
            m_manipulatorsCallbacks->Connect( m_manipulators[i], vtkCommand::InteractionEvent, this, SLOT(OnManipulatorsModified()) );
            m_manipulatorsCallbacks->Connect( m_manipulators[i], vtkCommand::StartInteractionEvent, this, SLOT(OnManipulatorsModified()) );
            m_manipulatorsCallbacks->Connect( m_manipulators[i], vtkCommand::EndInteractionEvent, this, SLOT(OnManipulatorsModified()) );
        }

        for( int i = 0; i < 4; ++i )
        {
            m_manipulators[i]->EnabledOn();
        }

        UpdateManipulators();
    }
    else
    {
        Q_ASSERT( m_manipulatorsCallbacks != 0 );
        if( m_manipulatorsCallbacks )
        {
            m_manipulatorsCallbacks->Delete();
            m_manipulatorsCallbacks = 0;
        }

        for( int i = 0; i < 4; i++ )
        {
            m_manipulators[i]->SetInteractor( 0 );
            m_manipulators[i]->EnabledOff();
            m_manipulators[i]->SetInteractor( 0 );
            m_manipulators[i]->Delete();
            m_manipulators[i] = 0;
        }
    }
}

#include "sceneobject.h"

bool FindIntersection( int manip, int seg, vtkMatrix4x4 * phantomMat, vtkMatrix4x4 * probeMat, USManualCalibrationPluginInterface * pi, double intersect[3] )
{
    // Transform extremities of the segment into image space
    const double * pt1 = pi->GetPhantomPoint( manip, seg );
    double pt1p[ 4 ]; pt1p[0] = pt1[0]; pt1p[1] = pt1[1]; pt1p[2] = pt1[2]; pt1p[3] = 1.0;
    phantomMat->MultiplyPoint( pt1p, pt1p );
    probeMat->MultiplyPoint( pt1p, pt1p );

    const double * pt2 = pi->GetPhantomPoint( manip, seg + 1 );
    double pt2p[ 4 ]; pt2p[0] = pt2[0]; pt2p[1] = pt2[1]; pt2p[2] = pt2[2]; pt2p[3] = 1.0;
    phantomMat->MultiplyPoint( pt2p, pt2p );
    probeMat->MultiplyPoint( pt2p, pt2p );

    // if US plane doesn't intersect with current segment
    if( ( pt1p[2] > 0.0 && pt2p[2] > 0.0 ) || ( pt1p[2] < 0.0 && pt2p[2] < 0.0 ) )
        return false;

    double ratio = pt1p[2] / ( pt2p[2] - pt1p[2] );
    for( int i = 0; i < 3; ++i )
        intersect[i] = pt1p[i] - ratio * ( pt2p[i] - pt1p[i] );

    return true;
}

void USManualCalibrationWidget::UpdateManipulators()
{
    for( int manip = 0; manip < 4; ++manip )
    {
        if( m_manipulators[ manip ] == 0 )
            return;
    }

    vtkMatrix4x4 * probeMat = vtkMatrix4x4::New();
    UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
    if( probe )
    {
        probe->GetWorldTransform()->GetMatrix( probeMat );
    }
    probeMat->Invert();

    vtkMatrix4x4 * phantomMat = vtkMatrix4x4::New();
    SceneObject * phantom = m_pluginInterface->GetPhantomWiresObject();
    phantom->GetWorldTransform()->GetMatrix( phantomMat );

    // for each manipulator
    for( int manip = 0; manip < 4; ++manip )
    {
        // for each segment of the N
        for( int seg = 0; seg < 3; ++seg )
        {
            double pt1Intersect[3];
            bool pt1Valid = FindIntersection( manip, 0, phantomMat, probeMat, m_pluginInterface, pt1Intersect );

            double pt2Intersect[3];
            bool pt2Valid = FindIntersection( manip, 1, phantomMat, probeMat, m_pluginInterface, pt2Intersect );

            double pt3Intersect[3];
            bool pt3Valid = FindIntersection( manip, 2, phantomMat, probeMat, m_pluginInterface, pt3Intersect );

            if( pt1Valid && pt2Valid && pt3Valid )
            {
                m_manipulators[ manip ]->SetPoint1( pt1Intersect );
                m_manipulators[ manip ]->SetPoint2( pt3Intersect );
                m_manipulators[ manip ]->SetMiddlePoint( pt2Intersect );
                m_manipulators[ manip ]->EnabledOn();
            }
            else
                m_manipulators[ manip ]->EnabledOff();
        }
    }

    probeMat->Delete();
    phantomMat->Delete();
}

void USManualCalibrationWidget::ComputeCalibration()
{

}

void USManualCalibrationWidget::UpdateUSProbeStatus()
{
    TrackerToolState newState = m_frozenState;
    if( !m_imageFrozen )
    {
        UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
        if( probe )
            newState = probe->GetState();
        else
            newState = Undefined;
    }

    switch( newState )
    {
    case Ok:
        ui->statusLabel->setText( "OK" );
        ui->statusLabel->setStyleSheet("background-color: lightGreen");
        break;
    case Missing:
        ui->statusLabel->setText( "Missing" );
        ui->statusLabel->setStyleSheet("background-color: red");
        break;
    case OutOfVolume:
        ui->statusLabel->setText( "Out of volume" );
        ui->statusLabel->setStyleSheet("background-color: yellow");
        break;
    case OutOfView:
        ui->statusLabel->setText( "Out of view" );
        ui->statusLabel->setStyleSheet("background-color: red");
        break;
	case HighError:
		ui->statusLabel->setText( "High error" );
		ui->statusLabel->setStyleSheet("background-color: red");
		break;
	case Disabled:
		ui->statusLabel->setText( "Disabled" );
		ui->statusLabel->setStyleSheet("background-color: grey");
		break;
    case Undefined:
        ui->statusLabel->setText( "Tracker not initialized" );
        ui->statusLabel->setStyleSheet("background-color: grey");
        break;
    }
}

#include "vtkLandmarkTransform.h"

void USManualCalibrationWidget::OnManipulatorsModified()
{
    if( m_imageFrozen )
    {
        // Compute each manipulator's center point position in phantom coordinate
        vtkMatrix4x4 * phantomToWorldMat = vtkMatrix4x4::New();
        m_pluginInterface->GetPhantomWiresObject()->GetWorldTransform()->GetMatrix( phantomToWorldMat );
        double manipWorldCoords[4][4];
        for( int i = 0; i < 4; ++i )
        {
            // Get phantom space coord
            double ratio = m_manipulators[i]->GetMiddlePoint();
            const double * p0 = m_pluginInterface->GetPhantomPoint( i, 1 );
            const double * p1 = m_pluginInterface->GetPhantomPoint( i, 2 );
            manipWorldCoords[i][ 0 ] = p0[0] + ratio * ( p1[0] - p0[0] );
            manipWorldCoords[i][ 1 ] = p0[1] + ratio * ( p1[1] - p0[1] );
            manipWorldCoords[i][ 2 ] = p0[2] + ratio * ( p1[2] - p0[2] );
            manipWorldCoords[i][ 3 ] = 1.0;

            // Convert coord to world space
            phantomToWorldMat->MultiplyPoint( manipWorldCoords[i], manipWorldCoords[i] );
        }

        // Compute world to image using landmark transform
        vtkLandmarkTransform * landmarkTransform = vtkLandmarkTransform::New();
        landmarkTransform->SetModeToSimilarity();  // allow isotropic scaling

        // source points
        vtkPoints * sourcePoints = vtkPoints::New();
        sourcePoints->SetNumberOfPoints( 4 );
        for( int i = 0; i < 4; ++i )
        {
            double manipMiddle[3];
            m_manipulators[ i ]->GetMiddlePoint( manipMiddle );
            sourcePoints->SetPoint( i, manipMiddle );
        }

        // target points
        vtkPoints * targetPoints = vtkPoints::New();
        targetPoints->SetNumberOfPoints( 4 );
        for( int i = 0; i < 4; ++i )
            targetPoints->SetPoint( i, manipWorldCoords[i] );

        landmarkTransform->SetSourceLandmarks( sourcePoints );
        landmarkTransform->SetTargetLandmarks( targetPoints );
        landmarkTransform->Update();

        // Compute calibration matrix
        vtkMatrix4x4 * inverseTrackerToWorld = vtkMatrix4x4::New();
        inverseTrackerToWorld->DeepCopy( m_frozenMatrix );
        inverseTrackerToWorld->Invert();
        vtkMatrix4x4 * calibrationMatrix = vtkMatrix4x4::New();
        vtkMatrix4x4::Multiply4x4( inverseTrackerToWorld, landmarkTransform->GetMatrix(), calibrationMatrix );

        // Send new calibration matrix to the US probe
        UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
        if( probe )
            probe->SetCurrentCalibrationMatrix( calibrationMatrix );

        // cleanup
        phantomToWorldMat->Delete();
        landmarkTransform->Delete();
        sourcePoints->Delete();
        targetPoints->Delete();
        inverseTrackerToWorld->Delete();
        calibrationMatrix->Delete();
    }
}

void USManualCalibrationWidget::on_freezeVideoButton_toggled( bool checked )
{
    Q_ASSERT( m_imageFrozen != checked );

    UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
    Q_ASSERT( probe );

    if( checked )
    {
        probe->GetUncalibratedTransform()->GetMatrix( m_frozenMatrix );
        m_frozenImage->DeepCopy( probe->GetVideoOutput() );
        m_imageActor->SetInputData( m_frozenImage );
        m_frozenState = probe->GetState();
    }
    else
    {
        m_imageActor->SetInputData( probe->GetVideoOutput() );
    }

    m_imageFrozen = checked;
    UpdateUi();
}

void USManualCalibrationWidget::on_resetButton_clicked()
{
    m_manipulators[ 0 ]->SetPoint1( 240.0, 380.0, 0.0 );
    m_manipulators[ 0 ]->SetPoint2( 400.0, 380.0, 0.0 );
    m_manipulators[ 0 ]->SetMiddlePoint( 0.5 );

    m_manipulators[ 1 ]->SetPoint1( 400.0, 360.0, 0.0 );
    m_manipulators[ 1 ]->SetPoint2( 400.0, 120.0, 0.0 );
    m_manipulators[ 1 ]->SetMiddlePoint( 0.5 );

    m_manipulators[ 2 ]->SetPoint1( 400.0, 100.0, 0.0 );
    m_manipulators[ 2 ]->SetPoint2( 240.0, 100.0, 0.0 );
    m_manipulators[ 2 ]->SetMiddlePoint( 0.5 );

    m_manipulators[ 3 ]->SetPoint1( 240.0, 100.0, 0.0 );
    m_manipulators[ 3 ]->SetPoint2( 240.0, 360.0, 0.0 );
    m_manipulators[ 3 ]->SetMiddlePoint( 0.5 );

    OnManipulatorsModified();
}

void USManualCalibrationWidget::on_depthComboBox_currentIndexChanged(int index)
{
    if( m_pluginInterface )
    {
        m_pluginInterface->SetPhatonSize(index);
    }
}