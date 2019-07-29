#ifndef PREFERENCEWIDGET_H
#define PREFERENCEWIDGET_H

#include <QWidget>
#include "ibispreferences.h"

class QVBoxLayout;

namespace Ui {
class PreferenceWidget;
}

class PreferenceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreferenceWidget(QWidget *parent = 0);
    ~PreferenceWidget();

    void SetPreferences( IbisPreferences *pref );

protected:
    void closeEvent( QCloseEvent * event );
    IbisPreferences *m_preferences;
    void UpdateUI();

private slots:
    void RemoveCustomVariable( QString varName );
    void ResetCustomVariable( QString customVarName, QString customVar );

private:
    Ui::PreferenceWidget *ui;
    QVBoxLayout *m_customVariablesLayout;
    void RemoveAllCustomVariablesWidgets();
};

#endif // PREFERENCEWIDGET_H
