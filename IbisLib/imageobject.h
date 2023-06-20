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

#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>

#include <QObject>
#include <QVector>
#include <map>

#include "ibisitkvtkconverter.h"
#include "sceneobject.h"
#include "serializer.h"

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

/**
 * @class   ImageObject
 * @brief   ImageObject is derived from SceneObject
 *
 * ImageObject is used to represents a geometric structure that is a topological and geometrical regular array of
 points.
 *
 * In IBIS we use ImageObject to show data from the following file types:
 *
 * Minc file: *.mnc *.mnc2 *.mnc.gz *.MNC *.MNC2 *.MNC.GZ\n
 * Nifti file *.nii\n
 * VTK file: *.vtk *.vtp\n
 *
 * Images loaded in IBIS are initially represented in grayscale. Colors can be set using a predefind LUT (Look Up
 Table).
 * All lookup tables are defined in LookupTableManager.
 *
 * Image data is kept in the class as: vtkImageData *Image\n
 * and as an ITK image:\n
 *    IbisItkFloat3ImageType::Pointer ItkImage  or  IbisItkUnsignedChar3ImageType::Pointer ItkLabelImage;

 *
 *  @sa SceneObject SceneManager LookupTableManager vtkImageData
 */

class ImageObject : public SceneObject
{
    Q_OBJECT

public:
    static ImageObject * New() { return new ImageObject; }
    vtkTypeMacro( ImageObject, SceneObject );

    ImageObject();
    virtual ~ImageObject();

    virtual void Serialize( Serializer * ser ) override;
    virtual void Export() override;
    virtual bool IsExportable() override { return true; }
    /** Save image data as a MINC2 file (*.mnc). */
    void SaveImageData( QString & name );
    /** Check if this is a label image. */
    bool IsLabelImage();
    /** Return image data, VTK format. */
    vtkImageData * GetImage();
    /** Set ITK image, all except labels. */
    bool SetItkImage( IbisItkFloat3ImageType::Pointer image );
    /** Set ITK image for labels. */
    bool SetItkLabelImage( IbisItkUnsignedChar3ImageType::Pointer image );
    /** Return image data, ITK format. */
    IbisItkFloat3ImageType::Pointer GetItkImage() { return this->ItkImage; }
    /** Return label image data, ITK format. */
    IbisItkUnsignedChar3ImageType::Pointer GetItkLabelImage() { return this->ItkLabelImage; }
    /** Set image data, VTK format.
     *  @param image input data
     *  @param tr initial transform
     */
    void SetImage( vtkImageData * image, vtkTransform * tr = 0 );

    /** Manage object actions after adding to the scene. */
    virtual void ObjectAddedToScene() override;
    /** Setup object in a specific view */
    virtual void Setup( View * view ) override;
    /** Release object from a specific view */
    virtual void Release( View * view ) override;
    /** Create widgets specific to the object, they are added as tabs to the basic settings dialog. */
    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override;

    /** Set bounding box visibility in 3D view. */
    void SetViewOutline( int isOn );
    /** Check bounding box visibility in 3D view. */
    int GetViewOutline();

    /** Choose from the set of lookup table templates available from the LookupTableManager. */
    int ChooseColorTable( int index );
    /** Get the index of currently used LUT. */
    int GetLutIndex() { return lutIndex; }
    /** Get currently used LUT. */
    vtkScalarsToColors * GetLut();
    /** Get LUT range. */
    double * GetLutRange();
    /** Set LUT range. */
    void SetLutRange( double r[2] );
    /** Get scalar range from Image. */
    void GetImageScalarRange( double * range );
    /** Get number of scalar components from Image. */
    int GetNumberOfScalarComponents();
    /** Get image bounds from Image. */
    void GetBounds( double bounds[6] );
    /** Get image center from Image. */
    void GetCenter( double center[3] );
    /** Get image spacing from Image. */
    double * GetSpacing();
    /** Set intensity factor, default is 1.0. */
    void SetIntensityFactor( double factor );
    /** Get intensity factor. */
    double GetIntensityFactor();

    /** Show the information found in the MINC file in a popup widget. */
    virtual void ShowMincInfo();

    vtkImageAccumulate * GetHistogramComputer();

    // vtk volume rendering
    /** Enable/disable volume rendering. */
    void SetVtkVolumeRenderingEnabled( bool on );
    /** Check if volume rendering is enabled. */
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

    void SetInternalImage( vtkImageData * );

    // Lookup table management
    void SetLut( vtkSmartPointer<vtkScalarsToColors> lut );

    // Setup histogram properties after new image is set.
    void SetupHistogramComputer();

    IbisItkVtkConverter * ItktovtkConverter;
    IbisItkFloat3ImageType::Pointer ItkImage;
    IbisItkUnsignedChar3ImageType::Pointer ItkLabelImage;

    vtkImageData * Image;
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

    typedef std::map<View *, PerViewElements *> ImageObjectViewAssociation;
    ImageObjectViewAssociation imageObjectInstances;

    bool SanityCheck( IbisItkFloat3ImageType::Pointer image );
    bool SanityCheck( IbisItkUnsignedChar3ImageType::Pointer image );
};

ObjectSerializationHeaderMacro( ImageObject );

#endif  // TAG_IMAGEOBJECT_H
