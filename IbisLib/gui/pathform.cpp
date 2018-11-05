#include "pathform.h"
#include "ui_pathform.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

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

void PathForm::on_browsePushButton_clicked()
{
    QString outputDir = QFileDialog::getExistingDirectory( this, "Custom Path", QDir::homePath() );
    ui->pathLineEdit->setText( outputDir );
}

void PathForm::PathLineEditChanged()
{
    QDir dir( ui->pathLineEdit->text() );
    if( !dir.exists() )
    {
        QString tmp("Directory:\n");
        tmp.append( ui->pathLineEdit->text() );
        tmp.append( "\ndoes not exist. Please enter a valid path." );
        QMessageBox::critical( 0, "Error", tmp, 1, 0 );
        ui->pathLineEdit->setText( "" );
    }
}
