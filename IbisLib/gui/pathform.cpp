#include "pathform.h"
#include "ui_pathform.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include "ibispreferences.h"

const QString PathForm::LabelWidgetName = "CustomVariableName";
const QString PathForm::CustomVariableWidgetName = "CustomVariable";

PathForm::PathForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PathForm)
{
    ui->setupUi(this);
    m_variableType = VARIABLE_TYPE::DIRECTORY_VARIABLE_TYPE;
    ui->label->setObjectName( PathForm::LabelWidgetName );
    ui->customVariableLineEdit->setObjectName( PathForm::CustomVariableWidgetName );
    connect( ui->customVariableLineEdit, SIGNAL(returnPressed()), this, SLOT( CustomVariableLineEditChanged() ) );
}

PathForm::~PathForm()
{
    delete ui;
}

void PathForm::SetCustomVariable(QString labelText, QString customVariableText , VARIABLE_TYPE varType)
{
    m_variableType = varType;
    ui->label->setText( labelText );
    ui->customVariableLineEdit->setText( customVariableText );
}

void PathForm::on_browsePushButton_clicked()
{
    QString outputText;
    if( m_variableType == VARIABLE_TYPE::DIRECTORY_VARIABLE_TYPE )
        outputText = QFileDialog::getExistingDirectory( this, "Custom Path", QDir::homePath() );
    else
        outputText = QFileDialog::getOpenFileName( this, "File", QDir::homePath() );
    if( !outputText.isEmpty() ) // otherwise dialog was cancelled an we want to keep the old variable
    {
        ui->customVariableLineEdit->setText( outputText );
        emit CustomVariableChanged( ui->label->text(), ui->customVariableLineEdit->text(), m_variableType );
    }
}

void PathForm::on_removePushButton_clicked()
{
    emit CustomVariableToRemove( ui->label->text() );
}

void PathForm::CustomVariableLineEditChanged()
{
    if( m_variableType == VARIABLE_TYPE::DIRECTORY_VARIABLE_TYPE )
    {
        QDir dir( ui->customVariableLineEdit->text() );
        if( !dir.exists() )
        {
            QString tmp("Directory:\n");
            tmp.append( ui->customVariableLineEdit->text() );
            tmp.append( "\ndoes not exist. Please enter a valid path." );
            QMessageBox::critical( 0, "Error", tmp, 1, 0 );
            ui->customVariableLineEdit->setText( "" );
        }
    }
    else
    {
        QFile f( ui->customVariableLineEdit->text() );
        if( !f.exists() )
        {
            QString tmp("File:\n");
            tmp.append( ui->customVariableLineEdit->text() );
            tmp.append( "\ndoes not exist. Please enter a valid filr name." );
            QMessageBox::critical( 0, "Error", tmp, 1, 0 );
            ui->customVariableLineEdit->setText( "" );
        }
    }
    emit CustomVariableChanged( ui->label->text(), ui->customVariableLineEdit->text(), m_variableType );
}
