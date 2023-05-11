#ifndef PATHFORM_H
#define PATHFORM_H

#include <QObject>
#include <QString>
#include <QWidget>

class FileSystemTree;

namespace Ui
{
class PathForm;
}

class PathForm : public QWidget
{
    Q_OBJECT

public:
    explicit PathForm( QWidget * parent = 0 );
    ~PathForm();

    static const QString LabelWidgetName;
    static const QString PathWidgetName;

    void SetPath( QString labelText, QString pathText );
    void SetOnlyPathText( QString pathText );

signals:
    void PathChanged( QString, QString );

private slots:
    void on_browsePushButton_clicked();
    void PathLineEditChanged();

private:
    Ui::PathForm * ui;
};

#endif  // PATHFORM_H
