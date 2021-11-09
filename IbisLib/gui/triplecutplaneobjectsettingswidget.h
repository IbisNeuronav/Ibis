/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TRIPLECUTPLANEOBJECTSETTINGSWIDGET_H
#define TRIPLECUTPLANEOBJECTSETTINGSWIDGET_H

#include <QWidget>
#include <QObject>

namespace Ui {
class TripleCutPlaneObjectSettingsWidget;
}

class TripleCutPlaneObject;
class QLabel;
class QSlider;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class ImageObject;

class TripleCutPlaneObjectSettingsWidget : public QWidget
{

    Q_OBJECT
    
public:

    explicit TripleCutPlaneObjectSettingsWidget( QWidget * parent = 0 );
    ~TripleCutPlaneObjectSettingsWidget();

    void SetTripleCutPlaneObject( TripleCutPlaneObject * obj );
    
private slots:

    void on_sagittalPlaneCheckBox_toggled(bool checked);
    void on_coronalPlaneCheckBox_toggled(bool checked);
    void on_transversePlaneCheckBox_toggled(bool checked);
    void on_viewAllPlanesButton_clicked();
    void on_viewNoPlaneButton_clicked();
    void on_resliceInterpolationComboBox_currentIndexChanged(int index);
    void on_displayInterpolationComboBox_currentIndexChanged(int index);
    void UpdateImageSliders();
    void on_sliceThicknessSpinBox_valueChanged( int val );
    void UpdateUI();

private:

    struct SlidersInfo
    {
        QHBoxLayout * labelLayout;
        QLabel * label;
        QComboBox * mixModeComboBox;
        QSlider * slider;
        ImageObject * object;
    };
    QList< SlidersInfo > m_imageSliders;

    TripleCutPlaneObject * m_cutPlaneObject;
    Ui::TripleCutPlaneObjectSettingsWidget * ui;
    QVBoxLayout * m_volumeContributionLayout;
};

#endif
