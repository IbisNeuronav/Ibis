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

#include <QtPlugin>
#include "gpu_rigidregistrationplugininterface.h"
#include "gpu_rigidregistrationwidget.h"

GPU_RigidRegistrationPluginInterface::GPU_RigidRegistrationPluginInterface() {}

GPU_RigidRegistrationPluginInterface::~GPU_RigidRegistrationPluginInterface() {}

bool GPU_RigidRegistrationPluginInterface::CanRun() { return true; }

QWidget * GPU_RigidRegistrationPluginInterface::CreateFloatingWidget()
{
    GPU_RigidRegistrationWidget * widget = new GPU_RigidRegistrationWidget;
    widget->SetPluginInterface( this );
    return widget;
}
