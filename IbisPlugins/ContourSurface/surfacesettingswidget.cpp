/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkPiecewiseFunctionLookupTable.h"
#include "surfacesettingswidget.h"
#include "ui_surfacesettingswidget.h"
#include "generatedsurface.h"
#include <QtGui>

SurfaceSettingsWidget::SurfaceSettingsWidget(QWidget *parent) :
    QWidget(parent)
    , m_surface(0)
    , m_contourValue(0)
    , m_min(0.0)
    , m_max(1.0)
    , m_reductionPercent(0)
    , ui(new Ui::SurfaceSettingsWidget)
{
    ui->setupUi(this);
}

SurfaceSettingsWidget::~SurfaceSettingsWidget()
{
    delete ui;
    if (m_surface)
        m_surface->UnRegister( 0 );
}

void SurfaceSettingsWidget::SetGeneratedSurface( GeneratedSurface * surface )
{
    if( surface == m_surface )
    {
        return;
    }
    if (m_surface)
        m_surface->UnRegister( 0 );
    m_surface = surface;
    if (m_surface)
    {
        m_surface->Register( 0 );
        m_reductionPercent = m_surface->GetReduction();
        if( m_surface->IsValid() )
            this->SetupHistogramWidget();
    }
}

void SurfaceSettingsWidget::ApplyButtonClicked()
{
    Q_ASSERT( m_surface );
    m_surface->SetContourValue(m_contourValue);
    m_surface->SetReduction(m_reductionPercent);
    m_surface->SetGaussianSmoothingFlag(ui->gaussianSmoothingCheckBox->isChecked());
    double tmp;
    bool ok;
    tmp = ui->radiusFactorLineEdit->text().toDouble(&ok);
    if (ok)
        m_surface->SetRadiusFactor(tmp);
    tmp = ui->standardDeviationlineEdit->text().toDouble(&ok);
    if (ok)
        m_surface->SetStandardDeviation(tmp);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_surface->GenerateSurface();
    QApplication::restoreOverrideCursor();
    ui->contourValueLineEdit->setText(QString::number(m_contourValue));
    m_surface->MarkModified();
}

void SurfaceSettingsWidget::RangeSlidersValuesChanged(double min, double max)
{
    // ignore new settings
    this->UpdateUI();
}

void SurfaceSettingsWidget::MidSliderValueChanged( double val )
{
    Q_ASSERT( m_surface );
    double imageRange[2];
    m_surface->GetImageScalarRange( imageRange );
    m_contourValue = imageRange[0] + val * (imageRange[1]-imageRange[0]);
    this->UpdateUI();
}

void SurfaceSettingsWidget::SetupHistogramWidget()
{
    Q_ASSERT( m_surface );
    double contourValue = m_surface->GetContourValue();
    ui->contourValueLineEdit->setText(QString::number(contourValue));
    // Setup Histogram widget
    ui->histogramWidget->SetHistogram(m_surface->GetImageHistogram());
    double imageRange[2];
    m_surface->GetImageScalarRange( imageRange );
    ui->histogramWidget->SetImageRange( imageRange[0], imageRange[1] );
    double * range = m_surface->GetImageLut()->GetRange();
    m_min = ( range[0] - imageRange[0] ) / ( imageRange[1] - imageRange[0] );
    m_max = ( range[1] - imageRange[0] ) / ( imageRange[1] - imageRange[0] );
    ui->histogramWidget->setMinSliderValue( m_min );
    ui->histogramWidget->setMaxSliderValue( m_max );
    ui->histogramWidget->setMidSliderEnabled( true );
    ui->histogramWidget->setMidSliderValue( (contourValue-imageRange[0])/(imageRange[1]-imageRange[0]) );
    vtkPiecewiseFunctionLookupTable * lut = vtkPiecewiseFunctionLookupTable::SafeDownCast( m_surface->GetImageLut() );
    if( lut )
        ui->histogramWidget->SetColorTransferFunction( lut->GetColorFunction() );
    this->UpdateUI();
}

void SurfaceSettingsWidget::PolygonReductionSpinBoxValueChanged(int red)
{
    m_reductionPercent = red;
}

void SurfaceSettingsWidget::UpdateUI()
{
    Q_ASSERT( m_surface );
    ui->gaussianSmoothingCheckBox->setChecked(m_surface->GetGaussianSmoothingFlag());
    ui->radiusFactorLineEdit->setText(QString::number(m_surface->GetRadiusFactor()));
    ui->standardDeviationlineEdit->setText(QString::number(m_surface->GetStandardDeviation()));
    ui->contourValueLineEdit->setText(QString::number(m_contourValue));
    ui->polygonReductionSpinBox->setValue(m_reductionPercent);
    if (ui->histogramWidget->minSliderValue() != m_min)
        ui->histogramWidget->setMinSliderValue(m_min);
    if (ui->histogramWidget->maxSliderValue() != m_max)
        ui->histogramWidget->setMaxSliderValue(m_max);
}

void SurfaceSettingsWidget::UpdateSettings()
{
    this->UpdateUI();
}
