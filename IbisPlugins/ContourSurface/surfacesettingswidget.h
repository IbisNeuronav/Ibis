/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef SURFACESETTINGSWIDGET_H
#define SURFACESETTINGSWIDGET_H

#include <QWidget>

namespace Ui
{
class SurfaceSettingsWidget;
}

class GeneratedSurface;

class SurfaceSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SurfaceSettingsWidget( QWidget * parent = 0 );
    virtual ~SurfaceSettingsWidget();

    void SetGeneratedSurface( GeneratedSurface * surface );

public slots:
    void UpdateSettings();

private slots:
    virtual void ApplyButtonClicked();
    virtual void RangeSlidersValuesChanged( double, double );
    virtual void MidSliderValueChanged( double val );
    virtual void SetupHistogramWidget();
    virtual void PolygonReductionSpinBoxValueChanged( int red );

protected:
    GeneratedSurface * m_surface;
    void UpdateUI();
    double m_contourValue;
    double m_min;
    double m_max;
    int m_reductionPercent;

private:
    Ui::SurfaceSettingsWidget * ui;
};

#endif  // SURFACESETTINGSWIDGET_H
