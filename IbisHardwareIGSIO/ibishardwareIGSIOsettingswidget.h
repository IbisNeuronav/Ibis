#ifndef __SettingsWidget_h_
#define __SettingsWidget_h_

#include <QWidget>

namespace Ui {
class IbisHardwareIGSIOSettingsWidget;
}

class IbisHardwareIGSIO;

class IbisHardwareIGSIOSettingsWidget : public QWidget
{
    Q_OBJECT

public:

    explicit IbisHardwareIGSIOSettingsWidget(QWidget *parent = 0);
    ~IbisHardwareIGSIOSettingsWidget();

    void SetIgsio( IbisHardwareIGSIO * igsio );

protected:

    void UpdateUI();

private slots:

    void on_applyConfigFileButton_clicked();
    void on_autoStartLastConfigCheckBox_toggled(bool checked);

private:

    IbisHardwareIGSIO * m_igsio;

    Ui::IbisHardwareIGSIOSettingsWidget *ui;
};

#endif
