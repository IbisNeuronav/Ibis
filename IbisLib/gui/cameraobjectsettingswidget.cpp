/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "cameraobjectsettingswidget.h"
#include "ui_cameraobjectsettingswidget.h"
#include "cameraobject.h"
#include "vtkQtMatrixDialog.h"
#include "vtkMatrix4x4Operators.h"
#include "vtkMatrix4x4.h"

static double minLensDisplacement = -5.0;
static double maxLensDisplacement = 5.0;

CameraObjectSettingsWidget::CameraObjectSettingsWidget(QWidget *parent) :
    QWidget(parent)
    , m_calibrationMatrixDialog(0)
    , ui( new Ui::CameraObjectSettingsWidget )
{
    m_camera = 0;
    ui->setupUi(this);
}

CameraObjectSettingsWidget::~CameraObjectSettingsWidget()
{
    if( m_calibrationMatrixDialog )
        m_calibrationMatrixDialog->close();
    delete ui;
}

void CameraObjectSettingsWidget::SetCamera( CameraObject * cam )
{
    Q_ASSERT( m_camera == 0 );
    m_camera = cam;
    connect( m_camera, SIGNAL(Modified()), this, SLOT(CameraModified()) );
    UpdateUI();
}

void CameraObjectSettingsWidget::UpdateUI()
{
    ui->trackCameraPushButton->blockSignals( true );
    ui->trackCameraPushButton->setHidden( false );
    ui->trackCameraPushButton->setChecked( m_camera->GetTrackCamera() );
    ui->trackCameraPushButton->blockSignals( false );

    if( m_camera->IsDrivenByHardware() )
    {    
        ui->freezeButton->blockSignals( true );
        ui->freezeButton->setHidden( false );
        ui->freezeButton->setChecked( m_camera->IsTransformFrozen() );
        ui->freezeButton->blockSignals( false );
        ui->snapshotButton->setHidden( false );
        ui->recordButton->setHidden( false );

        ui->calibrationMatrixButton->blockSignals( true );
        ui->calibrationMatrixButton->setHidden( false );
        ui->calibrationMatrixButton->setChecked( m_calibrationMatrixDialog != 0 );
        ui->calibrationMatrixButton->blockSignals( false );

        ui->currentFrameLabel->setHidden( true );
        ui->currentFrameSlider->setHidden( true );
        ui->currentFrameEdit->setHidden( true );
    }
    else
    {
        ui->freezeButton->setHidden( true );
        ui->snapshotButton->setHidden( true );
        ui->recordButton->setHidden( true );
        ui->calibrationMatrixButton->setHidden( true );

        if( m_camera->GetNumberOfFrames() > 1 )
        {
            ui->currentFrameLabel->setHidden( false );
            ui->currentFrameSlider->setHidden( false );
            ui->currentFrameEdit->setHidden( false );
            ui->currentFrameSlider->blockSignals( true );
            ui->currentFrameSlider->setRange( 0, m_camera->GetNumberOfFrames() - 1 );
            ui->currentFrameSlider->setValue( m_camera->GetCurrentFrame() );
            ui->currentFrameSlider->blockSignals( false );
            ui->currentFrameEdit->setText( QString::number( m_camera->GetCurrentFrame()) );
        }
        else
        {
            ui->currentFrameLabel->setHidden( true );
            ui->currentFrameSlider->setHidden( true );
            ui->currentFrameEdit->setHidden( true );
        }
    }

    ui->verticalAngleSpinBox->blockSignals( true );
    ui->verticalAngleSpinBox->setValue( m_camera->GetVerticalAngleDegrees() );
    ui->verticalAngleSpinBox->blockSignals( false );

    ui->imageDistanceSpinBox->blockSignals( true );
    ui->imageDistanceSpinBox->setValue( m_camera->GetImageDistance() );
    ui->imageDistanceSpinBox->blockSignals( false );

    ui->xFocalLineEdit->setText( QString::number( m_camera->GetIntrinsicParams().m_focal[0], 'f', 2 ) );
    ui->yFocalLineEdit->setText( QString::number( m_camera->GetIntrinsicParams().m_focal[1], 'f', 2 ) );

    double center[2] = { 0.0, 0.0 };
    m_camera->GetImageCenterPix( center[0], center[1] );

    ui->xImageCenterSpinBox->blockSignals( true );
    ui->xImageCenterSpinBox->setValue( center[0] );
    ui->xImageCenterSpinBox->blockSignals( false );

    ui->yImageCenterSpinBox->blockSignals( true );
    ui->yImageCenterSpinBox->setValue( center[1] );
    ui->yImageCenterSpinBox->blockSignals( false );

    ui->distortionSpinBox->blockSignals( true );
    ui->distortionSpinBox->setValue( m_camera->GetLensDistortion() );
    ui->distortionSpinBox->blockSignals( false );

    // Lens displacement
    ui->lensDisplacementSlider->blockSignals( true );
    int sliderVal = (int) ( ( m_camera->GetLensDisplacement() - minLensDisplacement ) / ( maxLensDisplacement - minLensDisplacement ) * 100.0 );
    ui->lensDisplacementSlider->setValue( sliderVal );
    ui->lensDisplacementSlider->blockSignals( false );
    ui->lensDisplacementSpinBox->blockSignals( true );
    ui->lensDisplacementSpinBox->setValue( m_camera->GetLensDisplacement() );
    ui->lensDisplacementSpinBox->blockSignals( false );

    // Update translation and rotation of calibration matrix
    double trans[3] = { 0.0, 0.0, 0.0 };
    double rot[3] = { 0.0, 0.0, 0.0 };
    vtkMatrix4x4Operators::MatrixToTransRot( m_camera->GetCalibrationMatrix(), trans, rot );
    ui->xtransSpinBox->blockSignals( true );
    ui->xtransSpinBox->setValue( trans[0] );
    ui->xtransSpinBox->blockSignals( false );
    ui->ytransSpinBox->blockSignals( true );
    ui->ytransSpinBox->setValue( trans[1] );
    ui->ytransSpinBox->blockSignals( false );
    ui->ztransSpinBox->blockSignals( true );
    ui->ztransSpinBox->setValue( trans[2] );
    ui->ztransSpinBox->blockSignals( false );
    ui->xrotSpinBox->blockSignals( true );
    ui->xrotSpinBox->setValue( rot[0] );
    ui->xrotSpinBox->blockSignals( false );
    ui->yrotSpinBox->blockSignals( true );
    ui->yrotSpinBox->setValue( rot[1] );
    ui->yrotSpinBox->blockSignals( false );
    ui->zrotSpinBox->blockSignals( true );
    ui->zrotSpinBox->setValue( rot[2] );
    ui->zrotSpinBox->blockSignals( false );


    ui->targetTransparencyGroupBox->blockSignals( true );
    ui->targetTransparencyGroupBox->setChecked( m_camera->IsUsingTransparency() );
    ui->targetTransparencyGroupBox->blockSignals( false );

    ui->useGradientCheckBox->blockSignals( true );
    ui->useGradientCheckBox->setChecked( m_camera->IsUsingGradient() );
    ui->useGradientCheckBox->blockSignals( false );

    ui->showMaskCheckBox->blockSignals( true );
    ui->showMaskCheckBox->setChecked( m_camera->IsShowingMask() );
    ui->showMaskCheckBox->blockSignals( false );

    ui->opacitySlider->blockSignals( true );
    ui->opacitySlider->setValue( (int)(m_camera->GetGlobalOpacity() * 100.0) );
    ui->opacitySlider->blockSignals( false );

    ui->saturationSlider->blockSignals( true );
    ui->saturationSlider->setValue( (int)(m_camera->GetSaturation() * 100.0) );
    ui->saturationSlider->blockSignals( false );

    ui->brightnessSlider->blockSignals( true );
    ui->brightnessSlider->setValue( (int)(m_camera->GetBrightness() * 100.0) );
    ui->brightnessSlider->blockSignals( false );

    ui->trackTransparencyCenterCheckBox->blockSignals( true );
    ui->trackTransparencyCenterCheckBox->setChecked( m_camera->IsTransparencyCenterTracked() );
    ui->trackTransparencyCenterCheckBox->blockSignals( false );

    ui->xTransparencyCenterSlider->blockSignals( true );
    ui->xTransparencyCenterSlider->setEnabled( !m_camera->IsTransparencyCenterTracked() );
    ui->xTransparencyCenterSlider->setValue( (int)(m_camera->GetTransparencyCenter()[0] / m_camera->GetImageWidth() * 100.0) );
    ui->xTransparencyCenterSlider->blockSignals( false );

    ui->yTransparencyCenterSlider->blockSignals( true );
    ui->yTransparencyCenterSlider->setEnabled( !m_camera->IsTransparencyCenterTracked() );
    ui->yTransparencyCenterSlider->setValue( (int)(m_camera->GetTransparencyCenter()[1] / m_camera->GetImageHeight() * 100.0) );
    ui->yTransparencyCenterSlider->blockSignals( false );

    ui->minTransparencyRadiusSlider->blockSignals( true );
    ui->minTransparencyRadiusSlider->setValue( (int)(m_camera->GetTransparencyRadius()[0] / 1000.0 * 100.0) );
    ui->minTransparencyRadiusSlider->blockSignals( false );

    ui->maxTransparencyRadiusSlider->blockSignals( true );
    ui->maxTransparencyRadiusSlider->setValue( (int)(m_camera->GetTransparencyRadius()[1] / 1000.0 * 100.0) );
    ui->maxTransparencyRadiusSlider->blockSignals( false );

}

void CameraObjectSettingsWidget::CameraModified()
{
    UpdateUI();
}

void CameraObjectSettingsWidget::on_trackCameraPushButton_toggled( bool checked )
{
    m_camera->SetTrackCamera( checked );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_imageDistanceSpinBox_valueChanged( double value )
{
    m_camera->SetImageDistance( value );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_calibrationMatrixButton_toggled( bool isOn )
{
    if( !isOn )
    {
        if( m_calibrationMatrixDialog )
        {
            m_calibrationMatrixDialog->close();
            m_calibrationMatrixDialog = 0;
        }
    }
    else
    {
        Q_ASSERT_X( m_camera, "CameraObjectSettingsWidget::on_calibrationMatrixButton_toggled(bool)", "Can't call this function without setting CameraObject." );

        if( m_camera->GetCalibrationMatrix() )
        {
            QString dialogTitle = m_camera->GetName();
            dialogTitle += ": Camera calibration matrix";
            m_calibrationMatrixDialog = new vtkQtMatrixDialog( true, 0 );
            m_calibrationMatrixDialog->setWindowTitle( dialogTitle );
            m_calibrationMatrixDialog->setAttribute( Qt::WA_DeleteOnClose );
            m_calibrationMatrixDialog->SetMatrix( m_camera->GetCalibrationMatrix() );
            m_calibrationMatrixDialog->show();
            connect( m_calibrationMatrixDialog, SIGNAL(destroyed()), this, SLOT(CalibrationMatrixDialogClosed()) );
        }
    }
}

void CameraObjectSettingsWidget::CalibrationMatrixDialogClosed()
{
    m_calibrationMatrixDialog = 0;
    ui->calibrationMatrixButton->setChecked( false );
}

void CameraObjectSettingsWidget::on_xTransparencyCenterSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    double realValue = (double)value * .01 * m_camera->GetImageWidth();
    double yRealValue = (double)(ui->yTransparencyCenterSlider->value()) * .01 * m_camera->GetImageHeight();
    m_camera->SetTransparencyCenter( realValue, yRealValue );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_yTransparencyCenterSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    double realValue = (double)value * .01 * m_camera->GetImageHeight();
    double xRealValue = (double)(ui->xTransparencyCenterSlider->value()) * .01 * m_camera->GetImageWidth();
    m_camera->SetTransparencyCenter( xRealValue, realValue );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_minTransparencyRadiusSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    double realValue = (double)value * .01 * 1000.0;
    double maxRealValue = (double)(ui->maxTransparencyRadiusSlider->value()) * .01 * 1000.0;
    if( realValue > maxRealValue )
        realValue = maxRealValue;
    m_camera->SetTransparencyRadius( realValue, maxRealValue );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_maxTransparencyRadiusSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    double realValue = (double)value * .01 * 1000.0;
    double minRealValue = (double)(ui->minTransparencyRadiusSlider->value()) * .01 * 1000.0;
    if( realValue < minRealValue )
        realValue = minRealValue;
    m_camera->SetTransparencyRadius( minRealValue, realValue );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_freezeButton_toggled(bool checked)
{
    Q_ASSERT( m_camera );
    if( checked )
        m_camera->FreezeTransform();
    else
        m_camera->UnFreezeTransform();
    UpdateUI();
}

void CameraObjectSettingsWidget::on_snapshotButton_clicked()
{
    Q_ASSERT( m_camera );
    m_camera->TakeSnapshot();
}

void CameraObjectSettingsWidget::on_opacitySlider_valueChanged( int value )
{
    Q_ASSERT( m_camera );
    double opacity = double( value ) / 100.0;
    m_camera->SetGlobalOpacity( opacity );
}

void CameraObjectSettingsWidget::on_targetTransparencyGroupBox_toggled(bool checked)
{
    Q_ASSERT( m_camera );
    m_camera->SetUseTransparency( checked );
}

void CameraObjectSettingsWidget::on_useGradientCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_camera );
    m_camera->SetUseGradient( checked );
}

void CameraObjectSettingsWidget::on_showMaskCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_camera );
    m_camera->SetShowMask( checked );
}

void CameraObjectSettingsWidget::on_xImageCenterSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_camera );
    double x = ui->xImageCenterSpinBox->value();
    double y = ui->yImageCenterSpinBox->value();
    m_camera->SetImageCenterPix( x, y );
}

void CameraObjectSettingsWidget::on_yImageCenterSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_camera );
    double x = ui->xImageCenterSpinBox->value();
    double y = ui->yImageCenterSpinBox->value();
    m_camera->SetImageCenterPix( x, y );
}

void CameraObjectSettingsWidget::on_distortionSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_camera );
    double dist = ui->distortionSpinBox->value();
    m_camera->SetLensDistortion( dist );
}

void CameraObjectSettingsWidget::on_recordButton_clicked()
{
    Q_ASSERT( m_camera );
    m_camera->ToggleRecording();
    if( m_camera->IsRecording() )
        ui->recordButton->setText( "Stop" );
    else
        ui->recordButton->setText( "Record" );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_currentFrameSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    m_camera->SetCurrentFrame( value );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_intrinsicParamsGroupBox_toggled(bool arg1)
{
    Q_ASSERT( m_camera );
    m_camera->SetIntrinsicEditable( arg1 );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_extrinsicParamsGroupBox_toggled(bool arg1)
{
    Q_ASSERT( m_camera );
    m_camera->SetExtrinsicEditable( arg1 );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_xtransSpinBox_valueChanged(double arg1)
{
    UpdateExtrinsicTransform();
}

void CameraObjectSettingsWidget::on_ytransSpinBox_valueChanged(double arg1)
{
    UpdateExtrinsicTransform();
}

void CameraObjectSettingsWidget::on_ztransSpinBox_valueChanged(double arg1)
{
    UpdateExtrinsicTransform();
}

void CameraObjectSettingsWidget::on_xrotSpinBox_valueChanged(double arg1)
{
    UpdateExtrinsicTransform();
}

void CameraObjectSettingsWidget::on_yrotSpinBox_valueChanged(double arg1)
{
    UpdateExtrinsicTransform();
}

void CameraObjectSettingsWidget::on_zrotSpinBox_valueChanged(double arg1)
{
    UpdateExtrinsicTransform();
}

void CameraObjectSettingsWidget::UpdateExtrinsicTransform()
{
    double trans[3];
    trans[0] = ui->xtransSpinBox->value();
    trans[1] = ui->ytransSpinBox->value();
    trans[2] = ui->ztransSpinBox->value();
    double rot[3];
    rot[0] = ui->xrotSpinBox->value();
    rot[1] = ui->yrotSpinBox->value();
    rot[2] = ui->zrotSpinBox->value();
    vtkMatrix4x4 * calMatrix = vtkMatrix4x4::New();
    vtkMatrix4x4Operators::TransRotToMatrix( trans, rot, calMatrix );
    m_camera->SetCalibrationMatrix( calMatrix );
    calMatrix->Delete();
}

void CameraObjectSettingsWidget::on_verticalAngleSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_camera );
    m_camera->SetVerticalAngleDegrees( arg1 );
}

void CameraObjectSettingsWidget::on_trackTransparencyCenterCheckBox_toggled(bool checked)
{
    Q_ASSERT( m_camera );
    m_camera->SetTransparencyCenterTracked( checked );
}

void CameraObjectSettingsWidget::on_saturationSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    double s = (double)value * 0.01;
    m_camera->SetSaturation( s );
}

void CameraObjectSettingsWidget::on_brightnessSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    double b = (double)value * 0.01;
    m_camera->SetBrightness( b );
}

void CameraObjectSettingsWidget::on_lensDisplacementSlider_valueChanged(int value)
{
    Q_ASSERT( m_camera );
    double newDisplacement = minLensDisplacement + (double)value / 100.0 * ( maxLensDisplacement - minLensDisplacement );
    m_camera->SetLensDisplacement( newDisplacement );
    UpdateUI();
}

void CameraObjectSettingsWidget::on_lensDisplacementSpinBox_valueChanged(double arg1)
{
    Q_ASSERT( m_camera );
    m_camera->SetLensDisplacement( arg1 );
    UpdateUI();
}
