/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Houssem Gueziri for writing this class

#include "recordtrackingplugininterface.h"
#include "recordtrackingwidget.h"
#include "ibisapi.h"
#include <QtPlugin>

RecordTrackingPluginInterface::RecordTrackingPluginInterface() : m_widget(nullptr)
{
}

RecordTrackingPluginInterface::~RecordTrackingPluginInterface()
{
}

bool RecordTrackingPluginInterface::CanRun()
{
    return true;
}

QWidget * RecordTrackingPluginInterface::CreateTab()
{
    m_widget = new RecordTrackingWidget;
    m_widget->SetPluginInterface( this );

    IbisAPI *ibisApi = this->GetIbisAPI();
    connect( ibisApi, SIGNAL( ObjectAdded(int) ), this, SLOT( OnTrackedToolAdded(int) ) );
    connect( ibisApi, SIGNAL( ObjectRemoved(int) ), this, SLOT( OnTrackedToolRemoved(int) ) );

    return m_widget;
}


bool RecordTrackingPluginInterface::WidgetAboutToClose()
{
    IbisAPI *ibisApi = this->GetIbisAPI();
    disconnect( ibisApi, SIGNAL( ObjectAdded(int) ), this, SLOT( OnTrackedToolAdded(int) ) );
    disconnect( ibisApi, SIGNAL( ObjectRemoved(int) ), this, SLOT( OnTrackedToolRemoved(int) ) );
}

void RecordTrackingPluginInterface::OnTrackedToolAdded(int id)
{
    if ((id != IbisAPI::InvalidId) && (m_widget))
    {
        m_widget->AddTrackedTool(id);
    }
}

void RecordTrackingPluginInterface::OnTrackedToolRemoved(int id)
{
    if (id != IbisAPI::InvalidId)
    {
        if (m_widget)
        {
            m_widget->RemoveTrackedTool(id);
        }
    }
}
