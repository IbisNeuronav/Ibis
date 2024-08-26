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

#ifndef RECORDTRACKINGPLUGININTERFACE_H
#define RECORDTRACKINGPLUGININTERFACE_H

#include "toolplugininterface.h"

class RecordTrackingWidget;

class RecordTrackingPluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.RecordTrackingPluginInterface" )

public:
    RecordTrackingPluginInterface();
    ~RecordTrackingPluginInterface();
    virtual QString GetPluginName() override { return QString( "RecordTracking" ); }
    virtual bool WidgetAboutToClose() override;
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString( "Record Tracking Information" ); }

    QWidget * CreateTab() override;

protected slots:

    void OnTrackedToolAdded( int );
    void OnTrackedToolRemoved( int );

private:
    RecordTrackingWidget * m_widget;
};

#endif
