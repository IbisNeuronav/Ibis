#include "pathform.h"

#include <QFile>
#include <QMessageBox>

#include "application.h"
#include "filesystemtree.h"
#include "ui_pathform.h"

const QString PathForm::LabelWidgetName = "CustomPathName";
const QString PathForm::PathWidgetName  = "CustomPath";

PathForm::PathForm( QWidget * parent ) : QWidget( parent ), ui( new Ui::PathForm )
{
    ui->setupUi( this );
    ui->label->setObjectName( PathForm::LabelWidgetName );
    ui->pathLineEdit->setObjectName( PathForm::PathWidgetName );
    connect( ui->pathLineEdit, SIGNAL( returnPressed() ), this, SLOT( PathLineEditChanged() ) );
}

PathForm::~PathForm() { delete ui; }

void PathForm::SetPath( QString labelText, QString pathText )
{
    ui->label->setText( labelText );
    ui->pathLineEdit->setText( pathText );
}

void PathForm::SetOnlyPathText( QString pathText )
{
    ui->pathLineEdit->setText( pathText );
    emit PathChanged( ui->label->text(), ui->pathLineEdit->text() );
}

void PathForm::on_browsePushButton_clicked()
{
    FileSystemTree * tree = new FileSystemTree;
    tree->setAttribute( Qt::WA_DeleteOnClose );
    tree->SetPathForm( this );
    Application::GetInstance().ShowFloatingDock( tree );
}

void PathForm::PathLineEditChanged()
{
    QFileInfo fi( ui->pathLineEdit->text() );
    if( !fi.exists() )
    {
        QString tmp( "File/Directory:\n" );
        tmp.append( ui->pathLineEdit->text() );
        tmp.append( "\ndoes not exist. Please enter a valid path." );
        QMessageBox::critical( 0, "Error", tmp, 1, 0 );
        ui->pathLineEdit->setText( "" );
    }
    emit PathChanged( ui->label->text(), ui->pathLineEdit->text() );
}
