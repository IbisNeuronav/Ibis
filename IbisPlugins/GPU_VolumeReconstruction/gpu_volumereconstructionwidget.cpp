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

#include "gpu_volumereconstructionwidget.h"
#include <QComboBox>
#include <QMessageBox>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
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
    m_application(0)
{
    ui->setupUi(this);    
    setWindowTitle( "US Volume Reconstruction with GPU" );

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->hide();
    connect(&this->m_futureWatcher, SIGNAL(finished()), this, SLOT(slot_finished()));

    UpdateUi();
}

GPU_VolumeReconstructionWidget::~GPU_VolumeReconstructionWidget()
{
    delete ui;
}

void GPU_VolumeReconstructionWidget::SetApplication( Application * app )
{
    m_application = app;
    UpdateUi();
}


void GPU_VolumeReconstructionWidget::slot_finished()
{
    int usAcquisitionObjectId = ui->usAcquisitionComboBox->itemData( ui->usAcquisitionComboBox->currentIndex() ).toInt();

    SceneManager * sm = m_application->GetSceneManager();

    // Get US Acquisition Object
    USAcquisitionObject * selectedUSAcquisitionObject = USAcquisitionObject::SafeDownCast( sm->GetObjectByID( usAcquisitionObjectId) );
    Q_ASSERT_X( selectedUSAcquisitionObject, "GPU_VolumeReconstructionWidget::slot_finished()", "Invalid target US object" );

    qint64 reconstructionTime = m_ReconstructionTimer.elapsed();

    //Add Reconstructed Volume to Scene
    vtkSmartPointer<ImageObject> reconstructedImage = vtkSmartPointer<ImageObject>::New();
    reconstructedImage->SetItkImage( m_Reconstructor->GetReconstructedVolume() );
    reconstructedImage->SetName("Reconstructed Volume");
    //reconstructedImage->SetLocalTransform( selectedUSAcquisitionObject->GetLocalTransform() );

    sm->AddObject(reconstructedImage, selectedUSAcquisitionObject->GetParent()->GetParent() );
    sm->SetCurrentObject( reconstructedImage );

#ifdef DEBUG
    std::cerr << "Done..." << std::endl;
    std::cout << "Volume Reconstruction took " << qreal(reconstructionTime)/1000.0 << "secs"<< std::endl;
#endif

    QString feedbackString = QString("Volume Reconstruction finished in %1 secs").arg(qreal(reconstructionTime)/1000.0);
    ui->userFeedbackLabel->setText(feedbackString);

    ui->progressBar->hide();

    m_Reconstructor = 0; // Destroy Object.
}


#include "ibisitkvtkconverter.h"
void GPU_VolumeReconstructionWidget::on_startButton_clicked()
{
    // Make sure all params have been specified
    int usAcquisitionObjectId = ui->usAcquisitionComboBox->itemData( ui->usAcquisitionComboBox->currentIndex() ).toInt();

    if( usAcquisitionObjectId == -1 )
    {
        QMessageBox::information( this, "Volume Reconstruction With GPU", "Need to specify US Acqusition." );
        return;
    }

    SceneManager * sm = m_application->GetSceneManager();
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

    ui->progressBar->show();
    ui->progressBar->repaint();
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

    IbisItkFloat3ImageType::Pointer itkSliceMask = IbisItkFloat3ImageType::New();
    vtkSmartPointer<vtkMatrix4x4> sliceMaskMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<IbisItkVtkConverter> converter = vtkSmartPointer<IbisItkVtkConverter>::New();
    converter->ConvertVtkImageToItkImage( itkSliceMask, selectedUSAcquisitionObject->GetMask(), sliceMaskMatrix.GetPointer());

#ifdef DEBUG
    std::cerr << "Constructing m_Reconstructor..." << std::endl;
#endif
    m_Reconstructor = VolumeReconstructionType::New();
    m_Reconstructor->SetNumberOfSlices( nbrOfSlices );
    m_Reconstructor->SetFixedSliceMask( itkSliceMask );
    m_Reconstructor->SetUSSearchRadius( usSearchRadius );
    m_Reconstructor->SetVolumeSpacing( usVolumeSpacing );
    m_Reconstructor->SetKernelStdDev( usVolumeSpacing/2.0 );

#ifdef DEBUG
    std::cerr << "Constructing m_Reconstructor...DONE" << std::endl;
#endif

    IbisItkFloat3ImageType::Pointer itkSliceImage[nbrOfSlices];

    vtkSmartPointer<vtkMatrix4x4> sliceTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New() ;
    vtkSmartPointer<vtkImageData> slice = vtkSmartPointer<vtkImageData>::New();
    for(unsigned int i=0; i<nbrOfSlices; i++)
    {
      itkSliceImage[i] = IbisItkFloat3ImageType::New();
      selectedUSAcquisitionObject->GetFrameData( i, slice.GetPointer(), sliceTransformMatrix.GetPointer() );
      if( converter->ConvertVtkImageToItkImage( itkSliceImage[i], slice.GetPointer(), sliceTransformMatrix.GetPointer() ) )
          m_Reconstructor->SetFixedSlice(i, itkSliceImage[i]);
    }

     //Construct ITK Matrix corresponding to VTK Local Matrix
    vtkMatrix4x4 * localMatrix =  selectedUSAcquisitionObject->GetLocalTransform()->GetMatrix();
    ItkRigidTransformType::Pointer itkTransform = ItkRigidTransformType::New();
    ItkRigidTransformType::OffsetType offset;
    vnl_matrix<double> M(3,3);
    for(unsigned int i=0; i<3; i++ )
     {
     for(unsigned int j=0; j<3; j++ )
       {
        M[i][j] = localMatrix->GetElement(i,j);
       }
      offset[i] = localMatrix->GetElement(i,3);
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
    center.Fill(0.0);

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


    m_Reconstructor->SetTransform( itkTransform );
#ifdef DEBUG
    std::cerr << "Starting reconstruction..." << std::endl;
#endif   

    QFuture<void> future = QtConcurrent::run(m_Reconstructor.GetPointer(), &VolumeReconstructionType::ReconstructVolume);
    this->m_futureWatcher.setFuture(future);

}

void GPU_VolumeReconstructionWidget::UpdateUi()
{

  ui->usAcquisitionComboBox->clear();
  ui->usSearchRadiusComboBox->clear();
  ui->usVolumeSpacingComboBox->clear();
  if( m_application )
    {
      SceneManager * sm = m_application->GetSceneManager();
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

}
