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

private slots:

    void UpdateInputs();
    void UpdateUi();
    void UpdateCurrentFrameUi();
    void UpdateViews();
    void VolumeModified(); //  what is this referring to? -> the current volume change, may need to add one


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

    // added May 13, 2015 by Xiao **
    void on_checkBoxImage2_toggled(bool checked); // checkbox for whether blend two MRIs
    void on_imageObjectsComboBox_2_currentIndexChanged(int index); // image list for added volume
    void on_opacitySlider_2_valueChanged(int value);
   // void AddedVolumeModified();

private:

    USAcquisitionPluginInterface * m_pluginInterface;

    vtkImageResliceToColors * m_reslice;
    vtkImageResliceToColors * m_reslice2;
    vtkImageBlend * m_blendedImage; //blended images
    vtkImageMask * m_imageMask;

    vtkRenderer * m_usRenderer;
    vtkImageActor * m_usActor;
    vtkRenderer * m_mriRenderer;
    vtkImageActor * m_mriActor;

    Ui::DoubleViewWidget *ui;

    // For red cross when probe is out of view?
    vtkActor * m_usLine1Actor;
    vtkActor * m_usLine2Actor;
    vtkActor * m_mriLine1Actor;
    vtkActor * m_mriLine2Actor;
};

#endif
