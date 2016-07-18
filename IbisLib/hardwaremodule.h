/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __HardwareModule_h_
#define __HardwareModule_h_

#include "ibisplugin.h"
#include "ibistypes.h"

class Application;
class SceneManager;
class QMenu;
class vtkImageData;
class vtkTransform;
class vtkMatrix4x4;
class CameraIntrinsicParams;
class PointerObject;
class TrackedSceneObject;

class HardwareModule : public IbisPlugin
{

public:

    vtkTypeMacro( HardwareModule, IbisPlugin );

    HardwareModule() {}
    virtual ~HardwareModule() {}

    virtual IbisPluginTypes GetPluginType() { return IbisPluginTypeHardwareModule; }

    virtual void AddSettingsMenuEntries( QMenu * menu ) {}
    virtual bool Init() = 0;
    virtual void Update() = 0;
    virtual bool ShutDown() = 0;

    virtual void AddToolObjectsToScene() = 0;
    virtual void RemoveToolObjectsFromScene() = 0;

    virtual vtkTransform * GetReferenceTransform() = 0;

    virtual bool IsTransformFrozen( TrackedSceneObject * obj ) = 0;
    virtual void FreezeTransform( TrackedSceneObject * obj, int nbSamples ) = 0;
    virtual void UnFreezeTransform( TrackedSceneObject * obj ) = 0;

    virtual void AddTrackedVideoClient( TrackedSceneObject * obj ) = 0;
    virtual void RemoveTrackedVideoClient( TrackedSceneObject * obj) = 0;

    virtual void StartTipCalibration( PointerObject * p ) = 0;
    virtual double DoTipCalibration( PointerObject * p, vtkMatrix4x4 * calibMat ) = 0;
    virtual bool IsCalibratingTip( PointerObject * p ) = 0;
    virtual void StopTipCalibration( PointerObject * p ) = 0;

protected:

    Application * m_application;

};

#endif
