/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: removepointdialog.h,v $
  Language:  C++
  Date:      $Date: 2010-06-30 18:03:12 $
  Version:   $Revision: 1.6 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill 
  All rights reserved.
  
=========================================================================*/

#ifndef REMOVEPOINTDIALOG_H
#define REMOVEPOINTDIALOG_H

#include "removepointdialogbase.h"
//Added by qt3to4:
#include <Q3HBoxLayout>

class PointsObject;

class RemovePointDialog : public RemovePointDialogBase
{
    Q_OBJECT

public:
    RemovePointDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    virtual ~RemovePointDialog();

    void SetPointsObject(PointsObject *o) {m_points = o;}
    void SetSelectedPointIndex(int num);

public slots:
    void OKButtonClicked();
    void CancelButtonClicked();

protected:
    Q3HBoxLayout* removePointLayout;

    int m_selectedPointIndex;
    PointsObject *m_points;
    
};

#endif // REMOVEPOINTDIALOG_H
