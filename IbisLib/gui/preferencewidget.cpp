#include "preferencewidget.h"
#include "ui_preferencewidget.h"
#include "ibisapi.h"
#include "pathform.h"
#include <QVBoxLayout>
#include <QMap>
#include <QLabel>
#include <QLineEdit>
#include <QDir>
#include <QMessageBox>
#include <QLayoutItem>

PreferenceWidget::PreferenceWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PreferenceWidget)
{
    ui->setupUi(this);
    m_preferences = nullptr;
    m_customVariablesLayout = nullptr;
}

PreferenceWidget::~PreferenceWidget()
{
    delete m_customVariablesLayout;
    delete ui;
}

void PreferenceWidget::SetPreferences( IbisPreferences *pref )
{
    m_preferences = pref;
    this->UpdateUI();
}

void PreferenceWidget::UpdateUI(  )
{
    if( m_preferences )
    {
        if( m_customVariablesLayout )
        {
            this->RemoveAllCustomVariablesWidgets();
            delete m_customVariablesLayout;
        }
        m_customVariablesLayout = new QVBoxLayout;
        m_customVariablesLayout->setSpacing( 4 );
        m_customVariablesLayout->setAlignment( Qt::AlignTop );
        ui->customPathsGroupBox->setLayout( m_customVariablesLayout );
        QMap< QString, IbisPreferences::TypedVariable> paths = m_preferences->GetCustomVariables();
        if( !paths.isEmpty() )
        {
            QMap< QString, IbisPreferences::TypedVariable >::iterator it;
            for( it = paths.begin(); it != paths.end(); ++it )
            {
                IbisPreferences::TypedVariable tvar = it.value();
                QString validVar = tvar.name;
                IbisPreferences::VARIABLE_TYPE  varType= tvar.varType;
                QFileInfo fi( validVar );
                if( !fi.exists( ) || !( fi.isDir() || fi.isFile() ) )
                    validVar = "";

                PathForm *nextPath = new PathForm;
                nextPath->SetCustomVariable( it.key(), validVar, varType );
                m_customVariablesLayout->addWidget( nextPath, 0, Qt::AlignTop );
                connect( nextPath, SIGNAL( CustomVariableToRemove(QString) ), this, SLOT( RemoveCustomVariable(QString) ) );
                connect( nextPath, SIGNAL( CustomVariableChanged(QString, QString) ), this, SLOT( ResetCustomVariable(QString, QString) ) );
            }
        }
    }
}

void PreferenceWidget::RemoveCustomVariable(QString varName )
{
    m_preferences->UnRegisterCustomVariable( varName );
    this->UpdateUI();
}

void PreferenceWidget::ResetCustomVariable(QString customVarName, QString customVar )
{
    m_preferences->UnRegisterCustomVariable( customVarName );
    m_preferences->RegisterCustomVariable( customVarName, customVar );
}

void PreferenceWidget::RemoveAllCustomVariablesWidgets()
{
    QLayoutItem* child;
    while ( m_customVariablesLayout->count() > 0 )
    {
        child = m_customVariablesLayout->itemAt(0);
        child->widget()->deleteLater();
        child->widget()->hide();
        child->widget()->setParent( nullptr );
    }
}

void PreferenceWidget::closeEvent( QCloseEvent * event )
{
    bool ok = true;
    if( m_preferences && !m_customVariablesLayout->isEmpty() )
    {
        QMap< QString, IbisPreferences::TypedVariable > paths;
        QWidget *w;
        for( int i = 0; i < m_customVariablesLayout->count(); i++ )
        {
            w = m_customVariablesLayout->itemAt( i )->widget();
            Q_ASSERT(w);
            QLabel *label = w->findChild<QLabel *>( PathForm::LabelWidgetName );
            Q_ASSERT(label);
            QLineEdit * lineEdit = w->findChild<QLineEdit *>( PathForm::CustomVariableWidgetName );
            Q_ASSERT(lineEdit);
            QFileInfo fi(lineEdit->text());
            IbisPreferences::TypedVariable tvar;
            tvar.name = lineEdit->text();
            tvar.varType = IbisPreferences::DIRECTORY_VARIABLE_TYPE;
            if( !fi.exists( ) || !(fi.isDir() || fi.isFile() ) )
            {
                QString tmp("File or directory:\n");
                tmp.append( lineEdit->text() );
                tmp.append( "\ndoes not exist. Please enter a valid name." );
                QMessageBox::critical( 0, "Error", tmp, 1, 0 );
                lineEdit->setText( "" );
                ok = false;
            }
            else
            {
                if( fi.isFile() )
                    tvar.varType = IbisPreferences::FILE_VARIABLE_TYPE;
                paths.insert( label->text(), tvar );
            }
        }
        if (ok)
        m_preferences->SetCustomPaths( paths );
    }
    if( ok )
        event->accept();
    else
        event->ignore();
}
