/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "imagefilterexampleplugininterface.h"
#include "imagefilterexamplewidget.h"
#include <QtPlugin>

ImageFilterExamplePluginInterface::ImageFilterExamplePluginInterface()
{
}

ImageFilterExamplePluginInterface::~ImageFilterExamplePluginInterface()
{
}

bool ImageFilterExamplePluginInterface::CanRun()
{
    return true;
}

QWidget * ImageFilterExamplePluginInterface::CreateFloatingWidget()
{
    ImageFilterExampleWidget * widget = new ImageFilterExampleWidget;
    widget->SetPluginInterface( this );
    return widget;
}

