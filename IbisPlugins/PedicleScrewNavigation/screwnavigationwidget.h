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

#ifndef __SCREWNAVIGATIONWIDGET_H__
#define __SCREWNAVIGATIONWIDGET_H__

// Qt includes
#include <QWidget>
#include <QDir>
#include <QLabel>
#include <QFileDialog>

// VTK includes
#include <QVTKOpenGLNativeWidget.h>
#include <vtkImageData.h>
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage2.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkImageActor.h>
#include <vtkImageResliceToColors.h>
#include <vtkImageProperty.h>
#include <vtkImageFlip.h>

// VTK (drawing)
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkCylinderSource.h>
#include <vtkConeSource.h>
#include <vtkSphereSource.h>
#include <vtkLineSource.h>
#include <vtkTextProperty.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkTransformPolyDataFilter.h>

// IBIS includes
#include "pediclescrewnavigationplugininterface.h"
#include "sceneobject.h"
#include "pointerobject.h"
#include "ibisapi.h"
#include "application.h"
#include "imageobject.h"
#include "polydataobject.h"

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

namespace Ui {
    class ScrewNavigationWidget;
}

class ScrewNavigationWidget : public QWidget
{

    Q_OBJECT

public:

    explicit ScrewNavigationWidget(std::vector<Screw *> plannedScrews=std::vector<Screw *>(), QWidget *parent=0);
    ~ScrewNavigationWidget();

    void SetPluginInterface( PedicleScrewNavigationPluginInterface * );
    void Navigate();
    void StopNavigation();

    void GetPlannedScrews(std::vector<Screw *> &screws);
    void SetPlannedScrews(std::vector<Screw *> screws);

    vtkRenderer * GetAxialRenderer();
    vtkRenderer * GetSagittalRenderer();

private:

    void GetAxialPositionAndOrientation(ImageObject *, vtkTransform *, double (&pos)[3], double (&orientation)[3]);
    void MoveAxialPlane(vtkSmartPointer<vtkImageResliceToColors> reslice, double pos[3], double orientation[3]);

    void GetSagittalPositionAndOrientation(ImageObject *, vtkTransform *, double (&pos)[3], double (&orientation)[3]);
    void MoveSagittalPlane(double pos[3], double orientation[3]);

    void UpdatePlannedScrews();
    void AddPlannedScrew(Screw *);
    void AddPlannedScrew( double position[3], double orientation[3], double screwLength, double screwDiameter, double screwTipSize, bool useWorld=false );
    bool GetPointerDirection(double (&dir)[3]);

    void SetDefaultView( vtkSmartPointer<vtkRenderer> );
    void InitializeScrewDrawing();
    void InitializeRulerDrawing();
    void InitializeAnnotationDrawing();

    void UpdatePointerDirection();
    void UpdateScrewDrawing();
    void UpdateRulerDrawing();
    void RecenterResliceAxes(vtkMatrix4x4 *);

    QString GetScrewName(double, double);

//    void UpdatePlanningImage();

    void InitializeUi();

private:

    Ui::ScrewNavigationWidget *ui;
    PedicleScrewNavigationPluginInterface * m_pluginInterface;

    int m_screwPlanImageDataId;
    double m_screwDiameter;
    double m_screwLength;
    double m_screwTipSize;
    int m_rulerLength;

    bool m_isNavigating;
    bool m_isSagittalViewFlipped;
    bool m_isAxialViewFlipped;

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
    std::vector<vtkActor *> m_AxialPlannedScrewActorList;
    std::vector<vtkActor *> m_SagittalPlannedScrewActorList;
    std::vector<vtkPolyData *> m_PlannedScrewPolyDataList;
    std::vector<vtkPolyData *> m_PlannedScrewCrossSectionPolyDataList;

    std::vector<Screw *> m_PlannedScrewList;

    vtkSmartPointer<vtkTransform> m_prevTransformTest;
    double m_savePos[3];
    double saveDirection[3];

    double m_currentAxialPosition[3];       // stores the cursor position in axial plane
    double m_currentSagittalPosition[3];    // stores the cursor position in sagittal plane
    double m_currentAxialOrientation[3];    // stores the cursor orientation in axial plane
    double m_currentSagittalOrientation[3]; // stores the cursor orientation in sagittal plane

public slots:
    void UpdateInputs();
    void OnPointerPositionUpdated();
    void OnScrewSizeComboBoxModified(int);
    void OnScrewListItemChanged(QListWidgetItem *);
    void on_displayScrewCheckBox_toggled(bool);
    void on_displayRulerCheckBox_toggled(bool);
    void on_displayPlanningCheckBox_toggled(bool);
    void on_openScrewTableButton_clicked();
    void on_saveScrewPositionButton_clicked();

private slots:
    void on_rulerSpinBox_valueChanged(int);

    void OnObjectAddedSlot( int );
    void OnObjectRemovedSlot( int );
    void UpdateScrewComboBox();
    void NavigationPointerChangedSlot();

    void on_flipAxialViewCheckBox_toggled(bool);
    void on_flipSagittalViewCheckBox_toggled(bool);

    void on_importPlanButton_clicked();
    void on_exportPlanButton_clicked();
    void on_closeButton_clicked();

signals:
    void CloseNavigationWidget();
};

#endif // __SCREWNAVIGATIONWIDGET_H__
