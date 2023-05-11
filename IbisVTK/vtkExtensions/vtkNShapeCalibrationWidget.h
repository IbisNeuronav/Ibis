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

// .NAME vtkNShapeCalibrationWidget - 3D widget for manipulating a line
// .SECTION Description
// Put a description here...
// .SECTION Caveats
// Note that handles and line can be picked even when they are "behind" other
// actors.  This is an intended feature and not a bug.
// .SECTION See Also
// vtk3DWidget

#ifndef __vtkNShapeCalibrationWidget_h
#define __vtkNShapeCalibrationWidget_h

#include <vtk3DWidget.h>
#include <vtkLineSource.h>  // For passing calls to it

class vtkActor;
class vtkPolyDataMapper;
class vtkPoints;
class vtkPolyData;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkCellPicker;
class vtkDiskSource;
class vtkCircleWithCrossSource;

class vtkNShapeCalibrationWidget : public vtk3DWidget
{
public:
    // Description:
    // Instantiate the object.
    static vtkNShapeCalibrationWidget * New();

    vtkTypeMacro( vtkNShapeCalibrationWidget, vtk3DWidget );
    void PrintSelf( ostream & os, vtkIndent indent ) override;

    // Separating code out with BTX and ETX makes the VTK
    // wrapping process ignore it.  For whatever reason, code
    // using arrays of vtkReals does not wrap correctly.
    // thus, functions using vtkreal arrays are excluded from wrapping

    // Description:
    // Methods that satisfy the superclass' API.
    virtual void SetEnabled( int ) override;
    // BTX
    virtual void PlaceWidget( double bounds[ 6 ] ) override;
    // ETX
    void PlaceWidget() override { this->Superclass::PlaceWidget(); }
    void PlaceWidget( double xmin, double xmax, double ymin, double ymax, double zmin, double zmax ) override
    {
        this->Superclass::PlaceWidget( xmin, xmax, ymin, ymax, zmin, zmax );
    }

    // Description:
    // Set/Get the position of first end point.
    // BTX
    void SetPoint1( double x, double y, double z );

    void SetPoint1( double x[ 3 ] ) { this->SetPoint1( x[ 0 ], x[ 1 ], x[ 2 ] ); }

    double * GetPoint1() { return this->LineSource->GetPoint1(); }

    void GetPoint1( double xyz[ 3 ] ) { this->LineSource->GetPoint1( xyz ); }

    // Description:
    // Set position of other end point.
    void SetPoint2( double x, double y, double z );
    void SetPoint2( double x[ 3 ] ) { this->SetPoint2( x[ 0 ], x[ 1 ], x[ 2 ] ); }

    double * GetPoint2() { return this->LineSource->GetPoint2(); }

    void GetPoint2( double xyz[ 3 ] ) { this->LineSource->GetPoint2( xyz ); }

    // Description:
    // Set the position of the middle handle by finding the point on
    // the line that is closest to the one passed in parameter. If the
    // perpendicular distance between the point passed in parameter and
    // the line doesn't exist, middle position is assigned 0 or 1, depending
    // on the segment end that is the closest to the point passed in parameters.
    void SetMiddlePoint( double x, double y, double z );
    void SetMiddlePoint( double x[ 3 ] ) { this->SetMiddlePoint( x[ 0 ], x[ 1 ], x[ 2 ] ); }
    void GetMiddlePoint( double xyz[ 3 ] );
    double GetMiddlePoint();
    void SetMiddlePoint( double ratio );
    // ETX
    //  Description:
    //  Scale uniformly all handles by a factor f. f = 1.0 has no effect. The handles of
    //  this class ignore the 3DWidget handles size principle that makes handle always
    //  the same size. Handles have a size in world space.
    void SetHandlesSize( double size );
    void ScaleHandles( double f );

    // Description:
    // Force the line widget to be aligned with one of the x-y-z axes.
    // Remember that when the state changes, a ModifiedEvent is invoked.
    // This can be used to snap the line to the axes if it is orginally
    // not aligned.
    vtkSetClampMacro( Align, int, XAxis, None );
    vtkGetMacro( Align, int );
    void SetAlignToXAxis() { this->SetAlign( XAxis ); }
    void SetAlignToYAxis() { this->SetAlign( YAxis ); }
    void SetAlignToZAxis() { this->SetAlign( ZAxis ); }
    void SetAlignToNone() { this->SetAlign( None ); }

    // Description:
    // Enable/disable clamping of the point end points to the bounding box
    // of the data. The bounding box is defined from the last PlaceWidget()
    // invocation, and includes the effect of the PlaceFactor which is used
    // to gram/shrink the bounding box.
    vtkSetMacro( ClampToBounds, int );
    vtkGetMacro( ClampToBounds, int );
    vtkBooleanMacro( ClampToBounds, int );

    // Description:
    // Grab the polydata (including points) that defines the line.  The
    // polydata consists of n+1 points, where n is the resolution of the
    // line. These point values are guaranteed to be up-to-date when either the
    // InteractionEvent or EndInteraction events are invoked. The user provides
    // the vtkPolyData and the points and polyline are added to it.
    void GetPolyData( vtkPolyData * pd );

    // Description:
    // Get the handle properties (the little balls are the handles). The
    // properties of the handles when selected and normal can be
    // manipulated.
    vtkGetObjectMacro( HandleProperty, vtkProperty );
    vtkGetObjectMacro( SelectedHandleProperty, vtkProperty );

    // Description:
    // Get the line properties. The properties of the line when selected
    // and unselected can be manipulated.
    vtkGetObjectMacro( LineProperty, vtkProperty );
    vtkGetObjectMacro( SelectedLineProperty, vtkProperty );

protected:
    vtkNShapeCalibrationWidget();
    ~vtkNShapeCalibrationWidget();

    // BTX - manage the state of the widget
    int State;
    enum WidgetState
    {
        Start = 0,
        MovingHandle,
        MovingLine,
        Scaling,
        ScalingHandles,
        Outside
    };
    // ETX

    // handles the events
    static void ProcessEvents( vtkObject * object, unsigned long event, void * clientdata, void * calldata );

    // ProcessEvents() dispatches to these methods.
    void OnLeftButtonDown();
    void OnLeftButtonUp();
    void OnMiddleButtonDown();
    void OnMiddleButtonUp();
    void OnRightButtonDown();
    void OnRightButtonUp();
    virtual void OnMouseMove();

    // controlling ivars
    int Align;

    // BTX
    enum AlignmentState
    {
        XAxis,
        YAxis,
        ZAxis,
        None
    };
    // ETX

    // the line
    vtkActor * LineActor;
    vtkPolyDataMapper * LineMapper;
    vtkLineSource * LineSource;
    void HighlightLine( int highlight );

    // glyphs representing hot spots (e.g., handles)
    vtkActor ** Handle;
    vtkPolyDataMapper * HandleMapper;
    vtkDiskSource * HandleGeometry;
    vtkActor ** HandleContour;
    vtkPolyDataMapper * HandleContourMapper;
    vtkCircleWithCrossSource * HandleContourGeometry;
    double MiddleHandlePosition;

    void BuildRepresentation();
    virtual void SizeHandles() override;
    void HandlesOn( double length );
    void HandlesOff();
    int HighlightHandle( vtkProp * prop );  // returns cell id
    void HighlightHandles( int highlight );

    // Do the picking
    vtkCellPicker * HandlePicker;
    vtkCellPicker * LinePicker;
    vtkActor * CurrentHandle;
    double LastPosition[ 3 ];
    void SetLinePosition( double x, double y, double z );
    void SetLinePosition( double x[ 3 ] ) { SetLinePosition( x[ 0 ], x[ 1 ], x[ 2 ] ); }

    // Methods to manipulate the hexahedron.
    void Scale( double * p1, double * p2, int X, int Y );

    // Initial bounds
    int ClampToBounds;
    void ClampPosition( double x[ 3 ] );
    int InBounds( double x[ 3 ] );

    // Properties used to control the appearance of selected objects and
    // the manipulator in general.
    vtkProperty * HandleProperty;
    vtkProperty * SelectedHandleProperty;
    vtkProperty * LineProperty;
    vtkProperty * SelectedLineProperty;
    void CreateDefaultProperties();

    void GenerateLine();

private:
    vtkNShapeCalibrationWidget( const vtkNShapeCalibrationWidget & );  // Not implemented
    void operator=( const vtkNShapeCalibrationWidget & );              // Not implemented
};

#endif
