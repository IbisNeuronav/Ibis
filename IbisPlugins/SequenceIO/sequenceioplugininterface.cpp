// Thanks to Houssem Gueziri for writing this class

#include "sequenceioplugininterface.h"
#include "sequenceiowidget.h"
#include "ibisapi.h"
#include <QtPlugin>

SequenceIOPluginInterface::SequenceIOPluginInterface() : m_widget(nullptr)
{
}

SequenceIOPluginInterface::~SequenceIOPluginInterface()
{
}

bool SequenceIOPluginInterface::CanRun()
{
    return true;
}

QWidget * SequenceIOPluginInterface::CreateFloatingWidget()
{
    m_widget = new SequenceIOWidget;
    m_widget->SetPluginInterface( this );

    IbisAPI *ibisApi = this->GetIbisAPI();
    connect( ibisApi, SIGNAL( ObjectAdded(int) ), this, SLOT( OnObjectAdded(int) ) );
    connect( ibisApi, SIGNAL( ObjectRemoved(int) ), this, SLOT( OnObjectRemoved(int) ) );

    return m_widget;
}


bool SequenceIOPluginInterface::WidgetAboutToClose()
{
    IbisAPI *ibisApi = this->GetIbisAPI();
    disconnect( ibisApi, SIGNAL( ObjectAdded(int) ), this, SLOT(OnObjectAdded(int) ) );
    disconnect( ibisApi, SIGNAL( ObjectRemoved(int) ), this, SLOT(OnObjectRemoved(int) ) );
    return true;
}

void SequenceIOPluginInterface::OnObjectAdded(int id)
{
    if ((id != IbisAPI::InvalidId) && (m_widget))
    {
        m_widget->AddAcquisition(id);
    }
}

void SequenceIOPluginInterface::OnObjectRemoved(int id)
{
    if ((id != IbisAPI::InvalidId) && (m_widget))
    {
        m_widget->RemoveAcquisition(id);
    }
}
