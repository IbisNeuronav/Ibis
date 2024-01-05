#ifndef FILESYSTEMTREE_H
#define FILESYSTEMTREE_H

#include <QFileSystemModel>
#include <QObject>
#include <QWidget>

#include "pathform.h"

namespace Ui
{
class FileSystemTree;
}

class FileSystemTree : public QWidget
{
    Q_OBJECT

public:
    explicit FileSystemTree( QWidget * parent = 0 );
    ~FileSystemTree();

    void SetPathForm( PathForm * pf ) { m_pathForm = pf; }

private slots:
    void on_selectPushButton_clicked();
    void on_cancelPushButton_clicked();

private:
    Ui::FileSystemTree * ui;
    QFileSystemModel * m_model;
    PathForm * m_pathForm;
};

#endif  // FILESYSTEMTREE_H
