/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "tractogramobjectsettingsdialog.h"
#include <QColorDialog>
#include <QButtonGroup>
#include <QtGui>
#include <QFileDialog>
#include <QDir>
#include <QRadioButton>
#include <vtkProperty.h>
#include "polydataobject.h"
#include "tractogramobject.h"
#include "imageobject.h"
#include "scenemanager.h"
#include "application.h"
#include "lookuptablemanager.h"

TractogramObjectSettingsDialog::TractogramObjectSettingsDialog( QWidget* parent, Qt::WindowFlags fl )
    : QWidget( parent, fl )
{
    setupUi(this);
    m_object = 0;
    this->vertexColorButtonGroup = new QButtonGroup;
    this->vertexColorButtonGroup->addButton( useDataScalarsRadioButton, 0 );
    this->vertexColorButtonGroup->addButton( sampleFromVolumeRadioButton, 1 );
    this->vertexColorButtonGroup->addButton( localColoringRadioButton, 2 );
    this->vertexColorButtonGroup->addButton( endPtsColoringRadioButton, 3 );
    QObject::connect( this->vertexColorButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT( VertexColorModeChanged(int)));
    this->displayModeButtonGroup = new QButtonGroup;
    this->displayModeButtonGroup->addButton(pointRadioButton, 0);
    this->displayModeButtonGroup->addButton(wireframeRadioButton, 1);
    this->displayModeButtonGroup->addButton(surfaceRadioButton, 2);
    QObject::connect(this->displayModeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(DisplayModeChanged(int)));
    QObject::connect(this->changeColorButton, SIGNAL(clicked()), this, SLOT(ColorSwatchClicked()));
    QObject::connect(this->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(OpacitySliderValueChanged(int)));
    QObject::connect(this->opacityEdit, SIGNAL(textChanged(QString)), this, SLOT(OpacityEditTextChanged(QString)));
    QObject::connect(this->crossSectionCheckBox, SIGNAL(toggled(bool)), this, SLOT(CrossSectionCheckBoxToggled(bool)));
}

TractogramObjectSettingsDialog::~TractogramObjectSettingsDialog()
{
}

#include <QGroupBox>

void TractogramObjectSettingsDialog::SetTractogramObject( TractogramObject * object )
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


void TractogramObjectSettingsDialog::OpacitySliderValueChanged( int value )
{
    double newOpacity = ((double)value) / 100.0;
    m_object->SetOpacity( newOpacity );
    this->UpdateOpacityUI();
}


void TractogramObjectSettingsDialog::OpacityEditTextChanged( const QString & text )
{
    double newOpacity = ((double)(text.toInt()))/100.0;
    m_object->SetOpacity( newOpacity );
    this->UpdateOpacityUI();
}

void TractogramObjectSettingsDialog::DisplayModeChanged( int mode )
{
    m_object->SetRenderingMode( mode );
}

void TractogramObjectSettingsDialog::UpdateSettings()
{
    this->UpdateUI();
}

void TractogramObjectSettingsDialog::UpdateUI()
{
    // Update color ui
    double * color = m_object->GetColor();
    QString styleColor = QString("background-color: rgb(%1,%2,%3);").arg( (int)(color[0] * 255) ).arg( (int)(color[1] * 255) ).arg( (int)(color[2] * 255) );
    QString style = QString("border-width: 2px; border-style: solid; border-radius: 7;" );
    styleColor += style;
    this->changeColorButton->setStyleSheet( styleColor );

    // update Scalar visibility ui
    bool showVertexColor = m_object->GetScalarsVisible() != 0 ? true : false;
    this->vertexColorGroupBox->setChecked( showVertexColor );
    this->useDataScalarsRadioButton->setEnabled( showVertexColor );
    this->sampleFromVolumeRadioButton->setEnabled( showVertexColor );
    this->localColoringRadioButton->setEnabled( showVertexColor );
    this->endPtsColoringRadioButton->setEnabled( showVertexColor );
    
    // Update vertex color mode ui
    vertexColorButtonGroup->blockSignals( true );
    int vertexColorMode = m_object->GetVertexColorMode();
    if( vertexColorMode == 0 )
        this->useDataScalarsRadioButton->setChecked(true);
    else if( vertexColorMode == 1 )
        this->sampleFromVolumeRadioButton->setChecked(true);
    else if( vertexColorMode == 2 )
        this->localColoringRadioButton->setChecked(true);
    else if( vertexColorMode == 3 )
        this->endPtsColoringRadioButton->setChecked(true);
    vertexColorButtonGroup->blockSignals( false );

    // Update list of luts
    this->lutComboBox->blockSignals( true );
    this->lutComboBox->clear();
    int nbLuts = Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables();
    for( int i = 0; i < nbLuts; ++i )
        this->lutComboBox->addItem( Application::GetLookupTableManager()->GetTemplateLookupTableName( i ) );
    if( nbLuts > 0 )
        this->lutComboBox->setCurrentIndex( m_object->GetLutIndex() );
    this->lutComboBox->blockSignals( false );

    // Update sample volume checkbox
    this->sampleVolumeComboBox->blockSignals( true );
    this->sampleVolumeComboBox->clear();
    QList< ImageObject* > imObjects;
    m_object->GetManager()->GetAllImageObjects( imObjects );
    int currentIndex = -1;
    for( int i = 0; i < imObjects.size(); ++i )
    {
        ImageObject * current = imObjects[i];
        this->sampleVolumeComboBox->addItem( current->GetName(), QVariant( current->GetObjectID() ) );
        if( current == m_object->GetScalarSource() )
            currentIndex = i;
    }
    this->sampleVolumeComboBox->addItem( "None", QVariant( (int)-1 ) );
    if( currentIndex == -1 )
        this->sampleVolumeComboBox->setCurrentIndex( imObjects.size() );
    else
        this->sampleVolumeComboBox->setCurrentIndex( currentIndex );
    this->sampleVolumeComboBox->blockSignals( false );
    
    // Update render mode radio group
    int renderMode = m_object->GetRenderingMode();
    if( renderMode == 2 )
        this->surfaceRadioButton->setChecked(true);
    else if( renderMode == 0 )
        this->pointRadioButton->setChecked(true);
	else
        this->wireframeRadioButton->setChecked(true);

    this->crossSectionCheckBox->blockSignals(true);
    this->crossSectionCheckBox->setChecked(m_object->GetCrossSectionVisible());
    this->crossSectionCheckBox->blockSignals(false);

    clippingGroupBox->blockSignals( true );
    clippingGroupBox->setChecked( m_object->IsClippingEnabled() );
    clippingGroupBox->blockSignals( false );

    xpRadioButton->blockSignals( true );
    xmRadioButton->blockSignals( true );
    xpRadioButton->setChecked( m_object->GetClippingPlanesOrientation(0) );
    xmRadioButton->setChecked( !m_object->GetClippingPlanesOrientation(0) );
    xmRadioButton->blockSignals( false );
    xpRadioButton->blockSignals( false );

    ypRadioButton->blockSignals( true );
    ymRadioButton->blockSignals( true );
    ypRadioButton->setChecked( m_object->GetClippingPlanesOrientation(1) );
    ymRadioButton->setChecked( !m_object->GetClippingPlanesOrientation(1) );
    ymRadioButton->blockSignals( false );
    ypRadioButton->blockSignals( false );

    zpRadioButton->blockSignals( true );
    zmRadioButton->blockSignals( true );
    zpRadioButton->setChecked( m_object->GetClippingPlanesOrientation(2) );
    zmRadioButton->setChecked( !m_object->GetClippingPlanesOrientation(2) );
    zmRadioButton->blockSignals( false );
    zpRadioButton->blockSignals( false );

    this->UpdateOpacityUI();
}

void TractogramObjectSettingsDialog::UpdateOpacityUI()
{
    this->opacitySlider->blockSignals( true );
    this->opacityEdit->blockSignals( true );
    double opacity = m_object->GetOpacity();
    this->opacitySlider->setValue( (int)( opacity * 100 ) );
    this->opacityEdit->setText( QString::number( (int)(opacity * 100 )) );
    this->opacitySlider->blockSignals( false );
    this->opacityEdit->blockSignals( false );
}

void TractogramObjectSettingsDialog::ColorSwatchClicked()
{
    double * oldColor = m_object->GetColor();
    QColor initial( (int)(oldColor[0] * 255), (int)(oldColor[1] * 255), (int)(oldColor[2] * 255) );
    QColor newColor = QColorDialog::getColor( initial );
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

void TractogramObjectSettingsDialog::VertexColorModeChanged( int id )
{
    m_object->SetVertexColorMode( id );
}

void TractogramObjectSettingsDialog::CrossSectionCheckBoxToggled(bool showCrossSection)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_object->SetCrossSectionVisible(showCrossSection);
    QApplication::restoreOverrideCursor();
}

void TractogramObjectSettingsDialog::on_sampleVolumeComboBox_currentIndexChanged( int index )
{
    int objectId = this->sampleVolumeComboBox->itemData( index ).toInt();
    if( objectId == SceneManager::InvalidId )
        m_object->SetScalarSource( 0 );
    else
    {
        SceneObject * obj = m_object->GetManager()->GetObjectByID( objectId );
        ImageObject * im = ImageObject::SafeDownCast( obj );
        m_object->SetScalarSource( im );
    }
}

void TractogramObjectSettingsDialog::on_vertexColorGroupBox_toggled( bool checked )
{
    m_object->SetScalarsVisible( checked ? 1 : 0 );
    UpdateUI();
}

void TractogramObjectSettingsDialog::on_lutComboBox_currentIndexChanged( int index )
{
    m_object->SetLutIndex( index );
}

void TractogramObjectSettingsDialog::on_xpRadioButton_toggled(bool checked)
{
    m_object->SetClippingPlanesOrientation( 0, xpRadioButton->isChecked() );
}

void TractogramObjectSettingsDialog::on_ypRadioButton_toggled(bool checked)
{
    m_object->SetClippingPlanesOrientation( 1, ypRadioButton->isChecked() );
}

void TractogramObjectSettingsDialog::on_zpRadioButton_toggled(bool checked)
{
    m_object->SetClippingPlanesOrientation( 2, zpRadioButton->isChecked() );
}

void TractogramObjectSettingsDialog::on_clippingGroupBox_toggled(bool arg1)
{
    m_object->SetClippingEnabled( arg1 );
}
