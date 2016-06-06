#ifndef VTKQTPROGRESSDIALOG_H
#define VTKQTPROGRESSDIALOG_H

#include "vtkCommand.h"


class QProgressDialog;


class vtkQtProgressDialog : public vtkCommand
{

public:

    static vtkQtProgressDialog * New() { return new vtkQtProgressDialog(); }

    void Execute( vtkObject * caller, unsigned long eventId, void * callData );

protected:

    vtkQtProgressDialog();
    ~vtkQtProgressDialog();

    QProgressDialog * m_dlg;

};


#endif
