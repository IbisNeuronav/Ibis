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

#ifndef IMAGEFILTEREXAMPLEPLUGININTERFACE_H
#define IMAGEFILTEREXAMPLEPLUGININTERFACE_H

#include "toolplugininterface.h"

class ImageFilterExampleWidget;

class ImageFilterExamplePluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.ImageFilterExamplePluginInterface" )

public:
    ImageFilterExamplePluginInterface();
    ~ImageFilterExamplePluginInterface();
    virtual QString GetPluginName() override { return QString( "ImageFilterExample" ); }
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString( "Image Filter Example" ); }

    QWidget * CreateFloatingWidget() override;
};

#endif
