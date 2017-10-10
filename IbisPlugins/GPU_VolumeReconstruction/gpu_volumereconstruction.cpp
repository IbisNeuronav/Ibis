#include <QApplication>
#include "gpu_volumereconstruction.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "usacquisitionobject.h"
#include "vtkTransform.h"
#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"

#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkImageLuminance.h"
#include "vtkMath.h"

#include "imageobject.h"


GPU_VolumeReconstruction::GPU_VolumeReconstruction()
{
}

GPU_VolumeReconstruction::~GPU_VolumeReconstruction()
{

}

void GPU_VolumeReconstruction::CreateReconstructor()
{
    m_VolReconstructor = VolumeReconstructionType::New();
}

void GPU_VolumeReconstruction::DestroyReconstructor()
{
    m_VolReconstructor = 0;
}

void GPU_VolumeReconstruction::SetNumberOfSlices( unsigned int nbrOfSlices )
{
    m_VolReconstructor->SetNumberOfSlices( nbrOfSlices );
}

void GPU_VolumeReconstruction::SetFixedSliceMask( IbisItkFloat3ImageType::Pointer itkSliceMask )
{
    m_VolReconstructor->SetFixedSliceMask( itkSliceMask );
}

void GPU_VolumeReconstruction::SetUSSearchRadius( unsigned int usSearchRadius )
{
    m_VolReconstructor->SetUSSearchRadius( usSearchRadius );
}

void GPU_VolumeReconstruction::SetVolumeSpacing( float usVolumeSpacing )
{
    m_VolReconstructor->SetVolumeSpacing( usVolumeSpacing );
}

void GPU_VolumeReconstruction::SetKernelStdDev( float stdDev )
{
    m_VolReconstructor->SetKernelStdDev( stdDev );
}

void GPU_VolumeReconstruction::SetFixedSlice( unsigned int index, IbisItkFloat3ImageType::Pointer itkSliceImage )
{
    m_VolReconstructor->SetFixedSlice( index, itkSliceImage );
}

void GPU_VolumeReconstruction::SetTransform( ItkRigidTransformType::Pointer itkTransform )
{
    m_VolReconstructor->SetTransform( itkTransform );
}

void GPU_VolumeReconstruction::run()
{
    m_VolReconstructor->ReconstructVolume();
    m_reconstructedImage = m_VolReconstructor->GetReconstructedVolume();
}
