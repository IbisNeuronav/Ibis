#include "vtkQtProgressDialog.h"
#include "vtkProcessObject.h"
#include <qprogressdialog.h>
#include <qapplication.h>

void vtkQtProgressDialog::Execute( vtkObject * caller, unsigned long eventId, void * callData )
{
    if( eventId == vtkCommand::StartEvent )
    {
        m_dlg->reset();
        m_dlg->showNormal();
    }
    else if( eventId == vtkCommand::EndEvent )
    {
        m_dlg->close();
    }
    else if ( eventId == vtkCommand::ProgressEvent )
    {
        vtkProcessObject * process = vtkProcessObject::SafeDownCast( caller );
        double percent = process->GetProgress();
        m_dlg->setValue( (int)(percent * 100) );
    }
}


vtkQtProgressDialog::vtkQtProgressDialog()
{
    m_dlg = new QProgressDialog( "", "Cancel", 0, 100 );
}


vtkQtProgressDialog::~vtkQtProgressDialog()
{
    delete m_dlg;
}
