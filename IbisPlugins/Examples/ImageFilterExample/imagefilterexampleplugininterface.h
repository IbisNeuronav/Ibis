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

#ifndef __ImageFilterExamplePluginInterface_h_
#define __ImageFilterExamplePluginInterface_h_

#include "toolplugininterface.h"

class ImageFilterExampleWidget;

class ImageFilterExamplePluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.ImageFilterExamplePluginInterface" )

public:

    ImageFilterExamplePluginInterface();
    ~ImageFilterExamplePluginInterface();
    virtual QString GetPluginName() { return QString("ImageFilterExample"); }
    bool CanRun();
    QString GetMenuEntryString() { return QString("Image Filter Example"); }

    QWidget * CreateFloatingWidget();

};

#endif
