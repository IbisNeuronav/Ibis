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
#include "gpu_rigidregistrationplugininterface.h"
#include <QComboBox>
#include <QMessageBox>
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include "vnl/algo/vnl_real_eigensystem.h"
#include "vtkSmartPointer.h"
#include "ibisapi.h"

GPU_RigidRegistrationWidget::GPU_RigidRegistrationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GPU_RigidRegistrationWidget),
    m_pluginInterface(0),
    m_OptimizationRunning(false)
{
    ui->setupUi(this);
    setWindowTitle( "Rigid Registration With GPU" );
    connect(ui->numebrOfPixelsDial , SIGNAL(valueChanged(int)), ui->numberOfSamplesValueLabel, SLOT(setNum(int)));
    connect(ui->selectivityDial, SIGNAL(valueChanged(int)), ui->parameterNValueLabel, SLOT(setNum(int)));
    connect(ui->populationSizeDial, SIGNAL(valueChanged(int)), ui->populationSizeValueLabel, SLOT(setNum(int)));
    connect(ui->debugCheckBox, SIGNAL(stateChanged(int)), this, SLOT(on_debugCheckBox_clicked()));
    ui->registrationOutputTextEdit->hide();

    m_rigidRegistrator = new GPU_RigidRegistration();

}

GPU_RigidRegistrationWidget::~GPU_RigidRegistrationWidget()
{
    delete ui;
}

void GPU_RigidRegistrationWidget::SetPluginInterface( GPU_RigidRegistrationPluginInterface * ifc )
{
    m_pluginInterface = ifc;
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
    IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
    Q_ASSERT(ibisAPI);
    ImageObject * sourceImageObject = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( sourceImageObjectId ) );
    Q_ASSERT_X( sourceImageObject, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid source object" );
    vtkTransform * sourceVtkTransform = vtkTransform::SafeDownCast( sourceImageObject->GetWorldTransform() );

    ImageObject * targetImageObject = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( targetImageObjectId ) );
    Q_ASSERT_X( targetImageObject, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid target object" );
    vtkTransform * targetVtkTransform = vtkTransform::SafeDownCast( targetImageObject->GetWorldTransform() );

    SceneObject * transformObject = ibisAPI->GetObjectByID( transformObjectId );
    vtkTransform * vtktransform = vtkTransform::SafeDownCast( transformObject->GetLocalTransform() );
    Q_ASSERT_X( vtktransform, "GPU_RigidRegistrationWidget::on_startButton_clicked()", "Invalid transform" );

    IbisItkFloat3ImageType::Pointer itkSourceImage = sourceImageObject->GetItkImage();
    IbisItkFloat3ImageType::Pointer itkTargetImage = targetImageObject->GetItkImage();

    ui->userFeedbackLabel->setText(QString("Processing..(patience is a virtue)"));
    /*
      Send std::cout from elastix to QTextEdit.
      Issue. QTextEdit doesn't sync properly, and we only get a text dump
      after the registration is finished.
    */
    bool debug = ui->debugCheckBox->isChecked();

    std::stringstream debugStringStream;

    QDebugStream qout(debugStringStream,  ui->registrationOutputTextEdit);

    //debugStringStream << "output to debugStringStream" << std::endl;
    //std::streambuf * buff = debugStringStream.rdbuf();
    //std::stringstream *teststream;
    ////teststream << buff;
    //teststream = &debugStringStream;
    //*teststream << "output to string stream 2" << std::endl;

    m_registrationTimer.start();

    // Initialize parameters
    m_rigidRegistrator->SetNumberOfPixels( ui->numebrOfPixelsDial->value() );
    m_rigidRegistrator->SetOrientationSelectivity( ui->selectivityDial->value() );
    m_rigidRegistrator->SetPopulationSize( ui->populationSizeDial->value() );
    m_rigidRegistrator->SetInitialSigma( ui->initialSigmaComboBox->itemData( ui->initialSigmaComboBox->currentIndex() ).toDouble() );
    m_rigidRegistrator->SetPercentile( ui->percentileComboBox->itemData( ui->percentileComboBox->currentIndex() ).toDouble() );
    m_rigidRegistrator->SetUseMask( ui->computeMaskCheckBox->isChecked() );
    m_rigidRegistrator->SetDebug( debug, &debugStringStream);

    // Set image inputs
    m_rigidRegistrator->SetItkSourceImage( itkSourceImage );
    m_rigidRegistrator->SetItkTargetImage( itkTargetImage );

    // Set transform inputs
    m_rigidRegistrator->SetVtkTransform( vtktransform );
    m_rigidRegistrator->SetSourceVtkTransform( sourceVtkTransform );
    m_rigidRegistrator->SetTargetVtkTransform( targetVtkTransform );

    if( transformObject->GetParent() )
    {
        vtkTransform * parentVtktransform = vtkTransform::SafeDownCast( transformObject->GetParent()->GetWorldTransform() );
        Q_ASSERT_X( parentVtktransform, "VertebraRegistrationWidget::AddImageToQueue()", "Invalid transform" );
        m_rigidRegistrator->SetParentVtkTransform(parentVtktransform);
    }

    // Run registration
//    transformObject->StartModifyingTransform();
    m_rigidRegistrator->runRegistration();
//    transformObject->FinishModifyingTransform();

    m_OptimizationRunning = true;

    qint64 registrationTime = m_registrationTimer.elapsed();


    QString feedbackString = QString("Full Registration finished in %1 secs").arg(qreal(registrationTime)/1000.0);
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

  IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
  Q_ASSERT(ibisAPI);
  const QList< SceneObject* > &allObjects = ibisAPI->GetAllObjects();
  for( int i = 0; i < allObjects.size(); ++i )
  {
      SceneObject * current = allObjects[i];
      if( current != ibisAPI->GetSceneRoot() && current->IsListable() && !current->IsManagedByTracker())
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


  if( ui->sourceImageComboBox->count() == 0 )
  {
      ui->sourceImageComboBox->addItem( "None", QVariant(-1) );
  }
  else
  {
      int sourceId = ui->sourceImageComboBox->itemData( ui->sourceImageComboBox->currentIndex() ).toInt();
      ImageObject * sourceImageObject = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( sourceId ) );

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
}

void GPU_RigidRegistrationWidget::on_sourceImageComboBox_activated(int index)
{  
  int sourceId = ui->sourceImageComboBox->itemData( ui->sourceImageComboBox->currentIndex() ).toInt();
  if( sourceId != -1)
  {
      ui->transformObjectComboBox->clear();    
      IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
      Q_ASSERT(ibisAPI);
      ImageObject * sourceImageObject = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( sourceId ) );
      ui->transformObjectComboBox->addItem( sourceImageObject->GetName(), QVariant( sourceId ) );
      if(sourceImageObject->GetParent() )
      {
          if(sourceImageObject->GetParent()->CanEditTransformManually())
              ui->transformObjectComboBox->addItem( sourceImageObject->GetParent()->GetName(), QVariant( sourceImageObject->GetParent()->GetObjectID() ) );
      }

  }
}

void GPU_RigidRegistrationWidget::on_debugCheckBox_clicked()
{
    if(ui->debugCheckBox->isChecked())
      ui->registrationOutputTextEdit->show();
    else
      ui->registrationOutputTextEdit->hide();
}
