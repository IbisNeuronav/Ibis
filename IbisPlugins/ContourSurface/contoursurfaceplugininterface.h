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

class GeneratedSurface;

class ContourSurfacePluginInterface : public QObject, public ObjectPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.ContourSurfacePluginInterface" )

public:

    ContourSurfacePluginInterface();
    ~ContourSurfacePluginInterface();

    // IbisPlugin interface
    virtual QString GetPluginName() { return QString("GeneratedSurface"); }

    // ObjectPluginInterface
    QString GetMenuEntryString() { return QString("Contour Surface"); }
    SceneObject * CreateObject();
    virtual bool CanBeActivated();

protected:
    GeneratedSurface *m_generatedSurface;
};

#endif
