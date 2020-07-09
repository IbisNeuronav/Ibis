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

#include "vtkNShapeCalibrationWidget.h"

#include <vtkActor.h>
#include <vtkAssemblyNode.h>
#include <vtkAssemblyPath.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPlanes.h>
#include <vtkPointWidget.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkDiskSource.h>

#include "vtkCircleWithCrossSource.h"

vtkStandardNewMacro(vtkNShapeCalibrationWidget);


vtkNShapeCalibrationWidget::vtkNShapeCalibrationWidget()
{
    int i;

    this->State = vtkNShapeCalibrationWidget::Start;
    this->EventCallbackCommand->SetCallback(vtkNShapeCalibrationWidget::ProcessEvents);
    this->Align = vtkNShapeCalibrationWidget::XAxis;
    
    //Build the representation of the widget
    
    // Represent the line
    this->LineSource = vtkLineSource::New();
    this->LineSource->SetResolution(1);
    this->LineMapper = vtkPolyDataMapper::New();
    this->LineMapper->SetInputConnection(this->LineSource->GetOutputPort());
    this->LineActor = vtkActor::New();
    this->LineActor->SetMapper(this->LineMapper);
    
    // Create the handles
    this->HandleGeometry = vtkDiskSource::New();
    this->HandleGeometry->SetInnerRadius( 0.0 );
    this->HandleGeometry->SetOuterRadius( 0.2 );
    this->HandleGeometry->SetCircumferentialResolution( 32 );
    this->HandleMapper = vtkPolyDataMapper::New();
    this->HandleMapper->SetInputConnection( this->HandleGeometry->GetOutputPort() );
    this->Handle = new vtkActor * [3];
    for (i=0; i<3; i++)
    {
        this->Handle[i] = vtkActor::New();
        this->Handle[i]->SetMapper( this->HandleMapper );
    }

    this->HandleContourGeometry = vtkCircleWithCrossSource::New();
    this->HandleContourGeometry->SetRadius( 0.2 );
    this->HandleContourGeometry->SetResolution( 3 );
    this->HandleContourMapper = vtkPolyDataMapper::New();
    this->HandleContourMapper->SetInputConnection( this->HandleContourGeometry->GetOutputPort() );
    this->HandleContour = new vtkActor * [3];
    for (i=0; i<3; i++)
    {
        this->HandleContour[i] = vtkActor::New();
        this->HandleContour[i]->SetMapper( this->HandleContourMapper );
    }

    MiddleHandlePosition = 0.5;
    
    // Define the point coordinates
    double bounds[6];
    bounds[0] = -0.5;
    bounds[1] = 0.5;
    bounds[2] = -0.5;
    bounds[3] = 0.5;
    bounds[4] = -0.5;
    bounds[5] = 0.5;
    this->PlaceFactor = 1.0; //overload parent's value
    this->PlaceWidget(bounds);
    this->ClampToBounds = 0;
    
    // Manage the picking stuff
    this->HandlePicker = vtkCellPicker::New();
    this->HandlePicker->SetTolerance(0.001);
    for (i=2; i>=0; i--)
    {
        this->HandlePicker->AddPickList(this->Handle[i]);
    }
    this->HandlePicker->PickFromListOn();
    
    this->LinePicker = vtkCellPicker::New();
    this->LinePicker->SetTolerance(0.005); //need some fluff
    this->LinePicker->AddPickList(this->LineActor);
    this->LinePicker->PickFromListOn();
    
    this->CurrentHandle = NULL;
    
    // Set up the initial properties
    this->CreateDefaultProperties();
 
}

vtkNShapeCalibrationWidget::~vtkNShapeCalibrationWidget()
{
    this->LineActor->Delete();
    this->LineMapper->Delete();
    this->LineSource->Delete();
    
    this->HandleGeometry->Delete();
    this->HandleMapper->Delete();
    int i;
    for (i=0; i<3; i++)
    {
        this->Handle[i]->Delete();
    }
    delete [] this->Handle;
    
    this->HandleContourGeometry->Delete();
    this->HandleContourMapper->Delete();
    for (i=0; i<3; i++)
    {
        this->HandleContour[i]->Delete();
    }
    delete [] this->HandleContour;

    this->HandlePicker->Delete();
    this->LinePicker->Delete();
    
    this->HandleProperty->Delete();
    this->SelectedHandleProperty->Delete();
    this->LineProperty->Delete();
    this->SelectedLineProperty->Delete();
    
}


void vtkNShapeCalibrationWidget::SetEnabled( int enabling )
{
    if ( ! this->Interactor )
    {
        vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
        return;
    }
    
    if ( enabling ) //-----------------------------------------------------------
    {
        vtkDebugMacro(<<"Enabling line widget");
        
        if ( this->Enabled ) //already enabled, just return
        {
            return;
        }
        
        if ( ! this->CurrentRenderer )
        {
            this->CurrentRenderer = 
                this->Interactor->FindPokedRenderer(this->Interactor->GetLastEventPosition()[0],this->Interactor->GetLastEventPosition()[1]);
            if (this->CurrentRenderer == NULL)
            {
                return;
            }
        }
        
        this->Enabled = 1;
        
        // listen for the following events
        vtkRenderWindowInteractor *i = this->Interactor;
        i->AddObserver(vtkCommand::MouseMoveEvent, 
            this->EventCallbackCommand, this->Priority);
        i->AddObserver(vtkCommand::LeftButtonPressEvent, 
            this->EventCallbackCommand, this->Priority);
        i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
            this->EventCallbackCommand, this->Priority);
        i->AddObserver(vtkCommand::MiddleButtonPressEvent, 
            this->EventCallbackCommand, this->Priority);
        i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, 
            this->EventCallbackCommand, this->Priority);
        i->AddObserver(vtkCommand::RightButtonPressEvent, 
            this->EventCallbackCommand, this->Priority);
        i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
            this->EventCallbackCommand, this->Priority);
        
        // Add the line
        this->CurrentRenderer->AddActor(this->LineActor);
        this->LineActor->SetProperty(this->LineProperty);
        
        // turn on the handles
        for (int j=0; j<3; j++)
        {
            this->CurrentRenderer->AddActor(this->Handle[j]);
            this->CurrentRenderer->AddActor(this->HandleContour[j]);
            this->Handle[j]->SetProperty(this->HandleProperty);
        }
        
        this->BuildRepresentation();
        
        this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
    
    else //disabling----------------------------------------------------------
    {
        vtkDebugMacro(<<"Disabling line widget");
        
        if ( ! this->Enabled ) //already disabled, just return
        {
            return;
        }
        
        this->Enabled = 0;
        
        // don't listen for events any more
        this->Interactor->RemoveObserver(this->EventCallbackCommand);
        
        // turn off the line
        this->CurrentRenderer->RemoveActor(this->LineActor);
        
        // turn off the handles
        for (int i=0; i<3; i++)
        {
            this->CurrentRenderer->RemoveActor(this->Handle[i]);
            this->CurrentRenderer->RemoveActor(this->HandleContour[i]);
        }
        
        
        this->CurrentHandle = NULL;
        this->InvokeEvent(vtkCommand::DisableEvent,NULL);
        this->CurrentRenderer = NULL;
    }
    
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                         unsigned long event,
                                         void* clientdata, 
                                         void* vtkNotUsed(calldata))
{
    vtkNShapeCalibrationWidget * self = reinterpret_cast<vtkNShapeCalibrationWidget *>( clientdata );
    
    //okay, let's do the right thing
    switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
        self->OnLeftButtonDown();
        break;
    case vtkCommand::LeftButtonReleaseEvent:
        self->OnLeftButtonUp();
        break;
    case vtkCommand::MiddleButtonPressEvent:
        self->OnMiddleButtonDown();
        break;
    case vtkCommand::MiddleButtonReleaseEvent:
        self->OnMiddleButtonUp();
        break;
    case vtkCommand::RightButtonPressEvent:
        self->OnRightButtonDown();
        break;
    case vtkCommand::RightButtonReleaseEvent:
        self->OnRightButtonUp();
        break;
    case vtkCommand::MouseMoveEvent:
        self->OnMouseMove();
        break;
    }
}

void vtkNShapeCalibrationWidget::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);
    
    if ( this->HandleProperty )
    {
        os << indent << "Handle Property: " << this->HandleProperty << "\n";
    }
    else
    {
        os << indent << "Handle Property: (none)\n";
    }
    if ( this->SelectedHandleProperty )
    {
        os << indent << "Selected Handle Property: " 
            << this->SelectedHandleProperty << "\n";
    }
    else
    {
        os << indent << "Selected Handle Property: (none)\n";
    }
    
    if ( this->LineProperty )
    {
        os << indent << "Line Property: " << this->LineProperty << "\n";
    }
    else
    {
        os << indent << "Line Property: (none)\n";
    }
    if ( this->SelectedLineProperty )
    {
        os << indent << "Selected Line Property: " 
            << this->SelectedLineProperty << "\n";
    }
    else
    {
        os << indent << "Selected Line Property: (none)\n";
    }
    
    os << indent << "Constrain To Bounds: " 
        << (this->ClampToBounds ? "On\n" : "Off\n");
    
    os << indent << "Align with: ";
    switch ( this->Align ) 
    {
    case XAxis:
        os << "X Axis";
        break;
    case YAxis:
        os << "Y Axis";
        break;
    case ZAxis:
        os << "Z Axis";
        break;
    default:
        os << "None";
    }
    
    double *pt1 = this->LineSource->GetPoint1();
    double *pt2 = this->LineSource->GetPoint2();
    
    os << indent << "Point 1: (" << pt1[0] << ", "
        << pt1[1] << ", "
        << pt1[2] << ")\n";
    os << indent << "Point 2: (" << pt2[0] << ", "
        << pt2[1] << ", "
        << pt2[2] << ")\n";
}

void vtkNShapeCalibrationWidget::BuildRepresentation()
{
    double *pt1 = this->LineSource->GetPoint1();
    double *pt2 = this->LineSource->GetPoint2();
    
    this->Handle[0]->SetPosition(pt1);
    this->Handle[1]->SetPosition(pt2);
    this->HandleContour[0]->SetPosition(pt1);
    this->HandleContour[1]->SetPosition(pt2);

    double diff[3];
    diff[0] = pt2[0] - pt1[0];
    diff[1] = pt2[1] - pt1[1];
    diff[2] = pt2[2] - pt1[2];
    diff[0] *= MiddleHandlePosition;
    diff[1] *= MiddleHandlePosition;
    diff[2] *= MiddleHandlePosition;
    double middlePosition[3];
    middlePosition[0] = pt1[0] + diff[0];
    middlePosition[1] = pt1[1] + diff[1];
    middlePosition[2] = pt1[2] + diff[2];
    this->Handle[2]->SetPosition( middlePosition );
    this->HandleContour[2]->SetPosition( middlePosition );
}

void vtkNShapeCalibrationWidget::SizeHandles()
{
}

int vtkNShapeCalibrationWidget::HighlightHandle(vtkProp *prop)
{
    // first unhighlight anything picked
    if ( this->CurrentHandle )
    {
        this->CurrentHandle->SetProperty(this->HandleProperty);
    }
    
    // set the current handle
    this->CurrentHandle = vtkActor::SafeDownCast(prop);
    
    // find the current handle
    if ( this->CurrentHandle )
    {
        this->ValidPick = 1;
        this->HandlePicker->GetPickPosition(this->LastPickPosition);
        this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
        if( this->CurrentHandle == this->Handle[0] ) return 0;
        else if( this->CurrentHandle == this->Handle[1] ) return 1;
        else return 2;
    }
    else
    {
        return -1;
    }
}


void vtkNShapeCalibrationWidget::HighlightHandles(int highlight)
{
    if ( highlight )
    {
        this->ValidPick = 1;
        this->HandlePicker->GetPickPosition(this->LastPickPosition);
        this->Handle[0]->SetProperty(this->SelectedHandleProperty);
        this->Handle[1]->SetProperty(this->SelectedHandleProperty);
        this->Handle[2]->SetProperty(this->SelectedHandleProperty);
    }
    else
    {
        this->Handle[0]->SetProperty(this->HandleProperty);
        this->Handle[1]->SetProperty(this->HandleProperty);
        this->Handle[2]->SetProperty(this->HandleProperty);
    }
}

void vtkNShapeCalibrationWidget::HighlightLine(int highlight)
{
    if ( highlight )
    {
        this->ValidPick = 1;
        this->LinePicker->GetPickPosition(this->LastPickPosition);
        this->LineActor->SetProperty(this->SelectedLineProperty);
    }
    else
    {
        this->LineActor->SetProperty(this->LineProperty);
    }
}

void vtkNShapeCalibrationWidget::OnLeftButtonDown()
{
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    
    // Okay, make sure that the pick is in the current renderer
    vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
    if ( ren != this->CurrentRenderer )
    {
        this->State = vtkNShapeCalibrationWidget::Outside;
        return;
    }
    
    // Okay, we can process this. Try to pick handles first;
    // if no handles picked, then try to pick the line.
    this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    vtkAssemblyPath * path = this->HandlePicker->GetPath();
    if ( path != NULL )
    {
        this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
        this->State = vtkNShapeCalibrationWidget::MovingHandle;
        this->HighlightHandle(path->GetFirstNode()->GetViewProp());
    }
    else
    {
        this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
        path = this->LinePicker->GetPath();
        if ( path != NULL )
        {
            this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
            this->State = vtkNShapeCalibrationWidget::MovingLine;
            this->HighlightLine(1);
            double x[3];
            this->LinePicker->GetPickPosition(x);
            this->LastPosition[0] = x[0];
            this->LastPosition[1] = x[1];
            this->LastPosition[2] = x[2];
        }
        else
        {
            this->State = vtkNShapeCalibrationWidget::Outside;
            this->HighlightHandle(NULL);
            return;
        }
    }
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::OnLeftButtonUp()
{
    if ( this->State == vtkNShapeCalibrationWidget::Outside ||
        this->State == vtkNShapeCalibrationWidget::Start )
    {
        return;
    }
    
    this->State = vtkNShapeCalibrationWidget::Start;
    this->HighlightHandle(NULL);
    this->HighlightLine(0);
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::OnRightButtonDown()
{
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    
    // Okay, make sure that the pick is in the current renderer
    vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
    if ( ren != this->CurrentRenderer )
    {
        this->State = vtkNShapeCalibrationWidget::Outside;
        return;
    }
    
    // Okay, we can process this. Try to pick handles first;
    // if no handles picked, then pick the bounding box.
    vtkAssemblyPath *path;
    this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->HandlePicker->GetPath();
    if ( path != NULL )
    {
        this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
        this->State = vtkNShapeCalibrationWidget::ScalingHandles;
        this->HighlightHandles(1);
        this->HighlightLine(1);
        double x[3];
        this->LinePicker->GetPickPosition(x);
        this->LastPosition[0] = x[0];
        this->LastPosition[1] = x[1];
        this->LastPosition[2] = x[2];
    }
    else
    {
        this->State = vtkNShapeCalibrationWidget::Outside;
        return;
    }
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::OnRightButtonUp()
{
    if ( this->State == vtkNShapeCalibrationWidget::Outside ||
        this->State == vtkNShapeCalibrationWidget::Start )
    {
        return;
    }
    
    this->State = vtkNShapeCalibrationWidget::Start;
    this->HighlightLine(0);
    this->HighlightHandles(0);
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::OnMiddleButtonDown()
{
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    
    // Okay, make sure that the pick is in the current renderer
    vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
    if ( ren != this->CurrentRenderer )
    {
        this->State = vtkNShapeCalibrationWidget::Outside;
        return;
    }
    
    // Okay, we can process this. Try to pick handles first;
    // if no handles picked, then pick the bounding box.
    vtkAssemblyPath *path;
    this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->HandlePicker->GetPath();
    if ( path != NULL )
    {
        this->HighlightLine(1);
        this->HighlightHandles(1);
        this->State = vtkNShapeCalibrationWidget::Scaling;
    }
    else
    {
        this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
        path = this->LinePicker->GetPath();
        if ( path != NULL )
        {
            this->HighlightHandles(1);
            this->HighlightLine(1);
            this->State = vtkNShapeCalibrationWidget::Scaling;
        }
        else
        {
            this->State = vtkNShapeCalibrationWidget::Outside;
            this->HighlightLine(0);
            return;
        }
    }
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::OnMiddleButtonUp()
{
    if ( this->State == vtkNShapeCalibrationWidget::Outside ||
        this->State == vtkNShapeCalibrationWidget::Start )
    {
        return;
    }
    
    this->State = vtkNShapeCalibrationWidget::Start;
    this->HighlightLine(0);
    this->HighlightHandles(0);
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::OnMouseMove()
{
    // See whether we're active
    if ( this->State == vtkNShapeCalibrationWidget::Outside || 
        this->State == vtkNShapeCalibrationWidget::Start )
    {
        return;
    }
    
    // see if there is an active camera
    vtkCamera * camera = this->CurrentRenderer->GetActiveCamera();
    if ( ! camera )
    {
        return;
    }

    // Compute the two points defining the motion vector
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    int lastX = this->Interactor->GetLastEventPosition()[0];
    int lastY = this->Interactor->GetLastEventPosition()[1];
    
    double focalPoint[4];
    double pickPoint[4];
    double prevPickPoint[4];
    double z;
    
    this->ComputeWorldToDisplay(this->LastPickPosition[0], this->LastPickPosition[1], this->LastPickPosition[2], focalPoint );
    z = focalPoint[2];
    this->ComputeDisplayToWorld(double(lastX), double(lastY), z, prevPickPoint);
    this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);
    
    // Process the motion
    if ( this->State == vtkNShapeCalibrationWidget::MovingHandle )
    {
        if( this->CurrentHandle == this->Handle[0] )
            this->SetPoint1( pickPoint[0], pickPoint[1], pickPoint[2] );
        else if( this->CurrentHandle == this->Handle[1] )
            this->SetPoint2( pickPoint[0], pickPoint[1], pickPoint[2] );
        else
            this->SetMiddlePoint( pickPoint[0], pickPoint[1], pickPoint[2] );

    }
    else if ( this->State == vtkNShapeCalibrationWidget::MovingLine )
    {
        this->SetLinePosition( pickPoint[0], pickPoint[1], pickPoint[2] );
    }
    else if ( this->State == vtkNShapeCalibrationWidget::Scaling )
    {
        this->Scale(prevPickPoint, pickPoint, X, Y);
    }
    else if ( this->State == vtkNShapeCalibrationWidget::ScalingHandles )
    {
        double factor = 1.0 + (double)(lastY - Y) / (double)100.0;
        this->ScaleHandles( factor );
    }
    
    // Interact, if desired
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkNShapeCalibrationWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
    //Get the motion vector
    double v[3];
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
    
    //int res = this->LineSource->GetResolution();
    double *pt1 = this->LineSource->GetPoint1();
    double *pt2 = this->LineSource->GetPoint2();
    
    double center[3];
    center[0] = (pt1[0]+pt2[0]) / 2.0;
    center[1] = (pt1[1]+pt2[1]) / 2.0;
    center[2] = (pt1[2]+pt2[2]) / 2.0;
    
    // Compute the scale factor
    double sf = vtkMath::Norm(v) / sqrt(vtkMath::Distance2BetweenPoints(pt1,pt2));
    if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
        sf = 1.0 + sf;
    }
    else
    {
        sf = 1.0 - sf;
    }
    
    // Move the end points
    double point1[3], point2[3];
    for (int i=0; i<3; i++)
    {
        point1[i] = sf * (pt1[i] - center[i]) + center[i];
        point2[i] = sf * (pt2[i] - center[i]) + center[i];
    }
    
    this->LineSource->SetPoint1(point1);
    this->LineSource->SetPoint2(point2);
    this->LineSource->Update();
    
    this->BuildRepresentation();
}

void vtkNShapeCalibrationWidget::CreateDefaultProperties()
{
    // Handle properties
    this->HandleProperty = vtkProperty::New();
    this->HandleProperty->SetColor(1,1,1);
    this->HandleProperty->SetLineWidth( 1.0 );
    this->HandleProperty->SetOpacity( 0.3 );
    
    this->SelectedHandleProperty = vtkProperty::New();
    this->SelectedHandleProperty->SetColor(1,0,0);
    this->SelectedHandleProperty->SetOpacity( 0.3 );
    this->SelectedHandleProperty->SetLineWidth( 1.0 );
    
    // Line properties
    this->LineProperty = vtkProperty::New();
    this->LineProperty->SetRepresentationToWireframe();
    this->LineProperty->SetAmbient(1.0);
    this->LineProperty->SetAmbientColor(1.0,1.0,1.0);
    this->LineProperty->SetLineWidth(1.0);
    
    this->SelectedLineProperty = vtkProperty::New();
    this->SelectedLineProperty->SetRepresentationToWireframe();
    this->SelectedLineProperty->SetAmbient(1.0);
    this->SelectedLineProperty->SetAmbientColor(0.0,1.0,0.0);
    this->SelectedLineProperty->SetLineWidth(1.0);
}

void vtkNShapeCalibrationWidget::PlaceWidget(double bds[6])
{
    int i;
    double bounds[6], center[3];
    
    this->AdjustBounds(bds, bounds, center);
    
    if ( this->Align == vtkNShapeCalibrationWidget::YAxis )
    {
        this->LineSource->SetPoint1(center[0],bounds[2],center[2]);
        this->LineSource->SetPoint2(center[0],bounds[3],center[2]);
    }
    else if ( this->Align == vtkNShapeCalibrationWidget::ZAxis )
    {
        this->LineSource->SetPoint1(center[0],center[1],bounds[4]);
        this->LineSource->SetPoint2(center[0],center[1],bounds[5]);
    }
    else if ( this->Align == vtkNShapeCalibrationWidget::XAxis )//default or x-aligned
    {
        this->LineSource->SetPoint1(bounds[0],center[1],center[2]);
        this->LineSource->SetPoint2(bounds[1],center[1],center[2]);
    }
    this->LineSource->Update();
    
    for (i=0; i<6; i++)
    {
        this->InitialBounds[i] = bounds[i];
    }
    this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
        (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
        (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
    
    // Position the handles at the end of the lines
    this->BuildRepresentation();
}

void vtkNShapeCalibrationWidget::SetPoint1(double x, double y, double z) 
{
    double xyz[3];
    xyz[0] = x; xyz[1] = y; xyz[2] = z;
    
    if ( this->ClampToBounds )
    {
        this->ClampPosition(xyz);
    }
    this->LineSource->SetPoint1(xyz); 
    this->BuildRepresentation();
}

void vtkNShapeCalibrationWidget::SetPoint2(double x, double y, double z) 
{
    double xyz[3];
    xyz[0] = x; xyz[1] = y; xyz[2] = z;
    
    if ( this->ClampToBounds )
    {
        this->ClampPosition(xyz);
    }
    this->LineSource->SetPoint2(xyz); 
    this->BuildRepresentation();
}

void vtkNShapeCalibrationWidget::SetMiddlePoint( double x, double y, double z )
{
    double * pt1 = this->GetPoint1();
    double * pt2 = this->GetPoint2();
    double v[3];
    v[0] = pt2[0] - pt1[0];
    v[1] = pt2[1] - pt1[1];
    v[2] = pt2[2] - pt1[2];
    double w[3];
    w[0] = x - pt1[0];
    w[1] = y - pt1[1];
    w[2] = z - pt1[2];

    double c1 = vtkMath::Dot(w,v);
    double c2 = vtkMath::Dot(v,v);
    if ( c1 <= 0 )
        this->MiddleHandlePosition = 0.0;
    else if ( c2 <= c1 )
        this->MiddleHandlePosition = 1.0;
    else
        this->MiddleHandlePosition = c1 / c2;
    
    this->BuildRepresentation();
}


void vtkNShapeCalibrationWidget::GetMiddlePoint( double xyz[3] )
{
    double * pt1 = this->GetPoint1();
    double * pt2 = this->GetPoint2();
    xyz[0] = pt1[0] + this->MiddleHandlePosition * ( pt2[0] - pt1[0] );
    xyz[1] = pt1[1] + this->MiddleHandlePosition * ( pt2[1] - pt1[1] );
    xyz[2] = pt1[2] + this->MiddleHandlePosition * ( pt2[2] - pt1[2] );
}


double vtkNShapeCalibrationWidget::GetMiddlePoint()
{
    return this->MiddleHandlePosition;
}


void  vtkNShapeCalibrationWidget::SetMiddlePoint( double ratio )
{
    this->MiddleHandlePosition = ratio;
}


void vtkNShapeCalibrationWidget::SetHandlesSize( double size )
{
    this->HandleGeometry->SetOuterRadius( size );
    this->HandleContourGeometry->SetRadius( size );
}


void vtkNShapeCalibrationWidget::ScaleHandles( double f )
{
    double outerRadius = this->HandleGeometry->GetOuterRadius();
    outerRadius *= f;
    this->HandleGeometry->SetOuterRadius( outerRadius );
    this->HandleContourGeometry->SetRadius( outerRadius );
}


void vtkNShapeCalibrationWidget::SetLinePosition( double x, double y, double z ) 
{
    double p1[3], p2[3], v[3];
    
    // vector of motion
    v[0] = x - this->LastPosition[0];
    v[1] = y - this->LastPosition[1];
    v[2] = z - this->LastPosition[2];
    
    // update position
    this->GetPoint1(p1);
    this->GetPoint2(p2);
    for (int i=0; i<3; i++)
    {
        p1[i] += v[i];
        p2[i] += v[i];
    }
    
    // See whether we can move
    if ( this->ClampToBounds && (!this->InBounds(p1) || !this->InBounds(p2)) )
    {
        return;
    }
    
    this->SetPoint1(p1);
    this->SetPoint2(p2);
    
    // remember last position
    this->LastPosition[0] = x;
    this->LastPosition[1] = y;
    this->LastPosition[2] = z;
}


void vtkNShapeCalibrationWidget::ClampPosition(double x[3])
{
    for (int i=0; i<3; i++)
    {
        if ( x[i] < this->InitialBounds[2*i] )
        {
            x[i] = this->InitialBounds[2*i];
        }
        if ( x[i] > this->InitialBounds[2*i+1] )
        {
            x[i] = this->InitialBounds[2*i+1];
        }
    }
}

int vtkNShapeCalibrationWidget::InBounds(double x[3])
{
    for (int i=0; i<3; i++)
    {
        if ( x[i] < this->InitialBounds[2*i] ||
            x[i] > this->InitialBounds[2*i+1] )
        {
            return 0;
        }
    }
    return 1;
}

void vtkNShapeCalibrationWidget::GetPolyData(vtkPolyData *pd)
{ 
    pd->ShallowCopy(this->LineSource->GetOutput()); 
}
