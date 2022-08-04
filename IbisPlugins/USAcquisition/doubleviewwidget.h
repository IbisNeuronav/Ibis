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

/**
 * @class   DoubleViewWidget
 * @brief   This class is used to show US acquisition together with MRI or other image data
 *
 *  DoubleViewWidget consists of two fixed size windows, placed horizontally side by side and surrounded by control buttons.
 *  The left window is used to show the acquired frames, the right window shows some image data, usually pre-operational MRI.
 *  Both images can be blended in the right side window.
 *  A second image may be added to the right window and blended with the first one.
 *  DoubleViewWidget may be used to show a live acquisition or a previously acquired and stored acquisition.
 *  Initially the class was developed for fixed size frames, changes must be made to accomodate different size frames.
 *
 *  Variables starting with us or m_us are used to handle elements of the left window.
 *  Variables starting with mri or m_mri are used to handle elements of the right window,
 *  although some variables for the right window don't use mri part, e.g. m_reslice, m_reslice2.
 *  Exceptionally m_usSlice is used in the right window, "us" just tells us that it holds an acquired frame.
 *
 *  @sa USAcquisitionObject ImageObject
 */

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
    
    /** Set pointer to USAcquisitionPluginInterface in order to establish the plugin pipeline and set up default elements to show in the double view. */
    void SetPluginInterface( USAcquisitionPluginInterface * interf );

protected:

    //Set EXTRACT_PRIVATE in Doxyfile to YES to view the comments below.
    /** Establish visibility and opacity, adjust pipeline if masking is required. */
    void UpdatePipelineConnections();
    /** When capturing live frames, the probe may momentarily become invisible, red cross will then show in both windows and frames won't be stored in the buffer as they are not valid. */
    void MakeCrossLinesToShowProbeIsOutOfView();
    /** Set camera focus and position in the center of the image defined by image actor bounds.*/
    void SetDefaultView( vtkSmartPointer<vtkImageSlice> actor, vtkSmartPointer<vtkRenderer> renderer );
    /** Calls SetDefaultView() first for the left then for the right window. */
    void SetDefaultViews();
    void SetDefaultView( vtkSmartPointer<vtkImageSlice> actor, vtkSmartPointer<vtkRenderer> renderer );

private slots:

    /** Set US images in the left window and data images in the right window, adjust position when necessary.*/
    void UpdateInputs();
    /** Enable/disable UI elements accordingly to the current acquisition state - live, blending, etc. */
    void UpdateUi();
    /** Update frame position on the frames slider and in the frames spin box. */
    void UpdateCurrentFrameUi();
    /** Render images in both windows, update status.*/
    void UpdateViews();

    /** The right window will show blended right side and left side images, if there is a second image in the right window it will also be blended in.*/
    void on_blendCheckBox_toggled(bool checked);
    /** opacitySlider_1 is used to adjust opacity of US frame blended into the image in the right window. */
    void on_opacitySlider_1_valueChanged(int value);
    /** Acqusition may use mask, checking the maskCheckBox will allow setting alpha blending value for the mask.*/
    void on_maskCheckBox_toggled(bool checked);
    /** Set alpha blending value for the mask. */
    void on_maskAlphaSlider_valueChanged(int value);
    /** Set default views in both windows. */
    void on_restoreViewsPushButton_clicked();
    /** Set the acquisition selected by index in the left window. The right window images will be adjusted accordingly.*/
    void on_acquisitionsComboBox_currentIndexChanged(int index);
    /** Set the primary data image in the right window. Views in both windows will be updated. */
    void on_imageObjectsComboBox_1_currentIndexChanged(int index);
    /** Show the first acquired frame in left window and the corresponding reslice in the right window. */
    void on_m_rewindButton_clicked();
    /** Show the frame <b>frameNo</b> acquired frame in left window and the corresponding reslice in the right window. */
    void on_currentFrameSpinBox_valueChanged(int frameNo);
    /** Show the frame <b>frameNo</b> acquired frame in left window and the corresponding reslice in the right window. */
    void on_m_frameSlider_valueChanged(int frameNo);
    /** If there is a US probe active, checking Live wil set the plugin in the acquisition mode. Left window will show currently acquired frames, right window will show respective reslie.*/
    void on_m_liveCheckBox_toggled(bool checked);
    /** Record will store the acquired frames in a buffer for later usage.*/
    void on_m_recordButton_clicked();
    /** Saving the acquired frames in a selected directory.*/
    void on_m_exportButton_clicked();
    /** Set visibility of actors in both windows. */
    void UpdateStatus();

    /** Add/remove second image in right window. */
    void on_checkBoxImage2_toggled(bool checked);
    /** Select an image from the list for the second data image in the right window. */
    void on_imageObjectsComboBox_2_currentIndexChanged(int index);
    /** opacitySlider_2 is used to adjust blending percentage of 2 data images shown in the right side window. */
    void on_opacitySlider_2_valueChanged(int value);

private:

    USAcquisitionPluginInterface * m_pluginInterface;

    /** Reslice of the first image data to use in the right window. */
    vtkSmartPointer<vtkImageResliceToColors> m_reslice;
    /** Reslice of the second, optional image data to use in the right window. */
    vtkSmartPointer<vtkImageResliceToColors> m_reslice2;
    /** Mask used for the acquisition.  Not very useful as it was hardcoded for the old US machine.*/
    vtkSmartPointer<vtkImageMask> m_imageMask;
    /** m_mriActor holds the stack of images to show in the right window. */
    vtkSmartPointer<vtkImageStack> m_mriActor;
    /** m_vol1Slice is used to render first image data in the right window. */
    vtkSmartPointer<vtkImageActor> m_vol1Slice;
    /** m_vol2Slice is used to render second image data in the right window. */
    vtkSmartPointer<vtkImageActor> m_vol2Slice;
    /** m_usSlice is used to render either a live frame or a frame from saved acquisition in the right window. */
    vtkSmartPointer<vtkImageSlice> m_usSlice;
    /** Renderer in the right window. */
    vtkSmartPointer<vtkRenderer> m_mriRenderer;
    /** Renderer in the left window. */
    vtkSmartPointer<vtkRenderer> m_usRenderer;
    /** m_usActor represents either a live frame or a frame from saved acquisition to be put in the left window. */
    vtkSmartPointer<vtkImageActor> m_usActor;

    Ui::DoubleViewWidget *ui;

    /** @name Red lines
     * @brief  For the red cross when probe is out of view
     */
    ///@{
    vtkSmartPointer<vtkActor> m_usLine1Actor;
    vtkSmartPointer<vtkActor> m_usLine2Actor;
    vtkSmartPointer<vtkActor> m_mriLine1Actor;
    vtkSmartPointer<vtkActor> m_mriLine2Actor;
    ///@}
};

#endif
