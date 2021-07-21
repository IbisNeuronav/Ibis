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
#include <vnl/algo/vnl_symmetric_eigensystem.h>
#include <vnl/algo/vnl_real_eigensystem.h>

#include "sceneobject.h"
#include "imageobject.h"
#include "usacquisitionobject.h"
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>

#include <itkImageFileWriter.h>
#include "ibisapi.h"

GPU_VolumeReconstructionWidget::GPU_VolumeReconstructionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GPU_VolumeReconstructionWidget),
    m_pluginInterface(nullptr)
{
    ui->setupUi(this);    
    setWindowTitle( "US Volume Reconstruction with GPU" );

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->hide();

    m_VolumeReconstructor = GPU_VolumeReconstruction::New();
    connect( m_VolumeReconstructor, SIGNAL(finished()), this, SLOT(slot_finished()) );
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

void GPU_VolumeReconstructionWidget::slot_finished()
{
    int usAcquisitionObjectId = ui->usAcquisitionComboBox->itemData( ui->usAcquisitionComboBox->currentIndex() ).toInt();

    IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
    Q_ASSERT(ibisAPI);

    // Get US Acquisition Object
    USAcquisitionObject * selectedUSAcquisitionObject = USAcquisitionObject::SafeDownCast( ibisAPI->GetObjectByID( usAcquisitionObjectId) );
    Q_ASSERT_X( selectedUSAcquisitionObject, "GPU_VolumeReconstructionWidget::slot_finished()", "Invalid target US object" );

    qint64 reconstructionTime = m_ReconstructionTimer.elapsed();

    //Add Reconstructed Volume to Scene
    vtkSmartPointer<ImageObject> reconstructedImage = vtkSmartPointer<ImageObject>::New();
    reconstructedImage->SetName("Reconstructed Volume");
    if( reconstructedImage->SetItkImage( m_VolumeReconstructor->GetReconstructedImage() ) )
    {
        ibisAPI->AddObject(reconstructedImage, selectedUSAcquisitionObject->GetParent()->GetParent() );
        ibisAPI->SetCurrentObject( reconstructedImage );

#ifdef DEBUG
    std::cerr << "Done..." << std::endl;
    std::cout << "Volume Reconstruction took " << qreal(reconstructionTime)/1000.0 << "secs"<< std::endl;
#endif

        QString feedbackString = QString("Volume Reconstruction finished in %1 secs").arg(qreal(reconstructionTime)/1000.0);
        ui->userFeedbackLabel->setText(feedbackString);
    }
    else
    {
        QString feedbackString = QString("Reconstruction failed.");
        ui->userFeedbackLabel->setText(feedbackString);
    }

    ui->progressBar->hide();
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

    IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
    Q_ASSERT(ibisAPI);
    // Get US Acquisition Object
    USAcquisitionObject * selectedUSAcquisitionObject = USAcquisitionObject::SafeDownCast( ibisAPI->GetObjectByID( usAcquisitionObjectId) );
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

#ifdef DEBUG
    std::cerr << "Constructing m_Reconstructor..." << std::endl;
#endif
    m_VolumeReconstructor->SetNumberOfSlices( nbrOfSlices );
    if (ui->useMaskCheckBox->isChecked())
	{
		m_VolumeReconstructor->SetFixedSliceMask(selectedUSAcquisitionObject->GetMask());
	}
    m_VolumeReconstructor->SetUSSearchRadius( usSearchRadius );
    m_VolumeReconstructor->SetVolumeSpacing( usVolumeSpacing );
    m_VolumeReconstructor->SetKernelStdDev( usVolumeSpacing/2.0 );

#ifdef DEBUG
    std::cerr << "Constructing m_Reconstructor...DONE" << std::endl;
#endif

    // Disable rendering while reconstructing
    ibisAPI->SetRenderingEnabled(false);

    vtkSmartPointer<vtkMatrix4x4> sliceTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New() ;
    vtkSmartPointer<vtkImageData> slice = vtkSmartPointer<vtkImageData>::New();
    for(unsigned int i=0; i<nbrOfSlices; i++)
    {
      selectedUSAcquisitionObject->GetFrameData( i, slice, sliceTransformMatrix );
      m_VolumeReconstructor->SetFixedSlice(i, slice, sliceTransformMatrix );
    }

     //Construct ITK Matrix corresponding to VTK Local Matrix
    m_VolumeReconstructor->SetTransform( selectedUSAcquisitionObject->GetLocalTransform()->GetMatrix() );
#ifdef DEBUG
    std::cerr << "Starting reconstruction..." << std::endl;
#endif   

    m_VolumeReconstructor->start();

    // re-enable rendering after reconstruction
    ibisAPI->SetRenderingEnabled(true);
}

void GPU_VolumeReconstructionWidget::UpdateUi()
{

  ui->usAcquisitionComboBox->clear();
  ui->usSearchRadiusComboBox->clear();
  ui->usVolumeSpacingComboBox->clear();
  IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
  Q_ASSERT(ibisAPI);
  const QList< SceneObject* > &allObjects = ibisAPI->GetAllObjects();
  for( int i = 0; i < allObjects.size(); ++i )
    {
      SceneObject * current = allObjects[i];
      if( current != ibisAPI->GetSceneRoot() && current->IsListable() && !current->IsManagedByTracker())
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
