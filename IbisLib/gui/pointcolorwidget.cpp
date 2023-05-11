/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "pointcolorwidget.h"

#include <QColorDialog>
#include <QLabel>
#include <QSlider>

#include "ui_pointcolorwidget.h"

PointColorWidget::PointColorWidget( QWidget * parent )
    : QWidget( parent ), m_points( 0 ), ui( new Ui::PointColorWidget )
{
    ui->setupUi( this );
    QObject::connect( ui->opacitySlider, SIGNAL( valueChanged( int ) ), this,
                      SLOT( OpacitySliderValueChanged( int ) ) );
    QObject::connect( ui->opacityEdit, SIGNAL( textChanged( QString ) ), this,
                      SLOT( OpacityTextEditChanged( QString ) ) );
    QObject::connect( ui->disabledSetColorButton, SIGNAL( clicked() ), this, SLOT( DisabledColorSetButtonClicked() ) );
    QObject::connect( ui->enabledSetColorButton, SIGNAL( clicked() ), this, SLOT( EnabledColorSetButtonClicked() ) );
    QObject::connect( ui->selectedSetColorButton, SIGNAL( clicked() ), this, SLOT( SelectedColorSetButtonClicked() ) );
    QObject::connect( ui->lineSetColorButton, SIGNAL( clicked() ), this, SLOT( LineColorSetButtonClicked() ) );
}

PointColorWidget::~PointColorWidget()
{
    if( m_points )
    {
        this->disconnect();
    }
    m_points = 0;
    delete ui;
}

void PointColorWidget::SetPointsObject( PointsObject * obj )
{
    if( m_points == obj )
    {
        return;
    }

    if( m_points )
    {
        this->disconnect();
    }

    m_points = obj;

    this->UpdateUI();
}

void PointColorWidget::OpacitySliderValueChanged( int value )
{
    double newOpacity = ( (double)value ) / 100.0;
    m_points->SetOpacity( newOpacity );
    this->UpdateOpacityUI();
}

void PointColorWidget::OpacityTextEditChanged( const QString & text )
{
    double newOpacity = text.toDouble();
    m_points->SetOpacity( newOpacity );
    this->UpdateOpacityUI();
}

void PointColorWidget::UpdateUI()
{
    if( m_points )
    {
        double * color = m_points->GetDisabledColor();
        QColor disabled( (int)( color[ 0 ] * 255 ), (int)( color[ 1 ] * 255 ), (int)( color[ 2 ] * 255 ) );
        QString styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                                 .arg( (int)( color[ 0 ] * 255 ) )
                                 .arg( (int)( color[ 1 ] * 255 ) )
                                 .arg( (int)( color[ 2 ] * 255 ) );
        QString style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
        styleColor += style;
        ui->disabledColorSwatch->setStyleSheet( styleColor );
        color = m_points->GetSelectedColor();
        QColor selected( (int)( color[ 0 ] * 255 ), (int)( color[ 1 ] * 255 ), (int)( color[ 2 ] * 255 ) );
        styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                         .arg( (int)( color[ 0 ] * 255 ) )
                         .arg( (int)( color[ 1 ] * 255 ) )
                         .arg( (int)( color[ 2 ] * 255 ) );
        style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
        styleColor += style;
        ui->selectedColorSwatch->setStyleSheet( styleColor );
        color = m_points->GetEnabledColor();
        QColor enabled( (int)( color[ 0 ] * 255 ), (int)( color[ 1 ] * 255 ), (int)( color[ 2 ] * 255 ) );
        styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                         .arg( (int)( color[ 0 ] * 255 ) )
                         .arg( (int)( color[ 1 ] * 255 ) )
                         .arg( (int)( color[ 2 ] * 255 ) );
        style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
        styleColor += style;
        ui->enabledColorSwatch->setStyleSheet( styleColor );

        color = m_points->GetLineToPointerColor();
        QColor lineColor( (int)( color[ 0 ] * 255 ), (int)( color[ 1 ] * 255 ), (int)( color[ 2 ] * 255 ) );
        styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                         .arg( (int)( color[ 0 ] * 255 ) )
                         .arg( (int)( color[ 1 ] * 255 ) )
                         .arg( (int)( color[ 2 ] * 255 ) );
        style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
        styleColor += style;
        ui->lineColorSwatch->setStyleSheet( styleColor );

        this->UpdateOpacityUI();
    }
}
void PointColorWidget::UpdateOpacityUI()
{
    ui->opacitySlider->blockSignals( true );
    ui->opacityEdit->blockSignals( true );
    double opacity = m_points->GetOpacity();
    ui->opacitySlider->setValue( (int)( opacity * 100 ) );
    ui->opacityEdit->setText( QString::number( opacity, 'g', 2 ) );
    ui->opacitySlider->blockSignals( false );
    ui->opacityEdit->blockSignals( false );
}

void PointColorWidget::DisabledColorSetButtonClicked()
{
    double * oldColor = m_points->GetDisabledColor();
    QColor initial( (int)( oldColor[ 0 ] * 255 ), (int)( oldColor[ 1 ] * 255 ), (int)( oldColor[ 2 ] * 255 ) );
    QColor newColor =
        QColorDialog::getColor( initial, nullptr, tr( "Choose Color" ), QColorDialog::DontUseNativeDialog );
    QString styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                             .arg( newColor.red() )
                             .arg( newColor.green() )
                             .arg( newColor.blue() );
    QString style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
    styleColor += style;
    ui->disabledColorSwatch->setStyleSheet( styleColor );
    double newColorfloat[ 3 ] = { 1, 1, 1 };
    newColorfloat[ 0 ]        = double( newColor.red() ) / 255.0;
    newColorfloat[ 1 ]        = double( newColor.green() ) / 255.0;
    newColorfloat[ 2 ]        = double( newColor.blue() ) / 255.0;
    m_points->SetDisabledColor( newColorfloat );
}

void PointColorWidget::EnabledColorSetButtonClicked()
{
    double * oldColor = m_points->GetEnabledColor();
    QColor initial( (int)( oldColor[ 0 ] * 255 ), (int)( oldColor[ 1 ] * 255 ), (int)( oldColor[ 2 ] * 255 ) );
    QColor newColor =
        QColorDialog::getColor( initial, nullptr, tr( "Choose Color" ), QColorDialog::DontUseNativeDialog );
    QString styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                             .arg( newColor.red() )
                             .arg( newColor.green() )
                             .arg( newColor.blue() );
    QString style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
    styleColor += style;
    ui->enabledColorSwatch->setStyleSheet( styleColor );
    double newColorfloat[ 3 ] = { 1, 1, 1 };
    newColorfloat[ 0 ]        = double( newColor.red() ) / 255.0;
    newColorfloat[ 1 ]        = double( newColor.green() ) / 255.0;
    newColorfloat[ 2 ]        = double( newColor.blue() ) / 255.0;
    m_points->SetEnabledColor( newColorfloat );
}

void PointColorWidget::SelectedColorSetButtonClicked()
{
    double * oldColor = m_points->GetSelectedColor();
    QColor initial( (int)( oldColor[ 0 ] * 255 ), (int)( oldColor[ 1 ] * 255 ), (int)( oldColor[ 2 ] * 255 ) );
    QColor newColor =
        QColorDialog::getColor( initial, nullptr, tr( "Choose Color" ), QColorDialog::DontUseNativeDialog );
    QString styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                             .arg( newColor.red() )
                             .arg( newColor.green() )
                             .arg( newColor.blue() );
    QString style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
    styleColor += style;
    ui->selectedColorSwatch->setStyleSheet( styleColor );
    double newColorfloat[ 3 ] = { 1, 1, 1 };
    newColorfloat[ 0 ]        = double( newColor.red() ) / 255.0;
    newColorfloat[ 1 ]        = double( newColor.green() ) / 255.0;
    newColorfloat[ 2 ]        = double( newColor.blue() ) / 255.0;
    m_points->SetSelectedColor( newColorfloat );
}

void PointColorWidget::LineColorSetButtonClicked()
{
    double * oldColor = m_points->GetLineToPointerColor();
    QColor initial( (int)( oldColor[ 0 ] * 255 ), (int)( oldColor[ 1 ] * 255 ), (int)( oldColor[ 2 ] * 255 ) );
    QColor newColor =
        QColorDialog::getColor( initial, nullptr, tr( "Choose Color" ), QColorDialog::DontUseNativeDialog );
    QString styleColor = QString( "background-color: rgb(%1,%2,%3);" )
                             .arg( newColor.red() )
                             .arg( newColor.green() )
                             .arg( newColor.blue() );
    QString style = QString( "border-width: 2px; border-style: solid; border-radius: 7;" );
    styleColor += style;
    ui->lineColorSwatch->setStyleSheet( styleColor );
    double newColorfloat[ 3 ] = { 1, 1, 1 };
    newColorfloat[ 0 ]        = double( newColor.red() ) / 255.0;
    newColorfloat[ 1 ]        = double( newColor.green() ) / 255.0;
    newColorfloat[ 2 ]        = double( newColor.blue() ) / 255.0;
    m_points->SetLineToPointerColor( newColorfloat );
}
