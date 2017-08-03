/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __USAcquisitionObject_h_
#define __USAcquisitionObject_h_

#include <stdio.h>
#include "serializer.h"
#include <QVector>
#include <QWidget>
#include <QObject>
#include <QString>
#include "sceneobject.h"
#include "scenemanager.h"
#include "imageobject.h"
#include "usprobeobject.h"
#include <itkImage.h>
#include "vtkSmartPointer.h"
#include "vtkImageActor.h"
#include "vtkImageMapToColors.h"
#include "vtkImageStencil.h"
#include "vtkTransform.h"

class TrackedVideoBuffer;
class vtkImageData;
class vtkImageProperty;
class vtkAlgorithmOutput;
class vtkImageToImageStencil;
class vtkPiecewiseFunctionLookupTable;
class USMask;
class vtkImageConstantPad;
class vtkPassThrough;

typedef itk::Image<float,3> IbisItk3DImageType;

#define ACQ_COLOR_RGB           "RGB"
#define ACQ_COLOR_GRAYSCALE     "Grayscale"
#define ACQ_BASE_DIR            "acquisitions"
#define ACQ_ACQUISITION_PREFIX  "acq"

struct ExportParams
{
    ExportParams() : outputDir(""), masked(false), useCalibratedTransform(false), relativeToID(SceneManager::InvalidId) {}
    QString outputDir;
    bool masked;
    bool useCalibratedTransform ;
    int relativeToID;
};

class USAcquisitionObject : public SceneObject
{
Q_OBJECT

public:
    
    static USAcquisitionObject * New() { return new USAcquisitionObject; }
    vtkTypeMacro(USAcquisitionObject,SceneObject);
    USAcquisitionObject();
    virtual ~USAcquisitionObject();

    void SetUsProbe( UsProbeObject * probe );

    virtual void Serialize( Serializer * serializer );
    virtual void Export();
    virtual bool IsExportable()  { return true; }

    bool Import();
    void    SetBaseDirectory(QString dir) {m_baseDirectory = dir;}
    QString GetBaseDirectory() {return m_baseDirectory;}
    void    ExportTrackedVideoBuffer(QString destDir = "", bool masked = false , bool useCalibratedTransform = false, int relativeToID = SceneManager::InvalidId );
    bool    LoadFramesFromMINCFile( QStringList & allMINCFiles );
    bool    LoadFramesFromMINCFile( Serializer * ser );

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);
    virtual void Setup( View * view );
    virtual void Release( View * view );


    bool IsUsingMask() { return m_isMaskOn; }
    void SetUseMask( bool useMask );
    bool IsUsingDoppler() { return m_isDopplerOn; }
    void SetUseDoppler( bool useDoppler );

    void SetCalibrationMatrix( vtkMatrix4x4 * mat );
    vtkTransform * GetCalibrationTransform();

    vtkImageData * GetVideoOutput();
    vtkTransform * GetTransform();

    void SetFrameAndMaskSize( int width, int height );

    // Return itk image of a given frame
    bool GetItkImage(IbisItk3DImageType::Pointer itkOutputImage, int frameNo, vtkMatrix4x4* sliceMatrix);
    void GetItkRGBImage(IbisRGBImageType::Pointer itkOutputImage, int frameNo, bool masked, bool useCalibratedTransform = false, vtkMatrix4x4 *relativeMatrix = 0 );

    // Display of current slice
    int GetSliceWidth();
    int GetSliceHeight();
    int  GetNumberOfSlices(void);
    int  GetCurrentSlice();
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

    vtkImageData *GetMask();

    bool IsRecording() { return m_isRecording; }

    void Record();
    void Stop();
    void SetCurrentFrame( int frameIndex );

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

    virtual void Hide();
    virtual void Show();
    void ObjectAddedToScene();
    void UpdatePipeline();
    bool m_isRecording;
    QString             m_baseDirectory;

    // Acquisition properties
    QString m_usDepth;
    UsProbeObject::ACQ_TYPE m_acquisitionType;
    int m_usProbeObjectId;  // probe we record from

    // Images and matrices
    int m_defaultImageSize[2];
    TrackedVideoBuffer * m_videoBuffer;

    // 3D viewing data
    struct PerViewElements
    {
        PerViewElements() : imageSlice(0) {}
        vtkSmartPointer<vtkImageActor> imageSlice;
        std::vector< vtkSmartPointer<vtkImageActor> > staticSlices;
    };
    typedef std::map<View*,PerViewElements> PerViewContainer;
    PerViewContainer m_perViews;

    // Current slice properties
    USMask                              * m_mask;
    vtkSmartPointer<vtkTransform>       m_currentImageTransform;
    vtkSmartPointer<vtkTransform>       m_sliceTransform;
    vtkSmartPointer<vtkTransform>       m_calibrationTransform;
    int                                 m_sliceLutIndex;
    vtkSmartPointer<vtkImageProperty>   m_sliceProperties;
    bool                                m_isMaskOn;
    bool                                m_isDopplerOn;
    vtkSmartPointer<vtkImageToImageStencil> m_imageStencilSource;
    vtkSmartPointer<vtkImageStencil>    m_sliceStencil;
    vtkSmartPointer<vtkImageStencil>    m_sliceStencilDoppler;
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
    bool               m_staticSlicesEnabled;
    int                m_numberOfStaticSlices;
    int                m_staticSlicesLutIndex;
    vtkSmartPointer<vtkImageProperty> m_staticSlicesProperties;

    // Static slices view-independent data
    void ComputeAllStaticSlicesData();
    void ComputeOneStaticSliceData( int sliceIndex );
    void ClearStaticSlicesData();
    struct PerStaticSlice
    {
        PerStaticSlice() : mapToColors(0), imageStencil(0), transform(0) {}
        vtkSmartPointer<vtkImageMapToColors> mapToColors;
        vtkSmartPointer<vtkImageStencil> imageStencil;
        vtkSmartPointer<vtkTransform> transform;
    };
    std::vector< PerStaticSlice > m_staticSlicesData;
    bool m_staticSlicesDataNeedUpdate;

    std::vector< IbisRGBImageType::Pointer > m_itkRGBImages;

    void Save( );
    void ConvertVtkImagesToItkRGBImages(bool masked = false, bool useCalibratedTransform = false, int relativeToID = SceneManager::InvalidId );
};

ObjectSerializationHeaderMacro( USAcquisitionObject );

#endif
