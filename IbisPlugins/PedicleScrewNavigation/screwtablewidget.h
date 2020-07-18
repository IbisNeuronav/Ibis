#ifndef __SCREWTABLEWIDGET_H__
#define __SCREWTABLEWIDGET_H__

#include <QWidget>
#include <QDir>
#include <QPair>
#include <QFile>
#include <QFileInfo>
#include <QTableWidgetItem>

namespace Ui {
    class ScrewTableWidget;
}

class ScrewTableWidget : public QWidget
{

    Q_OBJECT

public:

    typedef QPair<double, double> ScrewProperties;

    explicit ScrewTableWidget(QWidget *parent = 0, QString configDir = "");
    ~ScrewTableWidget();

    static QList<ScrewProperties> GetTable(QString);

protected:
    void UpdateTable();

private:

    QString m_configDir;
    Ui::ScrewTableWidget *ui;
    QList<ScrewProperties> m_screwList;

private slots:
    void on_addScrewButton_clicked();
    void on_closeButton_clicked();
    void on_removeScrewButton_clicked();
    void on_tableWidget_itemSelectionChanged();

signals:
    void WidgetAboutToClose();

};

#endif // __SCREWTABLEWIDGET_H__
