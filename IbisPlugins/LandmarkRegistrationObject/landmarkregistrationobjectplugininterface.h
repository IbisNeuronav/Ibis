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
    virtual QString GetPluginName() override { return QString("LandmarkRegistrationObject"); }

    // ObjectPluginInterface interface
    QString GetMenuEntryString() override { return QString("Landmark Registration Object"); }
    QString GetObjectClassName() { return "LandmarkRegistrationObject"; }

    virtual SceneObject * CreateObject() override;

    virtual void SceneAboutToLoad() override;

    QString GetPluginDescription() override
    {
        QString description;
        description = "Landmark Registration\n"
                      "This plugin is used to register two images by a least square fit of two sets of user defined points,\n"
                      "functionality provided by vtkLandmarkTransform.\n"
                      "\n";
        return description;
    }

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
