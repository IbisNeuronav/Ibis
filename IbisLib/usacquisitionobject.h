/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef USACQUISITIONOBJECT_H
#define USACQUISITIONOBJECT_H

#include <itkImage.h>
#include <stdio.h>

#include <QList>
#include <QObject>
#include <QString>
#include <QVector>
#include <QWidget>

#include "imageobject.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "serializer.h"
#include "usprobeobject.h"

class TrackedVideoBuffer;
class vtkImageData;
class vtkActor;
class vtkImageActor;
class vtkImageMapToColors;
class vtkImageProperty;
class vtkAlgorithmOutput;
class vtkImageStencil;
class vtkImageMapToColors;
class vtkImageToImageStencil;
class vtkPiecewiseFunctionLookupTable;
class USMask;
class vtkImageConstantPad;
class vtkPassThrough;

#define ACQ_COLOR_RGB "RGB"
#define ACQ_COLOR_GRAYSCALE "Grayscale"
#define ACQ_BASE_DIR "acquisitions"
#define ACQ_ACQUISITION_PREFIX "acq"

struct ExportParams
{
    ExportParams()
        : outputDir( "" ), masked( false ), useCalibratedTransform( false ), relativeToID( SceneManager::InvalidId )
    {
    }
    QString outputDir;
    bool masked;
    bool useCalibratedTransform;
    int relativeToID;
};

class USAcquisitionObject : public SceneObject
{
    Q_OBJECT

public:
    static USAcquisitionObject * New() { return new USAcquisitionObject; }
    vtkTypeMacro( USAcquisitionObject, SceneObject );
    USAcquisitionObject();
    virtual ~USAcquisitionObject();

    void SetUsProbe( UsProbeObject * probe );

    virtual void Serialize( Serializer * serializer ) override;
    virtual void Export() override;
    virtual bool IsExportable() override { return true; }

    bool Import();
    void SetBaseDirectory( QString dir ) { m_baseDirectory = dir; }
    QString GetBaseDirectory() { return m_baseDirectory; }
    void ExportTrackedVideoBuffer( QString destDir = "", bool masked = false, bool useCalibratedTransform = false,
                                   int relativeToID = SceneManager::InvalidId );
    bool LoadFramesFromMINCFile( Serializer * ser );

    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override;
    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;

    bool IsUsingMask() { return m_isMaskOn; }
    void SetUseMask( bool useMask );
    bool IsUsingDoppler() { return m_isDopplerOn; }
    void SetUseDoppler( bool useDoppler );

    void SetCalibrationMatrix( vtkMatrix4x4 * mat );
    vtkTransform * GetCalibrationTransform();
    bool IsCalibratioMatrixApplied() { return m_useCalibratedTransform; }

    vtkImageData * GetVideoOutput();
    vtkTransform * GetTransform();

    void SetFrameAndMaskSize( int width, int height );

    // Return frame data
    void GetFrameData( int index, vtkImageData * img, vtkMatrix4x4 * mat );
    double GetFrameTimestamp( int index );
    double GetCurrentFrameTimestamp();

    // Return itk image of a given frame
    void GetItkImage( IbisItkUnsignedChar3ImageType::Pointer itkOutputImage, int frameNo, bool masked,
                      bool useCalibratedTransform = false, int relativeToObjectID = SceneManager::InvalidId );
    void GetItkRGBImage( IbisRGBImageType::Pointer itkOutputImage, int frameNo, bool masked,
                         bool useCalibratedTransform = false, int relativeToObjectID = SceneManager::InvalidId );

    // Display of current slice
    int GetSliceWidth();
    int GetSliceHeight();
    int GetNumberOfSlices( void );
    int GetCurrentSlice();
    void SetSliceImageOpacity( double opacity );
    double GetSliceImageOpacity();
    int GetSliceLutIndex() { return m_sliceLutIndex; }
    void SetSliceLutIndex( int index );

    // Display of static slices
    void SetEnableStaticSlices( bool enable );
    bool IsStaticSlicesEnabled() { return m_staticSlicesEnabled; }
    void SetNumberOfStaticSlices( int nb );
    int GetNumberOfStaticSlices() { return m_numberOfStaticSlices; }
    void SetStaticSlicesOpacity( double opacity );
    double GetStaticSlicesOpacity();
    int GetStaticSlicesLutIndex() { return m_staticSlicesLutIndex; }
    void SetStaticSlicesLutIndex( int index );

    int GetMaxNumberOfSlices();
    vtkImageData * GetMask();

    bool IsRecording() { return m_isRecording; }

    void Record();
    void Stop();
    void SetCurrentFrame( int frameIndex );
    bool AddFrame( vtkImageData *, vtkMatrix4x4 *, double );

    void Clear();

    UsProbeObject::ACQ_TYPE GetAcquisitionType() { return m_acquisitionType; }
    QString GetAcquisitionTypeAsString();
    QString GetAcquisitionColor();
    QString GetUsDepth() { return m_usDepth; }

    // Outputs
    vtkAlgorithmOutput * GetMaskedOutputPort();
    vtkAlgorithmOutput * GetUnmaskedOutputPort();

private slots:

    void Updated();
    void UpdateMask();

protected:
    virtual void Hide() override;
    virtual void Show() override;
    void ObjectAddedToScene() override;
    void UpdatePipeline();
    bool m_isRecording;
    QString m_baseDirectory;

    // Acquisition properties
    QString m_usDepth;
    UsProbeObject::ACQ_TYPE m_acquisitionType;
    int m_usProbeObjectId;  // probe we record from

    // Images and matrices
    int m_defaultImageSize[2];
    TrackedVideoBuffer * m_videoBuffer;

    // Importing
    int m_componentsNumber;
    bool m_useCalibratedTransform;
    bool LoadFramesFromMINCFile( QStringList & allMINCFiles );
    bool LoadGrayFrames( QStringList & allMINCFiles );
    bool LoadRGBFrames( QStringList & allMINCFiles );
    void AdjustFrame( vtkImageData * frame, vtkMatrix4x4 * inputMatrix, vtkMatrix4x4 * outputMatrix );

    // 3D viewing data
    struct PerViewElements
    {
        PerViewElements() : imageSlice( 0 ) {}
        vtkImageActor * imageSlice;
        std::vector<vtkImageActor *> staticSlices;
    };
    typedef std::map<View *, PerViewElements> PerViewContainer;
    PerViewContainer m_perViews;

    // Current slice properties
    USMask * m_mask;
    vtkSmartPointer<vtkTransform> m_currentImageTransform;
    vtkSmartPointer<vtkTransform> m_sliceTransform;
    vtkSmartPointer<vtkTransform> m_calibrationTransform;
    int m_sliceLutIndex;
    vtkSmartPointer<vtkImageProperty> m_sliceProperties;
    bool m_isMaskOn;
    bool m_isDopplerOn;
    vtkSmartPointer<vtkImageToImageStencil> m_imageStencilSource;
    vtkSmartPointer<vtkImageStencil> m_sliceStencil;
    vtkSmartPointer<vtkImageStencil> m_sliceStencilDoppler;
    vtkSmartPointer<vtkImageMapToColors> m_mapToColors;
    vtkSmartPointer<vtkPiecewiseFunctionLookupTable> m_lut;
    vtkSmartPointer<vtkImageConstantPad> m_constantPad;

    // Outputs
    vtkSmartPointer<vtkPassThrough> m_maskedImageOutput;
    vtkSmartPointer<vtkPassThrough> m_unmaskedImageOutput;

    // Static slices properties
    void SetupAllStaticSlicesInAllViews();
    void SetupAllStaticSlices( View * view, PerViewElements & perView );
    void ReleaseAllStaticSlicesInAllViews();
    void ReleaseAllStaticSlices( View * view, PerViewElements & perView );
    void HideStaticSlices( PerViewElements & perView );
    void ShowStaticSlices( PerViewElements & perView );
    bool m_staticSlicesEnabled;
    int m_numberOfStaticSlices;
    int m_staticSlicesLutIndex;
    vtkSmartPointer<vtkImageProperty> m_staticSlicesProperties;

    // Static slices view-independent data
    void ComputeAllStaticSlicesData();
    void ComputeOneStaticSliceData( int sliceIndex );
    void ClearStaticSlicesData();
    struct PerStaticSlice
    {
        PerStaticSlice() : mapToColors( 0 ), imageStencil( 0 ), transform( 0 ) {}
        vtkImageMapToColors * mapToColors;
        vtkImageStencil * imageStencil;
        vtkTransform * transform;
    };
    std::vector<PerStaticSlice> m_staticSlicesData;
    bool m_staticSlicesDataNeedUpdate;

    void Save();
};

ObjectSerializationHeaderMacro( USAcquisitionObject );

#endif
