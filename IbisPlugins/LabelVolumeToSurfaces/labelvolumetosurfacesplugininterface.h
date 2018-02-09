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

#ifndef __LabelVolumeToSurfacesPluginInterface_h_
#define __LabelVolumeToSurfacesPluginInterface_h_

#include "generatorplugininterface.h"
#include "serializer.h"

class LabelVolumeToSurfacesPluginInterface : public GeneratorPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.LabelVolumeToSurfacesPluginInterface" )

public:

    LabelVolumeToSurfacesPluginInterface();
    ~LabelVolumeToSurfacesPluginInterface();

    // IbisPlugin interface
    QString GetPluginName() { return "LabelVolumeToSurfaces"; }

    // ObjectPluginInterface
    QString GetMenuEntryString() { return QString("Extract surfaces from label volume"); }
    bool CanRun();
    void Run();

};

#endif
