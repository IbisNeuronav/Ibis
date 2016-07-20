/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __GeneratorPluginInterface_h_
#define __GeneratorPluginInterface_h_

#include <QString>
#include <QObject>
#include "ibisplugin.h"

class SceneObject;
class QSettings;

// Description:
// Interface for a type of plugin that procedurally generates scene objects
class GeneratorPluginInterface : public IbisPlugin
{

public:

    vtkTypeMacro( GeneratorPluginInterface, IbisPlugin );

    GeneratorPluginInterface() {}
    virtual ~GeneratorPluginInterface() {}

    // Implementation of IbisPlugin interface
    IbisPluginTypes GetPluginType() { return IbisPluginTypeGenerator; }

    // Definition of the generator interface
    virtual QString GetMenuEntryString() = 0;
    virtual bool CanRun() { return true; }
    virtual void Run() = 0;

};


#endif
