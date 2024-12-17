/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef GLOBALOBJECTPLUGININTERFACE_H
#define GLOBALOBJECTPLUGININTERFACE_H

#include <QObject>
#include <QString>

#include "ibisplugin.h"

class SceneObject;
class QSettings;

/**
 * @class   GlobalObjectPluginInterface
 * @brief   Interface for a type of plugin that generates a singleton SceneObject
 *
 * Generates SceneObjects for which there is only one global instance.
 * @sa IbisPlugin
 */
class GlobalObjectPluginInterface : public IbisPlugin
{
public:
    vtkTypeMacro( GlobalObjectPluginInterface, IbisPlugin );

    GlobalObjectPluginInterface() {}
    virtual ~GlobalObjectPluginInterface() {}

    /** Implementation of IbisPlugin interface */
    IbisPluginTypes GetPluginType() override { return IbisPluginTypeGlobalObject; }

    virtual SceneObject * GetGlobalObjectInstance() = 0;
};

#endif
