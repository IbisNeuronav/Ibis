/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "videosettingsdialog.h"

#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qtextedit.h>
#include <qgroupbox.h>
#include <qspinbox.h>
#include <qsplitter.h>
#include <qlayout.h>
#include "videoviewdialog.h"
#include "automaticgui.h"
#include "vtkVideoSource2.h"
#include "trackedvideosource.h"

VideoSettingsDialog::VideoSettingsDialog( QWidget* parent )
    : QWidget(parent)
{
    setupUi(this);
    setWindowTitle("Video Settings");
    
    m_automaticGui = 0;
    m_source = 0;
    m_outputComboBox->addItem("Grayscale");
    m_outputComboBox->addItem("RGB");
}

VideoSettingsDialog::~VideoSettingsDialog()
{
    if(m_source)
        m_source->RemoveClient();
}

void VideoSettingsDialog::Update()
{
    // Time shift
    timeShiftSpinBox->setValue( m_source->GetTimeShift() * 1000.0 );

    // Output format
    if( m_source->OutputFormatIsLuminance() )
        m_outputComboBox->setCurrentIndex( 0 );
    else
        m_outputComboBox->setCurrentIndex( 1 );

    // Device type
    m_deviceTypeComboBox->clear();
    for( int i = 0; i < m_source->GetNumberOfDeviceTypes(); ++i )
    {
        m_deviceTypeComboBox->addItem( m_source->GetDeviceTypeName( i ) );
    }
    m_deviceTypeComboBox->setCurrentIndex( m_source->GetCurrentDeviceTypeIndex() );

    // Auto-generated settings based on device type
    if( m_automaticGui )
        delete m_automaticGui;
    m_automaticGui = new AutomaticGui( m_source->GetSource(), this );
    m_settingsLayout->addWidget( m_automaticGui );
}

void VideoSettingsDialog::SetSource( TrackedVideoSource * source )
{
    if( source != m_source )
    {
        if (m_source)
            m_source->RemoveClient();
        m_source = source;
        QWidget * videoView = m_source->CreateVideoViewDialog( this );
        this->layout()->addWidget( videoView );
        m_source->AddClient();
        Update();
    }
}

void VideoSettingsDialog::VideoOutputChanged( int outputType )
{
    if( outputType == 0 && !m_source->OutputFormatIsLuminance() )
        m_source->SetOutputFormatToLuminance();
    else if( outputType == 1 && !m_source->OutputFormatIsRGB() )
        m_source->SetOutputFormatToRGB();
}

void VideoSettingsDialog::on_m_deviceTypeComboBox_activated( int index )
{
    QString newType = m_deviceTypeComboBox->itemText( index );
    m_source->SetCurrentDeviceType( newType );
    m_source->AddClient();
    Update();
}

void VideoSettingsDialog::on_timeShiftSpinBox_valueChanged(double arg1)
{
    m_source->SetTimeShift( arg1 / 1000.00 );
}
