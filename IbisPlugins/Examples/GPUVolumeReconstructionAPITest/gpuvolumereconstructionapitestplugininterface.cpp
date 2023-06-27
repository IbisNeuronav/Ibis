
#include "gpuvolumereconstructionapitestplugininterface.h"

#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

#include <QMessageBox>
#include <QtPlugin>

#include "gpu_volumereconstruction.h"
#include "gpu_volumereconstructionplugininterface.h"
#include "ibisapi.h"
#include "imageobject.h"
#include "usacquisitionobject.h"

GPUVolumeReconstructionAPITestPluginInterface::GPUVolumeReconstructionAPITestPluginInterface() {}

GPUVolumeReconstructionAPITestPluginInterface::~GPUVolumeReconstructionAPITestPluginInterface() {}

bool GPUVolumeReconstructionAPITestPluginInterface::CanRun() { return true; }

QWidget * GPUVolumeReconstructionAPITestPluginInterface::CreateFloatingWidget()
{
    IbisAPI * ibisAPI = this->GetIbisAPI();
    Q_ASSERT( ibisAPI );
    ToolPluginInterface * toolPlugin = ibisAPI->GetToolPluginByName( "GPU_VolumeReconstruction" );
    if( !toolPlugin ) return 0;
    GPU_VolumeReconstructionPluginInterface * volumeReconstructorPlugin =
        GPU_VolumeReconstructionPluginInterface::SafeDownCast( toolPlugin );
    Q_ASSERT( volumeReconstructorPlugin );
    QList<USAcquisitionObject *> acquisitions;
    ibisAPI->GetAllUSAcquisitionObjects( acquisitions );

    int numberOfAcquisitions = acquisitions.count();
    if( numberOfAcquisitions > 0 )
    {
        GPU_VolumeReconstruction * reconstructor = GPU_VolumeReconstruction::New();
        reconstructor->SetDebugFlag( false );
        for( int i = 0; i < numberOfAcquisitions; i++ )
        {
            USAcquisitionObject * acq = acquisitions[i];
            int nbrOfSlices           = acq->GetNumberOfSlices();
            reconstructor->SetNumberOfSlices( nbrOfSlices );
            reconstructor->SetFixedSliceMask( acq->GetMask() );
            reconstructor->SetUSSearchRadius( 3 );
            reconstructor->SetVolumeSpacing( 1.0 );
            reconstructor->SetKernelStdDev( 0.5 );
            vtkSmartPointer<vtkMatrix4x4> sliceTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
            vtkSmartPointer<vtkImageData> slice                = vtkSmartPointer<vtkImageData>::New();
            for( int j = 0; j < nbrOfSlices; j++ )
            {
                acq->GetFrameData( j, slice, sliceTransformMatrix );
                reconstructor->SetFixedSlice( j, slice, sliceTransformMatrix );
            }

            // Construct ITK Matrix corresponding to VTK Local Matrix
            reconstructor->SetTransform( acq->GetLocalTransform()->GetMatrix() );
            reconstructor->start();
            reconstructor->wait();

            vtkSmartPointer<ImageObject> reconstructedImage = vtkSmartPointer<ImageObject>::New();
            if( reconstructedImage->SetItkImage( reconstructor->GetReconstructedImage() ) )
            {
                QString volName( "ReconstructedVolume" );
                volName.append( QString::number( i ) );
                reconstructedImage->SetName( volName );
                ibisAPI->AddObject( reconstructedImage, acq->GetParent()->GetParent() );
                ibisAPI->SetCurrentObject( reconstructedImage );
            }
            else
                QMessageBox::warning( 0, "Error", "Reconstruction failed." );
        }
        reconstructor->Delete();
        return 0;
    }
    QMessageBox::warning( 0, "Error", "No acquisition in scene." );
    return 0;
}
