#ifndef SETPLANESWORLDPOSITIONWIDGET_H
#define SETPLANESWORLDPOSITIONWIDGET_H

#include <QWidget>
#include "setplanesworldpositionplugininterface.h"

namespace Ui {
class SetPlanesWorldPositionWidget;
}

class SetPlanesWorldPositionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SetPlanesWorldPositionWidget(QWidget *parent = 0);
    ~SetPlanesWorldPositionWidget();

    void SetInterface( SetPlanesWorldPositionPluginInterface *intf );

public slots:
    void UpdateUI();

private slots:
    void on_applyPushButton_clicked();

private:
    SetPlanesWorldPositionPluginInterface *m_pluginInterface;

    Ui::SetPlanesWorldPositionWidget *ui;
};

#endif // SETPLANESWORLDPOSITIONWIDGET_H
