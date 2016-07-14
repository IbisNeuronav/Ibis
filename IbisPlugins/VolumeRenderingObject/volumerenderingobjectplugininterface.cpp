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

#include "volumerenderingobjectplugininterface.h"
#include "volumerenderingobject.h"
#include "application.h"
#include "scenemanager.h"
#include <QSettings>

VolumeRenderingObjectPluginInterface::VolumeRenderingObjectPluginInterface()
{
    m_vrObject = VolumeRenderingObject::New();
    m_vrObject->SetCanChangeParent( false );
    m_vrObject->SetName( "Volume Renderer" );
    m_vrObject->SetNameChangeable( false );
    m_vrObject->SetCanAppendChildren( false );
    m_vrObject->SetCanEditTransformManually( false );
    m_vrObject->SetHidden( true );
    m_vrObject->SetObjectManagedBySystem( true );
    m_vrObject->SetObjectDeletable(false);
}

VolumeRenderingObjectPluginInterface::~VolumeRenderingObjectPluginInterface()
{
    m_vrObject->Delete();
}

SceneObject * VolumeRenderingObjectPluginInterface::GetGlobalObjectInstance()
{
    return m_vrObject;
}

void VolumeRenderingObjectPluginInterface::LoadSettings( QSettings & s )
{
    m_vrObject->LoadCustomRayInitShaders();
    m_vrObject->LoadCustomShaderContribs();
    bool vrEnabled = true;
    vrEnabled = s.value( "VolumeRendererEnabled", vrEnabled ).toBool();
    m_vrObject->SetHidden( !vrEnabled );
}

void VolumeRenderingObjectPluginInterface::SaveSettings( QSettings & s )
{
    s.setValue( "VolumeRendererEnabled", !m_vrObject->IsHidden() );
    m_vrObject->SaveCustomRayInitShaders();
    m_vrObject->SaveCustomShaderContribs();
}
