#include "vtkProgressTerminalOutput.h"
#include "vtkProcessObject.h"


void vtkProgressTerminalOutput::Execute( vtkObject * caller, unsigned long eventId, void * callData )
{
    if( eventId == vtkCommand::StartEvent )
    {
        cout << caller->GetClassName() << ": starting" << endl;
        cout.flush();

    }
    else if( eventId == vtkCommand::EndEvent )
    {
        cout << endl << caller->GetClassName() << ": done" << endl;
        cout.flush();
    }
    else if ( eventId == vtkCommand::ProgressEvent )
    {
        /*vtkProcessObject * process = vtkProcessObject::SafeDownCast( caller );
        float percent = process->GetProgress();
        percent *= 100;*/
        cout << ".";
    }
}


void vtkProgressTerminalOutput::AddProcessObject( vtkProcessObject * object )
{
    object->AddObserver( vtkCommand::StartEvent, this );
    object->AddObserver( vtkCommand::EndEvent, this );
    object->AddObserver( vtkCommand::ProgressEvent, this );
}


vtkProgressTerminalOutput::vtkProgressTerminalOutput()
{
    cout.sync_with_stdio();
}


vtkProgressTerminalOutput::~vtkProgressTerminalOutput()
{
}
