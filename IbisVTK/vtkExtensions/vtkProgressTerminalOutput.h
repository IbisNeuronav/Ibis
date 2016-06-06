#ifndef VTKPROGRESSTERMINALOUTPUT_H
#define VTKPROGRESSTERMINALOUTPUT_H

#include "vtkCommand.h"

class vtkProcessObject;

class vtkProgressTerminalOutput : public vtkCommand
{

public:

    static vtkProgressTerminalOutput * New() { return new vtkProgressTerminalOutput(); }

    void Execute( vtkObject * caller, unsigned long eventId, void * callData );
    
    void AddProcessObject( vtkProcessObject * object );

protected:

    vtkProgressTerminalOutput();
    ~vtkProgressTerminalOutput();

    int PrevPos;

};


#endif
