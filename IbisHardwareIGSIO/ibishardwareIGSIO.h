/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __IbisHardwareIGSIO_h_
#define __IbisHardwareIGSIO_h_

#include "hardwaremodule.h"
#include "igtlioDevice.h"

namespace  igtlio {
    class Logic;
}
class QMenu;
class qIGTLIOLogicController;
class qIGTLIOClientWidget;


class IbisHardwareIGSIO : public QObject, public HardwareModule
{
    
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA(IID "Ibis.IbisHardwareIGSIO" )
    
public:

    vtkTypeMacro( IbisHardwareIGSIO, HardwareModule );

    static IbisHardwareIGSIO * New() { return new IbisHardwareIGSIO; }
    IbisHardwareIGSIO();
    ~IbisHardwareIGSIO();

    // Implementation of IbisPlugin interface
    virtual QString GetPluginName() { return QString("IbisHardwareIGSIO"); }

    // Implementation of the HardwareModule interface
    virtual void AddSettingsMenuEntries( QMenu * menu );
    virtual void Init();
    virtual void Update();
    virtual bool ShutDown();

    virtual void AddToolObjectsToScene();
    virtual void RemoveToolObjectsFromScene();

    virtual vtkTransform * GetReferenceTransform();

    virtual bool IsTransformFrozen( TrackedSceneObject * obj );
    virtual void FreezeTransform( TrackedSceneObject * obj, int nbSamples );
    virtual void UnFreezeTransform( TrackedSceneObject * obj );

    virtual void AddTrackedVideoClient( TrackedSceneObject * obj );
    virtual void RemoveTrackedVideoClient( TrackedSceneObject * obj);

    virtual void StartTipCalibration( PointerObject * p );
    virtual double DoTipCalibration( PointerObject * p, vtkMatrix4x4 * calibMat );
    virtual bool IsCalibratingTip( PointerObject * p );
    virtual void StopTipCalibration( PointerObject * p );

private slots:

    void OpenSettingsWidget();

protected:

    void FindNewTools();
    void FindRemovedTools();
    int FindToolByName( QString name );
    bool IoHasDevice( igtlio::DevicePointer device );
    bool ModuleHasDevice( igtlio::DevicePointer device );
    TrackedSceneObject * InstanciateSceneObjectFromDevice( igtlio::DevicePointer device );

    struct Tool
    {
        TrackedSceneObject * sceneObject;
        igtlio::DevicePointer ioDevice;
    };
    typedef QList< Tool* > toolList;
    toolList m_tools;

    vtkSmartPointer<igtlio::Logic> m_logic;
    qIGTLIOLogicController * m_logicController;
    qIGTLIOClientWidget * m_clientWidget;
};

#endif
