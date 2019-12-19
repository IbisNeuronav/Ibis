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

#ifndef __RecordTrackingPluginInterface_h_
#define __RecordTrackingPluginInterface_h_

#include "toolplugininterface.h"

class RecordTrackingWidget;

class RecordTrackingPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.RecordTrackingPluginInterface" )

public:

    RecordTrackingPluginInterface();
    ~RecordTrackingPluginInterface();
    virtual QString GetPluginName() { return QString("RecordTracking"); }
    virtual bool WidgetAboutToClose();
    bool CanRun();
    QString GetMenuEntryString() { return QString("Record Tracking Information"); }

    QWidget * CreateTab();

protected slots:

    void OnTrackedToolAdded(int);
    void OnTrackedToolRemoved(int);

private:
    RecordTrackingWidget * m_widget;

};

#endif
