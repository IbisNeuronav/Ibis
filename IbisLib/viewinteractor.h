#ifndef VIEWINTERACTOR_H
#define VIEWINTERACTOR_H

class View;

// ViewInteractor is an interface that is reimplemented by SceneObject and IbisPlugin (and potentially others)
// to be able to receive callbacks from Views on Interaction events. For ViewInteractor derived classes
// instances to receive the callback, corresponding virtual functions of the interface have to be reimplemented
// and the instance has to be registered with the view (View::AddInteractionObject)
class ViewInteractor
{
public:
    ViewInteractor() {}
    virtual ~ViewInteractor() {}

    virtual bool OnLeftButtonPressed( View *, int, int, unsigned ) { return false; }
    virtual bool OnLeftButtonReleased( View *, int, int, unsigned ) { return false; }
    virtual bool OnRightButtonPressed( View *, int, int, unsigned ) { return false; }
    virtual bool OnRightButtonReleased( View *, int, int, unsigned ) { return false; }
    virtual bool OnMiddleButtonPressed( View *, int, int, unsigned ) { return false; }
    virtual bool OnMiddleButtonReleased( View *, int, int, unsigned ) { return false; }
    virtual bool OnMouseMoved( View *, int, int, unsigned ) { return false; }
};

#endif
