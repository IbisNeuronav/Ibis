/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/


#include "vertebraregistrationwidget.h"
#include "ui_vertebraregistrationwidget.h"

#include <chrono>


VertebraRegistrationWidget::VertebraRegistrationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VertebraRegistrationWidget),
    m_application(0),
    m_isProcessing(false),
    m_targetVolumeLoaded(false),
    m_queueSemaphore(false)
{
    ui->setupUi(this);

    m_maximumFrameQueueSize = (unsigned int) ui->numberOfFramesSpinBox->value();

    // Create the 2 view windows
    vtkRenderWindowInteractor * usInteractor = ui->usImageWindow->GetInteractor();
    vtkSmartPointer<vtkInteractorStyleImage2> style = vtkSmartPointer<vtkInteractorStyleImage2>::New();
    usInteractor->SetInteractorStyle( style );

    m_usRenderer = vtkSmartPointer<vtkRenderer>::New();
    ui->usImageWindow->GetRenderWindow()->AddRenderer( m_usRenderer );

    m_usActor = vtkSmartPointer<vtkImageActor>::New();
    m_usActor->InterpolateOff();
    m_usActor->VisibilityOff();   // invisible until there is a valid input
    m_usRenderer->AddActor( m_usActor );

    m_reslice = vtkSmartPointer<vtkImageResliceToColors>::New();  // set up the reslice
    m_reslice->SetInterpolationModeToLinear( );
    m_reslice->SetOutputExtent(0, ui->usImageWindow->width()-1, 0, ui->usImageWindow->height()-1, 0, 1);
    m_reslice->SetOutputSpacing( 1.0, 1.0, 1.0 );
    m_reslice->SetOutputOrigin( 0.0, 0.0, 0.0 );

    m_imageMask = vtkSmartPointer<vtkImageMask>::New();
    m_imageMask->SetInputConnection( m_reslice->GetOutputPort() );

    m_usSlice = vtkSmartPointer<vtkImageActor>::New();
    m_usSlice->GetProperty()->SetLayerNumber( 2 );
    m_usSlice->VisibilityOff();

    m_usmask = USMask::New();

    this->MakeCrossLinesToShowProbeIsOutOfView();

    m_boneFilter = BoneFilterType::New();
    m_boneFilter->SetThreshold(15);

    m_ItktovtkConverter = IbisItkVtkConverter::New();

    m_ImageObjectId = SceneManager::InvalidId;

    m_targetVolume = 0;
    m_targetVolumeId = SceneManager::InvalidId;

    setWindowTitle( "Vertebra Registration" );
//    UpdateUi();
}

VertebraRegistrationWidget::~VertebraRegistrationWidget()
{
    delete ui;
    m_ItktovtkConverter->Delete();
}

void VertebraRegistrationWidget::SetApplication( Application * app )
{
    m_application = app;
    UpdateUi();
}

/*
void VertebraRegistrationWidget::on_startButton_clicked()
{
//    QDebugStream qout(std::cout,  ui->debugTextBrowser);

    // Make sure all params have been specified
    int sourceImageObjectId = ui->targetImageComboBox->itemData( ui->targetImageComboBox->currentIndex() ).toInt();

    // Get input images
    SceneManager * sm = m_pluginInterface->GetSceneManager();
    ImageObject * sourceImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( sourceImageObjectId ) );
    Q_ASSERT_X( sourceImageObject, "VertebraRegistrationWidget::on_startButton_clicked()", "Invalid source object" );

    IbisItkFloat3ImageType::Pointer itkSourceImage = sourceImageObject->GetItkImage();

    //====================================================================================
    // -----------------------     TODO    ---------------------------------------
    // We have pointers to source and target images, now do your processing here...
    //====================================================================================

    auto start_timer = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed;

//    typedef itk::BoneExtractionFilter<IbisItkFloat3ImageType> BoneFilterType;
//    BoneFilterType::Pointer boneFilter = BoneFilterType::New();
//    boneFilter->SetInput(itkSourceImage);
//    boneFilter->SetThreshold(15);
//    boneFilter->Update();


    IbisItkFloat3ImageType::Pointer itkImage = IbisItkFloat3ImageType::New();
    m_ItktovtkConverter->ConvertVtkImageToItkImage(itkImage, ui->usImageWindow->cachedImage(), vtkMatrix4x4::New());

//    m_boneFilter->SetInput(itkSourceImage);
//    m_boneFilter->Update();


//    IbisItkFloat3ImageType::Pointer itkResultImage = m_boneFilter->GetOutput();

    elapsed = std::chrono::high_resolution_clock::now() - start_timer;
    std::cout << "Filter implementation time: " << elapsed.count() << " s\n";

    this->AddImageToScene<IbisItkFloat3ImageType>(itkImage, "Filter Output Image");

//    this->AddImageToScene<IbisItkFloat3ImageType>(itkResultImage, "Filter Output Image");


    ui->progressBar->setEnabled( true );
    ui->startButton->setEnabled( false );

    ui->startButton->setEnabled( true );
    ui->progressBar->setValue( 0 );
    ui->progressBar->setEnabled( false );


    //====================================================================================
    // -----------------------     TODO    ---------------------------------------
    // Apply desired changes to the target transform.
    // transformObject->FinishModifyingTransform() will make sure changes you made to
    // the transform are reflected in the graphic windows and UI.
    //====================================================================================

}
//*/


void VertebraRegistrationWidget::on_addFrameButton_clicked()
{
//    QDebugStream qout(std::cout,  ui->debugTextBrowser);

    /*
    UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();

    unsigned int count = 0;
    vtkImageData * bufferImage = 0;
    if (probe)
    {
        while(count < m_maximumFrameQueueSize)
        {
            bufferImage = probe->GetVideoOutput();

            IbisItkFloat3ImageType::Pointer itkImage = IbisItkFloat3ImageType::New();
            m_ItktovtkConverter->ConvertVtkImageToItkImage(itkImage, bufferImage, vtkMatrix4x4::New());

            m_boneFilter->SetInput(itkImage);
            m_boneFilter->Update();

            IbisItkFloat3ImageType::Pointer itkResultImage = m_boneFilter->GetOutput();

            vtkImageData * im = m_ItktovtkConverter->ConvertItkImageToVtkImage( itkResultImage, 0 );

            IbisItkFloat3ImageType::Pointer itkConvertedImage = IbisItkFloat3ImageType::New();
            m_ItktovtkConverter->ConvertVtkImageToItkImage(itkConvertedImage, im, vtkMatrix4x4::New());

            this->AddImageToQueue(itkConvertedImage);
            count++;
            std::cout << "Frame: " << count << ": " << bufferImage << " == " << probe->GetVideoOutput() << std::endl;

        }
    }
    //*/


//    for (int i = 0; i < m_maximumFrameQueueSize; ++i) {
//       IbisItkFloat3ImageType::Pointer image = this->GetImageFromQueue();
//       std::cout << image << std::endl;
//       std::cout << " ------------------------------------------------------ \n";


//       ImageObject *imobj = ImageObject::New();
//       imobj->SetItkImage(image);
//       QString name = QString::fromStdString( "Image " + std::to_string(i) );
//       imobj->SetName(name);
//       imobj->Modified();
//       m_pluginInterface->GetSceneManager()->AddObject(imobj);
//       m_pluginInterface->GetSceneManager()->SetReferenceDataObject(imobj);

//    }

//    this->CreateVolumeFromSlices(1);


//    itk::TimeProbe clock;

//    clock.Start();

//    this->CreateVolumeFromSlices(1);

//    clock.Stop();

//    std::cout << "Volume size: " << m_sparseVolume->GetLargestPossibleRegion().GetSize() << std::endl;
//    std::cout << "Time: " << clock.GetMean() << std::endl;

//    ImageObject *imobj = ImageObject::New();
//    imobj->SetItkImage(m_sparseVolume);
//    imobj->SetName("Volume");
//    imobj->Modified();
//    m_pluginInterface->GetSceneManager()->AddObject(imobj);
//    m_pluginInterface->GetSceneManager()->SetReferenceDataObject(imobj);


}



void VertebraRegistrationWidget::CreateVolumeFromSlices(float spacingFactor)
{

    IbisItkFloat3ImageType::PointType minPoint;
    IbisItkFloat3ImageType::PointType maxPoint;
    minPoint.Fill(itk::NumericTraits<IbisItkFloat3ImageType::PixelType>::max());
    maxPoint.Fill(itk::NumericTraits<IbisItkFloat3ImageType::PixelType>::NonpositiveMin());

    std::cout << "Number of slices for reconstruction : " << m_inputImageList.size() << std::endl;

    std::vector< itk::SmartPointer<IbisItkFloat3ImageType> > inputImageVector;
    unsigned int N = m_inputImageList.size();
    for (int i = 0; i < N; ++i) {
        inputImageVector.push_back( this->GetImageFromQueue() );
        this->getMinimumMaximumVolumeExtent(inputImageVector[i], minPoint, maxPoint);
    }

    IbisItkFloat3ImageType::Pointer volume = IbisItkFloat3ImageType::New();
    IbisItkFloat3ImageType::SpacingType spacing;

    if (spacingFactor == 0)
        spacing = inputImageVector[0]->GetSpacing();
    else
        spacing.Fill(spacingFactor);
    volume->SetSpacing(spacing);
    IbisItkFloat3ImageType::SizeType size;
    IbisItkFloat3ImageType::IndexType startIndex;
    for (int i = 0; i < 3; ++i) {
        size[i] = std::round((maxPoint[i] - minPoint[i]) / spacing[i]);
        startIndex[i] = (int) (minPoint[i] / spacing[i]);
    }

    IbisItkFloat3ImageType::RegionType region;
    region.SetIndex(startIndex);
    region.SetSize(size);
    volume->SetRegions(region);
    volume->Allocate();
    volume->FillBuffer(0);

    IbisItkFloat3ImageType::PointType physicalPoint;
    IbisItkFloat3ImageType::IndexType volumeIndex;
    IbisItkFloat3ImageType::PixelType voxel;

    for (int i = 0; i < inputImageVector.size(); ++i) {
        IbisItkFloat3ImageType::Pointer image;
        image = inputImageVector[i];

        itk::ImageRegionIterator<IbisItkFloat3ImageType> itkIterator(image, image->GetLargestPossibleRegion());
        itkIterator.GoToBegin();

        while( !itkIterator.IsAtEnd() )
        {
            if (itkIterator.Get() > 0)
            {
                IbisItkFloat3ImageType::IndexType index;
                index = itkIterator.GetIndex();
                image->TransformIndexToPhysicalPoint(index, physicalPoint);
                volume->TransformPhysicalPointToIndex(physicalPoint, volumeIndex);

                if (region.IsInside(volumeIndex))
                {
                    voxel = image->GetPixel(index);
                    volume->SetPixel(volumeIndex, voxel);
                }
            }

            ++itkIterator;
        }
    }

    std::cout << "Size : " << size << std::endl;
    m_sparseVolume = volume;

//    this->m_sparseMask = mask;
}

void VertebraRegistrationWidget::getMinimumMaximumVolumeExtent(IbisItkFloat3ImageType::Pointer itkImage,
                                                               IbisItkFloat3ImageType::PointType & minPoint,
                                                               IbisItkFloat3ImageType::PointType & maxPoint)
{
    IbisItkFloat3ImageType::IndexType cornerIndex;
    IbisItkFloat3ImageType::IndexType zeroIndex;
    IbisItkFloat3ImageType::SizeType size;

    size = itkImage->GetLargestPossibleRegion().GetSize();

    IbisItkFloat3ImageType::IndexType index;
    IbisItkFloat3ImageType::PointType point;

    zeroIndex.Fill(0);
    for (int i = 0; i < 3; ++i) {
        cornerIndex[i] = size[i];
    }

    std::vector<IbisItkFloat3ImageType::IndexType> indices;
    indices.push_back(zeroIndex);
    indices.push_back(cornerIndex);

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                index[0] = indices[x][0];
                index[1] = indices[y][1];
                index[2] = indices[z][2];

                itkImage->TransformIndexToPhysicalPoint(index, point);
                for (int ii = 0; ii < 3; ++ii) {
                    minPoint[ii] = (point[ii] < minPoint[ii]) ? point[ii] : minPoint[ii];
                    maxPoint[ii] = (point[ii] > maxPoint[ii]) ? point[ii] : maxPoint[ii];
                }
            }

        }
    }


}


vtkImageData * VertebraRegistrationWidget::GetVtkBoneSurface(vtkImageData * inputVtkImage, vtkMatrix4x4 * transformMatrix)
{
    IbisItkFloat3ImageType::Pointer itkImage = IbisItkFloat3ImageType::New();
    m_ItktovtkConverter->ConvertVtkImageToItkImage(itkImage, inputVtkImage, transformMatrix);

    m_boneFilter->SetInput(itkImage);
    m_boneFilter->Update();

    IbisItkFloat3ImageType::Pointer itkResultImage = m_boneFilter->GetOutput();

    this->AddImageToQueue(itkResultImage);

    return m_ItktovtkConverter->ConvertItkImageToVtkImage( itkResultImage, 0 );
}

void VertebraRegistrationWidget::on_startButton_clicked()
{

//    QDebugStream qout(std::cout,  ui->debugTextBrowser);

    // consistency

    int sourceImageObjectId = ui->sourceImageComboBox->itemData( ui->sourceImageComboBox->currentIndex() ).toInt();
    int targetImageObjectId = ui->targetImageComboBox->itemData( ui->targetImageComboBox->currentIndex() ).toInt();

    // Get input images
    SceneManager * sm = m_pluginInterface->GetSceneManager();
    ImageObject * sourceImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( sourceImageObjectId ) );
    Q_ASSERT_X( sourceImageObject, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid source object" );

    ImageObject * targetImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( targetImageObjectId ) );
    Q_ASSERT_X( targetImageObject, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid target object" );

    IbisItkFloat3ImageType::Pointer itkSourceImage = sourceImageObject->GetItkImage();
    IbisItkFloat3ImageType::Pointer itkTargetImage = targetImageObject->GetItkImage();

    vtkTransform * resultTransform;

    m_pluginInterface->getGPURegistrationWidget()->SetItkSourceImage(itkSourceImage);
    m_pluginInterface->getGPURegistrationWidget()->SetItkTargetImage(itkTargetImage);
    m_pluginInterface->getGPURegistrationWidget()->SetSourceVtkTransform(vtkTransform::SafeDownCast( sourceImageObject->GetWorldTransform() ));
    m_pluginInterface->getGPURegistrationWidget()->SetTargetVtkTransform(vtkTransform::SafeDownCast( targetImageObject->GetWorldTransform() ));

    SceneObject * transformObject = m_pluginInterface->GetSceneManager()->GetObjectByID( sourceImageObjectId );
    if( transformObject->GetParent() )
    {
        vtkTransform * parentVtktransform = vtkTransform::SafeDownCast( transformObject->GetParent()->GetWorldTransform() );
        Q_ASSERT_X( parentVtktransform, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid transform" );
        m_pluginInterface->getGPURegistrationWidget()->SetParentVtkTransform(parentVtktransform);
    }

//    m_pluginInterface->getGPURegistrationWidget()->SetDebugOn();
    m_pluginInterface->getGPURegistrationWidget()->SetUseMask( true );
    m_pluginInterface->getGPURegistrationWidget()->runRegistration();
    resultTransform = m_pluginInterface->getGPURegistrationWidget()->GetResultTransform();
//    resultTransform->Inverse();
//    resultTransform->Print(std::cout);
    sourceImageObject->SetLocalTransform(resultTransform);
    sourceImageObject->Modified();

}


/*
void VertebraRegistrationWidget::on_startButton_clicked()
{
    auto start_timer = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed;

    IbisItkFloat3ImageType::Pointer itkImage = IbisItkFloat3ImageType::New();
    m_ItktovtkConverter->ConvertVtkImageToItkImage(itkImage, ui->usImageWindow->cachedImage(), vtkMatrix4x4::New());

    m_boneFilter->SetInput(itkImage);
    m_boneFilter->Update();

    IbisItkFloat3ImageType::Pointer itkResultImage = m_boneFilter->GetOutput();

    vtkImageData * im = m_ItktovtkConverter->ConvertItkImageToVtkImage( itkResultImage, 0 );

    IbisItkFloat3ImageType::Pointer itkConvertedImage = IbisItkFloat3ImageType::New();
    m_ItktovtkConverter->ConvertVtkImageToItkImage(itkConvertedImage, im, vtkMatrix4x4::New());


    elapsed = std::chrono::high_resolution_clock::now() - start_timer;
    std::cout << "Filter implementation time: " << elapsed.count() << " s\n";

    this->AddImageToScene<IbisItkFloat3ImageType>(itkResultImage, "Bone Image");
    this->AddImageToScene<IbisItkFloat3ImageType>(itkConvertedImage, "Converted Image");
}
//*/


template <typename TInputImage>
void VertebraRegistrationWidget::UpdateItkMask(TInputImage &)
{

}



template <typename TInputImage>
void VertebraRegistrationWidget::AddImageToScene(TInputImage * itkImage, QString name)
{
    ImageObject * image = ImageObject::New();
    image->SetName(name);

    typedef itk::CastImageFilter< TInputImage, IbisItkFloat3ImageType > CastFilter2Dto3DType;
    typename CastFilter2Dto3DType::Pointer castFilter2to3 = CastFilter2Dto3DType::New();
    castFilter2to3->SetInput(itkImage);
    IbisItkFloat3ImageType::Pointer ibisItkImage = castFilter2to3->GetOutput();

    ImageObject * referenceImageObject = m_pluginInterface->GetSceneManager()->GetReferenceDataObject();
    IbisItkFloat3ImageType::Pointer referenceItkImage = referenceImageObject->GetItkImage();
    this->SuperImposeImages(ibisItkImage, referenceItkImage);
    image->SetItkImage(ibisItkImage);
    m_pluginInterface->GetSceneManager()->AddObject(image);
    image->Delete();

}

template <typename TInputImage, typename TOutputImage>
void VertebraRegistrationWidget::SuperImposeImages(itk::SmartPointer<TInputImage> & inputImage, itk::SmartPointer<TOutputImage> outputImage)
{
    typedef itk::ChangeInformationImageFilter< TInputImage > InformationFilterType;
    typename InformationFilterType::Pointer informationFilter = InformationFilterType::New();

    informationFilter->SetInput( inputImage );
    informationFilter->SetOutputDirection(outputImage->GetDirection());
    informationFilter->ChangeDirectionOn();
    informationFilter->SetOutputSpacing(outputImage->GetSpacing());
    informationFilter->ChangeSpacingOn();
    informationFilter->SetOutputOrigin(outputImage->GetOrigin());
    informationFilter->ChangeOriginOn();
    inputImage = informationFilter->GetOutput();
    inputImage->Update();
}


void VertebraRegistrationWidget::on_refreshButton_clicked()
{
    this->UpdateUi();
}

void VertebraRegistrationWidget::on_debugCheckBox_stateChanged(int value)
{
    if (ui->debugCheckBox->checkState() == Qt::Unchecked)
        m_debug = false;
    else if (ui->debugCheckBox->checkState() == Qt::Checked)
        m_debug = true;
}

void VertebraRegistrationWidget::on_numberOfFramesSpinBox_valueChanged(int value)
{
    m_maximumFrameQueueSize = value;
}

void VertebraRegistrationWidget::on_betaSpinBox_valueChanged(double value)
{
    m_betaObjectness = value;
}

void VertebraRegistrationWidget::on_gammaSpinBox_valueChanged(double value)
{
    m_gammaObjectness = value;
}


void VertebraRegistrationWidget::UpdateUi()
{

    if( m_application )
    {
        ui->sourceImageComboBox->clear();
        ui->targetImageComboBox->clear();

        SceneManager * sm = m_pluginInterface->GetSceneManager();
        const SceneManager::ObjectList & allObjects = sm->GetAllObjects();
        for( int i = 0; i < allObjects.size(); ++i )
        {
            SceneObject * current = allObjects[i];
            if( current != sm->GetSceneRoot() && current->IsListable() )
            {
//                vtkTransform * localTransform = vtkTransform::SafeDownCast( current->GetLocalTransform() );
//                if( localTransform && current->CanEditTransformManually() )
//                    ui->sourceImageComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );

                if( current->IsA("ImageObject") )
                {
                    ui->targetImageComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
                    ui->sourceImageComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
                }
            }
        }

        if( ui->targetImageComboBox->count() == 0 )
        {
            ui->targetImageComboBox->addItem( "None", QVariant(-1) );
        }

        if( ui->sourceImageComboBox->count() == 0 )
        {
            ui->sourceImageComboBox->addItem( "None", QVariant(-1) );
        }




    }

    // Render graphic windows

    Q_ASSERT( m_pluginInterface );

    bool canCaptureTrackedVideo = m_pluginInterface->CanCaptureTrackedVideo();
    Q_ASSERT( !( m_pluginInterface->IsLive() && !canCaptureTrackedVideo ) );  // can't be live without the ability to capture video

    // Enable disable controls based on state
    if( m_pluginInterface->IsLive() )
    {
        ui->liveCheckBox->blockSignals( true );
        ui->liveCheckBox->setChecked( true );
        ui->liveCheckBox->setEnabled( true );
        ui->liveCheckBox->blockSignals( false );
    }
    else
    {
        ui->liveCheckBox->blockSignals( true );
        ui->liveCheckBox->setChecked( false );
        ui->liveCheckBox->setEnabled( canCaptureTrackedVideo && true );
        ui->liveCheckBox->blockSignals( false );
    }

    this->UpdateViews();

}


/* Code from USAcquisition plugin ***************************************************************************************************/

void VertebraRegistrationWidget::SetPluginInterface( VertebraRegistrationPluginInterface * interf )
{
    m_pluginInterface = interf;

    // watch changes in objects that are used by the window (volume, acquisition and probe)
    connect( m_pluginInterface, SIGNAL(ObjectsChanged()), this, SLOT(UpdateInputs()) );
    connect( m_pluginInterface, SIGNAL(ImageChanged()), this, SLOT(UpdateViews()) );

    this->UpdatePipelineConnections();   // make sure vtk pipeline is connected in a way that reflects current state
    this->UpdateInputs();                // make sure input volume and US image are valid
}

void VertebraRegistrationWidget::UpdateInputs()
{
    Q_ASSERT( m_pluginInterface );

//    ImageObject * im = m_pluginInterface->GetCurrentVolume();
//    if( im )
//    {
//        m_reslice->SetInputData(im->GetImage() );
//        m_reslice->SetLookupTable( im->GetLut() );
//    }
//    else
//    {
//        m_reslice->SetInputData( 0 );
//    }


    // validate us acquisition
//    USAcquisitionObject * acq = m_pluginInterface->GetCurrentAcquisition();
//    if( acq )
//    {
//        m_imageMask->SetMaskInputData( acq->GetMask() ); // ok so this about using mask function?
//        m_reslice->SetOutputExtent(0, acq->GetSliceWidth(), 0, acq->GetSliceHeight(), 0, 1);
//        acq->disconnect( this, SLOT(UpdateViews()) );
//    }

    // Validate live video source
    vtkTransform * usTransform = 0; // probe transform concatenated with calibration transform

     // choose which source to use for display: live or acquisition
    UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
    if( probe )
        probe->disconnect( this, SLOT(UpdateViews()) );

    probe->SetUseMask(true);

    if( m_pluginInterface->IsLive() )
    {
        Q_ASSERT( probe );
        connect( probe, SIGNAL(Modified()), this, SLOT(UpdateViews()) );
        connect( probe, SIGNAL(Modified()), this, SLOT(CollectImages()));
        usTransform = probe->GetWorldTransform();
        m_usActor->VisibilityOn();
        m_usActor->GetMapper()->SetInputConnection( probe->GetVideoOutputPort() );
        m_usSlice->GetMapper()->SetInputConnection( probe->GetVideoOutputPort() );
    }
//    else if( acq )
//    {
//        connect( acq, SIGNAL( Modified() ), SLOT( UpdateViews() ) );
//        usTransform = acq->GetTransform();
//        m_usActor->SetVisibility( acq->GetNumberOfSlices()>0 ? 1 : 0 );
//        m_usActor->GetMapper()->SetInputConnection( acq->GetUnmaskedOutputPort() );
//        m_usSlice->GetMapper()->SetInputConnection( acq->GetUnmaskedOutputPort() );
//    }

    // Compute slicing transform
    vtkSmartPointer<vtkTransform> concat = vtkSmartPointer<vtkTransform>::New();
    concat->Identity();
//    if( im )
//        concat->SetInput( im->GetWorldTransform()->GetLinearInverse() );
    if( usTransform )
    {
        concat->Concatenate( usTransform );
    }
    m_reslice->SetResliceTransform( concat );

    this->SetDefaultViews();

}



void VertebraRegistrationWidget::UpdateViews()
{
    ui->usImageWindow->update();
    UpdateStatus();
}

void VertebraRegistrationWidget::CollectImages()
{
    Q_ASSERT( m_pluginInterface );
    if ((m_pluginInterface) && (!m_isProcessing))
    {
        UsProbeObject * probe = m_pluginInterface->GetCurrentUsProbe();
        if ( m_pluginInterface->IsLive() )
        {
            if (probe->IsOk())
            {
                m_isProcessing = true;

    //            this->GetVtkBoneSurface(probe->GetVideoOutput(), probe->GetLocalTransform()->GetMatrix() ) ;
                vtkImageData * vtkimage =  probe->GetVideoOutput(); //this->GetVtkBoneSurface(probe->GetVideoOutput());
                IbisItkFloat3ImageType::Pointer itkimage = IbisItkFloat3ImageType::New();
                vtkMatrix4x4 * transformMatrix = probe->GetLocalTransform()->GetMatrix();
                m_ItktovtkConverter->ConvertVtkImageToItkImage( itkimage, vtkimage, transformMatrix);

                //transformMatrix->Print(std::cout);
                //std::cout << "itkimage matrix\n" << itkimage->GetDirection() << std::endl;

                m_boneFilter->SetInput(itkimage);
                m_boneFilter->Update();

                IbisItkFloat3ImageType::Pointer itkResultImage = m_boneFilter->GetOutput();

                //std::cout << "Result matrix\n" << itkResultImage->GetDirection() << std::endl;

                typedef itk::ImageDuplicator<IbisItkFloat3ImageType> DuplicatorType;
                DuplicatorType::Pointer duplicator = DuplicatorType::New();
                duplicator->SetInputImage(itkimage);
                duplicator->Update();

                //std::cout << "Duplicator matrix\n" << duplicator->GetOutput()->GetDirection() << std::endl;

                this->AddImageToQueue( duplicator->GetOutput() );

                m_isProcessing = false;

            }
            else
            {
                this->ClearQueue();
            }
        }

    }
}

void VertebraRegistrationWidget::UpdateStatus()
{
    bool visibility = false;

    Q_ASSERT ( m_pluginInterface );
    try{
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
    }
    catch(...){
        std::cout << "Can't Update Status\n";
    }

}

void VertebraRegistrationWidget::UpdatePipelineConnections()
{
    m_imageMask->SetMaskAlpha( m_pluginInterface->GetMaskingPercent() );

//    if( m_pluginInterface->IsMasking() )
//        m_vol1Slice->GetMapper()->SetInputConnection( m_imageMask->GetOutputPort() );
//    else
//        m_vol1Slice->GetMapper()->SetInputConnection( m_reslice->GetOutputPort() );

//    if( m_pluginInterface->GetCurrentVolume() )
//        m_vol1Slice->VisibilityOn();
//    else
//        m_vol1Slice->VisibilityOff();

    if( m_pluginInterface->GetCurrentAcquisition() )
        m_usSlice->SetVisibility( true );
    else
        m_usSlice->VisibilityOff();
    m_usSlice->GetProperty()->SetOpacity( 1.0 );
}


void VertebraRegistrationWidget::SetDefaultViews()
{
    VertebraRegistrationWidget::SetDefaultView( m_usActor, m_usRenderer );
}


void VertebraRegistrationWidget::SetDefaultView( vtkSmartPointer<vtkImageSlice> actor, vtkSmartPointer<vtkRenderer> renderer )
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

void VertebraRegistrationWidget::on_liveCheckBox_toggled(bool checked)
{
    m_pluginInterface->SetLive( checked );
    UpdateInputs();
}

void VertebraRegistrationWidget::on_targetImageComboBox_currentIndexChanged(int value)
{
    int targetObjectId = ui->targetImageComboBox->itemData( value ).toInt();
    m_targetVolumeLoaded = (targetObjectId != SceneManager::InvalidId) && (ui->targetImageComboBox->count() > 0);

    if (m_targetVolumeLoaded)
    {
        SceneManager * sm = m_pluginInterface->GetSceneManager();

        ImageObject * targetImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( targetObjectId ) );
        Q_ASSERT_X( targetImageObject, "VertebraRegistrationWidget::on_targetImageComboBox_currentIndexChanged()", "Invalid target object" );

        m_targetVolume = targetImageObject->GetItkImage();
        m_targetVolumeId = targetObjectId;

        std::cout << m_targetVolume->GetDirection();

    }
    else{
        m_targetVolumeId = SceneManager::InvalidId;
    }
}


void VertebraRegistrationWidget::MakeCrossLinesToShowProbeIsOutOfView()
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

    m_usLine1Actor = vtkSmartPointer<vtkActor>::New();
    m_usLine1Actor->SetMapper( mapper1 );
    m_usLine1Actor->GetProperty()->SetLineWidth(4.0);
    m_usLine1Actor->GetProperty()->SetColor(1, 0, 0);
    m_usLine1Actor->SetVisibility(0);
    m_usRenderer->AddViewProp( m_usLine1Actor );

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

    m_usLine2Actor = vtkSmartPointer<vtkActor>::New();
    m_usLine2Actor->SetMapper( mapper2 );
    m_usLine2Actor->GetProperty()->SetLineWidth(4.0);
    m_usLine2Actor->GetProperty()->SetColor(1, 0, 0);
    m_usLine2Actor->SetVisibility(0);
    m_usRenderer->AddViewProp( m_usLine2Actor );

}

void VertebraRegistrationWidget::AddImageToQueue( itk::SmartPointer<IbisItkFloat3ImageType> image )
{
    if (!m_queueSemaphore)
    {
//        m_queueSemaphore = true;

        m_inputImageList.push( image );
        if (m_inputImageList.size() > m_maximumFrameQueueSize)
        {
            m_inputImageList.pop();
//            m_queueSemaphore = false;

            m_isProcessing = true;

            if (m_targetVolumeLoaded)
            {
                std::cout << "Creating volume" << std::endl;
                this->CreateVolumeFromSlices(1);

                vtkTransform * resultTransform;

                m_pluginInterface->getGPURegistrationWidget()->SetItkSourceImage( m_sparseVolume );
                m_pluginInterface->getGPURegistrationWidget()->SetItkTargetImage( m_targetVolume );
    //            m_pluginInterface->getGPURegistrationWidget()->SetSourceVtkTransform( vtkTransform::New() );
    //            m_pluginInterface->getGPURegistrationWidget()->SetTargetVtkTransform(  );

                SceneObject * transformObject = m_pluginInterface->GetSceneManager()->GetObjectByID( m_targetVolumeId );
                if( transformObject->GetParent() )
                {
                    vtkTransform * parentVtktransform = vtkTransform::SafeDownCast( transformObject->GetParent()->GetWorldTransform() );
                    Q_ASSERT_X( parentVtktransform, "VertebraRegistrationWidget::AddImageToQueue()", "Invalid transform" );
                    m_pluginInterface->getGPURegistrationWidget()->SetParentVtkTransform(parentVtktransform);
                }

                m_pluginInterface->getGPURegistrationWidget()->SetUseMask( true );
                m_pluginInterface->getGPURegistrationWidget()->runRegistration();

                std::cout << "Performing registration..." << std::endl;
                resultTransform = m_pluginInterface->getGPURegistrationWidget()->GetResultTransform();
                std::cout << "Done." << std::endl;
                resultTransform->Print(std::cout);

//                IbisItkFloat3ImageType::DirectionType direction;

//                IbisItkFloat3ImageType::PointType origin;
//                for (int i = 0; i < 3; ++i) {
//                    origin[i] = resultTransform->GetMatrix()->GetElement(i,3);
//                    for (int j = 0; j < 3; ++j) {
//                        direction[i][j] = resultTransform->GetMatrix()->GetElement(i,j);
//                    }
//                }

                std::cout << "Direction :" << m_sparseVolume->GetDirection() << std::endl;
                std::cout << "Origin :" << m_sparseVolume->GetOrigin() << std::endl;


                std::cout << "Adding volume to scene" << std::endl;
                ImageObject *imobj = ImageObject::New();
                imobj->SetItkImage(m_sparseVolume);
                imobj->SetName("Volume");
                m_pluginInterface->GetSceneManager()->AddObject(imobj);
                m_pluginInterface->GetSceneManager()->SetReferenceDataObject(imobj);
                imobj->SetLocalTransform( resultTransform );
                imobj->Modified();




            }

            m_isProcessing = false;
        }
    }
}

itk::SmartPointer<IbisItkFloat3ImageType> VertebraRegistrationWidget::GetImageFromQueue()
{
    if (!m_queueSemaphore)
    {
        m_queueSemaphore = true;
        if (m_inputImageList.size() > 0)
        {
            itk::SmartPointer<IbisItkFloat3ImageType> itkimage = m_inputImageList.front();
            m_inputImageList.pop();
            m_queueSemaphore = false;
            return itkimage;
        }
        m_queueSemaphore = false;
    }
    return 0;

}

void VertebraRegistrationWidget::ClearQueue()
{
    if (!m_queueSemaphore)
    {
        m_queueSemaphore = true;
        while( !m_inputImageList.empty() )
            m_inputImageList.pop();

       m_queueSemaphore = false;
    }
}
