
#include "applytransformtoobjectplugininterface.h"
#include <QtPlugin>
#include <QString>
#include "applytransformtoobjectwidget.h"

ApplyTransformToObjectPluginInterface::ApplyTransformToObjectPluginInterface()
{
}

ApplyTransformToObjectPluginInterface::~ApplyTransformToObjectPluginInterface()
{
}

QWidget * ApplyTransformToObjectPluginInterface::CreateFloatingWidget()
{
    // Open the widget
    m_interfaceWidget = new ApplyTransformToObjectWidget( 0 );
    m_interfaceWidget->setAttribute( Qt::WA_DeleteOnClose, true );
    m_interfaceWidget->setWindowTitle( "Apply Transform To Object" );
    m_interfaceWidget->SetInterface( this );
    return m_interfaceWidget;
}
