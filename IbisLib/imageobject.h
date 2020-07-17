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
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>
#include "serializer.h"
#include <map>
#include <QVector>
#include <QObject>

#include "ibisitkvtkconverter.h"

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

class ImageObject : public SceneObject
{
    
Q_OBJECT

public:
        
    static ImageObject * New() { return new ImageObject; }
    vtkTypeMacro(ImageObject,SceneObject);

    ImageObject();
    virtual ~ImageObject();
    
    virtual void Serialize( Serializer * ser ) override;
    virtual void Export() override;
    virtual bool IsExportable()  override { return true; }
    void SaveImageData(QString &name);

    bool IsLabelImage();
    
    vtkImageData* GetImage( );
    bool SetItkImage( IbisItkFloat3ImageType::Pointer image );  // for all others
    bool SetItkLabelImage( IbisItkUnsignedChar3ImageType::Pointer image );  // for labels
    IbisItkFloat3ImageType::Pointer GetItkImage() { return this->ItkImage; }
    IbisItkUnsignedChar3ImageType::Pointer GetItkLabelImage() { return this->ItkLabelImage; }
    void SetImage( vtkImageData * image, vtkTransform * tr=0 );
    
    // Implementation of parent virtual method
    virtual void ObjectAddedToScene() override;
    virtual void Setup( View * view ) override;
    virtual void Release( View * view ) override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) override;

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
    
    virtual void Hide() override;
    virtual void Show() override;
    void SetupInCutPlanes();
    void Setup3DRepresentation( View * view );
    void Release3DRepresentation( View * view );
    void Setup2DRepresentation( int i, View * view );
    void Release2DRepresentation( int i, View * view );

    void SetInternalImage(vtkImageData *);

	// Lookup table management
    void SetLut(vtkSmartPointer<vtkScalarsToColors> lut);

    // Setup histogram properties after new image is set.
    void SetupHistogramComputer( );

    IbisItkVtkConverter *ItktovtkConverter;
    IbisItkFloat3ImageType::Pointer ItkImage;
    IbisItkUnsignedChar3ImageType::Pointer ItkLabelImage;

    vtkImageData* Image;
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

    bool SanityCheck( IbisItkFloat3ImageType::Pointer image );
    bool SanityCheck( IbisItkUnsignedChar3ImageType::Pointer image );
};

ObjectSerializationHeaderMacro( ImageObject );

#endif //TAG_IMAGEOBJECT_H

