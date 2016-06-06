/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef CONTOURSURFACEPLUGININTERFACE_H
#define CONTOURSURFACEPLUGININTERFACE_H

#include <QObject>
#include "objectplugininterface.h"

class ContourSurfacePluginInterface : public QObject, public ObjectPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(ObjectPluginInterface)
    Q_PLUGIN_METADATA(IID "Ibis.ContourSurfacePluginInterface" )

public:

    ContourSurfacePluginInterface();
    ~ContourSurfacePluginInterface();
    QString GetMenuEntryString() { return QString("Contour Surface"); }
    void CreateObject();
    QString GetObjectClassName() { return "GeneratedSurface"; }
};

#endif
