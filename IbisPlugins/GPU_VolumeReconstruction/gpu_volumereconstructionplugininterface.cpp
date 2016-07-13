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

#include "gpu_volumereconstructionplugininterface.h"
#include "gpu_volumereconstructionwidget.h"
#include "application.h"
#include <QtPlugin>


GPU_VolumeReconstructionPluginInterface::GPU_VolumeReconstructionPluginInterface()
{
}

GPU_VolumeReconstructionPluginInterface::~GPU_VolumeReconstructionPluginInterface()
{
}

bool GPU_VolumeReconstructionPluginInterface::CanRun()
{
    // This plugin can't run in viewer-only mode. Needs video capture and tracking.
    //if( m_application->IsViewerOnly() )
    //    return false;
    return true;
}

QWidget * GPU_VolumeReconstructionPluginInterface::CreateFloatingWidget()
{
    GPU_VolumeReconstructionWidget * widget = new GPU_VolumeReconstructionWidget;
    widget->SetApplication( GetApplication() );
    return widget;
}

