/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "mincinfowidget.h"
#include <qtextedit.h>

MincInfoWidget::MincInfoWidget( QWidget* parent, const char* name )
    : QWidget( parent )
{
    setupUi( this );
    setWindowTitle( "Minc Header" );
}

MincInfoWidget::~MincInfoWidget()
{
}

void MincInfoWidget::SetInfoText(QString info)
{
    mincInfoTextEdit->setText(info);
}

