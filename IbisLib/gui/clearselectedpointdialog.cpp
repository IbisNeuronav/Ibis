/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: clearselectedpointdialog.cpp,v $
  Language:  C++
  Date:      $Date: 2010-06-30 18:03:08 $
  Version:   $Revision: 1.7 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill 
  All rights reserved.
  
=========================================================================*/

#include "clearselectedpointdialog.h"

#include <QLineEdit>
#include <QMessageBox>
#include "removepointdialog.h"
#include "pointsobject.h"


ClearSelectedPointDialog::ClearSelectedPointDialog( QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
    : ClearSelectedPointDialogBase( parent, name, modal, fl )
{
    if ( !name )
    setName( "ClearSelectedPointDialog" );

    
    connect( pointNumberEdit, SIGNAL( returnPressed ( ) ), this, SLOT( OKButtonClicked( ) ) );
    m_numberOfPoints = 0;
    m_selectedPointNumber = 0;
    m_points = 0;
}

ClearSelectedPointDialog::~ClearSelectedPointDialog()
{
}


void ClearSelectedPointDialog::SetSelectedPointNumber(QString num)
{
    pointNumberEdit->setText(num);
}

void ClearSelectedPointDialog::PointNumberEditChanged()
{
    QString selectedPointText = pointNumberEdit->text();
    m_selectedPointNumber = selectedPointText.toInt();
    if (m_selectedPointNumber > 0 && m_selectedPointNumber <= m_numberOfPoints)
    {
        RemovePointDialog *dlg = new RemovePointDialog(0, "removePoint", TRUE, Qt::WDestructiveClose | Qt::WStyle_StaysOnTop);
        dlg->SetPointsObject(m_points);
        dlg->SetSelectedPointIndex(m_selectedPointNumber-1);
        dlg->show();
        close(TRUE);                   
    }
    else
    {
        QString tmp = "wrong point number " + selectedPointText;
        QMessageBox::warning( this, "Error", tmp, 1, 0 );
        m_selectedPointNumber = 0;
    }
}

void ClearSelectedPointDialog::SetPointsObject(PointsObject *o)
{
    m_points = o;
    m_numberOfPoints = m_points->GetNumberOfPoints();
}

void ClearSelectedPointDialog::OKButtonClicked()
{
    QString selectedPointText = pointNumberEdit->text();
    if (selectedPointText.isNull() || selectedPointText.isEmpty())
        close(TRUE);
    else
        PointNumberEditChanged();
}

void ClearSelectedPointDialog::CancelButtonClicked()
{
    close(TRUE);
}
