#ifndef GPU_VOLUMERECONSTRUCTION_H
#define GPU_VOLUMERECONSTRUCTION_H

#include "vtkObject.h"
#include "imageobject.h"
#include "vtkSmartPointer.h"
#include "itkEuler3DTransform.h"
#include "itkGPUVolumeReconstruction.h"

class vtkImageData;
class vtkMatrix4x4;

typedef itk::Euler3DTransform<float>                ItkRigidTransformType;

typedef itk::GPUVolumeReconstruction<IbisItkFloat3ImageType>
                                                    VolumeReconstructionType;
typedef VolumeReconstructionType::Pointer           VolumeReconstructionPointer;

class GPU_VolumeReconstruction : public vtkObject
{

public:

    static GPU_VolumeReconstruction * New() { return new GPU_VolumeReconstruction; }

    GPU_VolumeReconstruction();
    ~GPU_VolumeReconstruction();
    vtkTypeMacro( GPU_VolumeReconstruction,vtkObject )

    VolumeReconstructionPointer GetReconstructor() { return m_VolReconstructor; }

    void ReconstructVolume();
    IbisItkFloat3ImageType::Pointer GetReconstructedImage() { return m_reconstructedImage; }
    void SetNumberOfSlices( unsigned int nbrOfSlices );
    void SetFixedSliceMask( vtkImageData *mask );
    void SetUSSearchRadius( unsigned int usSearchRadius );
    void SetVolumeSpacing( float usVolumeSpacing );
    void SetKernelStdDev( float stdDev );
    void SetFixedSlice( int index, vtkImageData *slice, vtkMatrix4x4 *sliceTransformMatrix );
    void SetTransform( vtkMatrix4x4 *transformMatrix );

protected:

    VolumeReconstructionPointer m_VolReconstructor;
    IbisItkFloat3ImageType::Pointer m_reconstructedImage;
};

#endif // GPU_VOLUMERECONSTRUCTION_H
