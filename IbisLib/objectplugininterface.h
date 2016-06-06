/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef OBJECTPLUGININTERFACE_h_
#define OBJECTPLUGININTERFACE_h_

#include <QString>

class Application;
class SceneObject;
class Serializer;

class ObjectPluginInterface
{

public:
    
    ObjectPluginInterface() : m_application(0) {}
    virtual ~ObjectPluginInterface() {}
    
    void SetApplication( Application * app ) { m_application = app; }
    Application * GetApplication() { return m_application; }
    virtual QString GetMenuEntryString() = 0;
    virtual void CreateObject() = 0;
    virtual QString GetObjectClassName() = 0;

protected:

    Application * m_application;

};

Q_DECLARE_INTERFACE( ObjectPluginInterface, "ca.mcgill.mni.bic.Ibis.ObjectPluginInterface/1.0" );

#endif //OBJECTPLUGININTERFACE_h_
