/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "pointcloudobjectsettingsdialog.h"
#include <QColorDialog>
#include <QButtonGroup>
#include <QtGui>
#include <QFileDialog>
#include <QDir>
#include <vtkProperty.h>
#include "pointcloudobject.h"
#include "imageobject.h"
#include "scenemanager.h"
#include "application.h"

PointCloudObjectSettingsDialog::PointCloudObjectSettingsDialog( QWidget* parent, Qt::WindowFlags fl )
    : QWidget( parent, fl )
{
    setupUi(this);
    m_object = 0;
    QObject::connect(this->changeColorButton, SIGNAL(clicked()), this, SLOT(ColorSwatchClicked()));
    QObject::connect(this->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(OpacitySliderValueChanged(int)));
    QObject::connect(this->opacityEdit, SIGNAL(textChanged(QString)), this, SLOT(OpacityEditTextChanged(QString)));
}

PointCloudObjectSettingsDialog::~PointCloudObjectSettingsDialog()
{
}

void PointCloudObjectSettingsDialog::SetPointCloudObject( PointCloudObject * object )
{
    if( object == m_object )
    {
        return;
    }
    
    if( m_object )
    {
        disconnect( m_object, SIGNAL(ObjectModified()), this, SLOT(UpdateSettings()) );
    }
    
    m_object = object;
    
    if( m_object )
    {
        connect( m_object, SIGNAL(ObjectModified()), this, SLOT(UpdateSettings()) );
    }
    
    this->UpdateUI();
}


void PointCloudObjectSettingsDialog::OpacitySliderValueChanged( int value )
{
    double newOpacity = ((double)value) / 100.0;
    m_object->SetOpacity( newOpacity );
    this->UpdateOpacityUI();
}


void PointCloudObjectSettingsDialog::OpacityEditTextChanged( const QString & text )
{
    double newOpacity = ((double)(text.toInt()))/100.0;
    m_object->SetOpacity( newOpacity );
    this->UpdateOpacityUI();
}


void PointCloudObjectSettingsDialog::UpdateSettings()
{
    this->UpdateUI();
}

void PointCloudObjectSettingsDialog::UpdateUI()
{
    // Update color ui
    double * color = m_object->GetProperty()->GetColor();
    QString styleColor = QString("background-color: rgb(%1,%2,%3);").arg( (int)(color[0] * 255) ).arg( (int)(color[1] * 255) ).arg( (int)(color[2] * 255) );
    QString style = QString("border-width: 2px; border-style: solid; border-radius: 7; border-color: black;" );
    styleColor += style;
    this->changeColorButton->setStyleSheet( styleColor );

    this->UpdateOpacityUI();
}

void PointCloudObjectSettingsDialog::UpdateOpacityUI()
{
    this->opacitySlider->blockSignals( true );
    this->opacityEdit->blockSignals( true );
    double opacity = m_object->GetOpacity();
    this->opacitySlider->setValue( (int)( opacity * 100 ) );
    this->opacityEdit->setText( QString::number( (int)(opacity * 100 )) );
    this->opacitySlider->blockSignals( false );
    this->opacityEdit->blockSignals( false );
}

void PointCloudObjectSettingsDialog::ColorSwatchClicked()
{
    double * oldColor = m_object->GetProperty()->GetColor();
    QColor initial( (int)(oldColor[0] * 255), (int)(oldColor[1] * 255), (int)(oldColor[2] * 255) );
    QColor newColor = QColorDialog::getColor( initial, nullptr, tr("Choose Color"),  QColorDialog::DontUseNativeDialog );
    if( newColor.isValid() )
    {
        double newColorfloat[3] = { 1, 1, 1 };
        newColorfloat[0] = double( newColor.red() ) / 255.0;
        newColorfloat[1] = double( newColor.green() ) / 255.0;
        newColorfloat[2] = double( newColor.blue() ) / 255.0;
        m_object->SetColor( newColorfloat );

        UpdateUI();
    }
}



