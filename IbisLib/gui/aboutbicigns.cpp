/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "aboutbicigns.h"

#include <QLabel>

#include "ui_aboutbicigns.h"

AboutBICIgns::AboutBICIgns( QWidget * parent, const char * name ) : QDialog( parent ), ui( new Ui::AboutBICIgns )
{
    ui->setupUi( this );
}

AboutBICIgns::~AboutBICIgns() {}

void AboutBICIgns::Initialize( QString appName, QString version, QString buildDate )
{
    if( appName.isNull() || appName.isEmpty() )
        ui->applicationLabel->setText( "Intraoperative Brain Imaging System" );
    else
        ui->applicationLabel->setText( appName );
    ui->versionLabel->setText( version );
    ui->dateLabel->setText( buildDate );
}
