
#ifndef __SetPlanesWorldPositionPluginInterface_h_
#define __SetPlanesWorldPositionPluginInterface_h_

#include "toolplugininterface.h"

class SetPlanesWorldPositionWidget;

class SetPlanesWorldPositionPluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.SetPlanesWorldPositionPluginInterface" )

public:
    vtkTypeMacro( SetPlanesWorldPositionPluginInterface, ToolPluginInterface );

    SetPlanesWorldPositionPluginInterface();
    ~SetPlanesWorldPositionPluginInterface();
    virtual QString GetPluginName() override { return QString( "SetPlanesWorldPosition" ); }
    bool CanRun() override { return true; }
    QString GetMenuEntryString() override { return QString( "Set Planes World Position" ); }
    QWidget * CreateFloatingWidget() override;

protected:
    SetPlanesWorldPositionWidget * m_interfaceWidget;
};

#endif
