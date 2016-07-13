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

class ObjectPluginInterface : public IbisPlugin
{

public:

    vtkTypeMacro( ObjectPluginInterface, IbisPlugin );
    
    ObjectPluginInterface() {}
    virtual ~ObjectPluginInterface() {}

    // Implementation of IbisPlugin interface
    IbisPluginTypes GetPluginType() { return IbisPluginTypeObject; }
    
    virtual QString GetMenuEntryString() = 0;
    virtual SceneObject * CreateObject() = 0;
    virtual bool CanBeActivated() { return true; }
};

#endif
