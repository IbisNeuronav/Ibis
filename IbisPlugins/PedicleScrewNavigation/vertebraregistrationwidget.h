/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Author: Houssem-Eddine Gueziri

#ifndef __VertebraRegistrationWidget_h_
#define __VertebraRegistrationWidget_h_

// Qt includes
#include <QComboBox>
#include <QMessageBox>
#include <QWidget>
#include <QFileDialog>
#include <QElapsedTimer>
#include <QProgressDialog>
#include <QDockWidget>
#include <QFlags>

// ITK includes
#include <itkImageDuplicator.h>
#include <itkCastImageFilter.h>
#include <itkImageMomentsCalculator.h>

// VNL includes
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>
#include <vnl/algo/vnl_svd.h>

// VTK includes
#include <vtkLandmarkTransform.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

// IBIS includes
#include "usacquisitionobject.h"
#include "sceneobject.h"
#include "imageobject.h"
#include "pointerobject.h"
#include "pointsobject.h"
#include "ibisapi.h"

// Plugin includes
#include "pediclescrewnavigationplugininterface.h"
#include "ui_vertebraregistrationwidget.h"
#include "gpu_rigidregistration.h"
#include "gpu_volumereconstruction.h"
#include "gpu_weightrigidregistration.h"
#include "screwnavigationwidget.h"
#include "screwproperties.h"
#include "secondaryusacquisition.h"

class QDockWidget;
class PedicleScrewNavigationPluginInterface;
class USAcquisitionObject;
class ImageObject;
class vtkTransform;
class Screw;

namespace Ui
{
    class VertebraRegistrationWidget;
}

class VertebraRegistrationWidget : public QWidget
{

    Q_OBJECT

public:

    // additional internal image types
    using IbisItkFloat3ImagePointer = IbisItkFloat3ImageType::Pointer;
    using IbisItkUC3ImagePointer = IbisItkUnsignedChar3ImageType::Pointer;
    using ImageCastFilterUC2F = itk::CastImageFilter< IbisItkUnsignedChar3ImageType, IbisItkFloat3ImageType >;

    explicit VertebraRegistrationWidget(QWidget *parent = 0);
    ~VertebraRegistrationWidget();

    void SetPluginInterface( PedicleScrewNavigationPluginInterface * interf );

    // Access to screw navigation renderer
    vtkRenderer * GetScrewNavigationAxialRenderer();
    vtkRenderer * GetScrewNavigationSagittalRenderer();

private:

    void UpdateUi();

    // Reconstruction functionality
    bool CreateVolumeFromSlices(USAcquisitionObject *, float spacingFactor=0);
    
    // Registration functionality
    IbisItkFloat3ImageType::PointType GetCenterOfGravity( IbisItkFloat3ImagePointer );
    void PerformInitialAlignment(vtkTransform *&, IbisItkFloat3ImagePointer,
                                 std::vector< itk::SmartPointer<IbisItkFloat3ImageType> >,
                                 std::vector< itk::Point< double, 3> >, vtkTransform * parent=0);

    itk::Point< double, 3 > GetImageCenterPoint(itk::SmartPointer<IbisItkFloat3ImageType>);
    void GetUSScanDirection(itk::Vector<double, 3> &, itk::Point<double, 3> &, std::vector< itk::Point< double, 3> >,
                            vtkTransform * parent=0);
    void GetUSScanOrthogonalDirection(itk::Vector<double, 3> &,
                                      std::vector< itk::SmartPointer<IbisItkFloat3ImageType> >,
                                      std::vector< itk::Point< double, 3> >,
                                      vtkTransform * parent=0);
    bool Register();
    
    // Navigation functionality
    void StartNavigation();
    void StopNavigation();

private:

    Ui::VertebraRegistrationWidget * ui;
    PedicleScrewNavigationPluginInterface* m_pluginInterface;
    
    bool                                        m_isProcessing; // mutex
    
    // Recpmstruction attributes
    double                                      m_reconstructionResolution;
    unsigned int                                m_reconstructionSearchRadius;
    double                                      m_thresholdDistanceToAddImage; // in mm
    itk::SmartPointer<IbisItkFloat3ImageType>   m_sparseUsVolume; 

    // Initial alignment attributes
    std::vector< itk::SmartPointer<IbisItkFloat3ImageType> > m_inputImageList;
    std::vector< itk::Point< double, 3> >       m_usScanCenterPointList;
    std::string                                 m_sweepDirection;
    double                                      m_lambdaMetricBalance;
    
    // Navigation attributes
    ScrewNavigationWidget *                     m_navigationWidget;
    bool                                        m_isNavigating;
    std::vector<Screw *>                        m_PlannedScrewList;

    // Advanced settings
    bool                                        m_showAdvancedSettings;
    int                                         m_optNumberOfPixels;
    int                                         m_optSelectivity;
    int                                         m_optPopulationSize;
    double                                      m_optPercentile;
    double                                      m_optInitialSigma;

    int m_it;
    SecondaryUSAcquisition *                    m_secondaryAcquisitions;

private slots:

    void OnObjectAddedSlot( int );
    void OnObjectRemovedSlot( int );

    void on_initialAlignmentCheckBox_stateChanged(int);
    void on_sweepDirectionComboBox_currentIndexChanged(int);
    void on_startRegistrationButton_clicked();
    
    void on_navigateButton_clicked();
    void on_navigationWindowClosed();

    void on_presetVolumeButton_clicked();
    void on_opacityShiftSlider_valueChanged(int);

    // Advanced settings
    void on_ultrasoundResolutionComboBox_currentIndexChanged(int);
    void on_ultrasoundSearchRadiusComboBox_currentIndexChanged(int);
    void on_lambdaMetricSlider_valueChanged( int );
    void on_numberOfPixelsDial_valueChanged( int );
    void on_selectivityDial_valueChanged( int );
    void on_percentileDial_valueChanged( int );
    void on_optPopulationSizeComboBox_currentIndexChanged( int );
    void on_initialSigmaComboBox_currentIndexChanged( int );
    void on_advancedSettingsButton_clicked();
    void on_addUSAcquisitionButton_clicked();
    void on_removeUSAcquisitionButton_clicked();
};


#endif
