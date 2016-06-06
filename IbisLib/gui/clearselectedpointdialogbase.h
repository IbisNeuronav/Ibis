#ifndef CLEARSELECTEDPOINTDIALOGBASE_H
#define CLEARSELECTEDPOINTDIALOGBASE_H

#include "ui_clearselectedpointdialogbase.h"
class ClearSelectedPointDialogBase : public QDialog, public Ui::ClearSelectedPointDialogBase
{
    Q_OBJECT

public:
    ClearSelectedPointDialogBase(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~ClearSelectedPointDialogBase();

public slots:
    virtual void OKButtonClicked();
    virtual void CancelButtonClicked();

protected slots:
    virtual void languageChange();

};


#endif // CLEARSELECTEDPOINTDIALOGBASE_H
