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

#include <QObject>
#include <QWidget>
#include "objectplugininterface.h"

class LandmarkRegistrationObject;

class LandmarkRegistrationObjectPluginInterface : public QObject, public ObjectPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(ObjectPluginInterface)
    Q_PLUGIN_METADATA(IID "Ibis.LandmarkRegistrationObjectPluginInterface" )

public:

    LandmarkRegistrationObjectPluginInterface();
    ~LandmarkRegistrationObjectPluginInterface();
    QString GetMenuEntryString() { return QString("Landmark Registration Object"); }
    QString GetObjectClassName() { return "LandmarkRegistrationObject"; }
    virtual void CreateObject();
    LandmarkRegistrationObject *GetLandmarkRegistrationObject() { return m_landmarkRegistrationObject; }
protected:
    LandmarkRegistrationObject *m_landmarkRegistrationObject;
};

#endif //LandmarkRegistrationObjectPluginInterface_H
