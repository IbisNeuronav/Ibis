#ifndef SCREWTABLEWIDGET_H
#define SCREWTABLEWIDGET_H

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPair>
#include <QTableWidgetItem>
#include <QWidget>

namespace Ui
{
class ScrewTableWidget;
}

class ScrewTableWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QPair<double, double> ScrewProperties;

    explicit ScrewTableWidget( QWidget * parent = 0, QString configDir = "" );
    ~ScrewTableWidget();

    static QList<ScrewProperties> GetTable( QString );

protected:
    void UpdateTable();
    static bool ScrewComparator( const ScrewProperties & v1, const ScrewProperties & v2 );

private:
    QString m_configDir;
    Ui::ScrewTableWidget * ui;
    QList<ScrewProperties> m_screwList;

private slots:
    void on_addScrewButton_clicked();
    void on_closeButton_clicked();
    void on_removeScrewButton_clicked();
    void on_tableWidget_itemSelectionChanged();

signals:
    void WidgetAboutToClose();
};

#endif  // __SCREWTABLEWIDGET_H__
