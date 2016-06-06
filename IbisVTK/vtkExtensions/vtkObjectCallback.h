// .NAME vtkObjectCallback - supports function callbacks
// .SECTION Description
// Use vtkObjectCallback for generic function callbacks. That is, this class
// can be used when you wish to execute an object's function (of the signature
// described below) using the Command/Observer design pattern in VTK.
// The callback function should have the form
// <pre>
// void func(vtkObject*, unsigned long eid, void *calldata )
// </pre>
// where the parameter vtkObject* is the object invoking the event; eid is
// the event id (see vtkCommand.h); and calldata is
// data that the vtkObject::InvokeEvent() may send with the callback. For
// example, the invocation of the ProgressEvent sends along the progress
// value as calldata.
// .SECTION See Also
// vtkCommand vtkCallbackCommand

#ifndef __vtkObjectCallback_h
#define __vtkObjectCallback_h


#include "vtkCommand.h"


template <class T>
class vtkObjectCallback : public vtkCommand
{
    
public:
    
    typedef void (T::*MethodAttrib)( vtkObject *, unsigned long, void * );
    typedef void (T::*MethodNoAttrib)();
    
    static vtkObjectCallback *New()
    {
        return new vtkObjectCallback;
    };

    // Description:
    // Satisfy the superclass API for callbacks. Recall that the caller is
    // the instance invoking the event; eid is the event id (see
    // vtkCommand.h); and calldata is information sent when the callback
    // was invoked (e.g., progress value in the vtkCommand::ProgressEvent).
    void Execute(vtkObject *caller, unsigned long eid, void *callData)
    {
        if( this->Object )
        {
            if( this->CallbackAttrib )
                (this->Object->*CallbackAttrib)( caller, eid, callData );
            if( this->CallbackNoAttrib )
                (this->Object->*CallbackNoAttrib)();
        }
    }

    void SetCallback( T * object, MethodAttrib func )
    {
        this->Object = object;
        this->CallbackAttrib = func;
    }    
    
    void SetCallback( T * object, MethodNoAttrib func )
    {
        this->Object = object;
        this->CallbackNoAttrib = func;
    }
    

protected:
    T * Object;
    
    vtkObjectCallback() : Object(0), CallbackAttrib(0), CallbackNoAttrib(0) {}
    ~vtkObjectCallback() {}
    
    MethodAttrib CallbackAttrib;
    MethodNoAttrib CallbackNoAttrib;
};



#endif
