#include "clearselectedpointdialogbase.h"

#include <qvariant.h>
/*
 *  Constructs a ClearSelectedPointDialogBase as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
ClearSelectedPointDialogBase::ClearSelectedPointDialogBase(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
ClearSelectedPointDialogBase::~ClearSelectedPointDialogBase()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ClearSelectedPointDialogBase::languageChange()
{
    retranslateUi(this);
}

void ClearSelectedPointDialogBase::OKButtonClicked()
{
    qWarning("ClearSelectedPointDialogBase::OKButtonClicked(): Not implemented yet");
}

void ClearSelectedPointDialogBase::CancelButtonClicked()
{
    qWarning("ClearSelectedPointDialogBase::CancelButtonClicked(): Not implemented yet");
}

