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

#include "objectplugininterface.h"
#include "vtkSmartPointer.h"

class GeneratedSurface;

class ContourSurfacePluginInterface : public ObjectPluginInterface
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
    vtkSmartPointer<GeneratedSurface> m_generatedSurface;
};

#endif
