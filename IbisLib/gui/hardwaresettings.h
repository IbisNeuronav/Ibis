/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_HARDWARESETTINGS_H
#define TAG_HARDWARESETTINGS_H

#include "ui_hardwaresettings.h"
#include <QString>
#include <QCloseEvent>

class HardwareSettings :  public QDialog, public Ui::HardwareSettings
{
Q_OBJECT

public:

    HardwareSettings( QString defaultCfg, QWidget * parent = 0 );
    virtual ~HardwareSettings();
    
    QString  GetDefaultHardwareSettingsPath() { return DefaultHardwareSettingsPath; }
    QString  GetUserHardwareSettingsPath() { return UserHardwareSettingsPath; }
    QString  GetSelectedHardwareSettingsPath() { return SelectedHardwareSettingsPath; }
    bool GetSettingsStatus();
       
public slots:
    virtual void UserSettingsChanged();
    virtual void LoadUserSettings();
    virtual void FindUserSettingsFile();
    virtual void ActivateLoadButton();
    virtual void closeEvent( QCloseEvent* ce );
    virtual void QuitApplication();

protected:
    
    QString DefaultHardwareSettingsPath;
    QString UserHardwareSettingsPath;
    QString SelectedHardwareSettingsPath;
    bool checkOnClose;
    bool settingsOK; 
};


#endif  // TAG_HARDWARESETTINGS_H
