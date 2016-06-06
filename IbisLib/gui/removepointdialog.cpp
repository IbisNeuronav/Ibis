/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: removepointdialog.cpp,v $
  Language:  C++
  Date:      $Date: 2010-06-30 18:03:12 $
  Version:   $Revision: 1.8 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill 
  All rights reserved.
  
=========================================================================*/

#include "removepointdialog.h"
#include <qlabel.h>
#include "pointsobject.h"

RemovePointDialog::RemovePointDialog( QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
    : RemovePointDialogBase( parent, name, modal, fl )
{
    if ( !name )
    setName( "RemovePoint" );

    m_selectedPointIndex = -1;
    m_points = 0;
}

RemovePointDialog::~RemovePointDialog()
{
}

void RemovePointDialog::SetSelectedPointIndex(int num)
{
    m_selectedPointIndex = num;
    QString tmp = tr("Remove Point Number ") + QString::number(m_selectedPointIndex+1) +"?";
    removePointLabel->setText( tmp );
}

void RemovePointDialog::OKButtonClicked()
{
    if (m_points)
    {
        m_points->RemovePoint(m_selectedPointIndex);
    }
    close(TRUE);
}

void RemovePointDialog::CancelButtonClicked()
{
    close(TRUE);
}
