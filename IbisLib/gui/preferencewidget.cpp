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
    m_customPathsLayout = nullptr;
}

PreferenceWidget::~PreferenceWidget()
{
    delete m_customPathsLayout;
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
        if( m_customPathsLayout )
        {
            this->RemoveAllCustomPathsWidgets();
            delete m_customPathsLayout;
        }
        m_customPathsLayout = new QVBoxLayout;
        m_customPathsLayout->setSpacing( 4 );
        m_customPathsLayout->setAlignment( Qt::AlignTop );
        ui->customPathsGroupBox->setLayout( m_customPathsLayout );
        QMap< QString, QString> paths = m_preferences->GetCustomPaths();
        if( !paths.isEmpty() )
        {
            QMap< QString, QString >::iterator it;
            for( it = paths.begin(); it != paths.end(); ++it )
            {
                QString validPath( it.value() );
                QFile validFile( validPath );
                if( !validFile.exists( ) )
                    validPath = "";
                PathForm *nextPath = new PathForm;
                nextPath->SetPath( it.key(), validPath );
                m_customPathsLayout->addWidget( nextPath, 0, Qt::AlignTop );
                connect( nextPath, SIGNAL( PathChanged(QString, QString) ), this, SLOT( ResetPath(QString, QString) ) );
            }
        }
    }
}

void PreferenceWidget::ResetPath( QString pathName, QString path )
{
    m_preferences->RegisterPath( pathName, path );
}

void PreferenceWidget::RemoveAllCustomPathsWidgets()
{
    QLayoutItem* child;
    while ( m_customPathsLayout->count() > 0 )
    {
        child = m_customPathsLayout->itemAt(0);
        child->widget()->deleteLater();
        child->widget()->hide();
        child->widget()->setParent( nullptr );
    }
}

void PreferenceWidget::closeEvent( QCloseEvent * event )
{
    if( m_preferences && !m_customPathsLayout->isEmpty() )
    {
        QMap< QString, QString > paths;
        QWidget *w;
        for( int i = 0; i < m_customPathsLayout->count(); i++ )
        {
            w = m_customPathsLayout->itemAt( i )->widget();
            Q_ASSERT(w);
            QLabel *label = w->findChild<QLabel *>( PathForm::LabelWidgetName );
            Q_ASSERT(label);
            QLineEdit * lineEdit = w->findChild<QLineEdit *>( PathForm::PathWidgetName );
            Q_ASSERT(lineEdit);
            QFile validFile( lineEdit->text() );
            if( !validFile.exists( ) )
            {
                QString tmp("File/Directory:\n");
                tmp.append( lineEdit->text() );
                tmp.append( "\ndoes not exist." );
                QMessageBox msgBox;
                msgBox.setText( tmp );
                msgBox.setInformativeText( "Please enter a valid path." );
                msgBox.setStandardButtons( QMessageBox::Ignore | QMessageBox::Cancel );
                msgBox.setDefaultButton( QMessageBox::Ignore );
                int ret = msgBox.exec();
                lineEdit->setText( "" );
                switch (ret) {
                  case QMessageBox::Ignore:
                      break;
                  case QMessageBox::Cancel:
                  default:
                      event->ignore();
                      return;
                }
            }
            paths.insert( label->text(), lineEdit->text() );
        }
        m_preferences->SetCustomPaths( paths );
    }
    event->accept();
}
