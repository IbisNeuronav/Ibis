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

// .NAME vtkMultiImagePlaneWidget - 3D widget for reslicing image data
// .SECTION Description
// This 3D widget defines a plane that can be interactively placed in an
// image volume. A nice feature of the object is that the
// vtkMultiImagePlaneWidget, like any 3D widget, will work with the current
// interactor style. That is, if vtkMultiImagePlaneWidget does not handle an
// event, then all other registered observers (including the interactor
// style) have an opportunity to process the event. Otherwise, the
// vtkMultiImagePlaneWidget will terminate the processing of the event that it
// handles.
//
// The core functionality of the widget is provided by a vtkImageReslice
// object which passes its output onto a texture mapping pipeline for fast
// slicing through volumetric data. See the key methods: GenerateTexturePlane(),
// UpdateOrigin() and UpdateNormal() for implementation details.
//
// To use this object, just invoke SetInteractor() with the argument of the
// method a vtkRenderWindowInteractor.  You may also wish to invoke
// "PlaceWidget()" to initially position the widget. If the "i" key (for
// "interactor") is pressed, the vtkMultiImagePlaneWidget will appear. (See
// superclass documentation for information about changing this behavior.)
//
// Selecting the widget with the middle mouse button with and without holding
// the shift or control keys enables complex reslicing capablilites.
// To facilitate use, a set of 'margins' (left, right, top, bottom) are shown as
// a set of plane-axes aligned lines, the properties of which can be changed
// as a group.
// Without keyboard modifiers: selecting in the middle of the margins
// enables translation of the plane along its normal. Selecting one of the
// corners within the margins enables spinning around the plane's normal at its
// center.  Selecting within a margin allows rotating about the center of the
// plane around an axis aligned with the margin (i.e., selecting left margin
// enables rotating around the plane's local y-prime axis).
// With control key modifier: margin selection enables edge translation (i.e., a
// constrained form of scaling). Selecting within the margins enables
// translation of the entire plane.
// With shift key modifier: uniform plane scaling is enabled.  Moving the mouse
// up enlarges the plane while downward movement shrinks it.
//
// Window-level is achieved by using the right mouse button.
// The left mouse button can be used to query the underlying image data
// with a snap-to cross-hair cursor.  Currently, the nearest point in the input
// image data to the mouse cursor generates the cross-hairs.  With oblique
// slicing, this behaviour may appear unsatisfactory. Text display of
// window-level and image coordinates/data values are provided by a text
// actor/mapper pair.
// Events that occur outside of the widget (i.e., no part of the widget is
// picked) are propagated to any other registered obsevers (such as the
// interaction style). Turn off the widget by pressing the "i" key again
// (or invoke the Off() method).
//
// The vtkMultiImagePlaneWidget has several methods that can be used in
// conjunction with other VTK objects. The GetPolyData() method can be used
// to get the polygonal representation of the plane and can be used as input
// for other VTK objects. Typical usage of the widget is to make use of the
// StartInteractionEvent, InteractionEvent, and EndInteractionEvent
// events. The InteractionEvent is called on mouse motion; the other two
// events are called on button down and button up (either left or right
// button).
//
// Some additional features of this class include the ability to control the
// properties of the widget. You can set the properties of: the selected and
// unselected representations of the plane's outline; the text actor via its
// vtkTextProperty; the cross-hair cursor. In addition there are methods to
// constrain the plane so that it is aligned along the x-y-z axes.  Finally,
// one can specify the degree of interpolation (vtkImageReslice): nearest
// neighbour, linear, and cubic.

// .SECTION Thanks
// Thanks to Dean Inglis for developing and contributing this class.
// Based on the Python SlicePlaneFactory from Atamai, Inc.

// .SECTION Caveats
// Note that handles and plane can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkBoxWidget vtkLineWidget  vtkPlaneWidget vtkPointWidget
// vtkPolyDataSourceWidget vtkSphereWidget vtkImplicitPlaneWidget


#ifndef __vtkMultiImagePlaneWidget_h
#define __vtkMultiImagePlaneWidget_h

#include "vtkMulti3DWidget.h"
#include <vector>
#include "vtkMatrix4x4.h"

class vtkActor;
class vtkCellPicker;
class vtkDataSetMapper;
class vtkImageData;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkScalarsToColors;
class vtkLookupTable;
class vtkMatrix4x4;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;
class vtkTexture;
class vtkMultiTextureMapToPlane;
class vtkTransform;

#define VTK_NEAREST_RESLICE 0
#define VTK_LINEAR_RESLICE  1
#define VTK_CUBIC_RESLICE   2

class vtkMultiImagePlaneWidget : public vtkMulti3DWidget
{
public:
    // Description:
    // Instantiate the object.
    static vtkMultiImagePlaneWidget *New();

    vtkTypeMacro(vtkMultiImagePlaneWidget,vtkMulti3DWidget);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    // Description:
    // Methods that satisfy the superclass' API.
    virtual void PlaceWidget(double bounds[6]) override;
    void PlaceWidget() override;

    // Description:
    // Set the vtkImageData* input for the vtkImageReslice.
    int AddInput( vtkImageData * in, vtkScalarsToColors * lut, vtkTransform * t, bool canInterpolate );
    void SetImageHidden( vtkImageData * im, bool hidden );
	void ClearAllInputs();

    // Description:
    // Set the volume and transform that are used to compute the bounds inside which the plane can move
    void SetBoundingVolume( vtkImageData * boundingImage, vtkTransform * boundingTransform );

    // Description:
    // Hide/Show plane in all renderers or selected ones
    void Show( vtkRenderer * ren, int show );
    void Show( int renIndex, int show );
    int IsShown( vtkRenderer * ren );
    void ShowInAllRenderers( int show );

    // Description:
    // Set/Get the origin of the plane.
    void SetOrigin(double x, double y, double z);
    void SetOrigin(double xyz[3]);
    double* GetOrigin();
    void GetOrigin(double xyz[3]);

    // Description:
    // Set/Get the position of the point defining the first axis of the plane.
    void SetPoint1(double x, double y, double z);
    void SetPoint1(double xyz[3]);
    double* GetPoint1();
    void GetPoint1(double xyz[3]);

    // Description:
    // Set/Get the position of the point defining the second axis of the plane.
    void SetPoint2(double x, double y, double z);
    void SetPoint2(double xyz[3]);
    double* GetPoint2();
    void GetPoint2(double xyz[3]);

    // Description:
    // Get the center of the plane.
    double* GetCenter();
    void GetCenter(double xyz[3]);

    // Description:
    // Get the normal to the plane.
    double* GetNormal();
    void GetNormal(double xyz[3]);

    // Description:
    // Get the vector from the plane origin to point1.
    void GetVector1(double v1[3]);

    // Description:
    // Get the vector from the plane origin to point2.
    void GetVector2(double v2[3]);

    // Description:
    // Move the plane along its normal until it hits the point passed and then adjust the cursor to be on the point
    void SetGlobalPosition( double position[3] );  // position is global
    void SetPosition( double position[3] );        // position is in ref volume space
    void GetPosition( double position[3] );
    void MoveNSlices( int nbSlices );

    // Description:
    // Get the distance between the point passed and the plane. Point is in world coords
    double DistanceToPlane( double position[3] );

    // Description:
    // Manage cursor
    void ActivateCursor(int active);
    virtual void SetCursorProperty(vtkProperty*);
    vtkGetObjectMacro(CursorProperty,vtkProperty);

    // Description:
    // Set the interpolation to use when texturing the plane.
    void SetResliceInterpolate(int);
    vtkGetMacro(ResliceInterpolate,int);
    void SetResliceInterpolateToNearestNeighbour()
    {
        this->SetResliceInterpolate(VTK_NEAREST_RESLICE);
    }
    void SetResliceInterpolateToLinear()
    {
        this->SetResliceInterpolate(VTK_LINEAR_RESLICE);
    }
    void SetResliceInterpolateToCubic()
    {
        this->SetResliceInterpolate(VTK_CUBIC_RESLICE);
    }


    // Description:
    // Make sure that the plane remains within the volume.
    // Default is On.
    vtkSetMacro(RestrictPlaneToVolume,int);
    vtkGetMacro(RestrictPlaneToVolume,int);
    vtkBooleanMacro(RestrictPlaneToVolume,int);

    // Description:
    // Let the user control the lookup table. NOTE: apply this method BEFORE
    // applying the SetLookupTable method.
    // Default is Off.
    vtkSetMacro(UserControlledLookupTable,int);
    vtkGetMacro(UserControlledLookupTable,int);
    vtkBooleanMacro(UserControlledLookupTable,int);

    // Description:
    // Specify whether to interpolate the texture or not. When off, the
    // reslice interpolation is nearest neighbour regardless of how the
    // interpolation is set through the API. Set before setting the
    // vtkImageData imput. Default is On.
    void SetTextureInterpolate(int);
    vtkGetMacro(TextureInterpolate,int);
    vtkBooleanMacro(TextureInterpolate,int);

    // Description:
    // Grab the polydata (including points) that defines the plane.  The
    // polydata consists of (res+1)*(res+1) points, and res*res quadrilateral
    // polygons, where res is the resolution of the plane. These point values
    // are guaranteed to be up-to-date when either the InteractionEvent or
    // EndInteraction events are invoked. The user provides the vtkPolyData and
    // the points and polyplane are added to it.
    void GetPolyData(vtkPolyData *pd);

    // Description:
    // Satisfies superclass API.  This will change the state of the widget to
    // match changes that have been made to the underlying PolyDataSource
    void UpdatePlacement(void);

    // Description:
    // Set/Get the plane's outline properties. The properties of the plane's
    // outline when selected and unselected can be manipulated.
    virtual void SetPlaneProperty(vtkProperty*);
    vtkGetObjectMacro(PlaneProperty,vtkProperty);
    virtual void SetSelectedPlaneProperty(vtkProperty*);
    vtkGetObjectMacro(SelectedPlaneProperty,vtkProperty);

    // Description:
    // Convenience method sets the plane orientation normal to the
    // x, y, or z axes.  Default is XAxes (0).
    void SetPlaneOrientation(int);
    vtkGetMacro(PlaneOrientation,int);
    void SetPlaneOrientationToXAxes()
    {
        this->SetPlaneOrientation(0);
    }
    void SetPlaneOrientationToYAxes()
    {
        this->SetPlaneOrientation(1);
    }
    void SetPlaneOrientationToZAxes()
    {
        this->SetPlaneOrientation(2);
    }

    // Description:
    // Set the internal picker to one defined by the user.  In this way,
    // a set of three orthogonal planes can share the same picker so that
    // picking is performed correctly.  The default internal picker can be
    // re-set/allocated by setting to 0 (NULL).
    void SetPicker( int index, vtkCellPicker * picker );

    // Description:
    // Set/Get the internal lookuptable (lut) to one defined by the user, or,
	// alternatively, to the lut of another vtkImagePlaneWidget.  In this way,
    // a set of three orthogonal planes can share the same lut so that
    // window-levelling is performed uniformly among planes.  The default
    // internal lut can be re- set/allocated by setting to 0 (NULL).
	// the volume(vtkImageData) passed is used to identify which input will
	// be affected by the lut.
    virtual void SetLookupTable( vtkImageData * volume, vtkScalarsToColors * lut );
    vtkScalarsToColors * GetLookupTable( vtkImageData * volume );

	// Description:
	// Enable/disable plane rotation/spinning together with display of the margin.
	vtkSetMacro( DisableRotatingAndSpinning, int );
	vtkGetMacro( DisableRotatingAndSpinning, int );
	vtkBooleanMacro( DisableRotatingAndSpinning, int );

	// Description:
	// Show/Hide plane outline
	vtkSetMacro( ShowPlaneOutline, int );
	vtkGetMacro( ShowPlaneOutline, int );
	vtkBooleanMacro( ShowPlaneOutline, int );

	// Description:
	// Show/Hide highlighted plane outline
	vtkSetMacro( ShowHighlightedPlaneOutline, int );
	vtkGetMacro( ShowHighlightedPlaneOutline, int );
	vtkBooleanMacro( ShowHighlightedPlaneOutline, int );

    // Description:
    // Set the properties of the margins.
    virtual void SetMarginProperty(vtkProperty*);
    vtkGetObjectMacro(MarginProperty,vtkProperty);

    void SetSliceThickness( int nbSlices );
    void SetSliceMixMode( int imageIndex, int mode );

    void SetBlendingMode( int imageIndex, int mode );

    // Description:
    // Get the image coordinate position and voxel value.  Currently only
    // supports single component image data.
    int GetCursorData(double xyzv[4]);

    // Description:
    // Set action associated to buttons.
    //BTX
    enum
    {
		NO_ACTION			= 0,
		CURSOR_ACTION       = 1,
        SLICE_MOTION_ACTION = 2
    };
    //ETX
    vtkSetClampMacro(LeftButtonAction,int, NO_ACTION, SLICE_MOTION_ACTION);
    vtkGetMacro(LeftButtonAction, int);
    vtkSetClampMacro(MiddleButtonAction,int, NO_ACTION, SLICE_MOTION_ACTION);
    vtkGetMacro(MiddleButtonAction, int);
    vtkSetClampMacro(RightButtonAction,int, NO_ACTION, SLICE_MOTION_ACTION);
    vtkGetMacro(RightButtonAction, int);

    vtkSetClampMacro(LeftButton2DAction,int, NO_ACTION, SLICE_MOTION_ACTION);
    vtkGetMacro(LeftButton2DAction, int);

    vtkSetClampMacro(LeftButtonCtrlAction,int, NO_ACTION, SLICE_MOTION_ACTION);
	vtkGetMacro(LeftButtonCtrlAction, int);
    vtkSetClampMacro(MiddleButtonCtrlAction,int, NO_ACTION, SLICE_MOTION_ACTION);
	vtkGetMacro(MiddleButtonCtrlAction, int);
    vtkSetClampMacro(RightButtonCtrlAction,int, NO_ACTION, SLICE_MOTION_ACTION);
	vtkGetMacro(RightButtonCtrlAction, int);

    vtkSetClampMacro(LeftButtonShiftAction,int, NO_ACTION, SLICE_MOTION_ACTION);
	vtkGetMacro(LeftButtonShiftAction, int);
    vtkSetClampMacro(MiddleButtonShiftAction,int, NO_ACTION, SLICE_MOTION_ACTION);
	vtkGetMacro(MiddleButtonShiftAction, int);
    vtkSetClampMacro(RightButtonShiftAction,int, NO_ACTION, SLICE_MOTION_ACTION);
	vtkGetMacro(RightButtonShiftAction, int);

    double * GetCurrentRotationAngles(){ return this->CurrentRotationAngles; }
    void SetCurrentRotationAngles(double angles[3])
    {
        this->CurrentRotationAngles[0] = angles[0];
        this->CurrentRotationAngles[1] = angles[1];
        this->CurrentRotationAngles[2] = angles[2];
    }

    void UpdateCursorWithOrientation();

	enum PlaneMoveMethod
	{
		Move2D,
		Move3D
	};
	void SetPlaneMoveMethod( int rendererIndex, PlaneMoveMethod moveMethod );

    double * RotatePlaneOrientation(double angles[]);

protected:

    vtkMultiImagePlaneWidget();
    ~vtkMultiImagePlaneWidget();

    // Description:
    // Let subclasses react the the addition of interactors
    void InternalAddRenderer( vtkRenderer * ren, vtkAssembly * assembly ) override;
    void InternalRemoveRenderer( int index ) override;

	// Utility
    void AddTextureToPlane( vtkActor * planeActor, vtkPolyDataMapper * planeMapper, int inputIndex, int textureIndex );
    void UpdateTextureUnits();

    void InternalEnable() override;
    void InternalDisable() override;
    void DisableForOneRenderer( int rendererIndex );

    void ClearActors();
    void SetActorsTransforms();

    void EnforceRestrictPlaneToVolume();

    int LeftButtonAction;
    int MiddleButtonAction;
    int RightButtonAction;

    int LeftButton2DAction;

	int LeftButtonCtrlAction;
	int MiddleButtonCtrlAction;
	int RightButtonCtrlAction;

	int LeftButtonShiftAction;
	int MiddleButtonShiftAction;
	int RightButtonShiftAction;

    //BTX - manage the state of the widget
    int State;
    enum WidgetState
    {
        Start=0,
        Cursoring,
        Pushing,
        Spinning,
        Rotating,
        Moving,
        Scaling,
        Outside
    };
    //ETX

    // Catch events from vtk3DWidget2
    virtual void OnMouseMove() override;
    virtual void OnLeftButtonDown() override;
    virtual void OnLeftButtonUp() override;
    virtual void OnMiddleButtonDown() override;
    virtual void OnMiddleButtonUp() override;
    virtual void OnRightButtonDown() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheelForward() override;
    virtual void OnMouseWheelBackward() override;
	void OnButtonDown( int action, int shiftAction, int ctrlAction );
	void OnButtonUp( int action, int shiftAction, int ctrlAction );

    virtual void StartSliceMotion();
    virtual void StopSliceMotion();
    virtual void StartCursor();
    virtual void StopCursor();
    void SetCursorPosition( double pos[3] );

    // controlling ivars
    int   Interaction; // Is the widget responsive to mouse events
    int   PlaneOrientation;
    int   RestrictPlaneToVolume;
    double OriginalWindow;
    double OriginalLevel;
    double CurrentWindow;
    double CurrentLevel;
    int   ResliceInterpolate;
    int   TextureInterpolate;
    int   UserControlledLookupTable;
    int   DisplayText;
	int   DisableRotatingAndSpinning;
    double MarginSize;
	int  ShowHighlightedPlaneOutline;
	int  ShowPlaneOutline;
	std::vector< PlaneMoveMethod > PlaneMoveMethods;

    double CurrentRotationAngles[3];
    double PreviousRotationAngles[3];

    typedef std::vector<vtkActor*> ActorVec;
    typedef std::vector<vtkPolyDataMapper*> PolyDataMapperVec;

    // The geometric represenation of the plane and it's outline
    vtkPlaneSource    *PlaneSource;
    vtkMultiTextureMapToPlane *TexturePlaneCoords;
    double             Normal[3]; // plane normal normalized
    vtkPolyData       *PlaneOutlinePolyData;
    ActorVec           PlaneOutlineActors;
    PolyDataMapperVec  PlaneOutlineMappers;
    void               HighlightPlane(int highlight);
    void               GeneratePlaneOutline();

    // Re-builds the plane outline based on the plane source
    void BuildRepresentation();

    // Do the picking
    typedef std::vector<vtkCellPicker*> PickerVec;
    PickerVec PlanePickers;

    // Methods to manipulate the plane
    void WindowLevel(int X, int Y);
    void Push(double *p1, double *p2);
    void Spin(double *p1, double *p2);
    void Rotate(double *p1, double *p2, double *vpn);
    void Scale(double *p1, double *p2, int X, int Y);
    void Translate(double *p1, double *p2);

	// One per volume
	struct PerVolumeObjects
	{
        PerVolumeObjects() : ImageData(0), Reslice(0), ColorMap(0), Texture(0), LookupTable(0), IsHidden(false), CanInterpolate(true) {}
		vtkImageData         *ImageData;
		vtkImageReslice      *Reslice;
		vtkImageMapToColors  *ColorMap;
		vtkTexture           *Texture;
        vtkScalarsToColors   *LookupTable;
        bool                  IsHidden;
        bool                  CanInterpolate;
	};

	typedef std::vector< PerVolumeObjects > PerVolumeObjectsVec;
	PerVolumeObjectsVec Inputs;

    vtkImageData * BoundingImage;
    vtkTransform * BoundingTransform;

	int GetPerVolumeIndex( vtkImageData * volume );

	vtkLookupTable       *CreateDefaultLookupTable();

	// Working object (used locally but doesn't store info)
	vtkMatrix4x4         *ResliceAxes;
	vtkTransform         *Transform;

	// One per Renderer
    PolyDataMapperVec     TexturePlaneMappers;
    ActorVec              TexturePlaneActors;

    // Properties used to control the appearance of selected objects and
    // the manipulator in general.  The plane property is actually that for
	// the outline.
    vtkProperty   *PlaneProperty;
    vtkProperty   *SelectedPlaneProperty;
    vtkProperty   *CursorProperty;
    vtkProperty   *MarginProperty;
    void           CreateDefaultProperties();

    // Reslice and texture management
public:
    void UpdateNormal();
protected:
    void ComputeBounds( double globalBounds[] );
    void GetTextureCoordName( std::string & name, int index );

    // The cross-hair cursor
    int                CursorActive;
    vtkPolyData       *CursorPolyData;
    PolyDataMapperVec  CursorMappers;
    ActorVec           CursorActors;
    double             CursorPosition[2];  // (x,y) position of the cursor on the plane. Cursor position is updated using this and plane points and origin
    double             CurrentImageValue; // Set to VTK_FLOAT_MAX when invalid
    void               GenerateCursor();
    void               UpdateCursor(int,int);
    void               UpdateCursor();

    // Oblique reslice control
    double RotateAxis[3];
    double RadiusVector[3];
    void  AdjustState();

    // Visible margins to assist user interaction
    vtkPolyData       *MarginPolyData;
    PolyDataMapperVec  MarginMappers;
    ActorVec           MarginActors;
    int                MarginSelectMode;
    void               GenerateMargins();
    void               UpdateMargins();
    void               ActivateMargins(int);
    
    // Utility function used to transform a vector from
    // world space to input image space.
    void TransformToImageSpace( double * in, double * out );

private:
    vtkMultiImagePlaneWidget(const vtkMultiImagePlaneWidget&);  //Not implemented
    void operator=(const vtkMultiImagePlaneWidget&);  //Not implemented
};

#endif //__vtkMultiImagePlaneWidget_h
