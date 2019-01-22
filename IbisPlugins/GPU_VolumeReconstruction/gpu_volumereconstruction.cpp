#include "gpu_volumereconstruction.h"
#include "ibisitkvtkconverter.h"
#include "vtkMatrix4x4.h"

#include "vtkImageData.h"

GPU_VolumeReconstruction::GPU_VolumeReconstruction()
{
    m_VolReconstructor = GPU_VolumeReconstruction::VolumeReconstructionType::New();
    m_VolReconstructor->SetDebug( false );
}

GPU_VolumeReconstruction::~GPU_VolumeReconstruction()
{
}

void GPU_VolumeReconstruction::SetNumberOfSlices( unsigned int nbrOfSlices )
{
    m_VolReconstructor->SetNumberOfSlices( nbrOfSlices );
}

void GPU_VolumeReconstruction::SetFixedSliceMask( vtkImageData *mask )
{
    IbisItkFloat3ImageType::Pointer itkSliceMask = IbisItkFloat3ImageType::New();
    vtkSmartPointer<IbisItkVtkConverter> converter = vtkSmartPointer<IbisItkVtkConverter>::New();
    vtkSmartPointer<vtkMatrix4x4> sliceMaskMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    converter->ConvertVtkImageToItkImage( itkSliceMask, mask, sliceMaskMatrix );
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

void GPU_VolumeReconstruction::SetFixedSlice( int index, vtkImageData *slice, vtkMatrix4x4 *sliceTransformMatrix )
{
    IbisItkFloat3ImageType::Pointer itkSliceImage = IbisItkFloat3ImageType::New();
    itkSliceImage->Initialize();
    vtkSmartPointer<IbisItkVtkConverter> converter = vtkSmartPointer<IbisItkVtkConverter>::New();
    converter->ConvertVtkImageToItkImage( itkSliceImage, slice, sliceTransformMatrix );
    m_VolReconstructor->SetFixedSlice( index, itkSliceImage );
}

void GPU_VolumeReconstruction::SetTransform( vtkMatrix4x4 *transformMatrix )
{
    //First convert to itk
    GPU_VolumeReconstruction::ItkRigidTransformType::Pointer itkTransform = GPU_VolumeReconstruction::ItkRigidTransformType::New();
    GPU_VolumeReconstruction::ItkRigidTransformType::OffsetType offset;
    vnl_matrix<double> M(3,3);
    for(unsigned int i=0; i<3; i++ )
     {
     for(unsigned int j=0; j<3; j++ )
       {
        M[i][j] = transformMatrix->GetElement(i,j);
       }
      offset[i] = transformMatrix->GetElement(i,3);
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

    GPU_VolumeReconstruction::ItkRigidTransformType::ParametersType params = GPU_VolumeReconstruction::ItkRigidTransformType::ParametersType(6);
    params[0] = angleX; params[1] = angleY; params[2] = angleZ;

    GPU_VolumeReconstruction::ItkRigidTransformType::CenterType center;
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
    // set transform in reconstructor
    m_VolReconstructor->SetTransform( itkTransform );
}

void GPU_VolumeReconstruction::run()
{
    m_VolReconstructor->ReconstructVolume();
    m_reconstructedImage = m_VolReconstructor->GetReconstructedVolume();
}

void GPU_VolumeReconstruction::SetDebugFlag( bool debug )
{
    m_VolReconstructor->SetDebug( debug );
}
