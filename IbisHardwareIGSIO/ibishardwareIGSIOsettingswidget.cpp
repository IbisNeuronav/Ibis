#include "ibishardwareIGSIOsettingswidget.h"
#include "ui_ibishardwareIGSIOsettingswidget.h"
#include "ibishardwareIGSIO.h"
#include "logger.h"

#include <QDir>

IbisHardwareIGSIOSettingsWidget::IbisHardwareIGSIOSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_igsio(nullptr),
    ui(new Ui::IbisHardwareIGSIOSettingsWidget)
{
    ui->setupUi(this);
}

IbisHardwareIGSIOSettingsWidget::~IbisHardwareIGSIOSettingsWidget()
{
    delete ui;
}

void IbisHardwareIGSIOSettingsWidget::SetIgsio( IbisHardwareIGSIO * igsio )
{
    m_igsio = igsio;

    // Fill log textbox with current log text and watch
    ui->logTextEdit->setText( m_igsio->GetLogger()->GetAll() );
    connect( m_igsio->GetLogger(), SIGNAL(LogAdded(const QString&)), this, SLOT(OnLogAdded(const QString&)) );
    connect( m_igsio->GetLogger(), SIGNAL(LogCleared()), this, SLOT(OnLogCleared()) );

    // Update rest of gui
    UpdateUI();
}

void IbisHardwareIGSIOSettingsWidget::UpdateUI()
{
    QString ibisPlusConfigDir = m_igsio->GetIbisPlusConfigDirectory();

    // Get the list of all subdirectories
    QDir baseDir( ibisPlusConfigDir );
    QStringList filters;
    filters << "*.xml";
    baseDir.setNameFilters(filters);
    QFileInfoList allConfigFiles = baseDir.entryInfoList();

    ui->configFileComboBox->clear();
    ui->configFileComboBox->addItem( "None" );
    ui->configFileComboBox->setCurrentIndex( 0 );
    for( int i = 0; i < allConfigFiles.size(); ++i )
    {
        QString fileName = allConfigFiles[i].fileName();
        ui->configFileComboBox->addItem( fileName );
        if( fileName == m_igsio->GetLastIbisPlusConfigFilename() )
            ui->configFileComboBox->setCurrentIndex( i + 1 );
    }

    ui->autoStartLastConfigCheckBox->setChecked( m_igsio->GetAutoStartLastConfig() );
}

void IbisHardwareIGSIOSettingsWidget::on_applyConfigFileButton_clicked()
{
    QString configFileName = ui->configFileComboBox->currentText();
    if (configFileName != "None")
        m_igsio->StartConfig(configFileName);
    else
        m_igsio->ClearConfig();
}

void IbisHardwareIGSIOSettingsWidget::on_autoStartLastConfigCheckBox_toggled( bool checked )
{
    m_igsio->SetAutoStartLastConfig( checked );
}

void IbisHardwareIGSIOSettingsWidget::OnLogAdded(const QString & text)
{
    ui->logTextEdit->append( text );
}

void IbisHardwareIGSIOSettingsWidget::OnLogCleared()
{
    ui->logTextEdit->clear();
}
