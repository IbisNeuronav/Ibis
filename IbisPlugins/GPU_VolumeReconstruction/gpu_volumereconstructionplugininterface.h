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

#ifndef GPU_VOLUMERECONSTRUCTIONPLUGININTERFACE_H
#define GPU_VOLUMERECONSTRUCTIONPLUGININTERFACE_H

#include "toolplugininterface.h"

class GPU_VolumeReconstructionWidget;

class GPU_VolumeReconstructionPluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.GPU_VolumeReconstructionPluginInterface" )

public:
    vtkTypeMacro( GPU_VolumeReconstructionPluginInterface, ToolPluginInterface );

    GPU_VolumeReconstructionPluginInterface();
    ~GPU_VolumeReconstructionPluginInterface();
    virtual QString GetPluginName() override { return QString( "GPU_VolumeReconstruction" ); }
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString( "US Volume Reconstruction With GPU" ); }

    QWidget * CreateFloatingWidget() override;

protected:
    GPU_VolumeReconstructionWidget * m_volumeReconstructionWidget;
};

#endif
