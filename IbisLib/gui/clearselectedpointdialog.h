/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: clearselectedpointdialog.h,v $
  Language:  C++
  Date:      $Date: 2010-06-30 18:03:08 $
  Version:   $Revision: 1.5 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill 
  All rights reserved.
  
=========================================================================*/

#ifndef CLEARSELECTEDPOINTDIALOG_H
#define CLEARSELECTEDPOINTDIALOG_H
#include "clearselectedpointdialogbase.h"


 class PointsObject;

class ClearSelectedPointDialog : public ClearSelectedPointDialogBase
{
    Q_OBJECT

public:
    ClearSelectedPointDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    virtual ~ClearSelectedPointDialog();

    void SetPointsObject(PointsObject *o);
    void SetSelectedPointNumber(QString num);
    
public slots:
    void PointNumberEditChanged();
    void OKButtonClicked();
    void CancelButtonClicked();
    
protected:
    int m_numberOfPoints;
    int m_selectedPointNumber;
    PointsObject *m_points;
    
};

#endif // CLEARSELECTEDPOINTDIALOG_H
