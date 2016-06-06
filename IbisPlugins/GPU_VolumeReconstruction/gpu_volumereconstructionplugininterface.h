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

#ifndef __GPU_VolumeReconstructionPluginInterfacee_h_
#define __GPU_VolumeReconstructionPluginInterface_h_

#include <QObject>
#include "toolplugininterface.h"

class GPU_VolumeReconstructionWidget;

class GPU_VolumeReconstructionPluginInterface : public QObject, public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(ToolPluginInterface)
    Q_PLUGIN_METADATA(IID "Ibis.GPU_VolumeReconstructionPluginInterface" )

public:

    GPU_VolumeReconstructionPluginInterface();
    ~GPU_VolumeReconstructionPluginInterface();
    virtual QString GetPluginName() { return QString("GPU_VolumeReconstruction"); }
    bool CanRun();
    QString GetMenuEntryString() { return QString("US Volume Reconstruction With GPU"); }

    QWidget * CreateFloatingWidget();

};

#endif
