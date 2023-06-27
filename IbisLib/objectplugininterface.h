/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef OBJECTPLUGININTERFACE_h_
#define OBJECTPLUGININTERFACE_h_

#include <QString>

#include "ibisplugin.h"

class SceneObject;
class Serializer;

/**
 * @class   ObjectPluginInterface
 * @brief   Create a new object
 *
 * Create an object which must be derived from a SceneObject
 *
 * @sa IbisPlugin, SceneObject
 */
class ObjectPluginInterface : public IbisPlugin
{
public:
    vtkTypeMacro( ObjectPluginInterface, IbisPlugin );

    ObjectPluginInterface() {}
    virtual ~ObjectPluginInterface() {}

    /** Implementation of IbisPlugin interface. */
    IbisPluginTypes GetPluginType() override { return IbisPluginTypeObject; }

    /** @name Function that should be overriden in plugins.
     */
    ///@{
    /** Set a plugin name that will be displayed in the Plugins menu. */
    virtual QString GetMenuEntryString() = 0;
    /** Create the new object derived from SceneObject. */
    virtual SceneObject * CreateObject() = 0;
    ///@}
    /** Check if it is possible to create the object. */
    virtual bool CanBeActivated() { return true; }
};

#endif
