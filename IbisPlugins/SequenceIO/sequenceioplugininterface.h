// Thanks to Houssem Gueziri for writing this class

#ifndef __SequenceIOPluginInterface_h_
#define __SequenceIOPluginInterface_h_

#include "toolplugininterface.h"

class SequenceIOWidget;

class SequenceIOPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.SequenceIOPluginInterface" )

public:

    SequenceIOPluginInterface();
    ~SequenceIOPluginInterface();
    virtual QString GetPluginName() override { return QString("SequenceIO"); }
    virtual bool WidgetAboutToClose() override;
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString("Sequence I/O"); }

    QWidget * CreateFloatingWidget() override;

protected slots:

    void OnObjectAdded(int);
    void OnObjectRemoved(int);

private:
    SequenceIOWidget * m_widget;

};

#endif
