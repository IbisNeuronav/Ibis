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

#ifndef GPUVOLUMERECONSTRUCTIONAPITESTPLUGININTERFACE_H
#define GPUVOLUMERECONSTRUCTIONAPITESTPLUGININTERFACE_H

#include "toolplugininterface.h"

class GPUVolumeReconstructionAPITestPluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis._GPUVolumeReconstructionAPITestPluginInterface" )

public:
    vtkTypeMacro( GPUVolumeReconstructionAPITestPluginInterface, ToolPluginInterface );

    GPUVolumeReconstructionAPITestPluginInterface();
    ~GPUVolumeReconstructionAPITestPluginInterface();
    virtual QString GetPluginName() override { return QString( "GPUVolumeReconstructionAPITest" ); }
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString( "No GUI US Volume Reconstruction" ); }
    QWidget * CreateFloatingWidget() override;
};

#endif
