#ifndef APPLYTRANSFORMTOOBJECTWIDGET_H
#define APPLYTRANSFORMTOOBJECTWIDGET_H

#include <QWidget>
#include "applytransformtoobjectplugininterface.h"

class SceneObject;

namespace Ui {
class ApplyTransformToObjectWidget;
}

class vtkQtMatrixDialog;

class ApplyTransformToObjectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ApplyTransformToObjectWidget(QWidget *parent = 0);
    ~ApplyTransformToObjectWidget();
    void SetInterface( ApplyTransformToObjectPluginInterface *intface );

private slots:

    void on_transformPushButton_clicked();
    void on_sceneObjectsComboBox_currentIndexChanged(int index);
    void EditMatrixDialogClosed();
    void UpdateUI();

private:
    vtkQtMatrixDialog * m_matrixDialog;
    SceneObject *m_selectedObject;

    ApplyTransformToObjectPluginInterface *m_pluginInterface;

    Ui::ApplyTransformToObjectWidget *ui;
};

#endif // APPLYTRANSFORMTOOBJECTWIDGET_H
