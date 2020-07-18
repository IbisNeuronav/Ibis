/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Author: Houssem-Eddine Gueziri

#include "vertebraregistrationwidget.h"
#include "ui_vertebraregistrationwidget.h"

VertebraRegistrationWidget::VertebraRegistrationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VertebraRegistrationWidget),
    m_isProcessing(false),
    m_thresholdDistanceToAddImage(0.0),
    m_navigationWidget(nullptr),
    m_isNavigating(false),
    m_lambdaMetricBalance(0.5),
    m_showAdvancedSettings(false),
    m_optNumberOfPixels(128000),
    m_optSelectivity(32),
    m_optPercentile(0.8),
    m_optPopulationSize(60),
    m_optInitialSigma(1.0)
{
    m_pluginInterface = 0;
    ui->setupUi(this);

    m_reconstructionResolution = 1.0;
    m_reconstructionSearchRadius = 1;
    m_sweepDirection = "ItoS";

    std::srand(std::time(nullptr));

    setWindowTitle( "Vertebra Registration" );
}

VertebraRegistrationWidget::~VertebraRegistrationWidget()
{
    if(m_pluginInterface)
    {
        IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
        if (ibisApi)
        {
            disconnect( ibisApi, SIGNAL(ObjectAdded(int)), this, SLOT(OnObjectAddedSlot(int)) );
            disconnect( ibisApi, SIGNAL(ObjectRemoved(int)), this, SLOT(OnObjectRemovedSlot(int)) );
        }
    }
    delete ui;
    m_PlannedScrewList.clear();
    m_pluginInterface = 0;
}

void VertebraRegistrationWidget::SetPluginInterface( PedicleScrewNavigationPluginInterface * pluginInterf )
{
    m_pluginInterface = pluginInterf;
    if(m_pluginInterface)
    {
        IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
        if (ibisApi)
        {
            connect( ibisApi, SIGNAL(ObjectAdded(int)), this, SLOT(OnObjectAddedSlot(int)) );
            connect( ibisApi, SIGNAL(ObjectRemoved(int)), this, SLOT(OnObjectRemovedSlot(int)) );
        }
        this->UpdateUi();
    }
}

vtkRenderer * VertebraRegistrationWidget::GetScrewNavigationAxialRenderer()
{
    if( !m_isNavigating || !m_navigationWidget )
        return nullptr;
    return m_navigationWidget->GetAxialRenderer();
}

vtkRenderer * VertebraRegistrationWidget::GetScrewNavigationSagittalRenderer()
{
    if( !m_isNavigating || !m_navigationWidget )
        return nullptr;
    return m_navigationWidget->GetSagittalRenderer();
}

void VertebraRegistrationWidget::UpdateUi()
{
    IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
    Q_ASSERT(ibisAPI);
    if( ibisAPI )
    {
        ui->usImageComboBox->clear();
        ui->ctImageComboBox->clear();
        ui->volumeComboBox->clear();

        const SceneManager::ObjectList & allObjects = ibisAPI->GetAllObjects();
        for( int i = 0; i < allObjects.size(); ++i )
        {
            SceneObject * current = allObjects[i];
            if( current != ibisAPI->GetSceneRoot() && current->IsListable() )
            {
                if( current->IsA("ImageObject") )
                {
                    ui->ctImageComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
                    ui->volumeComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
                }
                else if (current->IsA("USAcquisitionObject"))
                {
                    ui->usImageComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
                }
                else if( current->IsA("PointerObject") )
                {
                    ui->navigateButton->setEnabled(true);
                }
            }
        }

        if( ui->ctImageComboBox->count() == 0 )
        {
            ui->ctImageComboBox->addItem( "None", QVariant(IbisAPI::InvalidId) );
            ui->volumeComboBox->addItem( "None", QVariant(IbisAPI::InvalidId) );
        }

        if( ui->usImageComboBox->count() == 0 )
        {
            ui->usImageComboBox->addItem("None", QVariant(IbisAPI::InvalidId));
        }
    }

    ui->ultrasoundResolutionComboBox->addItem(tr("0.5 x 0.5 x 0.5"), 0.5);
    ui->ultrasoundResolutionComboBox->addItem(tr("1 x 1 x 1"), 1.0);
    ui->ultrasoundResolutionComboBox->addItem(tr("1.5 x 1.5 x 1.5"), 1.5);
    ui->ultrasoundResolutionComboBox->setCurrentIndex(1);

    ui->ultrasoundSearchRadiusComboBox->addItem(tr("0"), 0);
    ui->ultrasoundSearchRadiusComboBox->addItem(tr("1"), 1);
    ui->ultrasoundSearchRadiusComboBox->addItem(tr("2"), 2);
    ui->ultrasoundSearchRadiusComboBox->setCurrentIndex(1);

    ui->optPopulationSizeComboBox->addItem( tr( "40" ), 40 );
    ui->optPopulationSizeComboBox->addItem( tr( "50" ), 50 );
    ui->optPopulationSizeComboBox->addItem( tr( "60" ), 60 );
    ui->optPopulationSizeComboBox->addItem( tr( "70" ), 70 );
    ui->optPopulationSizeComboBox->addItem( tr( "80" ), 80 );
    ui->optPopulationSizeComboBox->addItem( tr( "90" ), 90 );
    ui->optPopulationSizeComboBox->setCurrentIndex( 2 );
    
    ui->optInitialSigmaComboBox->addItem( tr( "0.1" ), 0.1 );
    ui->optInitialSigmaComboBox->addItem( tr( "1.0" ), 1.0 );
    ui->optInitialSigmaComboBox->addItem( tr( "2.0" ), 2.0 );
    ui->optInitialSigmaComboBox->addItem( tr( "4.0" ), 4.0 );
    ui->optInitialSigmaComboBox->addItem( tr( "8.0" ), 8.0 );
    ui->optInitialSigmaComboBox->setCurrentIndex( 1 );

    ui->advancedSettingsGroupBox->hide();
}

itk::Point< double, 3 > VertebraRegistrationWidget::GetImageCenterPoint(IbisItkFloat3ImagePointer image)
{
    IbisItkFloat3ImageType::PointType centerPoint;
    IbisItkFloat3ImageType::SizeType imageSize = image->GetLargestPossibleRegion().GetSize();
    IbisItkFloat3ImageType::IndexType centerIndex;

    for (int i = 0; i < image->GetImageDimension(); ++i) {
      centerIndex[i] = imageSize[i] / 2.0;
    }

    image->TransformIndexToPhysicalPoint(centerIndex, centerPoint);

    return centerPoint;

}

void VertebraRegistrationWidget::GetUSScanDirection(itk::Vector<double, 3> & directionVector,
                                                    itk::Point<double, 3> & centerPoint,
                                                    std::vector< itk::Point< double, 3> > usScanCenterPointList,
                                                    vtkTransform * parentTransform)
{
    itk::Point<double, 3> p;
    centerPoint.Fill(0);
    vnl_matrix<double> points(usScanCenterPointList.size(), 3);

    for (int i = 0; i < usScanCenterPointList.size(); ++i)
    {
        p = usScanCenterPointList[i];
        if (parentTransform)
        {
            float *temp;
            temp = parentTransform->TransformFloatPoint((float)p[0], (float)p[1], (float)p[2]);
            p[0] = temp[0];
            p[1] = temp[1];
            p[2] = temp[2];
        }
        for (int j = 0; j < 3; ++j) {
            points(i, j) = p[j];
            centerPoint[j] += p[j];
        }
    }

    centerPoint[0] /= usScanCenterPointList.size();
    centerPoint[1] /= usScanCenterPointList.size();
    centerPoint[2] /= usScanCenterPointList.size();

    for (int i = 0; i < usScanCenterPointList.size(); ++i)
    {
        points(i, 0) -= centerPoint[0];
        points(i, 1) -= centerPoint[1];
        points(i, 2) -= centerPoint[2];
    }

    vnl_svd<double> svd(points);
    vnl_matrix<double> V = svd.V();

    directionVector[0] = V(0, 0);
    directionVector[1] = V(1, 0);
    directionVector[2] = V(2, 0);

}

void VertebraRegistrationWidget::GetUSScanOrthogonalDirection(itk::Vector<double, 3> & directionVector,
                                                              std::vector< itk::SmartPointer<IbisItkFloat3ImageType> > inputImageList,
                                                              std::vector< itk::Point< double, 3> > usScanCenterPointList,
                                                              vtkTransform * parentTransform)
{
    IbisItkFloat3ImageType::IndexType index;
    itk::Point<double, 3> centerPoint;
    itk::Point<double, 3> bottomPoint;
    IbisItkFloat3ImagePointer itkImage = IbisItkFloat3ImageType::New();
    IbisItkFloat3ImageType::SizeType size;
    itk::Vector<double, 3> vector;

    // initialize direction to null
    directionVector.Fill(0);

    for (int i = 0; i < inputImageList.size(); ++i)
    {
        centerPoint = usScanCenterPointList[i];
        itkImage = inputImageList[i];
        size = itkImage->GetLargestPossibleRegion().GetSize();
        index[0] = size[0] / 2.0;
        index[1] = 0;
        index[2] = 0;
        itkImage->TransformIndexToPhysicalPoint(index, bottomPoint);

        if (parentTransform)
        {
            float *temp;
            temp = parentTransform->TransformFloatPoint((float)bottomPoint[0], (float)bottomPoint[1], (float)bottomPoint[2]);
            bottomPoint[0] = temp[0];
            bottomPoint[1] = temp[1];
            bottomPoint[2] = temp[2];

            temp = parentTransform->TransformFloatPoint((float)centerPoint[0], (float)centerPoint[1], (float)centerPoint[2]);
            centerPoint[0] = temp[0];
            centerPoint[1] = temp[1];
            centerPoint[2] = temp[2];
        }
        vector = bottomPoint - centerPoint;
        vector.Normalize();
        directionVector += vector;
    }
    directionVector.Normalize();
}


bool VertebraRegistrationWidget::CreateVolumeFromSlices(USAcquisitionObject *usAcquisitionObject, float spacingFactor)
{
    m_usScanCenterPointList.clear();
    m_inputImageList.clear();

    IbisItkFloat3ImageType::PointType prevCenterPoint;
    IbisItkFloat3ImageType::PointType currCenterPoint;
    prevCenterPoint.Fill(itk::NumericTraits<IbisItkFloat3ImageType::PixelType>::max());

    ImageCastFilterUC2F::Pointer caster = ImageCastFilterUC2F::New();

    int N = usAcquisitionObject->GetNumberOfSlices();

    bool processOK = true;
    for (int i = 0; i < N; ++i)
    {
        IbisItkFloat3ImagePointer itkImage = IbisItkFloat3ImageType::New();
        IbisItkUC3ImagePointer itkUCImage = IbisItkUnsignedChar3ImageType::New();

        usAcquisitionObject->GetItkImage(itkUCImage, i, true, true); // maybe use calibrated transform?
        caster->SetInput(itkUCImage);
        caster->Update();
        itkImage = caster->GetOutput();

        currCenterPoint = this->GetImageCenterPoint( itkImage );
        if ((i == 0) | (currCenterPoint.EuclideanDistanceTo(prevCenterPoint) >= m_thresholdDistanceToAddImage ))
        {
            // set previous point to current
            prevCenterPoint[0] = currCenterPoint[0];
            prevCenterPoint[1] = currCenterPoint[1];
            prevCenterPoint[2] = currCenterPoint[2];

            m_usScanCenterPointList.push_back( prevCenterPoint );

            // put image in vector
            typedef itk::ImageDuplicator<IbisItkFloat3ImageType> DuplicatorType;
            DuplicatorType::Pointer duplicator = DuplicatorType::New();
            duplicator->SetInputImage(itkImage);
            duplicator->Update();

            m_inputImageList.push_back( duplicator->GetOutput() );

        }
    }

    // US reconstruction
    GPU_VolumeReconstruction * volumeReconstructor = GPU_VolumeReconstruction::New();;
    volumeReconstructor->SetNumberOfSlices( usAcquisitionObject->GetNumberOfSlices() );
    if (ui->useMaskCheckBox->isChecked())
        {
         volumeReconstructor->SetFixedSliceMask(usAcquisitionObject->GetMask());
        }
    volumeReconstructor->SetUSSearchRadius( m_reconstructionSearchRadius );
    volumeReconstructor->SetVolumeSpacing( m_reconstructionResolution );
    volumeReconstructor->SetKernelStdDev( m_reconstructionResolution / 2.0 );

    vtkSmartPointer<vtkMatrix4x4> sliceTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New() ;
    vtkSmartPointer<vtkImageData> slice = vtkSmartPointer<vtkImageData>::New();
    for(unsigned int i=0; i<usAcquisitionObject->GetNumberOfSlices(); i++)
    {
        usAcquisitionObject->GetFrameData( i, slice, sliceTransformMatrix );
        volumeReconstructor->SetFixedSlice(i, slice, sliceTransformMatrix );
    }

     //Construct ITK Matrix corresponding to VTK Local Matrix
    volumeReconstructor->SetTransform( usAcquisitionObject->GetLocalTransform()->GetMatrix() );
    volumeReconstructor->start();
    volumeReconstructor->wait();
    m_sparseUsVolume = volumeReconstructor->GetReconstructedImage();

    return processOK;
}

IbisItkFloat3ImageType::PointType VertebraRegistrationWidget::GetCenterOfGravity(IbisItkFloat3ImagePointer itkImage)
{
    using FixedImageCalculatorType = itk::ImageMomentsCalculator<IbisItkFloat3ImageType>;
    FixedImageCalculatorType::Pointer fixedCalculator = FixedImageCalculatorType::New();
    fixedCalculator->SetImage( itkImage );
    fixedCalculator->Compute();

    IbisItkFloat3ImageType::PointType center = fixedCalculator->GetCenterOfGravity();
    return center;
}

void VertebraRegistrationWidget::PerformInitialAlignment(vtkTransform * &rigidTransform,
                                                         IbisItkFloat3ImagePointer itkCTImage,
                                                         std::vector< itk::SmartPointer<IbisItkFloat3ImageType> > inputImageList,
                                                         std::vector< itk::Point< double, 3> > usScanCenterPointList,
                                                         vtkTransform * parentUSAcquisitionTransform)
{
    PointsObject * usPointSet = PointsObject::New();
    PointsObject * ctPointSet = PointsObject::New();

    {
        // get 3 points from US acquisition
        IbisItkFloat3ImageType::PointType usEndPoint;
        IbisItkFloat3ImageType::PointType usCenterPoint;
        IbisItkFloat3ImageType::PointType usBottomPoint;
        itk::Vector<double, 3> usScanDirection;
        itk::Vector<double, 3> usOrthogonalDirection;

        usCenterPoint.Fill(0);
        usScanDirection.Fill(0);
        usOrthogonalDirection.Fill(0);

        this->GetUSScanDirection(usScanDirection, usCenterPoint, usScanCenterPointList, parentUSAcquisitionTransform);

        this->GetUSScanOrthogonalDirection(usOrthogonalDirection, inputImageList, usScanCenterPointList, parentUSAcquisitionTransform);

        if (m_sweepDirection == "ItoS")
            usEndPoint = usCenterPoint + usScanDirection * 20;
        else if (m_sweepDirection == "StoI")
            usEndPoint = usCenterPoint - usScanDirection * 20;
        else
            return;
        usBottomPoint = usCenterPoint + usOrthogonalDirection * 20;

        usPointSet->AddPoint("Center", usCenterPoint.GetDataPointer());
        usPointSet->AddPoint("End", usEndPoint.GetDataPointer());
        usPointSet->AddPoint("Bottom", usBottomPoint.GetDataPointer());

        usPointSet->SetName("US Points");
    }

    {
        // get 3 points from CT image
        IbisItkFloat3ImageType::IndexType ctIndex;
        IbisItkFloat3ImageType::PointType ctCenterPoint;
        IbisItkFloat3ImageType::PointType ctEndPoint;
        IbisItkFloat3ImageType::PointType ctBottomPoint;
        IbisItkFloat3ImageType::SizeType ctSize;
        itk::Vector<double, 3> ctScanDirection;
        itk::Vector<double, 3> ctOrthogonalDirection;

        ctSize = itkCTImage->GetLargestPossibleRegion().GetSize();

        // following the RAS convension
        ctCenterPoint = this->GetCenterOfGravity(itkCTImage);

        ctIndex[0] = ctSize[0] / 2.0;
        ctIndex[1] = ctSize[1] / 2.0;
        ctIndex[2] = ctSize[2];
        itkCTImage->TransformIndexToPhysicalPoint(ctIndex, ctEndPoint);

        ctScanDirection = ctEndPoint - ctCenterPoint;
        ctScanDirection.Normalize();
        ctEndPoint = ctCenterPoint + ctScanDirection * 20;

        ctIndex[0] = ctSize[0] / 2.0;
        ctIndex[1] = 0;
        ctIndex[2] = ctSize[2] / 2.0;
        itkCTImage->TransformIndexToPhysicalPoint(ctIndex, ctBottomPoint);

        ctOrthogonalDirection = ctBottomPoint - ctCenterPoint;
        ctOrthogonalDirection.Normalize();
        ctBottomPoint = ctCenterPoint + ctOrthogonalDirection * 20;

        ctPointSet->AddPoint("Center", ctCenterPoint.GetDataPointer());
        ctPointSet->AddPoint("End", ctEndPoint.GetDataPointer());
        ctPointSet->AddPoint("Bottom", ctBottomPoint.GetDataPointer());

        ctPointSet->SetName("CT Points");
    }

    vtkSmartPointer<vtkLandmarkTransform> landmarkTransform = vtkSmartPointer<vtkLandmarkTransform>::New();
    landmarkTransform->SetSourceLandmarks(ctPointSet->GetPoints());
    landmarkTransform->SetTargetLandmarks(usPointSet->GetPoints());
    landmarkTransform->SetModeToRigidBody();
    landmarkTransform->Update();

    rigidTransform->SetMatrix(landmarkTransform->GetMatrix());
}

bool VertebraRegistrationWidget::Register()
{
    IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
    Q_ASSERT(ibisAPI);

    QProgressDialog * progress = ibisAPI->StartProgress(4, tr("Reconstructing US volume"));
    progress->setLabelText(tr("Preparing registration..."));
    qApp->processEvents();
    if( progress->wasCanceled() )
    {
        QMessageBox::information(0, "Registering", "Process cancelled", 1, 0);
        return false;
    }
    // Get input CT image
    if (ui->ctImageComboBox->count() == 0)
    {
       QMessageBox::information( this, "Vertebra Rigid Registration", "Need to specify Moving image (CT image)." );
       return false;
    }
    int ctImageObjectId = ui->ctImageComboBox->itemData( ui->ctImageComboBox->currentIndex() ).toInt();
    ImageObject * ctImageObject = 0;
    vtkTransform * sourceVtkTransform = 0;
    if ( ctImageObjectId != SceneManager::InvalidId )
    {
        ctImageObject = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( ctImageObjectId ) );
        Q_ASSERT_X( ctImageObject, "VertebraRegistrationWidget::on_startRegistrationButton_clicked()", "Invalid CT Image Object" );
        sourceVtkTransform = ctImageObject->GetLocalTransform();
        Q_ASSERT_X( sourceVtkTransform, "VertebraRegistrationWidget::on_startRegistrationButton_clicked()", "Invalid CT Image Transform Object" );
    }
    else
    {
        QMessageBox::information( this, "Vertebra Rigid Registration", "CT volume not found." );
        return false;
    }

    IbisItkFloat3ImagePointer itkSourceImage;
    itkSourceImage = ctImageObject->GetItkImage();

    // Get input US acquisition
    if (ui->usImageComboBox->count() == 0)
    {
       QMessageBox::information( this, "Vertebra Rigid Registration", "Need to specify an US acquisition." );
       return false;
    }
    int usAcquisitionObjectId = ui->usImageComboBox->itemData( ui->usImageComboBox->currentIndex() ).toInt();
    USAcquisitionObject * usAcquisitionObject;
    if ( usAcquisitionObjectId != SceneManager::InvalidId )
    {
        usAcquisitionObject = USAcquisitionObject::SafeDownCast( ibisAPI->GetObjectByID( usAcquisitionObjectId ) );
        Q_ASSERT_X( usAcquisitionObject, "VertebraRegistrationWidget::on_startRegistrationButton_clicked()", "Invalid US Acquisition Object" );
    }
    else
    {
        QMessageBox::information( this, "Vertebra Rigid Registration", "US acquisition not found." );
        return false;
    }

    progress->setLabelText(tr("Reconstructing ultrasound volume..."));
    ibisAPI->UpdateProgress(progress, 1);
    qApp->processEvents();
    if( progress->wasCanceled() )
    {
        QMessageBox::information(0, "Vertebra Rigid Registration", "Process cancelled", 1, 0);
        return false;
    }

    if ( !this->CreateVolumeFromSlices(usAcquisitionObject, m_reconstructionResolution) )
        return false;
    
    if( ui->addUltrasoundReconstructionCheckBox->isChecked() )
    {
        ImageObject * imobj = ImageObject::New();
        imobj->SetItkImage(m_sparseUsVolume);
        imobj->SetName("Reconstructed US Volume");
        
        ibisAPI->AddObject(imobj);
        imobj->ChooseColorTable(1);
    }
    
    progress->setLabelText(tr("Performing initial alignment..."));
    ibisAPI->UpdateProgress(progress, 2);
    qApp->processEvents();
    if( progress->wasCanceled() )
    {
        QMessageBox::information(0, "Vertebra Rigid Registration", "Process cancelled", 1, 0);
        return false;
    }

    ImageObject * targetImageObject = ImageObject::New();
    Q_ASSERT_X( targetImageObject, "VertebraRegistrationWidget::on_startRegistrationButton_clicked()", "Invalid target object" );
    targetImageObject->SetItkImage(m_sparseUsVolume);
    vtkTransform * targetVtkTransform = vtkTransform::SafeDownCast( targetImageObject->GetWorldTransform() );

    IbisItkFloat3ImagePointer itkTargetImage = m_sparseUsVolume;

    if ( ui->initialAlignmentCheckBox->isChecked() )
    {
        this->PerformInitialAlignment(sourceVtkTransform, itkSourceImage, m_inputImageList, m_usScanCenterPointList);
        ctImageObject->Modified();
    }

    progress->setLabelText(tr("Registering..."));
    ibisAPI->UpdateProgress(progress, 3);
    qApp->processEvents();
    if( progress->wasCanceled() )
    {
        QMessageBox::information(0, "Vertebra Rigid Registration", "Process cancelled", 1, 0);
        return false;
    }

    if ( ui->gradientAlignmentCheckBox->isChecked() )
    {
        // Initialize parameters
        GPU_WeightRigidRegistration * rigidRegistrator = new GPU_WeightRigidRegistration();
	    if (m_lambdaMetricBalance == 1.0)
		    rigidRegistrator->SetRegistrationMetricToGradientOrientation();
	    else if (m_lambdaMetricBalance == 0)
		    rigidRegistrator->SetRegistrationMetricToIntensity();
	    else 
		    rigidRegistrator->SetRegistrationMetricToCombination();
        
        rigidRegistrator->SetSamplingStrategyToRandom();
        rigidRegistrator->SetUseMask( true );
        if (m_optNumberOfPixels == 128000)
        {
            rigidRegistrator->SetOrientationNumberOfPixels( itkTargetImage->GetRequestedRegion().GetNumberOfPixels() );
        }
        else
        {
            rigidRegistrator->SetOrientationNumberOfPixels( m_optNumberOfPixels );
        }
        rigidRegistrator->SetOrientationSelectivity( m_optSelectivity );
        rigidRegistrator->SetOrientationPercentile( m_optPercentile );
        rigidRegistrator->SetPopulationSize( m_optPopulationSize );
        rigidRegistrator->SetInitialSigma( m_optInitialSigma );
        rigidRegistrator->SetLambdaMetricBalance( m_lambdaMetricBalance );
        rigidRegistrator->SetDebug( false );

        // Set image inputs
        rigidRegistrator->SetItkSourceImage( itkSourceImage );
        rigidRegistrator->SetItkTargetImage( itkTargetImage );

        // Set transform inputs
        rigidRegistrator->SetVtkTransform( sourceVtkTransform );
        rigidRegistrator->SetSourceVtkTransform( sourceVtkTransform );
        rigidRegistrator->SetTargetVtkTransform( targetVtkTransform );

        rigidRegistrator->SetTargetMask(0);

        if( ctImageObject->GetParent() )
        {
           vtkTransform * parentVtktransform = vtkTransform::SafeDownCast( ctImageObject->GetParent()->GetWorldTransform() );
           Q_ASSERT_X( parentVtktransform, "VertebraRegistrationWidget::on_startRegistrationButton_clicked()", "Invalid transform" );
           rigidRegistrator->SetParentVtkTransform(parentVtktransform);
        }

        // Run registration
        ctImageObject->StartModifyingTransform();

        rigidRegistrator->runRegistration();

        ctImageObject->FinishModifyingTransform();
        ctImageObject->SetLocalTransform(sourceVtkTransform);
    }

    ibisAPI->StopProgress(progress);
    return true;
}

void VertebraRegistrationWidget::StartNavigation()
{
    if (!m_navigationWidget)
    {
        IbisAPI* ibisAPI = m_pluginInterface->GetIbisAPI();
        m_navigationWidget = new ScrewNavigationWidget( m_PlannedScrewList );
        m_navigationWidget->setWindowTitle( "Screw Navigation" );
        m_navigationWidget->setAttribute( Qt::WA_DeleteOnClose );
        m_navigationWidget->SetPluginInterface( m_pluginInterface );

        QFlags<QDockWidget::DockWidgetFeature> flags = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable;
        ibisAPI->ShowFloatingDock( m_navigationWidget, flags );
        connect( m_navigationWidget, SIGNAL( destroyed() ), this, SLOT( on_navigationWindowClosed() ) );
        connect( m_navigationWidget, SIGNAL(CloseNavigationWidget()), this, SLOT(on_navigateButton_clicked()));
    }
    else
    {
        m_navigationWidget->show();
    }

    ui->navigateButton->setStyleSheet( "QPushButton { background-color: green }" );
    m_navigationWidget->Navigate();
    m_isNavigating = true;
}

void VertebraRegistrationWidget::StopNavigation()
{
    m_navigationWidget->GetPlannedScrews( m_PlannedScrewList );
    m_navigationWidget->StopNavigation();
    m_navigationWidget->hide();
    ui->navigateButton->setStyleSheet( "" );
    m_isNavigating = false;
}

/* ******************************************
 *      Slot functions implementation       *
 * ******************************************/

void VertebraRegistrationWidget::on_startRegistrationButton_clicked()
{
    if( !m_isProcessing )
    {
        m_isProcessing = true;
        QElapsedTimer timer;
        timer.start();
        
        bool processOK;
        processOK = this->Register();
        
        double elapsedTime = double(timer.elapsed()) / 1000.0;
        if( processOK )
            ui->elapsedTimeLabel->setText(tr("Time: ") + QString::number(elapsedTime) + tr(" s"));
        m_isProcessing = false;
    }
}

void VertebraRegistrationWidget::on_sweepDirectionComboBox_currentIndexChanged(int value)
{
    switch (value)
    {
        case 0:
            m_sweepDirection = "StoI";
            break;
        case 1:
            m_sweepDirection = "ItoS";
            break;
    }
}

void VertebraRegistrationWidget::on_navigateButton_clicked()
{
    if (!m_isNavigating)
    {
        bool hasNavPointer = false;
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        const SceneManager::ObjectList & allObjects = ibisApi->GetAllObjects();
        for( int i = 0; i < allObjects.size(); ++i )
        {
            SceneObject * current = allObjects[i];
            if( current->IsA("PointerObject") )
            {
                hasNavPointer = true;
            }
        }
        
        if( hasNavPointer )
        {
            this->StartNavigation();
        }
        else
        {
            QMessageBox::information(this, "Pedicle Screw Navigation", "It seems that the plugin is unable to find a navigation pointer. Make sure that IGSIO is running properly.");
        }
    }
    else
    {
        this->StopNavigation();
    }
}

void VertebraRegistrationWidget::on_navigationWindowClosed()
{
    disconnect( m_navigationWidget, 0, 0, 0 );
    this->StopNavigation();
    m_navigationWidget = nullptr;
}

void VertebraRegistrationWidget::OnObjectAddedSlot( int imageObjectId )
{
    if (m_pluginInterface)
    {
        IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
        if (ibisApi)
        {
            SceneObject * sceneObject = ibisApi->GetObjectByID(imageObjectId);
            if( sceneObject->IsA("ImageObject") )
            {
                if(ui->ctImageComboBox->count() == 0)
                {
                    ui->ctImageComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                    ui->volumeComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                }
                else if(ui->ctImageComboBox->count() == 1)
                {
                    int currentItemId = ui->ctImageComboBox->itemData( ui->ctImageComboBox->currentIndex() ).toInt();

                    if(currentItemId == IbisAPI::InvalidId)
                    {
                        ui->ctImageComboBox->clear();
                        ui->volumeComboBox->clear();
                    }
                    ui->ctImageComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                    ui->volumeComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                }
                else
                {
                    ui->ctImageComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                    ui->volumeComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                }
            }
            else if( sceneObject->IsA("USAcquisitionObject") )
            {
                if(ui->usImageComboBox->count() == 0)
                {
                    ui->usImageComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                }
                else if(ui->usImageComboBox->count() == 1)
                {
                    int currentItemId = ui->usImageComboBox->itemData( ui->usImageComboBox->currentIndex() ).toInt();
                    if(currentItemId == IbisAPI::InvalidId)
                    {
                        ui->usImageComboBox->clear();
                    }
                    ui->usImageComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                }
                else
                {
                    ui->usImageComboBox->addItem(sceneObject->GetName(), QVariant(imageObjectId) );
                }
            }
        }
    }
}

void VertebraRegistrationWidget::OnObjectRemovedSlot( int imageObjectId )
{
    if (m_pluginInterface)
    {
        IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
        if (ibisApi)
        {
            for(int i=0; i < ui->ctImageComboBox->count(); ++i)
            {
                int currentItemId = ui->ctImageComboBox->itemData( i ).toInt();
                if(currentItemId == imageObjectId)
                {
                    ui->ctImageComboBox->removeItem(i);
                    ui->volumeComboBox->removeItem(i);
                }
            }

            for(int i=0; i < ui->usImageComboBox->count(); ++i)
            {
                int currentItemId = ui->usImageComboBox->itemData( i ).toInt();
                if(currentItemId == imageObjectId)
                {
                    ui->usImageComboBox->removeItem(i);
                }
            }

            if(ui->ctImageComboBox->count() == 0)
            {
                ui->ctImageComboBox->addItem(tr("None"), QVariant(IbisAPI::InvalidId));
                ui->volumeComboBox->addItem(tr("None"), QVariant(IbisAPI::InvalidId));
            }

            if(ui->usImageComboBox->count() == 0)
            {
                ui->usImageComboBox->addItem(tr("None"), QVariant(IbisAPI::InvalidId));
            }
        }
    }
}

void VertebraRegistrationWidget::on_presetVolumeButton_clicked()
{
    if(m_pluginInterface)
    {
        IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
        QList<SceneObject *> list;
        ibisApi->GetAllListableNonTrackedObjects(list);
        int objId = ui->volumeComboBox->itemData( ui->volumeComboBox->currentIndex() ).toInt();

        if( objId == IbisAPI::InvalidId )
            return;

        ImageObject *imObj = ImageObject::SafeDownCast(ibisApi->GetObjectByID(objId));
        vtkSmartPointer<vtkVolumeProperty> volumeProperty = imObj->GetVolumeProperty();

        double shift = -54;
        vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
        opacityTransferFunction->AddPoint(-3024 + shift, 0);
        opacityTransferFunction->AddPoint(143.556 + shift, 0);
        opacityTransferFunction->AddPoint(166.222 + shift, 0.686275);
        opacityTransferFunction->AddPoint(214.389 + shift, 0.696078);
        opacityTransferFunction->AddPoint(419.736 + shift, 0.833333);
        opacityTransferFunction->AddPoint(3071 + shift, 0.803922);
        vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
        colorTransferFunction->AddRGBPoint(-3024 + shift, 0, 0, 0);
        colorTransferFunction->AddRGBPoint(143.556 + shift, 0.615686, 0.356863, 0.184314);
        colorTransferFunction->AddRGBPoint(166.222 + shift, 0.882353, 0.603922, 0.290196);
        colorTransferFunction->AddRGBPoint(214.389 + shift, 1, 1, 1);
        colorTransferFunction->AddRGBPoint(419.736 + shift, 1, 0.937033, 0.954531);
        colorTransferFunction->AddRGBPoint(3071 + shift, 0.827451, 0.658824, 1);

        volumeProperty->SetColor(colorTransferFunction);
        volumeProperty->SetScalarOpacity(opacityTransferFunction);
        volumeProperty->SetSpecular(0.2);
        volumeProperty->SetShade(1);
        volumeProperty->SetAmbient(0.1);
        volumeProperty->SetDiffuse(0.9);
        volumeProperty->SetSpecularPower(10);
        imObj->SetVtkVolumeRenderingEnabled(true);
        imObj->Modified();

        ui->opacityShiftSlider->setEnabled(true);

    }
}

void VertebraRegistrationWidget::on_opacityShiftSlider_valueChanged(int value)
{
    if(m_pluginInterface)
    {
        IbisAPI *ibisApi = m_pluginInterface->GetIbisAPI();
        QList<SceneObject *> list;
        ibisApi->GetAllListableNonTrackedObjects(list);
        int objId = ui->volumeComboBox->itemData( ui->volumeComboBox->currentIndex() ).toInt();

        if( objId == IbisAPI::InvalidId )
            return;

        ImageObject *imObj = ImageObject::SafeDownCast(ibisApi->GetObjectByID(objId));

        if(imObj)
        {
            vtkSmartPointer<vtkVolumeProperty> volumeProperty = imObj->GetVolumeProperty();

            double shift = (double) ui->opacityShiftSlider->value();
            vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
            opacityTransferFunction->AddPoint(-3024 + shift, 0);
            opacityTransferFunction->AddPoint(143.556 + shift, 0);
            opacityTransferFunction->AddPoint(166.222 + shift, 0.686275);
            opacityTransferFunction->AddPoint(214.389 + shift, 0.696078);
            opacityTransferFunction->AddPoint(419.736 + shift, 0.833333);
            opacityTransferFunction->AddPoint(3071 + shift, 0.803922);
            vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
            colorTransferFunction->AddRGBPoint(-3024 + shift, 0, 0, 0);
            colorTransferFunction->AddRGBPoint(143.556 + shift, 0.615686, 0.356863, 0.184314);
            colorTransferFunction->AddRGBPoint(166.222 + shift, 0.882353, 0.603922, 0.290196);
            colorTransferFunction->AddRGBPoint(214.389 + shift, 1, 1, 1);
            colorTransferFunction->AddRGBPoint(419.736 + shift, 1, 0.937033, 0.954531);
            colorTransferFunction->AddRGBPoint(3071 + shift, 0.827451, 0.658824, 1);

            volumeProperty->SetColor(colorTransferFunction);
            volumeProperty->SetScalarOpacity(opacityTransferFunction);
            std::cout << ui->opacityShiftSlider->value() << std::endl;
            imObj->Modified();

        }

    }
}

void VertebraRegistrationWidget::on_ultrasoundResolutionComboBox_currentIndexChanged(int value)
{
    m_reconstructionResolution = ui->ultrasoundResolutionComboBox->itemData(value).toDouble();
}

void VertebraRegistrationWidget::on_ultrasoundSearchRadiusComboBox_currentIndexChanged(int value)
{
    m_reconstructionSearchRadius = ui->ultrasoundSearchRadiusComboBox->itemData(value).toUInt();
}

void VertebraRegistrationWidget::on_lambdaMetricSlider_valueChanged( int value )
{
    m_lambdaMetricBalance = (double)value / 10.0;
    QString glabel = QString{ "Gradient orientation\n( %1 )" }.arg( m_lambdaMetricBalance, 1, 'f', 1 );
    QString ilabel = QString{ "Intensity\n( %1 )" }.arg( 1 - m_lambdaMetricBalance, 1, 'f', 1 );
    ui->lambdaMetricGradientLabel->setText( glabel );
    ui->lambdaMetricIntensityLabel->setText( ilabel );
}

void VertebraRegistrationWidget::on_numberOfPixelsDial_valueChanged( int value )
{
    m_optNumberOfPixels = value;
    if (m_optNumberOfPixels == 128000)
    {
        ui->numberOfPixelsLabel->setText( tr( "Number of Pixels\nAll pixels" ) );
    }
    else
    {
        ui->numberOfPixelsLabel->setText( tr( "Number of Pixels\n" ) + QString{ "%1" }.arg( value ) );
    }
}

void VertebraRegistrationWidget::on_selectivityDial_valueChanged( int value )
{
    m_optSelectivity = value;
    ui->selectivityLabel->setText( tr( "Selectivity\n" ) + QString{ "%1" }.arg( value ) );
}

void VertebraRegistrationWidget::on_percentileDial_valueChanged( int value )
{
    m_optPercentile = (double)value / 100.0;
    ui->percentileLabel->setText( tr( "Percentile\n" ) + QString{ "%1" }.arg( m_optPercentile, 1, 'f', 1 ) );
}

void VertebraRegistrationWidget::on_optPopulationSizeComboBox_currentIndexChanged( int value )
{
    m_optPopulationSize = ui->optPopulationSizeComboBox->itemData(value).toInt();
}

void VertebraRegistrationWidget::on_initialSigmaComboBox_currentIndexChanged( int value )
{
    m_optInitialSigma = ui->optInitialSigmaComboBox->itemData(value).toDouble();
}

void VertebraRegistrationWidget::on_advancedSettingsButton_clicked()
{
    m_showAdvancedSettings = !m_showAdvancedSettings;
    if (m_showAdvancedSettings)
    {
        ui->advancedSettingsButton->setText( tr( "Hide advanced settings <<<" ) );
        ui->advancedSettingsGroupBox->show();
    }
    else
    {
        ui->advancedSettingsButton->setText( tr( "Show advanced settings >>>" ) );
        ui->advancedSettingsGroupBox->hide();
    }
}