/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "imageobjectsettingsdialog.h"

#include "imageobject.h"
#include "application.h"
#include "lookuptablemanager.h"
#include "vtkScalarsToColors.h"
#include "vtkPiecewiseFunctionLookupTable.h"

ImageObjectSettingsDialog::ImageObjectSettingsDialog( QWidget* parent, Qt::WindowFlags fl )
    : QWidget( parent, fl )
    , m_imageObject(0)
    , m_rangeSlider(100)
{
    setupUi(this);
    QObject::connect(selectColorTableComboBox, SIGNAL(activated(int)), this, SLOT(SelectColorTableComboBoxActivated(int)));
    QObject::connect(showBoundingBoxCheckBox, SIGNAL(toggled(bool)), this, SLOT(ViewBoundingBoxCheckboxToggled(bool)));
    QObject::connect(histogramWidget, SIGNAL(slidersValueChanged(double,double)), this, SLOT(RangeSlidersValuesChanged(double,double)));
}

ImageObjectSettingsDialog::~ImageObjectSettingsDialog()
{
    if (m_imageObject)
    {
        m_imageObject->UnRegister( 0 );
    }
}

void ImageObjectSettingsDialog::languageChange()
{
    retranslateUi(this);
}

void ImageObjectSettingsDialog::SetImageObject( ImageObject * obj )
{
    if( obj == m_imageObject )
    {
        return;
    }
    
    if( m_imageObject )
    {
        m_imageObject->UnRegister( 0 );
    }
    
    m_imageObject = obj;
    
    if( m_imageObject )
    {
        m_imageObject->Register( 0 );

        if( m_imageObject->IsLabelImage() )
        {
            selectColorTableComboBox->setHidden( true );
            selectColorTableTextLabel->setHidden( true );
            histogramWidget->setHidden( true );
        }
        else
        {
            // Setup color table combo box
            for (int i=0; i < Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables(); i++)
                selectColorTableComboBox->addItem(Application::GetLookupTableManager()->GetTemplateLookupTableName(i));
            selectColorTableComboBox->setCurrentIndex(m_imageObject->GetLutIndex());

            // Setup Histogram widget
            histogramWidget->SetHistogram( m_imageObject->GetHistogramComputer() );
            double imageRange[2];
            m_imageObject->GetImageScalarRange( imageRange );
            histogramWidget->SetImageRange( imageRange[0], imageRange[1] );
            double * range = m_imageObject->GetLutRange();
            double min = ( range[0] - imageRange[0] ) / ( imageRange[1] - imageRange[0] );
            double max = ( range[1] - imageRange[0] ) / ( imageRange[1] - imageRange[0] );
            histogramWidget->setMinSliderValue( min );
            histogramWidget->setMaxSliderValue( max );
        }

        this->UpdateUI();

        connect( m_imageObject, SIGNAL(Modified()), this, SLOT(UpdateUI()) );
   }
}

void ImageObjectSettingsDialog::UpdateUI()
{
    if( m_imageObject )
    {
        this->AllControlsBlockSignals(true);

        // Colors maps
        selectColorTableComboBox->setCurrentIndex(m_imageObject->GetLutIndex());
        showBoundingBoxCheckBox->setChecked( (bool)m_imageObject->GetViewOutline() );

        // Histogram widget
        vtkPiecewiseFunctionLookupTable * lut = vtkPiecewiseFunctionLookupTable::SafeDownCast( m_imageObject->GetLut() );
        if( lut )
            histogramWidget->SetColorTransferFunction( lut->GetColorFunction() );

        this->AllControlsBlockSignals(false);
    }
    else
    {
        this->setEnabled( false );
    }
}

void ImageObjectSettingsDialog::SelectColorTableComboBoxActivated(int index)
{
    Q_ASSERT(m_imageObject);
    m_imageObject->ChooseColorTable(index);
    this->UpdateUI();
}

void ImageObjectSettingsDialog::RangeSlidersValuesChanged( double min, double max )
{
    Q_ASSERT(m_imageObject);

    double imageRange[2];
    m_imageObject->GetImageScalarRange( imageRange );

    double newRange[2];
    newRange[0] = min * ( imageRange[1] - imageRange[0] ) + imageRange[0];
    newRange[1] = max * ( imageRange[1] - imageRange[0] ) + imageRange[0];
    m_imageObject->SetLutRange( newRange );
    this->UpdateUI();
}

void ImageObjectSettingsDialog::ViewBoundingBoxCheckboxToggled( bool on )
{
    if( on )
        m_imageObject->SetViewOutline( 1 );
    else
        m_imageObject->SetViewOutline( 0 );
    this->UpdateUI();
}

void ImageObjectSettingsDialog::AllControlsBlockSignals(bool yes)
{
    showBoundingBoxCheckBox->blockSignals(yes);
}
