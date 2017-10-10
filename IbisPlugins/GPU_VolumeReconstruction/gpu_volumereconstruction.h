#ifndef GPU_VOLUMERECONSTRUCTION_H
#define GPU_VOLUMERECONSTRUCTION_H

#include <QThread>
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

class GPU_VolumeReconstruction : public QThread, public vtkObject
{
    Q_OBJECT

public:

    static GPU_VolumeReconstruction * New() { return new GPU_VolumeReconstruction; }

    GPU_VolumeReconstruction();
    ~GPU_VolumeReconstruction();
    vtkTypeMacro( GPU_VolumeReconstruction,vtkObject )

    void CreateReconstructor();
    void DestroyReconstructor();

    IbisItkFloat3ImageType::Pointer GetReconstructedImage() { return m_reconstructedImage; }
    void SetNumberOfSlices( unsigned int nbrOfSlices );
    void SetFixedSliceMask( IbisItkFloat3ImageType::Pointer itkSliceMask );
    void SetUSSearchRadius( unsigned int usSearchRadius );
    void SetVolumeSpacing( float usVolumeSpacing );
    void SetKernelStdDev( float stdDev );
    void SetFixedSlice( unsigned int index, IbisItkFloat3ImageType::Pointer itkSliceImage );
    void SetTransform( ItkRigidTransformType::Pointer itkTransform );

protected:

    void run();
    VolumeReconstructionPointer m_VolReconstructor;
    IbisItkFloat3ImageType::Pointer m_reconstructedImage;
};

#endif // GPU_VOLUMERECONSTRUCTION_H
