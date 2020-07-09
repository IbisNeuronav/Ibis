/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

// .NAME vtkMultiInteractorObserver - an abstract superclass for classes observing events invoked by multiple vtkRenderWindowInteractor
// .SECTION Description
// vtkMultiInteractorObserver is an abstract superclass for subclasses that observe
// events invoked by more than one vtkRenderWindowInteractor. These subclasses are 
// typically things like 3D widgets; objects that interact with actors
// in the scene, or interactively probe the scene for information.
//
// vtkMultiInteractorObserver defines the method SetInteractor() and enables and
// disables the processing of events by the vtkMultiInteractorObserver. Use the
// methods EnabledOn() or SetEnabled(1) to turn on the interactor observer,
// and the methods EnabledOff() or SetEnabled(0) to turn off the interactor.
//
// To support interactive manipulation of objects, this class (and
// subclasses) invoke the events StartInteractionEvent, InteractionEvent, and
// EndInteractionEvent.  These events are invoked when the
// vtkMultiInteractorObserver enters a state where rapid response is desired:
// mouse motion, etc. The events can be used, for example, to set the desired
// update frame rate (StartInteractionEvent), operate on data or update a
// pipeline (InteractionEvent), and set the desired frame rate back to normal
// values (EndInteractionEvent). Two other events, EnableEvent and
// DisableEvent, are invoked when the interactor observer is enabled or
// disabled.

// .SECTION See Also
// vtkMulti3DWidget vtkMultiImagePlaneWidget

#ifndef __vtkMultiInteractorObserver_h
#define __vtkMultiInteractorObserver_h

#include <vtkObject.h>
#include <vector>

class vtkRenderWindowInteractor;
class vtkRenderer;
template< class T > class vtkObjectCallback;


class vtkMultiInteractorObserver : public vtkObject
{
    
public:
    
  vtkTypeMacro(vtkMultiInteractorObserver,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Methods for turning the interactor observer on and off, and determining
  // its state. All subclasses must provide the SetEnabled() method.
  // Enabling a vtkMultiInteractorObserver has the side effect of adding
  // observers; disabling it removes the observers. Prior to enabling the
  // vtkMultiInteractorObserver you must set the render window interactor (via
  // SetInteractor()).
  virtual void SetEnabled(int) {};
  int GetEnabled() {return this->Enabled;}
  void EnabledOn() {this->SetEnabled(1);}
  void EnabledOff() {this->SetEnabled(0);}
  void On() {this->SetEnabled(1);}
  void Off() {this->SetEnabled(0);}

  // Description:
  // This method is used to associate the widget with the render window
  // interactor.  Observers of the appropriate events invoked in the render
  // window interactor are set up as a result of this method invocation.
  // The SetInteractor() method must be invoked prior to enabling the
  // vtkMultiInteractorObserver.
  void AddInteractor( vtkRenderWindowInteractor * interactor );
  void RemoveInteractor( vtkRenderWindowInteractor * interactor );
  int GetNumberOfInteractors();
  vtkRenderWindowInteractor * GetInteractor( unsigned int index );
  
  // Description:
  // Set/Get the priority at which events are processed. This is used when
  // multiple interactor observers are used simultaneously. The default value
  // is 0.0 (lowest priority.) Note that when multiple interactor observer
  // have the same priority, then the last observer added will process the
  // event first. (Note: once the SetInteractor() method has been called,
  // changing the priority does not effect event processing. You will have
  // to SetInteractor(NULL), change priority, and then SetInteractor(iren)
  // to have the priority take effect.)
  vtkSetClampMacro(Priority,float,0.0f,1.0f);
  vtkGetMacro(Priority,float);

  // Description:
  // Enable/Disable of the use of a keypress to turn on and off the
  // interactor observer. (By default, the keypress is 'i' for "interactor
  // observer".)  Set the KeyPressActivationValue to change which key
  // activates the widget.)
  vtkSetMacro(KeyPressActivation,int);
  vtkGetMacro(KeyPressActivation,int);
  vtkBooleanMacro(KeyPressActivation,int);
  
  // Description:
  // Specify which key press value to use to activate the interactor observer
  // (if key press activation is enabled). By default, the key press
  // activation value is 'i'. Note: once the SetInteractor() method is
  // invoked, changing the key press activation value will not affect the key
  // press until SetInteractor(NULL)/SetInteractor(iren) is called.
  vtkSetMacro(KeyPressActivationValue,char);
  vtkGetMacro(KeyPressActivationValue,char);

  // Recover currently used interactor
  vtkGetMacro(CurrentInteractorIndex,int);

  // Sets up the keypress-i event.
  virtual void OnChar( vtkObject * caller, unsigned long eventId, void * callData );
  
protected:
    
  vtkMultiInteractorObserver();
  ~vtkMultiInteractorObserver();
  
  // Description:
  // This method is called when a new interactor is added to let
  // subclasses react.
  virtual void InternalAddInteractor() {}
  virtual void InternalRemoveInteractor( int i ) {}

  // Description:
  // Utility routines used to start and end interaction.
  // For example, it switches the display update rate. It does not invoke
  // the corresponding events.
  virtual void StartInteraction();
  virtual void EndInteraction();

  // The state of the widget, whether on or off (observing events or not)
  int Enabled;
  
  // Used to process events
  vtkObjectCallback<vtkMultiInteractorObserver> * KeyPressCallbackCommand; //listens to key activation

  // Priority at which events are processed
  float Priority;

  // Keypress activation controls
  int KeyPressActivation;
  char KeyPressActivationValue;

  // The interactors observed
  typedef std::vector<vtkRenderWindowInteractor*> InteractorVec;
  InteractorVec Interactors;
  int CurrentInteractorIndex;
  
private:
    
  vtkMultiInteractorObserver(const vtkMultiInteractorObserver&);  // Not implemented.
  void operator=(const vtkMultiInteractorObserver&);  // Not implemented.
  
};

#endif
