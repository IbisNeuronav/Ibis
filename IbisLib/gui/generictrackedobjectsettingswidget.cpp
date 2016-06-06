/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "generictrackedobjectsettingswidget.h"
#include "ui_generictrackedobjectsettingswidget.h"
#include "generictrackedobject.h"

GenericTrackedObjectSettingsWidget::GenericTrackedObjectSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GenericTrackedObjectSettingsWidget)
{
    ui->setupUi(this);
}

GenericTrackedObjectSettingsWidget::~GenericTrackedObjectSettingsWidget()
{
    delete ui;
}

void GenericTrackedObjectSettingsWidget::SetGenericTrackedObject( GenericTrackedObject *obj )
{
    m_genericObject = obj;
    connect( m_genericObject, SIGNAL( StatusChanged() ), this, SLOT( UpdateToolStatus() ), Qt::QueuedConnection );

    this->UpdateToolStatus();
}

void GenericTrackedObjectSettingsWidget::UpdateToolStatus()
{
    Q_ASSERT_X( m_genericObject, "GenericTrackedObjectSettingsWidget::UpdateToolStatus", "GenericTrackedObject not set" );
    TrackerToolState newState = m_genericObject->GetToolState( );
    if( newState != m_previousStatus )
    {
        switch( newState )
        {
        case Ok:
            ui->statusLabel->setText( "OK" );
            ui->statusLabel->setStyleSheet( "background-color: lightGreen" );
            break;
        case Missing:
            ui->statusLabel->setText( "Missing" );
            ui->statusLabel->setStyleSheet( "background-color: red" );
            break;
        case OutOfVolume:
            ui->statusLabel->setText( "Out of volume" );
            ui->statusLabel->setStyleSheet( "background-color: yellow" );
            break;
        case OutOfView:
            ui->statusLabel->setText( "Out of view" );
            ui->statusLabel->setStyleSheet( "background-color: red" );
            break;
        case Undefined:
            ui->statusLabel->setText( "Marker not initialized" );
            ui->statusLabel->setStyleSheet( "background-color: red" );
            break;
        }
    }
    m_previousStatus = newState;
}
