#include "filesystemtree.h"
#include "ui_filesystemtree.h"

FileSystemTree::FileSystemTree(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileSystemTree)
{
    ui->setupUi(this);
    m_pathForm = nullptr;
    m_model = new QDirModel( this );
    m_model->setReadOnly( true );
    m_model->setSorting( QDir::DirsFirst | QDir::IgnoreCase | QDir::Name );

    QModelIndex index = m_model->index( QDir::homePath() );

    ui->treeView->setModel( m_model );
    ui->treeView->expand( index );
    ui->treeView->scrollTo( index );
    ui->treeView->setCurrentIndex( index );
    ui->treeView->resizeColumnToContents( 0 );
}

FileSystemTree::~FileSystemTree()
{
    delete ui;
}

void FileSystemTree::on_selectPushButton_clicked()
{
    QModelIndex index = ui->treeView->currentIndex();
    if( index.isValid() )
    {
        QFileInfo fi = m_model->fileInfo( index );
        QString fileOrDir = fi.absoluteFilePath();
        m_pathForm->SetOnlyPathText( fileOrDir );
    }
    this->close();
}

void FileSystemTree::on_cancelPushButton_clicked()
{
    this->close();
}
