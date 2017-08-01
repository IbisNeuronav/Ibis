/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "doubleviewwidget.h"
#include "ui_doubleviewwidget.h"
#include "usacquisitionplugininterface.h"
#include "scenemanager.h"
#include "imageobject.h"
#include "usacquisitionobject.h"
#include "usprobeobject.h"
#include "hardwaremodule.h"
#include "guiutilities.h"

#include <QMessageBox>
#include "QVTKWidget.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage2.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkImageResliceToColors.h"
#include "vtkImageBlend.h"
#include "vtkImageMask.h"
#include "vtkImageData.h"
#include "vtkImageStack.h"
#include "vtkImageProperty.h"

DoubleViewWidget::DoubleViewWidget( QWidget * parent, Qt::WindowFlags f ) :
    QWidget(parent,f),
    ui(new Ui::DoubleViewWidget)
{
    ui->setupUi(this);

    // Create the 2 view windows
    vtkRenderWindowInteractor * usInteractor = ui->usImageWindow->GetInteractor();
    vtkInteractorStyleImage2 * style = vtkInteractorStyleImage2::New();
    usInteractor->SetInteractorStyle( style );
    style->Delete();

    m_usRenderer= vtkRenderer::New();
    ui->usImageWindow->GetRenderWindow()->AddRenderer( m_usRenderer );

    m_usActor = vtkImageActor::New();
    m_usActor->InterpolateOff();
    m_usActor->VisibilityOff();   // invisible until there is a valid input
    m_usRenderer->AddActor( m_usActor );

    m_reslice = vtkImageResliceToColors::New();  // set up the reslice
    m_reslice->SetInterpolationModeToLinear( );
    m_reslice->SetOutputExtent(0, ui->usImageWindow->width()-1, 0, ui->usImageWindow->height()-1, 0, 1);
    m_reslice->SetOutputSpacing( 1.0, 1.0, 1.0 );
    m_reslice->SetOutputOrigin( 0.0, 0.0, 0.0 );

    // put one more for the second MRI
    m_reslice2 = vtkImageResliceToColors::New();  // set up the reslice2
    m_reslice2->SetInterpolationModeToLinear( );
    m_reslice2->SetOutputExtent(0, ui->usImageWindow->width()-1, 0, ui->usImageWindow->height()-1, 0, 1);
    m_reslice2->SetOutputSpacing( 1.0, 1.0, 1.0 );
    m_reslice2->SetOutputOrigin( 0.0, 0.0, 0.0 );

    m_imageMask = vtkImageMask::New();
    m_imageMask->SetInputConnection( m_reslice->GetOutputPort() );

    m_vol1Slice = vtkImageActor::New();
    m_vol1Slice->GetMapper()->SetInputConnection( m_reslice->GetOutputPort() );
    m_vol1Slice->GetProperty()->SetLayerNumber( 0 );
    m_vol2Slice = vtkImageActor::New();
    m_vol2Slice->GetMapper()->SetInputConnection( m_reslice2->GetOutputPort() );
    m_vol2Slice->GetProperty()->SetLayerNumber( 1 );
    m_vol2Slice->VisibilityOff();
    m_usSlice = vtkImageActor::New();
    m_usSlice->GetProperty()->SetLayerNumber( 2 );
    m_usSlice->VisibilityOff();
    m_mriActor = vtkImageStack::New();
    m_mriActor->AddImage( m_vol1Slice );
    m_mriActor->AddImage( m_vol2Slice );
    m_mriActor->AddImage( m_usSlice );

    m_mriRenderer = vtkRenderer::New();
    ui->mriImageWindow->GetRenderWindow()->AddRenderer( m_mriRenderer ); // is this for one MRI or for two?
    m_mriRenderer->AddActor( m_mriActor );

    vtkRenderWindowInteractor * mriInteractor = ui->mriImageWindow->GetInteractor();
    vtkInteractorStyleImage2 * style2 = vtkInteractorStyleImage2::New();
    mriInteractor->SetInteractorStyle( style2 );
    style2->Delete();

    this->MakeCrossLinesToShowProbeIsOutOfView();
}

DoubleViewWidget::~DoubleViewWidget()
{
    m_usRenderer->Delete();
    m_mriRenderer->Delete();
    m_reslice->Delete();
    m_reslice2->Delete();
    m_usActor->Delete();
    m_imageMask->Delete();

    m_mriActor->Delete();
    m_vol1Slice->Delete();
    m_vol2Slice->Delete();
    m_usSlice->Delete();

    m_usLine1Actor->Delete();
    m_usLine2Actor->Delete();
    m_mriLine1Actor->Delete();
    m_mriLine2Actor->Delete();

    delete ui;
}

void DoubleViewWidget::SetPluginInterface( USAcquisitionPluginInterface * interf )
{
    m_pluginInterface = interf;

    // watch changes in objects that are used by the window (volume, acquisition and probe)
    connect( m_pluginInterface, SIGNAL(ObjectsChanged()), this, SLOT(UpdateInputs()) );
    connect( m_pluginInterface, SIGNAL(ImageChanged()), this, SLOT(UpdateViews()) );

    this->UpdatePipelineConnections();   // make sure vtk pipeline is connected in a way that reflects current state
    this->UpdateInputs();                // make sure input volume and US image are valid
}

void DoubleViewWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );

    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
    bool hasAcquisition =  acq != 0;
    bool isNotEmptyAcquisition = hasAcquisition && acq->GetNumberOfSlices() > 0;
    bool isRecording = hasAcquisition ? acq->IsRecording() : false;

    if( isRecording )
        ui->m_recordButton->setText("Stop");
    else
        ui->m_recordButton->setText("Record");

    bool canCaptureTrackedVideo = m_pluginInterface->CanCaptureTrackedVideo();
    Q_ASSERT( !( m_pluginInterface->IsLive() && !canCaptureTrackedVideo ) );  // can't be live without the ability to capture video

    // Enable disable controls based on state
    if( m_pluginInterface->IsLive() )
    {
        ui->blendCheckBox->setEnabled( true );
        ui->maskCheckBox->setEnabled( false );
        ui->m_liveCheckBox->blockSignals( true );
        ui->m_liveCheckBox->setChecked( true );
        ui->m_liveCheckBox->setEnabled( true );
        ui->m_liveCheckBox->blockSignals( false );
        ui->acquisitionsComboBox->setEnabled( false );
        ui->imageObjectsComboBox_1->setEnabled( false );
        ui->currentFrameSpinBox->setEnabled( false );
        ui->m_frameSlider->setEnabled( false );
        ui->m_recordButton->setEnabled( true );
        ui->m_exportButton->setEnabled( false );
        ui->m_rewindButton->setEnabled( false );

        // added new object states May 13, 2015 by Xiao ***
        ui->imageObjectsComboBox_2->setEnabled( false );
        ui->checkBoxImage2->setEnabled( false );
    }
    else
    {
        ui->maskCheckBox->setEnabled( true );
        ui->blendCheckBox->setEnabled( true );
        ui->m_liveCheckBox->blockSignals( true );
        ui->m_liveCheckBox->setChecked( false );
        ui->m_liveCheckBox->setEnabled( canCaptureTrackedVideo && true );
        ui->m_liveCheckBox->blockSignals( false );
        ui->acquisitionsComboBox->setEnabled( true );
        ui->imageObjectsComboBox_1->setEnabled( true );
        ui->currentFrameSpinBox->setEnabled( isNotEmptyAcquisition);
        ui->m_frameSlider->setEnabled( isNotEmptyAcquisition);
        ui->m_recordButton->setEnabled( false );
        ui->m_exportButton->setEnabled( isNotEmptyAcquisition );
        ui->m_rewindButton->setEnabled( isNotEmptyAcquisition );

        // added new object states May 13, 2015 by Xiao
        ui->imageObjectsComboBox_2->setEnabled( false );
        ui->checkBoxImage2->setEnabled( false );
    }

    // Update blending controls
    bool blending = m_pluginInterface->IsBlending();
    ui->blendCheckBox->blockSignals( true );
    ui->blendCheckBox->setChecked( blending );
    ui->blendCheckBox->blockSignals( false );
    ui->opacitySlider_1->blockSignals( true );
    ui->opacitySlider_1->setEnabled( blending );
    int blendingPercent = (int)round( m_pluginInterface->GetBlendingPercent() * 100.0 );
    ui->opacitySlider_1->setValue( blendingPercent );
    ui->opacitySlider_1->blockSignals( false );
    ui->opacityStepLineEdit_1->setEnabled( blending );
    ui->opacityStepLineEdit_1->setText( QString::number( blendingPercent ) );

    // blending controls of two MRIs
    bool blendingVolumes = m_pluginInterface->IsBlendingVolumes();  // add correspondance in interface.cpp
    // add control statement for enable secondary MRI
    bool blendingVolumesControl = blending && blendingVolumes;
    ui->imageObjectsComboBox_2->setEnabled( blendingVolumesControl ); // second MRI selection window activated when both checks
    ui->checkBoxImage2->setEnabled( blending );
    ui->checkBoxImage2->blockSignals( true );
    ui->checkBoxImage2->setChecked( blendingVolumes );
    ui->checkBoxImage2->blockSignals( false );
    ui->opacitySlider_2->blockSignals( true );
    ui->opacitySlider_2->setEnabled( blendingVolumesControl );
    int blendingVolumePercent = (int)round( m_pluginInterface->GetBlendingVolumesPercent() * 100.0 );
    ui->opacitySlider_2->setValue( blendingVolumePercent );
    ui->opacitySlider_2->blockSignals( false );
    ui->opacityStepLineEdit_2->setEnabled( blendingVolumesControl );
    ui->opacityStepLineEdit_2->setText( QString::number( blendingVolumePercent ) );


    // Update masking controls
    bool masking = m_pluginInterface->IsMasking();
    bool maskingControls = masking && !blending;
    int maskingPercent = (int)round( m_pluginInterface->GetMaskingPercent() * 100.0 );
    ui->maskCheckBox->blockSignals( true );
    ui->maskCheckBox->setEnabled( !blending );
    ui->maskCheckBox->setChecked( masking );
    ui->maskCheckBox->blockSignals( false );
    ui->maskAlphaSlider->blockSignals( true );
    ui->maskAlphaSlider->setEnabled( maskingControls );
    ui->maskAlphaSlider->setValue( maskingPercent );
    ui->maskAlphaSlider->blockSignals( false );
    ui->maskAlphaLineEdit->setEnabled( maskingControls );
    ui->maskAlphaLineEdit->setText( QString::number( maskingPercent ) );

    // Update acquisition combo
    QList<USAcquisitionObject*> acquisitions;
    m_pluginInterface->GetSceneManager()->GetAllUSAcquisitionObjects( acquisitions );
    GuiUtilities::UpdateSceneObjectComboBox( ui->acquisitionsComboBox, acquisitions, m_pluginInterface->GetCurrentAcquisitionObjectId());

    // Update volume combo
    QList<ImageObject*> images;
    m_pluginInterface->GetSceneManager()->GetAllImageObjects( images );
    GuiUtilities::UpdateSceneObjectComboBox( ui->imageObjectsComboBox_1, images, m_pluginInterface->GetCurrentVolumeObjectId());

    // Update added volume combo
    GuiUtilities::UpdateSceneObjectComboBox( ui->imageObjectsComboBox_2, images, m_pluginInterface->GetAddedVolumeObjectId());

    // Render graphic windows
    this->UpdateViews();
}

void DoubleViewWidget::UpdateCurrentFrameUi()
{
    Q_ASSERT( m_pluginInterface );

    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
    int currentSlice = ( acq && acq->GetNumberOfSlices() > 0) ? acq->GetCurrentSlice() : 0;
    int numberOfSlices = acq ? acq->GetNumberOfSlices() : 0;

    // current frame spin box
    ui->currentFrameSpinBox->blockSignals( true );
    ui->currentFrameSpinBox->setRange( 0, numberOfSlices - 1 );
    ui->currentFrameSpinBox->setValue( currentSlice );
    ui->currentFrameSpinBox->blockSignals( false );

    // current frame slider
    ui->m_frameSlider->blockSignals( true );
    ui->m_frameSlider->setRange( 0, numberOfSlices - 1 );
    ui->m_frameSlider->setValue( currentSlice );
    ui->m_frameSlider->blockSignals( false );
}

void DoubleViewWidget::UpdateViews()
{
    ui->usImageWindow->update();
    ui->mriImageWindow->update();
    this->UpdateCurrentFrameUi();
    UpdateStatus();
}

void DoubleViewWidget::UpdateInputs()
{
    Q_ASSERT( m_pluginInterface );

    ImageObject * im = m_pluginInterface->GetCurrentVolume();
    if( im )
    {
        m_reslice->SetInputData(im->GetImage() );
        m_reslice->SetLookupTable( im->GetLut() );
        m_mriActor->VisibilityOn();
    }
    else
    {
        m_reslice->SetInputData( 0 );
        m_mriActor->VisibilityOff();
    }

    ImageObject * im2 = m_pluginInterface->GetAddedVolume();
    if( im2 )
    {
        m_reslice2->SetInputData( im2->GetImage() );
        m_reslice2->SetLookupTable( im2->GetLut() );
    }
    else
    {
        m_reslice2->SetInputData( 0 );
    }

    // validate us acquisition
    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
    if( acq )
    {
        m_imageMask->SetMaskInputData( acq->GetMask() ); // ok so this about using mask function?
        m_reslice->SetOutputExtent(0, acq->GetSliceWidth(), 0, acq->GetSliceHeight(), 0, 1);
        acq->disconnect( this, SLOT(UpdateViews()) );
    }

    // Validate live video source
    vtkTransform * usTransform = 0; // probe transform concatenated with calibration transform

     // choose which source to use for display: live or acquisition
    UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
    if( probe )
        probe->disconnect( this, SLOT(UpdateViews()) );

    if( m_pluginInterface->IsLive() )
    {
        Q_ASSERT( probe );
        connect( probe, SIGNAL(Modified()), this, SLOT(UpdateViews()) );
        usTransform = probe->GetWorldTransform();
        m_usActor->VisibilityOn();
        m_usActor->GetMapper()->SetInputConnection( probe->GetVideoOutputPort() );
        m_usSlice->GetMapper()->SetInputConnection( probe->GetVideoOutputPort() );
    }
    else if( acq )
    {
        connect( acq, SIGNAL( Modified() ), SLOT( UpdateViews() ) );
        usTransform = acq->GetTransform();
        m_usActor->SetVisibility( acq->GetNumberOfSlices()>0 ? 1 : 0 );
        m_usActor->GetMapper()->SetInputConnection( acq->GetUnmaskedOutputPort() );
        m_usSlice->GetMapper()->SetInputConnection( acq->GetUnmaskedOutputPort() );
    }

    // Compute slicing transform
    vtkTransform * concat = vtkTransform::New();
    concat->Identity();
    if( im )
        concat->SetInput( im->GetWorldTransform()->GetLinearInverse() );
    if( usTransform )
    {
        concat->Concatenate( usTransform );
    }
    m_reslice->SetResliceTransform( concat );
    concat->Delete();

    // Compute slice transform for the second MRI
    vtkTransform * concat2 = vtkTransform::New();
    concat2->Identity();
    if( im2 )
        concat2->SetInput( im2->GetWorldTransform()->GetLinearInverse() );
    if( usTransform)
    {
        concat2->Concatenate( usTransform );
    }
    m_reslice2->SetResliceTransform( concat2 );
    concat2->Delete();

    this->SetDefaultViews();
    this->UpdateUi();
    UpdateCurrentFrameUi();
}

void DoubleViewWidget::UpdatePipelineConnections()
{
    m_imageMask->SetMaskAlpha( m_pluginInterface->GetMaskingPercent() );

    if( m_pluginInterface->IsMasking() )
        m_vol1Slice->GetMapper()->SetInputConnection( m_imageMask->GetOutputPort() );
    else
        m_vol1Slice->GetMapper()->SetInputConnection( m_reslice->GetOutputPort() );

    if( m_pluginInterface->GetCurrentVolume() )
        m_vol1Slice->VisibilityOn();
    else
        m_vol1Slice->VisibilityOff();
    if( m_pluginInterface->IsBlendingVolumes() && m_pluginInterface->GetAddedVolume() )
        m_vol2Slice->VisibilityOn();
    else
        m_vol2Slice->VisibilityOff();
    m_vol2Slice->GetProperty()->SetOpacity( m_pluginInterface->GetBlendingVolumesPercent() );

    if( m_pluginInterface->GetCurrentAcquisition() )
        m_usSlice->SetVisibility( m_pluginInterface->IsBlending() ? 1 : 0 );
    else
        m_usSlice->VisibilityOff();
    m_usSlice->GetProperty()->SetOpacity( m_pluginInterface->GetBlendingPercent() );
}

#include "vtkSmartPointer.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"

void DoubleViewWidget::UpdateStatus()
{
    bool visibility = false;

    if( m_pluginInterface->IsLive() )
    {
        UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
        if( probe )
        {
            TrackerToolState state = probe->GetState();
            if( state != Ok )
                visibility = true;
        }
        else
            visibility = true;
    }

    m_usLine1Actor->SetVisibility(visibility);
    m_usLine2Actor->SetVisibility(visibility);
    m_mriLine1Actor->SetVisibility(visibility);
    m_mriLine2Actor->SetVisibility(visibility);
}

void DoubleViewWidget::MakeCrossLinesToShowProbeIsOutOfView()
{
    double p1[3] = {0.0, 0.0, 0.0} , p2[3];
    p2[0] = (double)ui->usImageWindow->width();
    p2[1] = (double)ui->usImageWindow->height();
    p2[2] = 0;
    vtkSmartPointer<vtkLineSource> line1 = vtkSmartPointer<vtkLineSource>::New();
    line1->SetPoint1(p1);
    line1->SetPoint2(p2);

    vtkSmartPointer<vtkPolyDataMapper> mapper1 = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper1->SetInputConnection(line1->GetOutputPort(0));

    m_usLine1Actor = vtkActor::New();
    m_usLine1Actor->SetMapper(mapper1);
    m_usLine1Actor->GetProperty()->SetLineWidth(4.0);
    m_usLine1Actor->GetProperty()->SetColor(1, 0, 0);
    m_usLine1Actor->SetVisibility(0);
    m_usRenderer->AddViewProp(m_usLine1Actor);

    double p3[3], p4[3];
    p3[0] = 0.0;
    p3[1] = (double)ui->usImageWindow->height();
    p3[2] = 0.0;
    p4[0] = (double)ui->usImageWindow->width();
    p4[1] = 0.0;
    p4[2] = 0.0;
    vtkSmartPointer<vtkLineSource> line2 = vtkSmartPointer<vtkLineSource>::New();
    line2->SetPoint1(p3);
    line2->SetPoint2(p4);

    vtkSmartPointer<vtkPolyDataMapper> mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper2->SetInputConnection(line2->GetOutputPort(0));

    m_usLine2Actor = vtkActor::New();
    m_usLine2Actor->SetMapper(mapper2);
    m_usLine2Actor->GetProperty()->SetLineWidth(4.0);
    m_usLine2Actor->GetProperty()->SetColor(1, 0, 0);
    m_usLine2Actor->SetVisibility(0);
    m_usRenderer->AddViewProp(m_usLine2Actor);


    p2[0] = (double)ui->mriImageWindow->width();
    p2[1] = (double)ui->mriImageWindow->height();
    p2[2] = 0;
    vtkSmartPointer<vtkLineSource> line3 = vtkSmartPointer<vtkLineSource>::New();
    line3->SetPoint1(p1);
    line3->SetPoint2(p2);

    vtkSmartPointer<vtkPolyDataMapper> mapper3 = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper3->SetInputConnection(line3->GetOutputPort(0));

    m_mriLine1Actor = vtkActor::New();
    m_mriLine1Actor->SetMapper(mapper3);
    m_mriLine1Actor->GetProperty()->SetLineWidth(4.0);
    m_mriLine1Actor->GetProperty()->SetColor(1, 0, 0);
    m_mriLine1Actor->SetVisibility(0);
    m_mriRenderer->AddViewProp(m_mriLine1Actor);

    p3[0] = 0.0;
    p3[1] = (double)ui->mriImageWindow->height();
    p3[2] = 0.0;
    p4[0] = (double)ui->mriImageWindow->width();
    p4[1] = 0.0;
    p4[2] = 0.0;
    vtkSmartPointer<vtkLineSource> line4 = vtkSmartPointer<vtkLineSource>::New();
    line4->SetPoint1(p3);
    line4->SetPoint2(p4);

    vtkSmartPointer<vtkPolyDataMapper> mapper4 = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper4->SetInputConnection(line4->GetOutputPort(0));

    m_mriLine2Actor = vtkActor::New();
    m_mriLine2Actor->SetMapper(mapper4);
    m_mriLine2Actor->GetProperty()->SetLineWidth(4.0);
    m_mriLine2Actor->GetProperty()->SetColor(1, 0, 0);
    m_mriLine2Actor->SetVisibility(0);
    m_mriRenderer->AddViewProp(m_mriLine2Actor);
}

void DoubleViewWidget::on_blendCheckBox_toggled( bool checked )
{
    m_pluginInterface->SetBlending( checked );
    UpdatePipelineConnections();
    UpdateUi();
}

void DoubleViewWidget::on_checkBoxImage2_toggled(bool checked)
{
    m_pluginInterface->SetBlendingVolumes( checked );
    UpdatePipelineConnections();
    UpdateUi();
}

void DoubleViewWidget::on_opacitySlider_1_valueChanged( int value )
{
    double blendPercent = value / 100.0;
    m_pluginInterface->SetBlendingPercent( blendPercent );
    m_usSlice->GetProperty()->SetOpacity( blendPercent );
    UpdateUi();
}

void DoubleViewWidget::on_opacitySlider_2_valueChanged( int value )
{
    double blendVolumePercent = value / 100.0;
    m_pluginInterface->SetBlendingVolumesPercent( blendVolumePercent );
    m_vol2Slice->GetProperty()->SetOpacity( blendVolumePercent );
    UpdateUi();
}

void DoubleViewWidget::on_maskCheckBox_toggled( bool checked )
{
    m_pluginInterface->SetMasking( checked );
    UpdatePipelineConnections();
    UpdateUi();
}

void DoubleViewWidget::on_maskAlphaSlider_valueChanged( int value )
{
    double maskPercent = value / 100.0;
    m_pluginInterface->SetMaskingPercent( maskPercent );
    m_imageMask->SetMaskAlpha( maskPercent );
    UpdateUi();
}

#include "vtkCamera.h"

void SetDefaultView( vtkImageSlice * actor, vtkRenderer * renderer )
{
    actor->Update();
    double *bounds = actor->GetBounds();
    double diffx = bounds[1] - bounds[0] + 1;
    double scalex = diffx / 2.0;
    double diffy = bounds[3] - bounds[2] + 1;
    double scaley = diffy / 2.0;
    vtkCamera * cam = renderer->GetActiveCamera();
    renderer->ResetCamera();
    cam->ParallelProjectionOn();
    cam->SetParallelScale(scaley);
    double * prevPos = cam->GetPosition();
    double * prevFocal = cam->GetFocalPoint();
    cam->SetPosition( scalex, scaley, prevPos[2] );
    cam->SetFocalPoint( scalex, scaley, prevFocal[2] );
}

void DoubleViewWidget::SetDefaultViews()
{
    SetDefaultView( m_usActor, m_usRenderer );
    // adjust position of left image
    SetDefaultView( m_mriActor, m_mriRenderer );
}

void DoubleViewWidget::on_restoreViewsPushButton_clicked()
{
    this->SetDefaultViews();
    this->UpdateViews();
}

void DoubleViewWidget::on_acquisitionsComboBox_currentIndexChanged( int index )
{
    int objectId = GuiUtilities::ObjectIdFromObjectComboBox( ui->acquisitionsComboBox, index );
    m_pluginInterface->SetCurrentAcquisitionObjectId( objectId );
    this->UpdateInputs();
    this->UpdateViews();
}

void DoubleViewWidget::on_imageObjectsComboBox_1_currentIndexChanged( int index )
{
    int objectId = GuiUtilities::ObjectIdFromObjectComboBox( ui->imageObjectsComboBox_1, index );
    m_pluginInterface->SetCurrentVolumeObjectId( objectId );
    this->UpdateInputs();
    this->UpdateViews();
}

void DoubleViewWidget::on_imageObjectsComboBox_2_currentIndexChanged( int index )
{
    int objectId = GuiUtilities::ObjectIdFromObjectComboBox( ui->imageObjectsComboBox_2, index );
    m_pluginInterface->SetAddedVolumeObjectId( objectId );
    this->UpdateInputs();
    this->UpdateViews();
}

void DoubleViewWidget::on_m_rewindButton_clicked()
{
    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
    Q_ASSERT( acq );
    acq->SetCurrentFrame( 0 );
}

void DoubleViewWidget::on_currentFrameSpinBox_valueChanged( int newFrame )
{
    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
    Q_ASSERT( acq );
    acq->SetCurrentFrame( newFrame );
}

void DoubleViewWidget::on_m_frameSlider_valueChanged( int newFrame )
{
    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
    Q_ASSERT( acq );
    acq->SetCurrentFrame( newFrame );
}

void DoubleViewWidget::on_m_liveCheckBox_toggled(bool checked)
{
    m_pluginInterface->SetLive( checked );
    UpdateInputs();
}

void DoubleViewWidget::on_m_recordButton_clicked()
{   
    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();

    if( acq && acq->IsRecording() )
    {
        acq->Stop();
    }
    else
    {
        // Get the current probe to record from
        UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
        Q_ASSERT( probe );

        // Create a new acquisition to record to
        m_pluginInterface->NewAcquisition();
        USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
        Q_ASSERT( acq );

        acq->SetUsProbe( probe );
        acq->Record();
        UpdateInputs();
    }

    UpdateUi();
}

void DoubleViewWidget::on_m_exportButton_clicked()
{
    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
    Q_ASSERT( acq );

    this->setEnabled( false );
    acq->Export();
    this->setEnabled( true );
}
