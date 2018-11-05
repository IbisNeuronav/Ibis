#ifndef PREFERENCEWIDGET_H
#define PREFERENCEWIDGET_H

#include <QWidget>
#include "ibispreferences.h"

class QHBoxLayout;

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

private:
    Ui::PreferenceWidget *ui;
    QHBoxLayout *m_customPathsLayout;
};

#endif // PREFERENCEWIDGET_H
