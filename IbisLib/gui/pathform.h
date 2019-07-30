#ifndef PATHFORM_H
#define PATHFORM_H

#include <QWidget>
#include <QString>
#include "ibispreferences.h"

namespace Ui {
class PathForm;
}

class PathForm : public QWidget
{
    Q_OBJECT

public:
    explicit PathForm(QWidget *parent = 0);
    ~PathForm();

    static const QString LabelWidgetName;
    static const QString CustomVariableWidgetName;

    void SetCustomVariable( QString labelText, QString customVariableText, VARIABLE_TYPE varType = VARIABLE_TYPE::DIRECTORY_VARIABLE_TYPE );

signals:
    void CustomVariableToRemove( QString );
    void CustomVariableChanged( QString, QString, VARIABLE_TYPE varType );

private slots:
    void on_browsePushButton_clicked();
    void on_removePushButton_clicked();
    void CustomVariableLineEditChanged();

private:
    Ui::PathForm *ui;
    VARIABLE_TYPE m_variableType;
};

#endif // PATHFORM_H
