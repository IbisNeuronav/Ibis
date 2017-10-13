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

#include "gpu_volumereconstructionplugininterface.h"
#include "gpu_volumereconstructionwidget.h"
#include <QComboBox>
#include <QMessageBox>
#include <QApplication>
#include <QProgressBar>
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include "vnl/algo/vnl_real_eigensystem.h"

#include "application.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "usacquisitionobject.h"
#include "vtkImageData.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"

#include "itkImageFileWriter.h"

GPU_VolumeReconstructionWidget::GPU_VolumeReconstructionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GPU_VolumeReconstructionWidget),
    m_pluginInterface(0)
{
    ui->setupUi(this);    
    setWindowTitle( "US Volume Reconstruction with GPU" );

    m_VolumeReconstructor = GPU_VolumeReconstruction::New();
}

void GPU_VolumeReconstructionWidget::SetPluginInterface( GPU_VolumeReconstructionPluginInterface *ifc )
{
    m_pluginInterface = ifc;
    UpdateUi();
}

GPU_VolumeReconstructionWidget::~GPU_VolumeReconstructionWidget()
{
    delete ui;
    m_VolumeReconstructor->Delete();
}

void GPU_VolumeReconstructionWidget::FinishReconstruction()
{
    int usAcquisitionObjectId = ui->usAcquisitionComboBox->itemData( ui->usAcquisitionComboBox->currentIndex() ).toInt();

    SceneManager * sm = m_pluginInterface->GetSceneManager();

    // Get US Acquisition Object
    USAcquisitionObject * selectedUSAcquisitionObject = USAcquisitionObject::SafeDownCast( sm->GetObjectByID( usAcquisitionObjectId) );
    Q_ASSERT_X( selectedUSAcquisitionObject, "GPU_VolumeReconstructionWidget::slot_finished()", "Invalid target US object" );

    qint64 reconstructionTime = m_ReconstructionTimer.elapsed();

    //Add Reconstructed Volume to Scene
    vtkSmartPointer<ImageObject> reconstructedImage = vtkSmartPointer<ImageObject>::New();
    reconstructedImage->SetItkImage( m_VolumeReconstructor->GetReconstructedImage() );
    reconstructedImage->SetName("Reconstructed Volume");

    sm->AddObject(reconstructedImage, selectedUSAcquisitionObject->GetParent()->GetParent() );
    sm->SetCurrentObject( reconstructedImage );

#ifdef DEBUG
    std::cerr << "Done..." << std::endl;
    std::cout << "Volume Reconstruction took " << qreal(reconstructionTime)/1000.0 << "secs"<< std::endl;
#endif

    QString feedbackString = QString("Volume Reconstruction finished in %1 secs").arg(qreal(reconstructionTime)/1000.0);
    ui->userFeedbackLabel->setText(feedbackString);

    m_VolumeReconstructor->DestroyReconstructor();
}

void GPU_VolumeReconstructionWidget::on_startButton_clicked()
{
    // Make sure all params have been specified
    int usAcquisitionObjectId = ui->usAcquisitionComboBox->itemData( ui->usAcquisitionComboBox->currentIndex() ).toInt();

    if( usAcquisitionObjectId == -1 )
    {
        QMessageBox::information( this, "Volume Reconstruction With GPU", "Need to specify US Acqusition." );
        return;
    }

    SceneManager * sm = m_pluginInterface->GetSceneManager();
    // Get US Acquisition Object
    USAcquisitionObject * selectedUSAcquisitionObject = USAcquisitionObject::SafeDownCast( sm->GetObjectByID( usAcquisitionObjectId) );
    Q_ASSERT_X( selectedUSAcquisitionObject, "GPU_VolumeReconstructionWidget::on_startButton_clicked()", "Invalid target US object" );

    unsigned int nbrOfSlices = selectedUSAcquisitionObject->GetNumberOfSlices();

#ifdef DEBUG
    std::cerr << "Number Of Valid Slices Found..." << nbrOfSlices << std::endl;
#endif

    if( nbrOfSlices == 0 )
    {
        QMessageBox::information( this, "Volume Reconstruction With GPU", "US Acqusition contains 0 slices." );
        return;
    }

    QProgressBar *progress = new QProgressBar(this);
    progress->setMaximum(0);
    progress->setMinimum(0);
    ui->verticalLayout->addWidget( progress );
    progress->show();
    ui->userFeedbackLabel->setText(QString("Processing..(patience is a virtue)"));
    ui->userFeedbackLabel->repaint();
    QApplication::flush();

    unsigned int usSearchRadius = ui->usSearchRadiusComboBox->itemData( ui->usSearchRadiusComboBox->currentIndex() ).toInt();
    float usVolumeSpacing = ui->usVolumeSpacingComboBox->itemData( ui->usVolumeSpacingComboBox->currentIndex() ).toFloat();

    m_ReconstructionTimer.start();

#ifdef DEBUG
    std::cerr << "Setting up volume reconstruction on GPU..." << std::endl;
    std::cerr << "US Acquisition Object with " << nbrOfSlices << " slices." << std::endl;
#endif    

#ifdef DEBUG
    std::cerr << "Constructing m_Reconstructor..." << std::endl;
#endif
    m_VolumeReconstructor->CreateReconstructor();
    m_VolumeReconstructor->SetNumberOfSlices( nbrOfSlices );
    m_VolumeReconstructor->SetFixedSliceMask( selectedUSAcquisitionObject->GetMask() );
    m_VolumeReconstructor->SetUSSearchRadius( usSearchRadius );
    m_VolumeReconstructor->SetVolumeSpacing( usVolumeSpacing );
    m_VolumeReconstructor->SetKernelStdDev( usVolumeSpacing/2.0 );

#ifdef DEBUG
    std::cerr << "Constructing m_Reconstructor...DONE" << std::endl;
#endif

    vtkSmartPointer<vtkMatrix4x4> sliceTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New() ;
    vtkSmartPointer<vtkImageData> slice = vtkSmartPointer<vtkImageData>::New();
    for(unsigned int i=0; i<nbrOfSlices; i++)
    {
      selectedUSAcquisitionObject->GetFrameData( i, slice.GetPointer(), sliceTransformMatrix.GetPointer() );
      m_VolumeReconstructor->SetFixedSlice(i, slice.GetPointer(), sliceTransformMatrix.GetPointer() );
    }

     //Construct ITK Matrix corresponding to VTK Local Matrix
    m_VolumeReconstructor->SetTransform( selectedUSAcquisitionObject->GetLocalTransform()->GetMatrix() );
#ifdef DEBUG
    std::cerr << "Starting reconstruction..." << std::endl;
#endif   

    m_VolumeReconstructor->start();
    m_VolumeReconstructor->wait();
    this->FinishReconstruction();
    delete progress;
}

void GPU_VolumeReconstructionWidget::UpdateUi()
{

  ui->usAcquisitionComboBox->clear();
  ui->usSearchRadiusComboBox->clear();
  ui->usVolumeSpacingComboBox->clear();
  SceneManager * sm = m_pluginInterface->GetSceneManager();
  const SceneManager::ObjectList & allObjects = sm->GetAllObjects();
  for( int i = 0; i < allObjects.size(); ++i )
    {
      SceneObject * current = allObjects[i];
      if( current != sm->GetSceneRoot() && current->IsListable() && !current->IsManagedByTracker())
        {
          if( current->IsA("USAcquisitionObject") )
            {
              USAcquisitionObject * currentUSAcquisitionObject = USAcquisitionObject::SafeDownCast( current );
              ui->usAcquisitionComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
            }
        }
    }

  if( ui->usAcquisitionComboBox->count() == 0 )
    {
      ui->usAcquisitionComboBox->addItem( "None", QVariant(-1) );
    }

  for(unsigned int i=0; i <=3 ; i++)
    {
      ui->usSearchRadiusComboBox->addItem( QString::number(i) , QVariant(i) );
    }

  ui->usVolumeSpacingComboBox->addItem( QString("1.0 mm x 1.0 mm x 1.0 mm"), QVariant( 1.0 ) );
  ui->usVolumeSpacingComboBox->addItem( QString("0.5 mm x 0.5 mm x 0.5 mm"), QVariant( 0.5 ) );
}
