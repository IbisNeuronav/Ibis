
#include "setplanesworldpositionplugininterface.h"
#include <QtPlugin>
#include <QString>
#include "setplanesworldpositionwidget.h"

SetPlanesWorldPositionPluginInterface::SetPlanesWorldPositionPluginInterface()
{
}

SetPlanesWorldPositionPluginInterface::~SetPlanesWorldPositionPluginInterface()
{
}

QWidget * SetPlanesWorldPositionPluginInterface::CreateFloatingWidget()
{
    // Open the widget
    m_interfaceWidget = new SetPlanesWorldPositionWidget( 0 );
    m_interfaceWidget->setAttribute( Qt::WA_DeleteOnClose, true );
    m_interfaceWidget->setWindowTitle( "Set Planes World Position" );
    m_interfaceWidget->SetInterface( this );
    return m_interfaceWidget;
}
