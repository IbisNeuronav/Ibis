/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: savedirectorydialog.cpp,v $
  Language:  C++
  Date:      $Date: 2010-06-30 18:03:12 $
  Version:   $Revision: 1.3 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill 
  All rights reserved.
  
=========================================================================*/
#include "savedirectorydialog.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>

SaveDirectoryDialog::SaveDirectoryDialog( QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
    : EnterTextDialogBase( parent, name, modal, fl )
{
    setName( "SaveDirectoryDialog" );
    setSizeGripEnabled( FALSE );

}

SaveDirectoryDialog::~SaveDirectoryDialog()
{
    // no need to delete child widgets, Qt does it all for us
}


void SaveDirectoryDialog::OKButtonClicked()
{
    accept();
}

void SaveDirectoryDialog::SetInfoText(const QString & info)
{
    infoTextLabel->setText(info);
}

void SaveDirectoryDialog::SetInputLineText(QString & text)
{
    inputTextLineEdit->setText(text);
}

void SaveDirectoryDialog::TextEntered()
{
}

