#include "pathform.h"
#include "ui_pathform.h"
#include <QFile>
#include <QMessageBox>
#include "filesystemtree.h"

const QString PathForm::LabelWidgetName = "CustomPathName";
const QString PathForm::PathWidgetName = "CustomPath";

PathForm::PathForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PathForm)
{
    ui->setupUi(this);
    ui->label->setObjectName( PathForm::LabelWidgetName );
    ui->pathLineEdit->setObjectName( PathForm::PathWidgetName );
    connect( ui->pathLineEdit, SIGNAL(returnPressed()), this, SLOT( PathLineEditChanged() ) );
}

PathForm::~PathForm()
{
    delete ui;
}

void PathForm::SetPath( QString labelText, QString pathText )
{
    ui->label->setText( labelText );
    ui->pathLineEdit->setText( pathText );
}

void PathForm::SetOnlyPathText( QString pathText )
{
    ui->pathLineEdit->setText( pathText );
}

void PathForm::on_browsePushButton_clicked()
{
    FileSystemTree *tree = new FileSystemTree;
    tree->setAttribute(Qt::WA_DeleteOnClose);
    tree->SetPathForm( this );
    tree->show();
}

void PathForm::on_removePushButton_clicked()
{
    emit PathToRemove( ui->label->text() );
}

void PathForm::PathLineEditChanged()
{
    QFile f( ui->pathLineEdit->text() );
    if( !f.exists() )
    {
        QString tmp("File/Directory:\n");
        tmp.append( ui->pathLineEdit->text() );
        tmp.append( "\ndoes not exist. Please enter a valid path." );
        QMessageBox::critical( 0, "Error", tmp, 1, 0 );
        ui->pathLineEdit->setText( "" );
    }
    emit PathChanged( ui->label->text(), ui->pathLineEdit->text() );
}
