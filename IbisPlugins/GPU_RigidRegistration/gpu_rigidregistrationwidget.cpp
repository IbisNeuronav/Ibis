/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Dante De Nigris for writing this class
#include "gpu_rigidregistrationwidget.h"
#include <QComboBox>
#include <QMessageBox>
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include "vnl/algo/vnl_real_eigensystem.h"
#include "vtkSmartPointer.h"

class CommandIterationUpdateOpenCL : public itk::Command
{
public:
  typedef  CommandIterationUpdateOpenCL   Self;
  typedef  itk::Command             Superclass;
  typedef itk::SmartPointer<Self>  Pointer;
  itkNewMacro( Self );

protected:
  CommandIterationUpdateOpenCL() {};

public:

  typedef const OptimizerType                         *OptimizerPointer;
  typedef const GPUCostFunctionType                   *GPUConstCostFunctionPointer;
  typedef ItkRigidTransformType::Pointer              ItkRigidTransformPointer;
  Application                                         *m_app;
  int                                                 m_transformObjectID;  
  int                                                 m_targetImageObjectID;
  bool                                                m_Debug;

  void SetApplication(Application * app)
  {
      m_app = app;
  }

  void SetDebug(bool debug)
  {
    m_Debug = debug;
  }

  void SetTransformObjectID( int transformObjectID )
  {
      m_transformObjectID = transformObjectID;
  }
  
  void SetTargetImageObjectID( int transformObjectID )
  {
      m_targetImageObjectID = transformObjectID;
  }  

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }

  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    OptimizerPointer optimizer =
                         dynamic_cast< OptimizerPointer >( object );
    GPUConstCostFunctionPointer   metric = dynamic_cast< GPUConstCostFunctionPointer >( optimizer->GetCostFunction() );

    if( ! itk::IterationEvent().CheckEvent( &event ) )
      {
      return;
      }

    if(m_Debug)
      std::cout << "Optimizer Value:\t" << optimizer->GetCurrentValue() << std::endl;

    if(m_app)
    {
      ItkRigidTransformPointer itkTransform = ItkRigidTransformType::New();      

      ItkRigidTransformType::ParametersType params = optimizer->GetCurrentPosition();
      ItkRigidTransformType::CenterType center = metric->GetCenter();
      itkTransform->SetCenter(center);
      itkTransform->SetParameters(params);

      SceneObject * transformObject = m_app->GetSceneManager()->GetObjectByID( m_transformObjectID );
      vtkTransform * vtktransform = vtkTransform::SafeDownCast( transformObject->GetLocalTransform() );
      Q_ASSERT_X( vtktransform, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid transform" );

      SceneObject * targetImageObject = m_app->GetSceneManager()->GetObjectByID( m_targetImageObjectID );
      vtkTransform * targetImageVtktransform = vtkTransform::SafeDownCast( targetImageObject->GetWorldTransform() );
      Q_ASSERT_X( targetImageVtktransform, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid transform" );

      transformObject->StartModifyingTransform();

      ItkRigidTransformType::MatrixType matrix = 
        itkTransform->GetMatrix();
    
      ItkRigidTransformType::OffsetType offset = 
        itkTransform->GetOffset();
    
      vtkSmartPointer<vtkMatrix4x4> localMatrix_inv = vtkSmartPointer<vtkMatrix4x4>::New();

      for(unsigned int i=0; i<3; i++ )
        {
        for(unsigned int j=0; j<3; j++ )
          {
          localMatrix_inv->SetElement(i,j,
            matrix.GetVnlMatrix().get(i,j));   
          }
    
        localMatrix_inv->SetElement( i, 3, offset[i]);        
       }
      localMatrix_inv->Invert();
       
      vtkSmartPointer<vtkMatrix4x4> targetLocalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      targetImageVtktransform->GetMatrix( targetLocalMatrix.GetPointer() );
       
      vtkSmartPointer<vtkMatrix4x4> finalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
       
      if( transformObject->GetParent() )
        {
        vtkTransform * parentVtktransform = vtkTransform::SafeDownCast( transformObject->GetParent()->GetWorldTransform() );
        Q_ASSERT_X( parentVtktransform, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid transform" );
        vtkSmartPointer<vtkMatrix4x4> parentWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
        parentVtktransform->GetInverse( parentWorldMatrix.GetPointer() );
        finalMatrix->Multiply4x4( parentWorldMatrix.GetPointer(), localMatrix_inv.GetPointer(), localMatrix_inv.GetPointer() );
        }

      finalMatrix->Multiply4x4( localMatrix_inv.GetPointer(), targetLocalMatrix.GetPointer(), finalMatrix.GetPointer() );
      vtktransform->SetMatrix( finalMatrix.GetPointer() );
      vtktransform->Modified();
      transformObject->FinishModifyingTransform();

     }    
  }

};


GPU_RigidRegistrationWidget::GPU_RigidRegistrationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GPU_RigidRegistrationWidget),
    m_application(0),
    m_OptimizationRunning(false)
{
    ui->setupUi(this);
    setWindowTitle( "Rigid Registration With GPU" );
    connect(ui->numebrOfPixelsDial , SIGNAL(valueChanged(int)), ui->numberOfSamplesValueLabel, SLOT(setNum(int)));
    connect(ui->selectivityDial, SIGNAL(valueChanged(int)), ui->parameterNValueLabel, SLOT(setNum(int)));
    connect(ui->populationSizeDial, SIGNAL(valueChanged(int)), ui->populationSizeValueLabel, SLOT(setNum(int)));
    connect(ui->debugCheckBox, SIGNAL(stateChanged(int)), this, SLOT(on_debugCheckBox_clicked()));
    ui->registrationOutputTextEdit->hide();
    UpdateUi();

}

GPU_RigidRegistrationWidget::~GPU_RigidRegistrationWidget()
{
    delete ui;
}



void GPU_RigidRegistrationWidget::SetApplication( Application * app )
{
    m_application = app;
    UpdateUi();
}

void GPU_RigidRegistrationWidget::on_startButton_clicked()
{
    // Make sure all params have been specified
    int sourceImageObjectId = ui->sourceImageComboBox->itemData( ui->sourceImageComboBox->currentIndex() ).toInt();
    int targetImageObjectId = ui->targetImageComboBox->itemData( ui->targetImageComboBox->currentIndex() ).toInt();
    int transformObjectId = ui->transformObjectComboBox->itemData( ui->transformObjectComboBox->currentIndex() ).toInt();

    if( transformObjectId == -1 || sourceImageObjectId == -1 || targetImageObjectId == -1 || sourceImageObjectId == targetImageObjectId)
    {
        QMessageBox::information( this, "Rigid Registration on GPU", "Need to specify Fixed image, Moving image (different from Fixed Image) and Transform Object before processing" );
        return;
    }

    // Get input images
    SceneManager * sm = m_application->GetSceneManager();
    ImageObject * sourceImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( sourceImageObjectId ) );
    Q_ASSERT_X( sourceImageObject, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid source object" );
    vtkTransform * sourceVtkTransform = vtkTransform::SafeDownCast( sourceImageObject->GetWorldTransform() );

    ImageObject * targetImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( targetImageObjectId ) );
    Q_ASSERT_X( targetImageObject, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid target object" );
    vtkTransform * targetVtkTransform = vtkTransform::SafeDownCast( targetImageObject->GetWorldTransform() );

    IbisItk3DImageType::Pointer itkSourceImage = sourceImageObject->GetItkImage();
    IbisItk3DImageType::Pointer itkTargetImage = targetImageObject->GetItkImage();


    QElapsedTimer timer;
    timer.start();


    ui->userFeedbackLabel->setText(QString("Processing..(patience is a virtue)"));
    /*
      Send std::cout from elastix to QTextEdit.
      Issue. QTextEdit doesn't sync properly, and we only get a text dump
      after the registration is finished.
    */
    bool debug = ui->debugCheckBox->isChecked();

    QDebugStream qout(std::cout,  ui->registrationOutputTextEdit);

    if(debug)
      std::cout << "Setting up registration..." << std::endl;
    /** Registration */

    ItkRigidTransformType::Pointer    itkTransform  = ItkRigidTransformType::New();
    
    OptimizerType::Pointer      optimizer     = OptimizerType::New();   

    double percentile =  ui->percentileComboBox->itemData( ui->percentileComboBox->currentIndex() ).toDouble();
    double initialSigma = ui->initialSigmaComboBox->itemData( ui->initialSigmaComboBox->currentIndex() ).toDouble();
    double gradientScale = 1.0;
    unsigned int numberOfPixels = ui->numebrOfPixelsDial->value();
    unsigned int orientationSelectivity = ui->selectivityDial->value();
    unsigned int populationSize = ui->populationSizeDial->value();

    GPUMetricPointer metric = GPUMetricType::New();
    metric->SetFixedImage(itkTargetImage);
    metric->SetMovingImage(itkSourceImage);

    /* Initialize Transform */
    vtkSmartPointer<vtkMatrix4x4> localMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    sourceVtkTransform->GetInverse(localMatrix.GetPointer());
  
    vtkSmartPointer<vtkMatrix4x4> finalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    finalMatrix->Multiply4x4( targetVtkTransform->GetMatrix(), localMatrix.GetPointer(), finalMatrix.GetPointer());

    ItkRigidTransformType::OffsetType offset;
 
    vnl_matrix<double> M(3,3); 

    for(unsigned int i=0; i<3; i++ )
     {
     for(unsigned int j=0; j<3; j++ )
       {
        M[i][j] = finalMatrix->GetElement(i,j);
       }     
      offset[i] = finalMatrix->GetElement(i,3);
      }

    double angleX, angleY, angleZ;
    angleX = vcl_asin(M[2][1]);
    double A = vcl_cos(angleX);
    if( vcl_fabs(A) > 0.00005 )
      {
      double x = M[2][2] / A;
      double y = -M[2][0] / A;
      angleY = vcl_atan2(y, x);

      x = M[1][1] / A;
      y = -M[0][1] / A;
      angleZ = vcl_atan2(y, x);
      }
    else
      {
      angleZ = 0;
      double x = M[0][0];
      double y = M[1][0];
      angleY = vcl_atan2(y, x);
      }

    ItkRigidTransformType::ParametersType params = ItkRigidTransformType::ParametersType(6);
    params[0] = angleX; params[1] = angleY; params[2] = angleZ;

    ItkRigidTransformType::CenterType center;
    center[0] = itkTargetImage->GetOrigin()[0] + itkTargetImage->GetSpacing()[0] * itkTargetImage->GetBufferedRegion().GetSize()[0] / 2.0;
    center[1] = itkTargetImage->GetOrigin()[1] + itkTargetImage->GetSpacing()[1] * itkTargetImage->GetBufferedRegion().GetSize()[1] / 2.0;
    center[2] = itkTargetImage->GetOrigin()[2] + itkTargetImage->GetSpacing()[2] * itkTargetImage->GetBufferedRegion().GetSize()[2] / 2.0;  


    for( unsigned int i = 0; i < 3; i++ )
      {
      params[i+3] = offset[i] - center[i];
      for( unsigned int j = 0; j < 3; j++ )
        {
        params[i+3] += M[i][j] * center[j];
        }
      }

    itkTransform->SetCenter(center);
    itkTransform->SetParameters(params);

    metric->SetTransform(itkTransform);
    metric->SetNumberOfPixels( numberOfPixels );
    metric->SetPercentile(           percentile          );
    metric->SetN(         orientationSelectivity           );
    metric->SetComputeMask( ui->computeMaskCheckBox->isChecked()  );
    metric->SetGradientScale(       gradientScale                 );
    GPUCostFunctionPointer costFunction = GPUCostFunctionType::New();
    costFunction->SetGPUMetric( metric );

    metric->Update();
    qint64 preprocessingTime = timer.elapsed();


    optimizer->SetCostFunction( costFunction ); 
    optimizer->SetInitialPosition( itkTransform->GetParameters() );
    OptimizerType::ScalesType   scales =  OptimizerType::ScalesType(itkTransform->GetNumberOfParameters());
    scales[0] = 3500;  scales[1] = 3500;  scales[2] = 3500;
    scales[3] = 0.1;  scales[4] = 0.1;  scales[5] = 0.1;
    optimizer->SetScales( scales );
    optimizer->SetUseCovarianceMatrixAdaptation( true );
    optimizer->SetUpdateBDPeriod( 0 );
    optimizer->SetValueTolerance(0.001);
    optimizer->SetMaximumDeviation( 1 );
    optimizer->SetUseScales( true );
    optimizer->SetPopulationSize( populationSize );
    optimizer->SetNumberOfParents( 0 );
    optimizer->SetMaximumNumberOfIterations( 300 );
    optimizer->SetInitialSigma( initialSigma );


    CommandIterationUpdateOpenCL::Pointer observer = CommandIterationUpdateOpenCL::New();
    observer->SetApplication( m_application );
    observer->SetTransformObjectID( transformObjectId );
    observer->SetTargetImageObjectID( targetImageObjectId );
    observer->SetDebug(debug);
    optimizer->AddObserver( itk::IterationEvent(), observer );

    if(debug)
      std::cout << "Starting registration..." << std::endl;

//    Qt::WindowFlags originalFlags = windowFlags() ;
//    Qt::WindowFlags flags = windowFlags() ;
//    flags &= ~Qt::WindowCloseButtonHint ;
//    setWindowFlags( Qt::Tool | Qt::WindowTitleHint| Qt::CustomizeWindowHint ) ;

    m_OptimizationRunning = true;
    try
    {
      optimizer->StartOptimization();
    }
    catch( itk::ExceptionObject & err )
    {
      std::cerr << "ExceptionObject caught !" << std::endl;
      std::cerr << err << std::endl;
    }

    qint64 registrationTime = timer.elapsed();

    if(debug)
      std::cout << "Done." << std::endl;

    if(debug)      
      std::cout << "Rigid Registration took " << qreal(registrationTime)/1000.0 << "secs"<< std::endl;

    QString feedbackString = QString("Full Registration finished in %1 secs ( Pre in %2 secs )").arg(qreal(registrationTime)/1000.0).arg(qreal(preprocessingTime)/1000.0);
    ui->userFeedbackLabel->setText(feedbackString);
    m_OptimizationRunning = false;

//    setWindowFlags( originalFlags ) ;

//    this->updateTagsDistance();
}

void GPU_RigidRegistrationWidget::UpdateUi()
{
  ui->sourceImageComboBox->clear();
  ui->targetImageComboBox->clear();
  ui->transformObjectComboBox->clear();
//  ui->usTagsComboBox->clear();
//  ui->mriTagsComboBox->clear();
  if( m_application )
  {
      //Set Parameters First
      ui->percentileComboBox->addItem( "0.6", QVariant( 0.6 ) );
      ui->percentileComboBox->addItem( "0.7", QVariant( 0.7 ) );
      ui->percentileComboBox->addItem( "0.8", QVariant( 0.8 ) );
      ui->percentileComboBox->addItem( "0.9", QVariant( 0.9 ) );
      ui->percentileComboBox->setCurrentIndex( 2 );

      ui->initialSigmaComboBox->addItem( "0.1", QVariant( 0.1 ) );
      ui->initialSigmaComboBox->addItem( "1.0", QVariant( 1.0 ) );
      ui->initialSigmaComboBox->addItem( "2.0", QVariant( 2.0 ) );
      ui->initialSigmaComboBox->addItem( "4.0", QVariant( 4.0 ) );
      ui->initialSigmaComboBox->addItem( "8.0", QVariant( 8.0 ) );
      ui->initialSigmaComboBox->setCurrentIndex( 1 );

      SceneManager * sm = m_application->GetSceneManager();
      const SceneManager::ObjectList & allObjects = sm->GetAllObjects();
      for( int i = 0; i < allObjects.size(); ++i )
      {
          SceneObject * current = allObjects[i];
          if( current != sm->GetSceneRoot() && current->IsListable() && !current->IsManagedByTracker())
          {
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
      // else
      //   {
      //     int targetId = ui->targetImageComboBox->itemData( ui->targetImageComboBox->currentIndex() ).toInt();
      //     SceneObject * targetParentObject = SceneObject::SafeDownCast( sm->GetObjectByID( targetId )->GetParent() );          
      //     for(unsigned int i=0; i < targetParentObject->GetNumberOfChildren(); i++)
      //     {
      //         if( targetParentObject->GetChild(i)->inherits("PointsObject") )
      //         {
      //             ui->usTagsComboBox->addItem( targetParentObject->GetChild(i)->GetName(), QVariant( targetParentObject->GetChild(i)->GetObjectID() ) );
      //         }
      //     }
      //   }

      // if( ui->usTagsComboBox->count() == 0 )
      //     ui->usTagsComboBox->addItem( "None", QVariant(-1) );


      if( ui->sourceImageComboBox->count() == 0 )
      {
          ui->sourceImageComboBox->addItem( "None", QVariant(-1) );        
      }
      else
      {
          int sourceId = ui->sourceImageComboBox->itemData( ui->sourceImageComboBox->currentIndex() ).toInt();
          ImageObject * sourceImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( sourceId ) );

          if( sourceImageObject->CanEditTransformManually() )
          {
              ui->transformObjectComboBox->addItem( sourceImageObject->GetName(), QVariant( sourceId ) );
          }
          if(sourceImageObject->GetParent() )
          {
              if(sourceImageObject->GetParent()->CanEditTransformManually())
                  ui->transformObjectComboBox->addItem( sourceImageObject->GetParent()->GetName(), QVariant( sourceImageObject->GetParent()->GetObjectID() ) );
          }
      }


      if( ui->transformObjectComboBox->count() == 0 )
        {
          ui->transformObjectComboBox->addItem( "None", QVariant(-1) );
        }
      // else
      //   {
      //     int transformId = ui->transformObjectComboBox->itemData( ui->transformObjectComboBox->currentIndex() ).toInt();
      //     SceneObject * transformObject = SceneObject::SafeDownCast( sm->GetObjectByID( transformId ) );          
      //     for(unsigned int i=0; i < transformObject->GetNumberOfChildren(); i++)
      //       {
      //         if( transformObject->GetChild(i)->inherits("PointsObject") )
      //           {
      //             ui->mriTagsComboBox->addItem( transformObject->GetChild(i)->GetName(), QVariant( transformObject->GetChild(i)->GetObjectID() ) );
      //           }
      //       }
      //   }
      // if( ui->mriTagsComboBox->count() == 0 )
      //     ui->mriTagsComboBox->addItem( "None", QVariant(-1) );
  }

}

void GPU_RigidRegistrationWidget::on_sourceImageComboBox_activated(int index)
{  
  int sourceId = ui->sourceImageComboBox->itemData( ui->sourceImageComboBox->currentIndex() ).toInt();
  if( m_application && sourceId != -1)
  {
      ui->transformObjectComboBox->clear();    
      SceneManager * sm = m_application->GetSceneManager();
      ImageObject * sourceImageObject = ImageObject::SafeDownCast( sm->GetObjectByID( sourceId ) );
      ui->transformObjectComboBox->addItem( sourceImageObject->GetName(), QVariant( sourceId ) );
      if(sourceImageObject->GetParent() )
      {
          if(sourceImageObject->GetParent()->CanEditTransformManually())
              ui->transformObjectComboBox->addItem( sourceImageObject->GetParent()->GetName(), QVariant( sourceImageObject->GetParent()->GetObjectID() ) );
      }

  }
}

//void GPU_RigidRegistrationWidget::on_targetImageComboBox_activated(int index)
//{
//    ui->usTagsComboBox->clear();
//    int targetId = ui->targetImageComboBox->itemData( ui->targetImageComboBox->currentIndex() ).toInt();
////    if( m_application && targetId != -1 )
////    {
////        SceneManager * sm = m_application->GetSceneManager();
////        SceneObject * targetParentObject = SceneObject::SafeDownCast( sm->GetObjectByID( targetId )->GetParent() );
////        for(unsigned int i=0; i < targetParentObject->GetNumberOfChildren(); i++)
////        {
////            if( targetParentObject->GetChild(i)->inherits("PointsObject") )
////            {
////                ui->usTagsComboBox->addItem( targetParentObject->GetChild(i)->GetName(), QVariant( targetParentObject->GetChild(i)->GetObjectID() ) );
////            }
////        }

////    }
////    if( ui->usTagsComboBox->count() == 0 )
////        ui->usTagsComboBox->addItem( "None", QVariant(-1) );

//}

//void GPU_RigidRegistrationWidget::on_transformObjectComboBox_activated(int index)
//{
////    ui->mriTagsComboBox->clear();
////    int transformId = ui->transformObjectComboBox->itemData( ui->transformObjectComboBox->currentIndex() ).toInt();
////    if( m_application && transformId != -1)
////    {
////        SceneManager * sm = m_application->GetSceneManager();
////        SceneObject * transformObject = SceneObject::SafeDownCast( sm->GetObjectByID( transformId ) );
////        for(unsigned int i=0; i < transformObject->GetNumberOfChildren(); i++)
////        {
////            if( transformObject->GetChild(i)->inherits("PointsObject") )
////            {
////                ui->mriTagsComboBox->addItem( transformObject->GetChild(i)->GetName(), QVariant( transformObject->GetChild(i)->GetObjectID() ) );
////            }
////        }
////    }
////    if( ui->mriTagsComboBox->count() == 0 )
////        ui->mriTagsComboBox->addItem( "None", QVariant(-1) );
//}


//void GPU_RigidRegistrationWidget::on_mriTagsComboBox_activated(int index)
//{
//  this->updateTagsDistance();
//}

//void GPU_RigidRegistrationWidget::on_usTagsComboBox_activated(int index)
//{
//  this->updateTagsDistance();
//}

//void GPU_RigidRegistrationWidget::updateTagsDistance()
//{
//  int usTagsId = ui->usTagsComboBox->itemData( ui->usTagsComboBox->currentIndex() ).toInt();
//  int mriTagsId = ui->mriTagsComboBox->itemData( ui->mriTagsComboBox->currentIndex() ).toInt();

//  if( usTagsId == -1 || mriTagsId == -1 )
//  {
//     ui->tagsDistanceValue->setText("N/A");
//     return;
//  }

//  if( m_application )
//  {
//      SceneManager * sm = m_application->GetSceneManager();
//      PointsObject * usTagsObject = PointsObject::SafeDownCast( sm->GetObjectByID( usTagsId ) );
//      PointsObject * mriTagsObject = PointsObject::SafeDownCast( sm->GetObjectByID( mriTagsId ) );

//      vtkTransform * vtktransform = vtkTransform::SafeDownCast( mriTagsObject->GetWorldTransform() );

//      ui->tagsDistanceValue->setText("..under construction");

//      double meanDistance = 0;
//      for(unsigned int n=0; n < usTagsObject->GetNumberOfPoints(); n++)
//        {
//          double usPoint[3];
//          usTagsObject->GetPoint(n)->GetPosition(usPoint);

//          double mriPoint[3];
//          mriTagsObject->GetPoint(n)->GetPosition(mriPoint);

//          double mriTransPoint[3];
//          vtktransform->TransformPoint(mriPoint, mriTransPoint);

//          double distance = 0;
//          distance += pow(usPoint[0]-mriTransPoint[0],2.0);
//          distance += pow(usPoint[1]-mriTransPoint[1],2.0);
//          distance += pow(usPoint[2]-mriTransPoint[2],2.0);
//          distance = sqrt(distance);

//          meanDistance += distance;
//        }
//      meanDistance /= usTagsObject->GetNumberOfPoints();

//      ui->tagsDistanceValue->setText(QString::number(meanDistance));
//  }

//}



void GPU_RigidRegistrationWidget::on_debugCheckBox_clicked()
{
    if(ui->debugCheckBox->isChecked())
      ui->registrationOutputTextEdit->show();
    else
      ui->registrationOutputTextEdit->hide();
}
