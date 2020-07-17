/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef LandmarkRegistrationObjectPluginInterface_H
#define LandmarkRegistrationObjectPluginInterface_H

#include <QWidget>
#include "objectplugininterface.h"
#include <vtkSmartPointer.h>
#include <vector>

class LandmarkRegistrationObject;

class LandmarkRegistrationObjectPluginInterface : public ObjectPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.LandmarkRegistrationObjectPluginInterface" )

public:

    vtkTypeMacro( LandmarkRegistrationObjectPluginInterface, ObjectPluginInterface );

    LandmarkRegistrationObjectPluginInterface();
    ~LandmarkRegistrationObjectPluginInterface();

    // IbisPlugin interface
    virtual QString GetPluginName() { return QString("LandmarkRegistrationObject"); }

    // ObjectPluginInterface interface
    QString GetMenuEntryString() { return QString("Landmark Registration Object"); }
    QString GetObjectClassName() { return "LandmarkRegistrationObject"; }

    virtual SceneObject * CreateObject();

    virtual void SceneAboutToLoad() override;

protected:
    void Clear();

public slots:

    void OnObjectRemoved(int objID);

protected:

    typedef std::vector< int > RegistrationObjectsContainer;
    RegistrationObjectsContainer m_landmarkRegistrationObjectIds;
    int m_currentRegistrationObjectId;
};

#endif //LandmarkRegistrationObjectPluginInterface_H
