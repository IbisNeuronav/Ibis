/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: savedirectorydialog.h,v $
  Language:  C++
  Date:      $Date: 2010-06-30 18:03:12 $
  Version:   $Revision: 1.3 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill 
  All rights reserved.
  
=========================================================================*/

#ifndef TAG_SAVEDIRECTORYDIALOG_H
#define TAG_SAVEDIRECTORYDIALOG_H

#include "entertextdialogbase.h"
#include <QDialog>

class SaveDirectoryDialog : public EnterTextDialogBase
{
    Q_OBJECT

public:
    SaveDirectoryDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    virtual ~SaveDirectoryDialog();

    virtual void SetInfoText(const QString &);
    virtual void SetInputLineText(QString & text);
    
public slots:
    virtual void OKButtonClicked();
    virtual void TextEntered();

protected:

};

#endif // TAG_SAVEDIRECTORYDIALOG_H
