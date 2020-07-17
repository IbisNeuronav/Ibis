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

// .NAME vtkMulti3DWidget - 3D widgets to be used with multiple windows.
// .SECTION Description
// This class is the base class of all 3D widgets that support the use of
// multiple windows. It is used pretty much like vtk3DWidget except that
// any number of interactor can be added. If functions of base classes are
// used, it generally affects the widgets in all the window that are controled
// by the object. For every interactor, it is also possible to specify a
// vtkProp3D on which the geometry of the widget can be added for every interactor.
// .SECTION See Also
// vtkImagePlaneWidget2


#ifndef __vtkMulti3DWidget_h
#define __vtkMulti3DWidget_h


#include <vtkSmartPointer.h>
#include "vtkMultiInteractorObserver.h"
#include <vector>

class vtkAssembly;
class vtkDataSet;
class vtkRenderer;
class vtkProp3D;
template< class T > class vtkObjectCallback;


class vtkMulti3DWidget : public vtkMultiInteractorObserver
{

public:

    vtkMulti3DWidget();
    ~vtkMulti3DWidget();

    vtkTypeMacro(vtkMulti3DWidget,vtkMultiInteractorObserver);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    // Description:
    // Methods that satisfy the superclass' API.
    void SetEnabled(int) override;

    // Description:
    // This method is used to initially place the widget.  The placement of the
    // widget depends on whether a Prop3D or input dataset is provided. If one
    // of these two is provided, they will be used to obtain a bounding box,
    // around which the widget is placed. Otherwise, you can manually specify a
    // bounds with the PlaceWidget(bounds) method. Note: PlaceWidget(bounds)
    // is required by all subclasses; the other methods are provided as
    // convenience methods.
    virtual void PlaceWidget(double bounds[6]) = 0;
	virtual void PlaceWidget() = 0;
    virtual void PlaceWidget(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);

    // Description:
    // these methods are used to manipulate the set of renderers who's scene
    // might be changed. for example, when a subclass of vtk3dwidget is enabled,
    // the geometry of the widget will be added to all the renderers specified
    // using the following set of functions. It is also possible to associate
    // a prop3D to each renderer. That way, the geometry will be added has a
    // child of the associated Prop3D in each renderer.
	int AddRenderer( vtkRenderer * ren, vtkAssembly * assembly = 0 );
    int GetNumberOfRenderers();
    vtkRenderer * GetRenderer( int index );
    vtkAssembly * GetAssembly( int index );
    vtkAssembly * GetAssembly( vtkRenderer * ren );
    void RemoveRenderer( vtkRenderer * ren );


    // Description:
    // Set/Get a factor representing the scaling of the widget upon placement
    // (via the PlaceWidget() method). Normally the widget is placed so that
    // it just fits within the bounding box defined in PlaceWidget(bounds).
    // The PlaceFactor will make the widget larger (PlaceFactor > 1) or smaller
    // (PlaceFactor < 1). By default, PlaceFactor is set to 0.5.
    vtkSetClampMacro(PlaceFactor,double,0.01,VTK_FLOAT_MAX);
    vtkGetMacro(PlaceFactor,double);

    // Description:
    // Set/Get the factor that controls the size of the handles that
    // appear as part of the widget. These handles (like spheres, etc.)
    // are used to manipulate the widget, and are sized as a fraction of
    // the screen diagonal.
    vtkSetClampMacro(HandleSize,double,0.001,0.5);
    vtkGetMacro(HandleSize,double);

    // Description:
    // Enable/disable mouse interaction so the widget remains on display.
    void SetInteraction(int interact);
    vtkGetMacro(Interaction,int);
    vtkBooleanMacro(Interaction,int);

    // Description:
    // Let the user disable the widget if shift and/or control key is pressed
	vtkGetMacro( NoModifierDisables, int );
	vtkSetMacro( NoModifierDisables, int );
	vtkBooleanMacro( NoModifierDisables, int );
    vtkGetMacro( ControlDisables, int );
    vtkSetMacro( ControlDisables, int );
    vtkBooleanMacro( ControlDisables, int );
    vtkGetMacro( ShiftDisables, int );
    vtkSetMacro( ShiftDisables, int );
    vtkBooleanMacro( ShiftDisables, int );

protected:

    // Description:
    // Let subclasses react the the addition of renderers
    virtual void InternalAddRenderer( vtkRenderer * ren, vtkAssembly * assembly ) {}
    virtual void InternalRemoveRenderer( int index ) {}
    
    // Description:
    // Let subclasses react to the addition-removal of interactors
    virtual void InternalRemoveInteractor( int index ) override;

    // Description:
    // Get the index of the renderer passed in parameter, -1
    // if this renderer is not referenced by this object.
    int GetRendererIndex( vtkRenderer * ren );

    // Description:
    // Helper method for subclasses. index is the index of the renderer for which we
    // want to perform the operation.
    void ComputeDisplayToWorld( unsigned int index, double x, double y, double z, double worldPt[4] );
    void ComputeWorldToDisplay( unsigned int index, double x, double y, double z, double displayPt[3] );

    // Description:
    // Add/Remove all observers to/from all interactors
    void AddObservers();
    void RemoveObservers();

    // Description:
    // Let subclasses add their geometry to the renderers.
    virtual void InternalEnable() {}
    virtual void InternalDisable() {}

    // Description:
    // Render all RenderWindowInteractors
    void RenderAll();
    
    // Description:
    // Utility function to find the uppermost parent of a vtkProp. If the prop passed in
    // parameter is not attached to the renderer passed in parameter, then 0 is returned.
    vtkAssembly * GetUppermostParent( vtkRenderer * ren, vtkProp3D * prop );

    enum
    {
        NO_BUTTON     = 0,
        LEFT_BUTTON   = 1,
        MIDDLE_BUTTON = 2,
        RIGHT_BUTTON  = 3
    };
    int LastButtonPressed;

    // Let the user disable the widget if shift or control key is pressed
	int NoModifierDisables;
    int ControlDisables;
    int ShiftDisables;

    // Handles the events
    void ProcessEvents( vtkObject * object, unsigned long event, void * calldata );

    // ProcessEvents() dispatches to these methods.
    virtual void OnMouseMove() {}
    virtual void OnLeftButtonDown() {}
    virtual void OnLeftButtonUp() {}
    virtual void OnMiddleButtonDown() {}
    virtual void OnMiddleButtonUp() {}
    virtual void OnRightButtonDown() {}
    virtual void OnRightButtonUp() {}
    virtual void OnMouseWheelForward() {}
    virtual void OnMouseWheelBackward() {}

    // utility function to find the proper renderer
    int FindPokedRenderer( vtkRenderWindowInteractor * interactor, int x, int y );

    // Callback used to observe the interactors
    vtkSmartPointer< vtkObjectCallback<vtkMulti3DWidget> > Callback;

    // Event observed
    typedef std::vector<unsigned long> EventIdVec;
    EventIdVec EventsObserved;

    // Remembers if Interaction is enabled
    int Interaction;

    // has the widget ever been placed
    double PlaceFactor;
    int Placed;
    void AdjustBounds(double bounds[6], double newBounds[6], double center[3]);

    // control the size of handles (if there are any)
    double InitialBounds[6];
    double InitialLength;
    double HandleSize;
    double SizeHandles(double factor);
    virtual void SizeHandles() {}//subclass in turn invokes parent's SizeHandles()

    // used to track the depth of the last pick; also interacts with handle sizing
    int   ValidPick;
    double LastPickPosition[3];

    // The renderers used to interact with the scene
    typedef std::vector<vtkRenderer*> RendererVec;
    RendererVec Renderers;
    typedef std::vector<vtkAssembly*> AssemblyVec;
    AssemblyVec Assemblies;
    int CurrentRendererIndex;

private:

    vtkMulti3DWidget(const vtkMulti3DWidget&);  //Not implemented
    void operator=(const vtkMulti3DWidget&);  //Not implemented
};

#endif
