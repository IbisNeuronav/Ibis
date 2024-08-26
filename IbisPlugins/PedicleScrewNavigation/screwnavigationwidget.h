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

#ifndef SCREWNAVIGATIONWIDGET_H
#define SCREWNAVIGATIONWIDGET_H

// Qt includes
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QWidget>

// VTK includes
#include <QVTKOpenGLNativeWidget.h>
#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageFlip.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceToColors.h>
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkInteractorStyleImage2.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

// VTK (drawing)
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkConeSource.h>
#include <vtkCutter.h>
#include <vtkCylinderSource.h>
#include <vtkLineSource.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>
#include <vtkTransformPolyDataFilter.h>

// IBIS includes
#include "application.h"
#include "ibisapi.h"
#include "imageobject.h"
#include "pediclescrewnavigationplugininterface.h"
#include "pointerobject.h"
#include "polydataobject.h"
#include "sceneobject.h"

// Plugin includes
#include "screwproperties.h"

class PedicleScrewNavigationPluginInterface;
class SceneObject;
class ImageObject;

class vtkImageData;
class vtkRenderer;
class vtkImageResliceToColors;
class vtkActor;
class vtkImageActor;
class vtkTransform;
class vtkImageStack;
class vtkPolyData;
class vtkLineSource;
class vtkMatrix4x4;
class vtkMultiImagePlaneWidget;

class QListWidgetItem;

class Screw;

namespace Ui
{
class ScrewNavigationWidget;
}

class ScrewNavigationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScrewNavigationWidget( std::vector<Screw *> plannedScrews = std::vector<Screw *>(), QWidget * parent = 0 );
    ~ScrewNavigationWidget();

    void SetPluginInterface( PedicleScrewNavigationPluginInterface * );
    void Navigate();
    void StopNavigation();

    void GetPlannedScrews( std::vector<Screw *> & screws );
    void SetPlannedScrews( std::vector<Screw *> screws );

    vtkRenderer * GetAxialRenderer();
    vtkRenderer * GetSagittalRenderer();

private:
    void GetAxialPositionAndOrientation( ImageObject *, vtkTransform *, double ( &pos )[3],
                                         double ( &orientation )[3] );
    void MoveAxialPlane( vtkSmartPointer<vtkImageResliceToColors> reslice, double pos[3], double orientation[3] );

    void GetSagittalPositionAndOrientation( ImageObject *, vtkTransform *, double ( &pos )[3],
                                            double ( &orientation )[3] );
    void MoveSagittalPlane( double pos[3], double orientation[3] );

    void UpdatePlannedScrews();
    void AddPlannedScrew( Screw * );
    void AddPlannedScrew( double position[3], double orientation[3], double screwLength, double screwDiameter,
                          double screwTipSize, bool useWorld = false );
    bool GetPointerDirection( double ( &dir )[3] );

    void SetDefaultView( vtkSmartPointer<vtkRenderer> );
    void UpdateInstrumentDrawing( vtkSmartPointer<vtkRenderer> );
    void InitializeAnnotationDrawing();

    void UpdatePointerDirection();
    void UpdateRulerDrawing( vtkSmartPointer<vtkActor> );

    void InitializeUi();

private:
    Ui::ScrewNavigationWidget * ui;
    PedicleScrewNavigationPluginInterface * m_pluginInterface;

    int m_screwPlanImageDataId;
    double m_screwDiameter;
    double m_screwLength;
    double m_screwTipSize;
    int m_rulerLength;

    bool m_isNavigating;
    bool m_isSagittalViewFlipped;
    bool m_isAxialViewFlipped;

    bool m_showScrew;
    bool m_showRuler;

    vtkSmartPointer<vtkActor> m_screwActor;
    vtkSmartPointer<vtkActor> m_rulerActor;

    vtkSmartPointer<vtkLineSource> m_axialScrewSource;
    vtkSmartPointer<vtkRenderer> m_axialInstrumentRenderer;
    vtkSmartPointer<vtkRenderer> m_sagittalInstrumentRenderer;

    vtkSmartPointer<vtkImageResliceToColors> m_axialReslice;
    vtkSmartPointer<vtkImageResliceToColors> m_axialScrewReslice;
    vtkSmartPointer<vtkRenderer> m_axialRenderer;
    vtkSmartPointer<vtkImageActor> m_axialActor;
    vtkSmartPointer<vtkImageData> m_axialImage;

    vtkSmartPointer<vtkImageResliceToColors> m_sagittalReslice;
    vtkSmartPointer<vtkRenderer> m_sagittalRenderer;
    vtkSmartPointer<vtkImageActor> m_sagittalActor;
    vtkSmartPointer<vtkImageData> m_sagittalImage;

    double m_pointerDirection[3];
    std::vector<Screw *> m_PlannedScrewList;

    double m_currentAxialPosition[3];        // stores the cursor position in axial plane
    double m_currentSagittalPosition[3];     // stores the cursor position in sagittal plane
    double m_currentAxialOrientation[3];     // stores the cursor orientation in axial plane
    double m_currentSagittalOrientation[3];  // stores the cursor orientation in sagittal plane

public slots:
    void UpdateInputs();
    void OnPointerPositionUpdated();
    void OnScrewSizeComboBoxModified( int );
    void OnScrewListItemChanged( QListWidgetItem * );
    void on_displayScrewCheckBox_toggled( bool );
    void on_displayRulerCheckBox_toggled( bool );
    void on_displayPlanningCheckBox_toggled( bool );
    void on_openScrewTableButton_clicked();
    void on_saveScrewPositionButton_clicked();

private slots:
    void on_rulerSpinBox_valueChanged( int );

    void OnObjectAddedSlot( int );
    void OnObjectRemovedSlot( int );
    void UpdateScrewComboBox();
    void NavigationPointerChangedSlot();

    void on_resetDefaultViewButton_clicked();

    void on_flipAxialViewCheckBox_toggled( bool );
    void on_flipSagittalViewCheckBox_toggled( bool );

    void on_importPlanButton_clicked();
    void on_exportPlanButton_clicked();
    void on_closeButton_clicked();

signals:
    void CloseNavigationWidget();
};

#endif  // __SCREWNAVIGATIONWIDGET_H__
