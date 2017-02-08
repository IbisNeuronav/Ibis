/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_IMAGEOBJECT_H
#define TAG_IMAGEOBJECT_H

#include "sceneobject.h"
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"
#include "serializer.h"
#include <map>
#include <QVector>

#include "itkVTKImageExport.h"
#include <itkImageRegionIterator.h>
#include <itkImage.h>
#include "itkRGBPixel.h"

class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkOutlineFilter;
class vtkActor;
class vtkTransform;
class vtkGPUVolumeRayCastMapper;
class vtkIbisGLSLVolumeRaycastMapper;
class vtkVolume;
class vtkImageImport;
class vtkBoxWidget2;
class vtkScalarsToColors;
class vtkImageAccumulate;
class vtkVolumeProperty;
class vtkImageData;

typedef itk::RGBPixel< unsigned char > RGBPixelType;
typedef itk::Image< RGBPixelType, 3 > IbisRGBImageType;
typedef itk::Image<float,3> IbisItk3DImageType;
typedef itk::Image < unsigned char,3 > IbisItk3DLabelType;
typedef itk::ImageRegionIterator< IbisItk3DImageType > IbisItk3DImageIteratorType;

// Reimplement origin callback from itk::VTKImageExport to take dir cosines into account correctly
template< class TInputImage >
class IbisItkVTKImageExport : public itk::VTKImageExport< TInputImage >
{
public:
    typedef IbisItkVTKImageExport<TInputImage>   Self;
    typedef itk::SmartPointer< Self >            Pointer;
    typedef TInputImage InputImageType;
    itkNewMacro(Self);
protected:
    typedef typename InputImageType::Pointer    InputImagePointer;
    IbisItkVTKImageExport();
    double * OriginCallback();
    double vtkOrigin[3];
};
typedef IbisItkVTKImageExport< IbisItk3DImageType > ItkExporterType;
typedef IbisItkVTKImageExport< IbisRGBImageType > ItkRGBImageExporterType;
typedef IbisItkVTKImageExport< IbisItk3DLabelType > ItkLabelExporterType;

class ImageObject : public SceneObject
{
    
Q_OBJECT

public:
        
    static ImageObject * New() { return new ImageObject; }
    vtkTypeMacro(ImageObject,SceneObject);
    
    ImageObject();
    virtual ~ImageObject();
    
    virtual void Serialize( Serializer * ser );
    virtual void Export();
    virtual bool IsExportable()  { return true; }
    void SaveImageData(QString &name);

    bool IsLabelImage();
    
    vtkImageData* GetImage( );
    void SetItkImage( IbisItk3DImageType::Pointer image );  // for all others
    void SetItkImage( IbisRGBImageType::Pointer image );  // for RGB images
    void SetItkLabelImage( IbisItk3DLabelType::Pointer image );  // for labels
    IbisItk3DImageType::Pointer GetItkImage() { return this->ItkImage; }
    IbisRGBImageType::Pointer GetItkRGBImage() { return this->ItkRGBImage; }
    IbisItk3DLabelType::Pointer GetItkLabelImage() { return this->ItkLabelImage; }
    void SetImage( vtkImageData * image );
    void ForceUpdatePixels();
    
    // Implementation of parent virtual method
    virtual void ObjectAddedToScene();
    virtual void Setup( View * view );
    virtual void Release( View * view );
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

    void SetViewOutline( int isOn );
    int GetViewOutline();

	// Choose from the set of lookup table templates available from the SceneManager
    int ChooseColorTable(int index);
    int GetLutIndex() {return lutIndex;}
    vtkScalarsToColors * GetLut();
    double * GetLutRange();
    void SetLutRange( double r[2] );
    void GetImageScalarRange(double *range);
    int GetNumberOfScalarComponents();
    void GetBounds( double bounds[6] );
    void GetCenter( double center[3] );
    double * GetSpacing();
    void SetIntensityFactor( double factor );
    double GetIntensityFactor();

    virtual void ShowMincInfo( );

    vtkImageAccumulate * GetHistogramComputer();

    // vtk volume rendering
    void SetVtkVolumeRenderingEnabled( bool on );
    bool GetVtkVolumeRenderingEnabled() { return m_vtkVolumeRenderingEnabled; }
    vtkVolumeProperty * GetVolumeProperty();
    void SetVolumeRenderingWindow( double window );
    double GetVolumeRenderingWindow() { return m_colorWindow; }
    void SetVolumeRenderingLevel( double level );
    double GetVolumeRenderingLevel() { return m_colorLevel; }
    void SetAutoSampleDistance( bool on );
    bool GetAutoSampleDistance() { return m_autoSampleDistance; }
    void SetSampleDistance( double dist );
    double GetSampleDistance() { return m_sampleDistance; }
    bool IsShowingVolumeClippingWidget() { return m_showVolumeClippingBox; }
    void SetShowVolumeClippingWidget( bool show );

signals:

    void LutChanged( int );
    void VisibilityChanged( int );

protected slots:

    void OnVolumeClippingBoxModified( vtkObject * caller );

protected:
    
    virtual void Hide();
    virtual void Show();
    void SetupInCutPlanes();
    void Setup3DRepresentation( View * view );
    void Release3DRepresentation( View * view );
    void Setup2DRepresentation( int i, View * view );
    void Release2DRepresentation( int i, View * view );

	// Lookup table management
    void SetLut(vtkScalarsToColors *lut);

    // Setup histogram properties after new image is set.
    void SetupHistogramComputer( );

    void BuildItkToVtkExport();
    void BuildItkRGBImageToVtkExport();
    void BuildItkToVtkLabelExport();
    void BuildVtkImport( itk::VTKImageExportBase * exporter );

    IbisItk3DImageType::Pointer ItkImage;
    ItkExporterType::Pointer ItkToVtkExporter;
    IbisRGBImageType::Pointer ItkRGBImage;
    ItkRGBImageExporterType::Pointer ItkRGBImageToVtkExporter;
    IbisItk3DLabelType::Pointer ItkLabelImage;
    ItkLabelExporterType::Pointer ItkToVtkLabelExporter;
    vtkSmartPointer<vtkImageImport> ItkToVtkImporter;

    vtkSmartPointer<vtkImageData> Image;
    vtkSmartPointer<vtkScalarsToColors> Lut;
    vtkSmartPointer<vtkOutlineFilter> OutlineFilter;
    static const int NumberOfBinsInHistogram;
    vtkSmartPointer<vtkImageAccumulate> HistogramComputer;
    
    int viewOutline;
    int outlineWasVisible;
    int lutIndex;
    double lutRange[2];
    double intensityFactor;

    // vtk volume rendering attributes
    void UpdateVolumeRenderingParamsInMapper();
    void SetVolumeClippingEnabled( vtkBoxWidget2 * widget, bool enabled );

    bool m_vtkVolumeRenderingEnabled;
    vtkSmartPointer<vtkVolumeProperty> m_volumeProperty;
    vtkSmartPointer<vtkEventQtSlotConnect> m_volumePropertyWatcher;
    double m_colorWindow;
    double m_colorLevel;
    bool m_autoSampleDistance;
    double m_sampleDistance;

    bool m_showVolumeClippingBox;
    double m_volumeRenderingBounds[6];
    vtkSmartPointer<vtkEventQtSlotConnect> m_volumeClippingBoxWatcher;
    
    struct PerViewElements
    {
        PerViewElements();
        ~PerViewElements();
        vtkSmartPointer<vtkActor> outlineActor;
        vtkSmartPointer<vtkVolume> volume;
        vtkSmartPointer<vtkBoxWidget2> volumeClippingWidget;
    };
    
    typedef std::map<View*,PerViewElements*> ImageObjectViewAssociation;
    ImageObjectViewAssociation imageObjectInstances;
};

ObjectSerializationHeaderMacro( ImageObject );

#endif //TAG_IMAGEOBJECT_H

