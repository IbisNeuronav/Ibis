/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef DOUBLEVIEWWIDGET_H
#define DOUBLEVIEWWIDGET_H

#include <QWidget>
#include <vtkSmartPointer.h>

namespace Ui {
class DoubleViewWidget;
}

class USAcquisitionPluginInterface;
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
class vtkImageSlice;

class DoubleViewWidget : public QWidget
{
    Q_OBJECT
    
public:

    explicit DoubleViewWidget( QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~DoubleViewWidget();
    
    void SetPluginInterface( USAcquisitionPluginInterface * interf );

protected:

    void UpdatePipelineConnections();
    void MakeCrossLinesToShowProbeIsOutOfView();
    void SetDefaultViews();
    void SetDefaultView( vtkSmartPointer<vtkImageSlice> actor, vtkSmartPointer<vtkRenderer> renderer );

private slots:

    void UpdateInputs();
    void UpdateUi();
    void UpdateCurrentFrameUi();
    void UpdateViews();

    void on_blendCheckBox_toggled(bool checked);
    void on_opacitySlider_1_valueChanged(int value);
    void on_maskCheckBox_toggled(bool checked);
    void on_maskAlphaSlider_valueChanged(int value);
    void on_restoreViewsPushButton_clicked();
    void on_acquisitionsComboBox_currentIndexChanged(int index);
    void on_imageObjectsComboBox_1_currentIndexChanged(int index);
    void on_m_rewindButton_clicked();
    void on_currentFrameSpinBox_valueChanged(int arg1);
    void on_m_frameSlider_valueChanged(int value);
    void on_m_liveCheckBox_toggled(bool checked);
    void on_m_recordButton_clicked();
    void on_m_exportButton_clicked();
    void UpdateStatus();

    void on_checkBoxImage2_toggled(bool checked); // checkbox for whether blend two MRIs
    void on_imageObjectsComboBox_2_currentIndexChanged(int index); // image list for added volume
    void on_opacitySlider_2_valueChanged(int value);

private:

    USAcquisitionPluginInterface * m_pluginInterface;

    vtkSmartPointer<vtkImageResliceToColors> m_reslice;
    vtkSmartPointer<vtkImageResliceToColors> m_reslice2;
    vtkSmartPointer<vtkImageMask> m_imageMask;

    vtkSmartPointer<vtkImageStack> m_mriActor;
    vtkSmartPointer<vtkImageActor> m_vol1Slice;
    vtkSmartPointer<vtkImageActor> m_vol2Slice;
    vtkSmartPointer<vtkImageSlice> m_usSlice;
    vtkSmartPointer<vtkRenderer> m_mriRenderer;
    vtkSmartPointer<vtkRenderer> m_usRenderer;
    vtkSmartPointer<vtkImageActor> m_usActor;

    Ui::DoubleViewWidget *ui;

    // For red cross when probe is out of view?
    vtkSmartPointer<vtkActor> m_usLine1Actor;
    vtkSmartPointer<vtkActor> m_usLine2Actor;
    vtkSmartPointer<vtkActor> m_mriLine1Actor;
    vtkSmartPointer<vtkActor> m_mriLine2Actor;
};

#endif
