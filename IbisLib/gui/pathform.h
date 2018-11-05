#ifndef PATHFORM_H
#define PATHFORM_H

#include <QWidget>
#include <QString>

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
    static const QString PathWidgetName;

    void SetPath( QString labelText, QString pathText );

private slots:
    void on_browsePushButton_clicked();
    void PathLineEditChanged();

private:
    Ui::PathForm *ui;
};

#endif // PATHFORM_H
