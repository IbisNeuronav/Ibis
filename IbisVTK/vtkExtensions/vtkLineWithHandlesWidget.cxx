/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkLineWithHandlesWidget.cxx,v $
  Language:  C++
  Date:      $Date: 2006-04-11 20:03:10 $
  Version:   $Revision: 1.3 $
  
    Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
    All rights reserved.
    See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
    
      This software is distributed WITHOUT ANY WARRANTY; without even 
      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
      PURPOSE.  See the above copyright notice for more information.
      
=========================================================================*/
#include "vtkLineWithHandlesWidget.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkPointWidget.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkDiskSource.h"
#include "vtkCircleWithCrossSource.h"

vtkCxxRevisionMacro(vtkLineWithHandlesWidget, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkLineWithHandlesWidget);


vtkLineWithHandlesWidget::vtkLineWithHandlesWidget()
{
    int i;

    this->State = vtkLineWithHandlesWidget::Start;
    this->EventCallbackCommand->SetCallback(vtkLineWithHandlesWidget::ProcessEvents);
    this->Align = vtkLineWithHandlesWidget::XAxis;
    
    //Build the representation of the widget
    
    // Represent the line
    this->LineSource = vtkLineSource::New();
    this->LineSource->SetResolution(1);
    this->LineMapper = vtkPolyDataMapper::New();
    this->LineMapper->SetInput(this->LineSource->GetOutput());
    this->LineActor = vtkActor::New();
    this->LineActor->SetMapper(this->LineMapper);
    
    // Create the handles
    this->HandleGeometry = vtkDiskSource::New();
    this->HandleGeometry->SetInnerRadius( 0.0 );
    this->HandleGeometry->SetOuterRadius( 0.2 );
    this->HandleGeometry->SetCircumferentialResolution( 32 );
    this->HandleMapper = vtkPolyDataMapper::New();
    this->HandleMapper->SetInput( this->HandleGeometry->GetOutput() );
    this->Handle = new vtkActor * [2];
    for (i=0; i<2; i++)
    {
        this->Handle[i] = vtkActor::New();
        this->Handle[i]->SetMapper( this->HandleMapper );
    }

    this->HandleContourGeometry = vtkCircleWithCrossSource::New();
    this->HandleContourGeometry->SetRadius( 0.2 );
    this->HandleContourGeometry->SetResolution( 3 );
    this->HandleContourMapper = vtkPolyDataMapper::New();
    this->HandleContourMapper->SetInput( this->HandleContourGeometry->GetOutput() );
    this->HandleContour = new vtkActor * [2];
    for (i=0; i<2; i++)
    {
        this->HandleContour[i] = vtkActor::New();
        this->HandleContour[i]->SetMapper( this->HandleContourMapper );
    }


    
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
    for (i=1; i>=0; i--)
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

vtkLineWithHandlesWidget::~vtkLineWithHandlesWidget()
{
    this->LineActor->Delete();
    this->LineMapper->Delete();
    this->LineSource->Delete();
    
    this->HandleGeometry->Delete();
    this->HandleMapper->Delete();
    int i;
    for (i=0; i<2; i++)
    {
        this->Handle[i]->Delete();
    }
    delete [] this->Handle;
    
    this->HandleContourGeometry->Delete();
    this->HandleContourMapper->Delete();
    for (i=0; i<2; i++)
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


void vtkLineWithHandlesWidget::SetEnabled( int enabling )
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
        for (int j=0; j<2; j++)
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
        for (int i=0; i<2; i++)
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

void vtkLineWithHandlesWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                         unsigned long event,
                                         void* clientdata, 
                                         void* vtkNotUsed(calldata))
{
    vtkLineWithHandlesWidget * self = reinterpret_cast<vtkLineWithHandlesWidget *>( clientdata );
    
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

void vtkLineWithHandlesWidget::PrintSelf(ostream& os, vtkIndent indent)
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

void vtkLineWithHandlesWidget::BuildRepresentation()
{
    double *pt1 = this->LineSource->GetPoint1();
    double *pt2 = this->LineSource->GetPoint2();
    
    this->Handle[0]->SetPosition(pt1);
    this->Handle[1]->SetPosition(pt2);
    this->HandleContour[0]->SetPosition(pt1);
    this->HandleContour[1]->SetPosition(pt2);
}

void vtkLineWithHandlesWidget::SizeHandles()
{
}

int vtkLineWithHandlesWidget::HighlightHandle(vtkProp *prop)
{
    // first unhighlight anything picked
    if ( this->CurrentHandle )
    {
        this->CurrentHandle->SetProperty(this->HandleProperty);
    }
    
    // set the current handle
    this->CurrentHandle = (vtkActor *)prop;
    
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


void vtkLineWithHandlesWidget::HighlightHandles(int highlight)
{
    if ( highlight )
    {
        this->ValidPick = 1;
        this->HandlePicker->GetPickPosition(this->LastPickPosition);
        this->Handle[0]->SetProperty(this->SelectedHandleProperty);
        this->Handle[1]->SetProperty(this->SelectedHandleProperty);
    }
    else
    {
        this->Handle[0]->SetProperty(this->HandleProperty);
        this->Handle[1]->SetProperty(this->HandleProperty);

    }
}

void vtkLineWithHandlesWidget::HighlightLine(int highlight)
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

void vtkLineWithHandlesWidget::OnLeftButtonDown()
{
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    
    // Okay, make sure that the pick is in the current renderer
    vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
    if ( ren != this->CurrentRenderer )
    {
        this->State = vtkLineWithHandlesWidget::Outside;
        return;
    }
    
    // Okay, we can process this. Try to pick handles first;
    // if no handles picked, then try to pick the line.
    this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
    vtkAssemblyPath * path = this->HandlePicker->GetPath();
    if ( path != NULL )
    {
        this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
        this->State = vtkLineWithHandlesWidget::MovingHandle;
        this->HighlightHandle(path->GetFirstNode()->GetViewProp());
    }
    else
    {
        this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
        path = this->LinePicker->GetPath();
        if ( path != NULL )
        {
            this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
            this->State = vtkLineWithHandlesWidget::MovingLine;
            this->HighlightLine(1);
            double x[3];
            this->LinePicker->GetPickPosition(x);
            this->LastPosition[0] = x[0];
            this->LastPosition[1] = x[1];
            this->LastPosition[2] = x[2];
        }
        else
        {
            this->State = vtkLineWithHandlesWidget::Outside;
            this->HighlightHandle(NULL);
            return;
        }
    }
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->Interactor->Render();
}

void vtkLineWithHandlesWidget::OnLeftButtonUp()
{
    if ( this->State == vtkLineWithHandlesWidget::Outside ||
        this->State == vtkLineWithHandlesWidget::Start )
    {
        return;
    }
    
    this->State = vtkLineWithHandlesWidget::Start;
    this->HighlightHandle(NULL);
    this->HighlightLine(0);
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkLineWithHandlesWidget::OnRightButtonDown()
{
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    
    // Okay, make sure that the pick is in the current renderer
    vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
    if ( ren != this->CurrentRenderer )
    {
        this->State = vtkLineWithHandlesWidget::Outside;
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
        this->State = vtkLineWithHandlesWidget::ScalingHandles;
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
        this->State = vtkLineWithHandlesWidget::Outside;
        return;
    }
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->Interactor->Render();
}

void vtkLineWithHandlesWidget::OnRightButtonUp()
{
    if ( this->State == vtkLineWithHandlesWidget::Outside ||
        this->State == vtkLineWithHandlesWidget::Start )
    {
        return;
    }
    
    this->State = vtkLineWithHandlesWidget::Start;
    this->HighlightLine(0);
    this->HighlightHandles(0);
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkLineWithHandlesWidget::OnMiddleButtonDown()
{
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    
    // Okay, make sure that the pick is in the current renderer
    vtkRenderer *ren = this->Interactor->FindPokedRenderer(X,Y);
    if ( ren != this->CurrentRenderer )
    {
        this->State = vtkLineWithHandlesWidget::Outside;
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
        this->State = vtkLineWithHandlesWidget::Scaling;
    }
    else
    {
        this->LinePicker->Pick(X,Y,0.0,this->CurrentRenderer);
        path = this->LinePicker->GetPath();
        if ( path != NULL )
        {
            this->HighlightHandles(1);
            this->HighlightLine(1);
            this->State = vtkLineWithHandlesWidget::Scaling;
        }
        else
        {
            this->State = vtkLineWithHandlesWidget::Outside;
            this->HighlightLine(0);
            return;
        }
    }
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkLineWithHandlesWidget::OnMiddleButtonUp()
{
    if ( this->State == vtkLineWithHandlesWidget::Outside ||
        this->State == vtkLineWithHandlesWidget::Start )
    {
        return;
    }
    
    this->State = vtkLineWithHandlesWidget::Start;
    this->HighlightLine(0);
    this->HighlightHandles(0);
    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkLineWithHandlesWidget::OnMouseMove()
{
    // See whether we're active
    if ( this->State == vtkLineWithHandlesWidget::Outside || 
        this->State == vtkLineWithHandlesWidget::Start )
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
    if ( this->State == vtkLineWithHandlesWidget::MovingHandle )
    {
        if( this->CurrentHandle == this->Handle[0] )
            this->SetPoint1( pickPoint[0], pickPoint[1], pickPoint[2] );
        else if( this->CurrentHandle == this->Handle[1] )
            this->SetPoint2( pickPoint[0], pickPoint[1], pickPoint[2] );


    }
    else if ( this->State == vtkLineWithHandlesWidget::MovingLine )
    {
        this->SetLinePosition( pickPoint[0], pickPoint[1], pickPoint[2] );
    }
    else if ( this->State == vtkLineWithHandlesWidget::Scaling )
    {
        this->Scale(prevPickPoint, pickPoint, X, Y);
    }
    else if ( this->State == vtkLineWithHandlesWidget::ScalingHandles )
    {
        double factor = 1.0 + (double)(lastY - Y) / (double)100.0;
        this->ScaleHandles( factor );
    }
    
    // Interact, if desired
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    this->Interactor->Render();
}

void vtkLineWithHandlesWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
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

void vtkLineWithHandlesWidget::CreateDefaultProperties()
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

void vtkLineWithHandlesWidget::PlaceWidget(double bds[6])
{
    int i;
    double bounds[6], center[3];
    
    this->AdjustBounds(bds, bounds, center);
    
    if ( this->Align == vtkLineWithHandlesWidget::YAxis )
    {
        this->LineSource->SetPoint1(center[0],bounds[2],center[2]);
        this->LineSource->SetPoint2(center[0],bounds[3],center[2]);
    }
    else if ( this->Align == vtkLineWithHandlesWidget::ZAxis )
    {
        this->LineSource->SetPoint1(center[0],center[1],bounds[4]);
        this->LineSource->SetPoint2(center[0],center[1],bounds[5]);
    }
    else if ( this->Align == vtkLineWithHandlesWidget::XAxis )//default or x-aligned
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

void vtkLineWithHandlesWidget::SetPoint1(double x, double y, double z) 
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

void vtkLineWithHandlesWidget::SetPoint2(double x, double y, double z) 
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



void vtkLineWithHandlesWidget::SetHandlesSize( double size )
{
    this->HandleGeometry->SetOuterRadius( size );
    this->HandleContourGeometry->SetRadius( size );
}


void vtkLineWithHandlesWidget::ScaleHandles( double f )
{
    double outerRadius = this->HandleGeometry->GetOuterRadius();
    outerRadius *= f;
    this->HandleGeometry->SetOuterRadius( outerRadius );
    this->HandleContourGeometry->SetRadius( outerRadius );
}


void vtkLineWithHandlesWidget::SetLinePosition( double x, double y, double z ) 
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


void vtkLineWithHandlesWidget::ClampPosition(double x[3])
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

int vtkLineWithHandlesWidget::InBounds(double x[3])
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

void vtkLineWithHandlesWidget::GetPolyData(vtkPolyData *pd)
{ 
    pd->ShallowCopy(this->LineSource->GetOutput()); 
}
