/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "hardwaresettings.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QEvent>
#include "ignsconfig.h"
#include "ignsmsg.h"
#include "application.h"

HardwareSettings::HardwareSettings( QString defaultCfg, QWidget * parent )
    : QDialog(parent)
{
    setupUi(this);
    QString tmp;
    if( !defaultCfg.isEmpty() )
        tmp = defaultCfg;
    else
        tmp = QDir::homePath() + "/" + IGNS_CONFIGURATION_SUBDIRECTORY + "/" + IGNS_IBIS_CONFIG;
    QFile settings(tmp);
    if (settings.exists())
    {
        DefaultHardwareSettingsPath = tmp;
        UserHardwareSettingsPath = DefaultHardwareSettingsPath;
        SelectedHardwareSettingsPath = DefaultHardwareSettingsPath;
        DefaultHardwareSettingsFilename->setText(DefaultHardwareSettingsPath);
        UserSettingsLineEdit->setText(UserHardwareSettingsPath);
        settingsOK = true;
    }
    else
    {
        DefaultHardwareSettingsPath = "";
        UserHardwareSettingsPath = DefaultHardwareSettingsPath;
        SelectedHardwareSettingsPath = DefaultHardwareSettingsPath;
        DefaultHardwareSettingsFilename->setText(IGNS_MSG_NO_CONFIG_FILE);
        QPalette palette;
        palette.setColor( DefaultHardwareSettingsFilename->foregroundRole(), Qt::red );
        DefaultHardwareSettingsFilename->setPalette(palette);
        UserSettingsLineEdit->setText(UserHardwareSettingsPath);
        LoadUserSettingsButton->setEnabled(0);
        settingsOK = false;
    }
    checkOnClose = true;
}

HardwareSettings::~HardwareSettings()
{
}

void HardwareSettings::UserSettingsChanged()
{
    QString tmp;
    settingsOK = false;
    UserHardwareSettingsPath = UserSettingsLineEdit->text();
    if (!UserHardwareSettingsPath.isEmpty())
    {
        LoadUserSettingsButton->setEnabled(1);
        QFile f(UserHardwareSettingsPath);
        if (f.exists())
            settingsOK = true;
    }
    else
    {
        if (!DefaultHardwareSettingsPath.isEmpty())
        {
            tmp = IGNS_MSG_FILE_NOT_EXISTS + UserHardwareSettingsPath + IGNS_MSG_RESTORE_DEFAULT;
            UserSettingsLineEdit->setText(DefaultHardwareSettingsPath);
            QMessageBox::critical( this, IGNS_MSG_ERROR, tmp, 1, 0 );
            settingsOK = true;
            LoadUserSettingsButton->setEnabled(1);
        }
    }
    if (!settingsOK)
    {
        if (UserHardwareSettingsPath.isEmpty())
            tmp = IGNS_MSG_MAKE_CONFIG;
        else
            tmp = IGNS_MSG_NO_CONFIG_FILE;
        QMessageBox::critical( this, IGNS_MSG_ERROR, tmp, 1, 0 );
        LoadUserSettingsButton->setEnabled(0);
    }
    else
        LoadUserSettings();
    
}

void HardwareSettings::LoadUserSettings()
{
    QFile f(UserSettingsLineEdit->text());
    if (f.exists())
    {
        SelectedHardwareSettingsPath = UserHardwareSettingsPath;
        checkOnClose = false;
        settingsOK = true;
        accept(); 
    } 
    else                 
        UserSettingsChanged();
}

void HardwareSettings::FindUserSettingsFile()
{
    QString dir(QDir::homePath() + "/" + IGNS_CONFIGURATION_SUBDIRECTORY);
    QString filename = Application::GetInstance().GetOpenFileName( IGNS_MSG_OPEN_CFG_FILE, dir, tr("Hardware Settings file (*.xml)") );
    if( !filename.isNull() )
    {
        UserHardwareSettingsPath = filename;
        UserSettingsLineEdit->setText(filename);
    }
}

void HardwareSettings::ActivateLoadButton()
{
    UserHardwareSettingsPath = UserSettingsLineEdit->text();
    if (!UserHardwareSettingsPath.isEmpty())
        LoadUserSettingsButton->setEnabled(1);
    else
        LoadUserSettingsButton->setEnabled(0);
}

void HardwareSettings::closeEvent( QCloseEvent* ce )
{
    if (checkOnClose)
    {
        QString tmp;
        settingsOK = false;
        QFile f(UserSettingsLineEdit->text());
        if (f.exists())
        {
            tmp = IGNS_MSG_LOAD_USER_CFG;
            QMessageBox::critical( this, IGNS_MSG_INFO, tmp, 1, 0 );
            SelectedHardwareSettingsPath = UserSettingsLineEdit->text();
            settingsOK = true;
        }
        else if (!DefaultHardwareSettingsPath.isEmpty())
        {
            tmp = IGNS_MSG_LOAD_DEFAULT_CFG;
            QMessageBox::critical( this, IGNS_MSG_INFO, tmp, 1, 0 );
            SelectedHardwareSettingsPath = DefaultHardwareSettingsPath;
            settingsOK = true;
        }
    }
    ce->accept();                   
}

bool HardwareSettings::GetSettingsStatus()
{
    if (!settingsOK)
        QMessageBox::critical( this, IGNS_MSG_INFO, IGNS_MSG_MAKE_CONFIG, 1, 0 );
    return settingsOK;
}

void HardwareSettings::QuitApplication()
{
    reject();
}
