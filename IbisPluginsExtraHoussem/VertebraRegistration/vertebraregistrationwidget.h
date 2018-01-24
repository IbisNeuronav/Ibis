/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __VertebraRegistrationWidget_h_
#define __VertebraRegistrationWidget_h_

#include <QWidget>
#include "qdebugstream.h"
#include "ui_vertebraregistrationwidget.h"
#include "application.h"
#include "scenemanager.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "vtkTransform.h"
#include <QComboBox>
#include <QMessageBox>
#include <unistd.h>

//#include <itkHessianRecursiveGaussianImageFilter.h>
//#include <itkCastImageFilter.h>
//#include <itkConvolutionImageFilter.h>
//#include <itkGaborImageSource.h>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <itkCastImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageFileReader.h>
#include <itkMaskImageFilter.h>
#include <type_traits>
#include <itkBresenhamLine.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>

#include <itkImageFileWriter.h>


#include "vertebraregistrationplugininterface.h"
#include "usacquisitionobject.h"
#include "usprobeobject.h"
#include "hardwaremodule.h"
#include "guiutilities.h"


#include "QVTKWidget.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage2.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkImageResliceToColors.h"
#include "vtkImageBlend.h"
#include "vtkImageMask.h"
#include "vtkImageData.h"
#include "vtkImageStack.h"
#include "vtkImageProperty.h"

#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"

#include "vtkCamera.h"
#include "usmask.h"
#include "itkVTKImageToImageFilter.h"

#include <math.h>

#include"vtkMath.h"

#include "itkBoneExtractionFilter.h"
#include "itkNumericTraits.h"
#include "itkTimeProbe.h"

class Application;
class VertebraRegistrationPluginInterface;
class USAcquisitionObject;
class ImageObject;
class vtkRenderer;
class vtkImageResliceToColors;
class vtkImageBlend;
class vtkImageMask;
class vtkActor;
class vtkImageActor;
class vtkAlgorithmOutput;
class vtkTransform;
class vtkImageStack;
class USMask;

namespace Ui
{
    class VertebraRegistrationWidget;
}


class VertebraRegistrationWidget : public QWidget
{

    Q_OBJECT

public:

    explicit VertebraRegistrationWidget(QWidget *parent = 0);
    ~VertebraRegistrationWidget();

    void SetApplication( Application * app );
    void SetPluginInterface( VertebraRegistrationPluginInterface * interf );

    static void SetDefaultView( vtkSmartPointer<vtkImageSlice>, vtkSmartPointer<vtkRenderer> );

    Ui::VertebraRegistrationWidget * GetUi() { return ui; }


private:

    // additional internal image types
    typedef itk::Image <IbisItkFloat3ImageType::PixelType, 2> IbisItkFloat2ImageType;
    typedef itk::Image <unsigned char, 3> IbisItkUnsignedChar3ImageType;
    typedef itk::Image <unsigned char, 2> IbisItkUnsignedChar2ImageType;
    typedef itk::BoneExtractionFilter<IbisItkFloat3ImageType> BoneFilterType;

    void UpdateUi();
    template <typename TInputImage=IbisItkFloat3ImageType>
    void AddImageToScene(TInputImage *, QString);

    template<typename TInputImage, typename TOutputImage>
    void SuperImposeImages(itk::SmartPointer<TInputImage> &, itk::SmartPointer<TOutputImage>);

    template <typename TOutputImage = IbisItkUnsignedChar3ImageType>
    void CreateItkMask(itk::SmartPointer<TOutputImage> &);

    template <typename TInputImage>
    void UpdateItkMask(TInputImage &);

//    itk::SmartPointer<IbisItkUnsignedChar2ImageType> RayCastByMask(itk::SmartPointer<IbisItkFloat3ImageType>);
//    IndexType2D GetMaxLinePixels(itk::SmartPointer<IbisItkFloat3ImageType>, IndexType2D, IndexType2D);
//    itk::SmartPointer<IbisItkUnsignedChar2ImageType> DilateMaskImage(itk::SmartPointer<IbisItkUnsignedChar2ImageType>, int);
//    void ComputeDistancemap(itk::SmartPointer<IbisItkUnsignedChar2ImageType>, itk::SmartPointer<IbisItkFloat2ImageType> &);
//    void ThresholdImage(itk::SmartPointer<IbisItkFloat2ImageType>, itk::SmartPointer<IbisItkUnsignedChar2ImageType> &, double);
//    void ThresholdByDistanceMap(itk::SmartPointer<IbisItkFloat3ImageType> &, itk::SmartPointer<IbisItkUnsignedChar2ImageType>, double);

    vtkImageData * GetVtkBoneSurface(vtkImageData *);

    void CreateVolumeFromSlices(float spacingFactor=0);
    void getMinimumMaximumVolumeExtent(IbisItkFloat3ImageType::Pointer,
                                       IbisItkFloat3ImageType::PointType &,
                                       IbisItkFloat3ImageType::PointType &);


    void UpdateItkImage(itk::SmartPointer<IbisItkFloat3ImageType>, int);
//    void applyGaborFilter(IbisItkFloat3ImageType::Pointer, IbisItkFloat3ImageType::Pointer);
//    void applyGaborFilter(IbisItkFloat2ImageType::Pointer, IbisItkFloat2ImageType::Pointer);
//    void CreateKernel(IbisItkFloat3ImageType::Pointer, unsigned int);


    Ui::VertebraRegistrationWidget * ui;
    Application * m_application;

    int m_idImageObject = SceneManager::InvalidId;
    bool m_debug = false;
    double m_alphaObjectness;
    double m_betaObjectness;
    double m_gammaObjectness;

    VertebraRegistrationPluginInterface * m_pluginInterface;

    vtkSmartPointer<vtkImageResliceToColors> m_reslice;
    vtkSmartPointer<vtkImageMask> m_imageMask;


    vtkSmartPointer<vtkImageActor> m_vol1Slice;
    vtkSmartPointer<vtkImageActor> m_usSlice;
    vtkSmartPointer<vtkRenderer> m_usRenderer;
    vtkSmartPointer<vtkImageActor> m_usActor;

    // For red cross when probe is out of view?
    vtkSmartPointer<vtkActor> m_usLine1Actor;
    vtkSmartPointer<vtkActor> m_usLine2Actor;

    USMask * m_usmask;

    itk::SmartPointer<BoneFilterType> m_boneFilter;
    IbisItkVtkConverter * m_ItktovtkConverter;
    int m_ImageObjectId;

    std::vector< itk::SmartPointer<IbisItkFloat3ImageType> > m_inputImageList;
    itk::SmartPointer<IbisItkFloat3ImageType> m_sparseVolume;


protected:

    void UpdatePipelineConnections();
    void MakeCrossLinesToShowProbeIsOutOfView();
    void SetDefaultViews();

private slots:

    void UpdateInputs();
    void UpdateViews();
    void UpdateStatus();

    void on_liveCheckBox_toggled(bool checked);

private slots:

    void on_addFrameButton_clicked();
    void on_startButton_clicked();
    void on_refreshButton_clicked();
    void on_debugCheckBox_stateChanged(int);
    void on_alphaSpinBox_valueChanged(double);
    void on_betaSpinBox_valueChanged(double);
    void on_gammaSpinBox_valueChanged(double);

};


#endif
