#include "ibishardwareIGSIOsettingswidget.h"
#include "ui_ibishardwareIGSIOsettingswidget.h"
#include "ibishardwareIGSIO.h"
#include <QDir>

IbisHardwareIGSIOSettingsWidget::IbisHardwareIGSIOSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_igsio(0),
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
