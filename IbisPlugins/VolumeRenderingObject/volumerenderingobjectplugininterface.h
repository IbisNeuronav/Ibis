/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __VolumeRenderingObjectPluginInterface_h_
#define __VolumeRenderingObjectPluginInterface_h_

#include <QObject>
#include "globalobjectplugininterface.h"

class VolumeRenderingObject;

class VolumeRenderingObjectPluginInterface : public QObject, public GlobalObjectPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.VolumeRenderingObjectPluginInterface" )

public:

    VolumeRenderingObjectPluginInterface();
    ~VolumeRenderingObjectPluginInterface();

    // Implement IbisPlugin
    virtual QString GetPluginName() { return QString("VolumeRenderingObject"); }

    // Implement GlobalObjectPluginInterface
    SceneObject * GetGlobalObjectInstance();
    QString GetGlobalObjectClassName() { return "VolumeRederingObject"; }
    virtual void LoadSettings( QSettings & s );
    virtual void SaveSettings( QSettings & s );

    VolumeRenderingObject * GetVolumeRenderingObjectInstance() { return m_vrObject; }

protected:

    VolumeRenderingObject * m_vrObject;
};

#endif
