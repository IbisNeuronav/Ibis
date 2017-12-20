/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Dante De Nigris for writing this class
#ifndef __GPU_RigidRegistrationPluginInterface_h_
#define __GPU_RigidRegistrationPluginInterface_h_

#include "toolplugininterface.h"

class GPU_RigidRegistrationWidget;

class GPU_RigidRegistrationPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.GPU_RigidRegistrationPluginInterface" )

public:

    vtkTypeMacro( GPU_RigidRegistrationPluginInterface, ToolPluginInterface );

    GPU_RigidRegistrationPluginInterface();
    ~GPU_RigidRegistrationPluginInterface();
    virtual QString GetPluginName() { return QString("GPU_RigidRegistration"); }
    bool CanRun();
    QString GetMenuEntryString() { return QString("Rigid Registration With GPU"); }

    QWidget * CreateFloatingWidget();

};

#endif
