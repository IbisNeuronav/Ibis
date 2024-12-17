#ifndef IBISHARDWAREIGSIOSETTINGSWIDGET_H
#define IBISHARDWAREIGSIOSETTINGSWIDGET_H

#include <QWidget>

namespace Ui
{
class IbisHardwareIGSIOSettingsWidget;
}

class IbisHardwareIGSIO;

class IbisHardwareIGSIOSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IbisHardwareIGSIOSettingsWidget( QWidget * parent = 0 );
    ~IbisHardwareIGSIOSettingsWidget();

    void SetIgsio( IbisHardwareIGSIO * igsio );

protected:
    void UpdateUI();

private slots:

    void on_applyConfigFileButton_clicked();
    void on_autoStartLastConfigCheckBox_toggled( bool checked );
    void OnLogAdded( const QString & );
    void OnLogCleared();

private:
    IbisHardwareIGSIO * m_igsio;

    Ui::IbisHardwareIGSIOSettingsWidget * ui;
};

#endif
