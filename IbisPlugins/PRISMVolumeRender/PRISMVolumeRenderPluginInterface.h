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

#ifndef __PRISMVolumeRenderPluginInterface_h_
#define __PRISMVolumeRenderPluginInterface_h_

#include "vtkSmartPointer.h"
#include <QObject>
#include "globalobjectplugininterface.h"

class VolumeRenderingObject;

class PRISMVolumeRenderPluginInterface : public QObject, public GlobalObjectPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.PRISMVolumeRenderPluginInterface" )

public:

    PRISMVolumeRenderPluginInterface();
    ~PRISMVolumeRenderPluginInterface();

    // Implement IbisPlugin
    virtual QString GetPluginName() { return QString("PRISMVolumeRender"); }

    // Implement GlobalObjectPluginInterface
    SceneObject * GetGlobalObjectInstance();
    QString GetGlobalObjectClassName() { return "VolumeRederingObject"; }
    virtual void LoadSettings( QSettings & s );
    virtual void SaveSettings( QSettings & s );

    VolumeRenderingObject * GetVolumeRenderingObjectInstance();
protected:

    vtkSmartPointer<VolumeRenderingObject> m_vrObject;
};

#endif
