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

#include "vtkMultiImagePlaneWidget.h"

#include <vtkActor.h>
#include <vtkAssembly.h>
#include <vtkAssemblyNode.h>
#include <vtkAssemblyPath.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkScalarsToColors.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTexture.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>

#include "vtkMultiTextureMapToPlane.h"
#include "vtkObjectCallback.h"

#include <sstream>

vtkStandardNewMacro(vtkMultiImagePlaneWidget);

vtkCxxSetObjectMacro(vtkMultiImagePlaneWidget, PlaneProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkMultiImagePlaneWidget, SelectedPlaneProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkMultiImagePlaneWidget, CursorProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkMultiImagePlaneWidget, MarginProperty, vtkProperty);

vtkMultiImagePlaneWidget::vtkMultiImagePlaneWidget() : vtkMulti3DWidget()
{
    this->State = vtkMultiImagePlaneWidget::Start;

    // Specify the events to be observed
    this->EventsObserved.push_back( vtkCommand::LeftButtonPressEvent );
    this->EventsObserved.push_back( vtkCommand::LeftButtonReleaseEvent );
    this->EventsObserved.push_back( vtkCommand::MiddleButtonPressEvent );
    this->EventsObserved.push_back( vtkCommand::MiddleButtonReleaseEvent );
    this->EventsObserved.push_back( vtkCommand::RightButtonPressEvent );
    this->EventsObserved.push_back( vtkCommand::RightButtonReleaseEvent );
    this->EventsObserved.push_back( vtkCommand::MouseMoveEvent );
    this->EventsObserved.push_back( vtkCommand::MouseWheelForwardEvent );
    this->EventsObserved.push_back( vtkCommand::MouseWheelBackwardEvent );

    this->PlaneOrientation         = 0;
    this->RestrictPlaneToVolume    = 1;
    this->OriginalWindow           = 1.0;
    this->OriginalLevel            = 0.5;
    this->CurrentWindow            = 1.0;
    this->CurrentLevel             = 0.5;
    this->TextureInterpolate       = 1;
    this->ResliceInterpolate       = VTK_LINEAR_RESLICE;
    this->UserControlledLookupTable= 0;
    this->DisplayText              = 0;
	this->DisableRotatingAndSpinning = 0;
	this->MarginSize = 0.05;
	this->ShowHighlightedPlaneOutline = 1;
    this->ShowPlaneOutline = 0;
    this->CurrentImageValue        = VTK_FLOAT_MAX;
    this->MarginSelectMode         = 8;
    this->BoundingImage            = nullptr;
    this->BoundingTransform        = vtkTransform::New();

    // Represent the plane's outline geometry
    //
    this->PlaneSource = vtkPlaneSource::New();
    this->PlaneSource->SetXResolution(1);
    this->PlaneSource->SetYResolution(1);
    this->TexturePlaneCoords = vtkMultiTextureMapToPlane::New();
    this->TexturePlaneCoords->SetInputConnection( this->PlaneSource->GetOutputPort() );
    this->PlaneOutlinePolyData = vtkPolyData::New();

    this->GeneratePlaneOutline();

	// Work objects used in the updating process
	this->ResliceAxes = vtkMatrix4x4::New();
	this->Transform   = vtkTransform::New();

    // Represent the cross hair cursor
    this->CursorActive = 0;
    this->CursorPolyData = vtkPolyData::New();
    this->GenerateCursor();

    // Represent the oblique positioning margins
    this->MarginPolyData = vtkPolyData::New();
    this->GenerateMargins();

    // Define some default point coordinates
    double bounds[6];
    bounds[0] = -0.5;
    bounds[1] =  0.5;
    bounds[2] = -0.5;
    bounds[3] =  0.5;
    bounds[4] = -0.5;
    bounds[5] =  0.5;

    // Initial creation of the widget, serves to initialize it
    this->PlaceWidget(bounds);

    // Set up the initial properties
    this->PlaneProperty         = nullptr;
    this->SelectedPlaneProperty = nullptr;
    this->CursorProperty        = nullptr;
    this->MarginProperty        = nullptr;
    this->CreateDefaultProperties();

    // Set up actions
    this->LeftButtonAction = vtkMultiImagePlaneWidget::CURSOR_ACTION;
    this->MiddleButtonAction = vtkMultiImagePlaneWidget::SLICE_MOTION_ACTION;
    this->RightButtonAction = vtkMultiImagePlaneWidget::NO_ACTION;
    this->LeftButton2DAction = vtkMultiImagePlaneWidget::CURSOR_ACTION;
	this->LeftButtonCtrlAction = vtkMultiImagePlaneWidget::NO_ACTION;
	this->MiddleButtonCtrlAction = vtkMultiImagePlaneWidget::NO_ACTION;
	this->RightButtonCtrlAction = vtkMultiImagePlaneWidget::NO_ACTION;
	this->LeftButtonShiftAction = vtkMultiImagePlaneWidget::NO_ACTION;
	this->MiddleButtonShiftAction = vtkMultiImagePlaneWidget::NO_ACTION;
	this->RightButtonShiftAction = vtkMultiImagePlaneWidget::NO_ACTION;

    this->LastButtonPressed = vtkMultiImagePlaneWidget::NO_BUTTON;
}

vtkMultiImagePlaneWidget::~vtkMultiImagePlaneWidget()
{
	this->ClearAllInputs();
    this->ClearActors();

    this->PlaneOutlinePolyData->Delete();
    this->PlaneSource->Delete();

    if ( this->PlaneProperty )
    {
        this->PlaneProperty->Delete();
    }

    if ( this->SelectedPlaneProperty )
    {
        this->SelectedPlaneProperty->Delete();
    }

    if ( this->CursorProperty )
    {
        this->CursorProperty->Delete();
    }

    if ( this->MarginProperty )
    {
        this->MarginProperty->Delete();
    }

    this->ResliceAxes->Delete();
    this->Transform->Delete();

    this->TexturePlaneCoords->Delete();

    this->CursorPolyData->Delete();

    this->MarginPolyData->Delete();

    if( this->BoundingImage )
        this->BoundingImage->UnRegister( this );
    if( this->BoundingTransform  )
        this->BoundingTransform->UnRegister( this );
}

template< class T >
void ClearVec( std::vector< T > & vec )
{
    typename std::vector< T >::iterator it = vec.begin();
    for( ; it != vec.end(); ++it )
    {
        if( *it )
            (*it)->Delete();
    }
    vec.clear();
}


void vtkMultiImagePlaneWidget::ClearActors()
{
    ClearVec( this->PlaneOutlineMappers );
    ClearVec( this->PlaneOutlineActors );
    ClearVec( this->TexturePlaneMappers );
    ClearVec( this->TexturePlaneActors );
    ClearVec( this->CursorMappers );
    ClearVec( this->CursorActors );
    ClearVec( this->MarginMappers );
    ClearVec( this->MarginActors );
    ClearVec( this->PlanePickers );
}

void vtkMultiImagePlaneWidget::SetActorsTransforms()
{
    for( int i = 0; i < this->PlaneOutlineActors.size(); ++i )
        this->PlaneOutlineActors[i]->SetUserTransform( this->BoundingTransform );
    for( int i = 0; i < this->TexturePlaneActors.size(); ++i )
        this->TexturePlaneActors[i]->SetUserTransform( this->BoundingTransform );
    for( int i = 0; i < this->CursorActors.size(); ++i )
        this->CursorActors[i]->SetUserTransform( this->BoundingTransform );
    for( int i = 0; i < this->MarginActors.size(); ++i )
        this->MarginActors[i]->SetUserTransform( this->BoundingTransform );
}

std::string vtkMultiImagePlaneWidget::ComposeTextureCoordName( int index )
{
    std::ostringstream os;
    os << "TexCoord_";
    os.width(3);
    os.fill('0');
    os << index;
    return os.str();
}

std::string vtkMultiImagePlaneWidget::ComposeTextureName( int index )
{
    std::ostringstream os;
    os << "Texture_";
    os.width(3);
    os.fill('0');
    os << index;
    return os.str();
}

void vtkMultiImagePlaneWidget::AddTextureToPlane( vtkActor * planeActor, vtkPolyDataMapper * planeMapper, int inputIndex )
{
    vtkTexture * tex = this->Inputs[inputIndex].Texture;
    std::string textureName = ComposeTextureName( inputIndex );
    std::string texCoordName = ComposeTextureCoordName(inputIndex);
    planeMapper->MapDataArrayToMultiTextureAttribute( textureName.c_str(), texCoordName.c_str(), vtkDataObject::FIELD_ASSOCIATION_POINTS );
    planeActor->GetProperty()->SetTexture( textureName.c_str(), tex );
}

void vtkMultiImagePlaneWidget::InternalAddRenderer( vtkRenderer * ren, vtkAssembly * assembly )
{
	this->PlaneMoveMethods.push_back( Move3D );

    // plane outline
    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
    vtkActor * actor = vtkActor::New();
    mapper->SetInputData( this->PlaneOutlinePolyData );
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    actor->SetMapper( mapper );
    actor->PickableOff();
    actor->SetUserTransform( this->BoundingTransform );
    this->PlaneOutlineMappers.push_back( mapper );
    this->PlaneOutlineActors.push_back( actor );

    // texture plane
    this->TexturePlaneCoords->Update();
    mapper = vtkPolyDataMapper::New();
    vtkActor * texturePlaneActor = vtkActor::New();
    mapper->SetInputConnection(this->TexturePlaneCoords->GetOutputPort());
    texturePlaneActor->SetMapper( mapper );
    texturePlaneActor->SetUserTransform( this->BoundingTransform );
    vtkProperty * properties = texturePlaneActor->GetProperty();
    properties->SetColor( 1.0, 1.0, 1.0 );
    properties->SetAmbient( 0.0 );
    properties->SetDiffuse( 1.0 );
    properties->LightingOff();
    properties->ShadingOff();
    for( int i = 0; i < this->Inputs.size(); ++i )
	{
        if( !this->Inputs[ i ].IsHidden )
        {
            AddTextureToPlane( texturePlaneActor, mapper, i );
        }
	}
    texturePlaneActor->PickableOn();
    this->TexturePlaneMappers.push_back( mapper );
    this->TexturePlaneActors.push_back( texturePlaneActor );

    // Cursor
    mapper = vtkPolyDataMapper::New();
    actor = vtkActor::New();
    mapper->SetInputData( this->CursorPolyData );
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    actor->SetMapper( mapper );
    actor->PickableOff();
    actor->SetUserTransform( this->BoundingTransform );
    this->CursorMappers.push_back( mapper );
    this->CursorActors.push_back( actor );

    // Margin
    mapper = vtkPolyDataMapper::New();
    actor = vtkActor::New();
    mapper->SetInputData( this->MarginPolyData );
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    actor->SetMapper( mapper );
    actor->PickableOff();
    actor->VisibilityOff();
    actor->SetUserTransform( this->BoundingTransform );
    this->MarginMappers.push_back( mapper );
    this->MarginActors.push_back( actor );

    // Picker
    vtkCellPicker * picker = vtkCellPicker::New();
    picker->SetTolerance(0.005); //need some fluff
    
    picker->PickFromListOn();
    this->PlanePickers.push_back( picker );
}


template< typename T >
void RemoveVecElement( int index, T & vec )
{
    if( index < vec.size() )
    {
        typename T::iterator it = vec.begin() + index;
        if( (*it) )
            (*it)->Delete();
        vec.erase( it );
    }
}

void vtkMultiImagePlaneWidget::InternalRemoveRenderer( int index )
{
    this->DisableForOneRenderer( index );

	if( index < this->PlaneMoveMethods.size() )
	{
		std::vector< PlaneMoveMethod >::iterator it = this->PlaneMoveMethods.begin() + index;
		this->PlaneMoveMethods.erase( it );
	}

    RemoveVecElement( index, this->PlaneOutlineMappers );
    RemoveVecElement( index, this->PlaneOutlineActors );
    RemoveVecElement( index, this->TexturePlaneMappers );
    RemoveVecElement( index, this->TexturePlaneActors );
    RemoveVecElement( index, this->CursorMappers );
    RemoveVecElement( index, this->CursorActors );
    RemoveVecElement( index, this->MarginMappers );
    RemoveVecElement( index, this->MarginActors );
    RemoveVecElement( index, this->PlanePickers );
}

void vtkMultiImagePlaneWidget::InternalEnable()
{
    int index = 0;
    for( RendererVec::iterator it = this->Renderers.begin(); it != this->Renderers.end(); ++it, ++index )
    {
        // plane outline
        vtkActor * actor = this->PlaneOutlineActors[ index ];
		if( this->ShowPlaneOutline )
			actor->VisibilityOn();
		else
			actor->VisibilityOff();
        actor->SetProperty( this->PlaneProperty );
        if( this->Assemblies[ index ] )
            this->Assemblies[ index ]->AddPart( actor );
        else
            (*it)->AddViewProp( actor );

        // texture plane
        actor = this->TexturePlaneActors[ index ];
        actor->PickableOn();

        if( this->Assemblies[ index ] )
            this->Assemblies[ index ]->AddPart( actor );
        else
            (*it)->AddViewProp( actor );

        // Cursor
        actor = this->CursorActors[ index ];
        actor->SetProperty( this->CursorProperty );
        if( this->Assemblies[ index ] )
            this->Assemblies[ index ]->AddPart( actor );
        else
            (*it)->AddViewProp( actor );

        // Margin
        actor = this->MarginActors[ index ];
        actor->SetProperty( this->MarginProperty );
        if( this->Assemblies[ index ] )
            this->Assemblies[ index ]->AddPart( actor );
        else
            (*it)->AddViewProp( actor );

        // Picker
        if( this->PlanePickers[ index ] )
        {
            vtkAssembly * topAssembly = this->GetUppermostParent( (*it), this->TexturePlaneActors[ index ] );
            if( topAssembly )
            {
                this->PlanePickers[ index ]->AddPickList( topAssembly );
            }
            else
            {
                this->PlanePickers[ index ]->AddPickList( this->TexturePlaneActors[ index ] );
            }
            this->TexturePlaneActors[ index ]->PickableOn();
        }
    }
}


void vtkMultiImagePlaneWidget::InternalDisable()
{
    RendererVec::iterator it = this->Renderers.begin();
    for( int i = 0; i < this->Renderers.size(); ++i, ++it )
    {
        this->DisableForOneRenderer( i );
    }
}

void vtkMultiImagePlaneWidget::DisableForOneRenderer( int rendererIndex )
{
    // turn off the plane
    if( this->Assemblies[ rendererIndex ] )
    {
        this->Assemblies[ rendererIndex ]->RemovePart( this->PlaneOutlineActors[ rendererIndex ] );

        //turn off the texture plane
        this->Assemblies[ rendererIndex ]->RemovePart( this->TexturePlaneActors[ rendererIndex ] );

        //turn off the cursor
        this->Assemblies[ rendererIndex ]->RemovePart( this->CursorActors[ rendererIndex ] );

        //turn off the margins
        this->Assemblies[ rendererIndex ]->RemovePart( this->MarginActors[ rendererIndex ] );
    }
    else
    {
        this->Renderers[ rendererIndex ]->RemoveViewProp( this->PlaneOutlineActors[ rendererIndex ] );

        //turn off the texture plane
        this->Renderers[ rendererIndex ]->RemoveViewProp( this->TexturePlaneActors[ rendererIndex ] );

        //turn off the cursor
        this->Renderers[ rendererIndex ]->RemoveViewProp( this->CursorActors[ rendererIndex ] );

        //turn off the margins
        this->Renderers[ rendererIndex ]->RemoveViewProp( this->MarginActors[ rendererIndex ] );
    }

    if ( this->PlanePickers[ rendererIndex ] )
    {
        this->TexturePlaneActors[ rendererIndex ]->PickableOff();
    }
}

void vtkMultiImagePlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);

    if ( this->PlaneProperty )
    {
        os << indent << "Plane Property:\n";
        this->PlaneProperty->PrintSelf(os,indent.GetNextIndent());
    }
    else
    {
        os << indent << "Plane Property: (none)\n";
    }

    if ( this->SelectedPlaneProperty )
    {
        os << indent << "Selected Plane Property:\n";
        this->SelectedPlaneProperty->PrintSelf(os,indent.GetNextIndent());
    }
    else
    {
        os << indent << "Selected Plane Property: (none)\n";
    }

	os << indent << "Input Volumes (" << this->Inputs.size() << "): \n";
	for( int i = 0; i < this->Inputs.size(); ++i )
	{
		vtkIndent volIndent = indent.GetNextIndent();
		PerVolumeObjects & objects = this->Inputs[i];
		os << volIndent << "Volume " << i << "\n";
		os << volIndent << "LookupTable:\n";
		objects.LookupTable->PrintSelf(os,volIndent.GetNextIndent());
		os << volIndent << "ColorMap:\n";
		objects.ColorMap->PrintSelf(os,volIndent.GetNextIndent());
	}

    if ( this->CursorProperty )
    {
        os << indent << "Cursor Property:\n";
        this->CursorProperty->PrintSelf(os,indent.GetNextIndent());
    }
    else
    {
        os << indent << "Cursor Property: (none)\n";
    }

    if ( this->MarginProperty )
    {
        os << indent << "Margin Property:\n";
        this->MarginProperty->PrintSelf(os,indent.GetNextIndent());
    }
    else
    {
        os << indent << "Margin Property: (none)\n";
    }

    double *o = this->PlaneSource->GetOrigin();
    double *pt1 = this->PlaneSource->GetPoint1();
    double *pt2 = this->PlaneSource->GetPoint2();

    os << indent << "Origin: (" << o[0] << ", "
    << o[1] << ", "
    << o[2] << ")\n";
    os << indent << "Point 1: (" << pt1[0] << ", "
    << pt1[1] << ", "
    << pt1[2] << ")\n";
    os << indent << "Point 2: (" << pt2[0] << ", "
    << pt2[1] << ", "
    << pt2[2] << ")\n";

    os << indent << "Plane Orientation: " << this->PlaneOrientation << "\n";
    os << indent << "Reslice Interpolate: " << this->ResliceInterpolate << "\n";
    os << indent << "Texture Interpolate: "
    << (this->TextureInterpolate ? "On\n" : "Off\n") ;
    os << indent << "Restrict Plane To Volume: "
    << (this->RestrictPlaneToVolume ? "On\n" : "Off\n") ;
    os << indent << "Interaction: "
    << (this->Interaction ? "On\n" : "Off\n") ;
    os << indent << "User Controlled Lookup Table: "
    << (this->UserControlledLookupTable ? "On\n" : "Off\n") ;
    os << indent << "LeftButtonAction: " << this->LeftButtonAction << endl;
    os << indent << "MiddleButtonAction: " << this->MiddleButtonAction << endl;
    os << indent << "RightButtonAction: " << this->RightButtonAction << endl;
    os << indent << "LeftButton2DAction: " << this->LeftButton2DAction << endl;
	os << indent << "LeftButtonCtrlAction: " << this->LeftButtonCtrlAction << endl;
	os << indent << "MiddleButtonCtrlAction: " << this->MiddleButtonCtrlAction << endl;
	os << indent << "RightButtonCtrlAction: " << this->RightButtonCtrlAction << endl;
	os << indent << "LeftButtonShiftAction: " << this->LeftButtonShiftAction << endl;
	os << indent << "MiddleButtonShiftAction: " << this->MiddleButtonShiftAction << endl;
	os << indent << "RightButtonShiftAction: " << this->RightButtonShiftAction << endl;
}

void vtkMultiImagePlaneWidget::BuildRepresentation()
{
    double *o = this->PlaneSource->GetOrigin();
    double *pt1 = this->PlaneSource->GetPoint1();
    double *pt2 = this->PlaneSource->GetPoint2();

    double x[3];
    x[0] = o[0] + (pt1[0]-o[0]) + (pt2[0]-o[0]);
    x[1] = o[1] + (pt1[1]-o[1]) + (pt2[1]-o[1]);
    x[2] = o[2] + (pt1[2]-o[2]) + (pt2[2]-o[2]);

    vtkPoints* points = this->PlaneOutlinePolyData->GetPoints();
    points->SetPoint(0,o);
    points->SetPoint(1,pt1);
    points->SetPoint(2,x);
    points->SetPoint(3,pt2);
    this->PlaneOutlinePolyData->Modified();

    this->PlaneSource->GetNormal(this->Normal);
    vtkMath::Normalize(this->Normal);
}


void vtkMultiImagePlaneWidget::HighlightPlane( int highlight )
{
    if ( highlight )
    {
        for( int i = 0; i < this->PlaneOutlineActors.size(); ++i )
        {
            this->PlaneOutlineActors[ i ]->SetProperty( this->SelectedPlaneProperty );
            if( this->ShowHighlightedPlaneOutline && this->TexturePlaneActors[ i ]->GetVisibility() )
				this->PlaneOutlineActors[ i ]->VisibilityOn();
			else
				this->PlaneOutlineActors[ i ]->VisibilityOff();
        }
        this->PlanePickers[ this->CurrentRendererIndex ]->GetPickPosition( this->LastPickPosition );
    }
    else
    {
        for( int i = 0; i < this->PlaneOutlineActors.size(); ++i )
        {
            this->PlaneOutlineActors[ i ]->SetProperty( this->PlaneProperty );
			if( this->ShowPlaneOutline )
				this->PlaneOutlineActors[ i ]->VisibilityOn();
			else
				this->PlaneOutlineActors[ i ]->VisibilityOff();
        }
    }
}


void vtkMultiImagePlaneWidget::OnLeftButtonDown()
{
    int action = this->LeftButtonAction;
    if( this->PlaneMoveMethods[ this->CurrentInteractorIndex ] == Move2D )
        action = this->LeftButton2DAction;
    OnButtonDown( action, this->LeftButtonShiftAction, this->LeftButtonCtrlAction );
}

void vtkMultiImagePlaneWidget::OnLeftButtonUp()
{
    int action = this->LeftButtonAction;
    if( this->PlaneMoveMethods[ this->CurrentInteractorIndex ] == Move2D )
        action = this->LeftButton2DAction;
    OnButtonUp( action, this->LeftButtonShiftAction, this->LeftButtonCtrlAction );
}

void vtkMultiImagePlaneWidget::OnMiddleButtonDown()
{
	OnButtonDown( this->MiddleButtonAction, this->MiddleButtonShiftAction, this->MiddleButtonCtrlAction );
}

void vtkMultiImagePlaneWidget::OnMiddleButtonUp()
{
	OnButtonUp( this->MiddleButtonAction, this->MiddleButtonShiftAction, this->MiddleButtonCtrlAction );
}

void vtkMultiImagePlaneWidget::OnRightButtonDown()
{
	OnButtonDown( this->RightButtonAction, this->RightButtonShiftAction, this->RightButtonCtrlAction );
}

void vtkMultiImagePlaneWidget::OnRightButtonUp()
{
	OnButtonUp( this->RightButtonAction, this->RightButtonShiftAction, this->RightButtonCtrlAction );
}

void vtkMultiImagePlaneWidget::OnMouseWheelForward()
{
    if( this->State == vtkMultiImagePlaneWidget::Start )
    {
        if( this->PlaneMoveMethods[ this->CurrentInteractorIndex ] == Move2D )
        {
            this->MoveNSlices( 1 );
            this->Callback->AbortFlagOn();
            this->InvokeEvent(vtkCommand::InteractionEvent,0);
            this->RenderAll();
        }
    }
}

void vtkMultiImagePlaneWidget::OnMouseWheelBackward()
{
    if( this->State == vtkMultiImagePlaneWidget::Start )
    {
        if( this->PlaneMoveMethods[ this->CurrentInteractorIndex ] == Move2D )
        {
            this->MoveNSlices( -1 );
            this->Callback->AbortFlagOn();
            this->InvokeEvent(vtkCommand::InteractionEvent,0);
            this->RenderAll();
        }
    }
}

void vtkMultiImagePlaneWidget::OnButtonDown( int action, int shiftAction, int ctrlAction )
{
	bool shiftOn = this->Interactors[ this->CurrentInteractorIndex ]->GetShiftKey();
	bool CtrlOn = this->Interactors[ this->CurrentInteractorIndex ]->GetControlKey();
	if( CtrlOn )
	{
		switch ( ctrlAction )
		{
		case vtkMultiImagePlaneWidget::CURSOR_ACTION:
			this->StartCursor();
			break;
		case vtkMultiImagePlaneWidget::SLICE_MOTION_ACTION:
			this->StartSliceMotion();
			break;
		}
	}
	else if( shiftOn )
	{
		switch ( shiftAction )
		{
		case vtkMultiImagePlaneWidget::CURSOR_ACTION:
			this->StartCursor();
			break;
		case vtkMultiImagePlaneWidget::SLICE_MOTION_ACTION:
			this->StartSliceMotion();
			break;
		}
	}
	else
	{
		switch ( action )
		{
		case vtkMultiImagePlaneWidget::CURSOR_ACTION:
			this->StartCursor();
			break;
		case vtkMultiImagePlaneWidget::SLICE_MOTION_ACTION:
			this->StartSliceMotion();
			break;
		}
	}
}

void vtkMultiImagePlaneWidget::OnButtonUp( int action, int shiftAction, int ctrlAction )
{
    if ( this->State == vtkMultiImagePlaneWidget::Pushing ||
         this->State == vtkMultiImagePlaneWidget::Spinning ||
         this->State == vtkMultiImagePlaneWidget::Rotating ||
         this->State == vtkMultiImagePlaneWidget::Scaling ||
         this->State == vtkMultiImagePlaneWidget::Moving )
        this->StopSliceMotion();

    else if( this->State == vtkMultiImagePlaneWidget::Cursoring )
        this->StopCursor();
}

void vtkMultiImagePlaneWidget::StartSliceMotion()
{
    vtkRenderWindowInteractor * interactor = this->Interactors[ this->CurrentInteractorIndex ];
    int X = interactor->GetEventPosition()[0];
    int Y = interactor->GetEventPosition()[1];

    // Okay, make sure that the pick is in the current renderer
    if( this->CurrentRendererIndex == -1 )
    {
        this->State = vtkMultiImagePlaneWidget::Outside;
        return;
    }
    vtkRenderer * currentRenderer = this->Renderers[ this->CurrentRendererIndex ];

    // Okay, we can process this. If anything is picked, then we
    // can start pushing or check for adjusted states.
    vtkAssemblyPath * path;
    this->PlanePickers[ this->CurrentRendererIndex ]->Pick(X,Y,0.0,currentRenderer);
    path = this->PlanePickers[ this->CurrentRendererIndex ]->GetPath();

    int found = 0;
    int i;
    if ( path != 0 )
    {
        // Deal with the possibility that we may be using a shared picker
        path->InitTraversal();
        vtkAssemblyNode *node;
        for(i = 0; i< path->GetNumberOfItems() && !found ;i++)
        {
            node = path->GetNextNode();
            if(node->GetViewProp() == vtkProp::SafeDownCast(this->TexturePlaneActors[ this->CurrentRendererIndex ]) )
            {
                found = 1;
            }
        }
    }

    if ( !found || path == 0 )
    {
        this->State = vtkMultiImagePlaneWidget::Outside;
        this->HighlightPlane(0);
        this->ActivateMargins(0);
        return;
    }
    else
    {
        this->State = vtkMultiImagePlaneWidget::Pushing;
        this->HighlightPlane(1);
		if( !DisableRotatingAndSpinning )
			this->ActivateMargins(1);
		else
			this->ActivateMargins( 0 );
        this->AdjustState();
        this->UpdateMargins();
    }

    this->Callback->SetAbortFlag(1);
    this->StartInteraction();
    this->InvokeEvent(vtkCommand::StartInteractionEvent,0);
    this->RenderAll();
}

void vtkMultiImagePlaneWidget::StopSliceMotion()
{
    if ( this->State == vtkMultiImagePlaneWidget::Outside || this->State == vtkMultiImagePlaneWidget::Start )
    {
        return;
    }

    this->State = vtkMultiImagePlaneWidget::Start;
    this->HighlightPlane(0);
    this->ActivateMargins(0);

    this->Callback->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,0);
    this->RenderAll();
}

void vtkMultiImagePlaneWidget::StartCursor()
{
    // Get the position of the pick
    vtkRenderWindowInteractor * interactor = this->Interactors[ this->CurrentInteractorIndex ];
    int X = interactor->GetEventPosition()[0];
    int Y = interactor->GetEventPosition()[1];

    // Make sure we have a valid renderer to pick from
    if( this->CurrentRendererIndex == -1 )
    {
        this->State = vtkMultiImagePlaneWidget::Outside;
        return;
    }
    vtkRenderer * currentRenderer = this->Renderers[ this->CurrentRendererIndex ];

    // Okay, we can process this. If anything is picked, then we
    // can start pushing the plane.
    vtkAssemblyPath * path;
    this->PlanePickers[ this->CurrentRendererIndex ]->Pick(X,Y,0.0,currentRenderer);
    path = this->PlanePickers[ this->CurrentRendererIndex ]->GetPath();

    int found = 0;
    int i;
    if ( path != 0 )
    {
        // Deal with the possibility that we may be using a shared picker
        path->InitTraversal();
        vtkAssemblyNode * node;
        for ( i = 0; i < path->GetNumberOfItems() && !found ; i++ )
        {
            node = path->GetNextNode();
            if ( node->GetViewProp() == vtkProp::SafeDownCast( this->TexturePlaneActors[ this->CurrentRendererIndex ] ) )
            {
                found = 1;
            }
        }
    }

    if( ! found || path == 0 )
    {
        this->State = vtkMultiImagePlaneWidget::Outside;
        this->HighlightPlane(0);
        return;
    }
    else
    {
        this->State = vtkMultiImagePlaneWidget::Cursoring;
        this->HighlightPlane(1);
        this->UpdateCursor(X,Y);
    }

    this->Callback->SetAbortFlag(1);
    this->StartInteraction();
    this->InvokeEvent(vtkCommand::StartInteractionEvent,0);

    this->RenderAll();
}

void vtkMultiImagePlaneWidget::StopCursor()
{
    if ( this->State == vtkMultiImagePlaneWidget::Outside || this->State == vtkMultiImagePlaneWidget::Start )
    {
        return;
    }

    this->State = vtkMultiImagePlaneWidget::Start;
    this->HighlightPlane(0);
    if( !this->CursorActive )
    {
        for( int i = 0; i < this->CursorActors.size(); ++i )
            this->CursorActors[i]->VisibilityOff();
    }

    this->Callback->SetAbortFlag(1);
    this->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent,0);
    this->RenderAll();
}

void vtkMultiImagePlaneWidget::OnMouseMove()
{
    // See whether we're active
    //
    if ( this->State == vtkMultiImagePlaneWidget::Outside || this->State == vtkMultiImagePlaneWidget::Start )
    {
        return;
    }

    vtkRenderWindowInteractor * interactor = this->Interactors[ this->CurrentInteractorIndex ];
    int X = interactor->GetEventPosition()[0];
    int Y = interactor->GetEventPosition()[1];

    // Do different things depending on state
    // Calculations everybody does
    //
    double focalPoint[4], pickPoint[4], prevPickPoint[4];
    double z, vpn[3];

    vtkRenderer * currentRenderer = this->Renderers[ this->CurrentRendererIndex ];
    if (this->CurrentRendererIndex < 0)
    {
        this->State = vtkMultiImagePlaneWidget::Outside;
        return;
    }
    vtkCamera * camera = currentRenderer->GetActiveCamera();
    if ( ! camera )
    {
        return;
    }

    // Compute the two points defining the motion vector in the world coordinate system
    //
    this->ComputeWorldToDisplay( this->CurrentRendererIndex, this->LastPickPosition[0], this->LastPickPosition[1], this->LastPickPosition[2], focalPoint );
    z = focalPoint[2];
    this->ComputeDisplayToWorld( this->CurrentRendererIndex, double(interactor->GetLastEventPosition()[0]), double(interactor->GetLastEventPosition()[1]), z, prevPickPoint);
    this->ComputeDisplayToWorld( this->CurrentRendererIndex, double(X), double(Y), z, pickPoint);

    double isPickPoint[3];
    this->TransformToImageSpace( pickPoint, isPickPoint );
    double isPrevPickPoint[3];
    this->TransformToImageSpace( prevPickPoint, isPrevPickPoint );

    if ( this->State == vtkMultiImagePlaneWidget::Pushing )
    {
        this->Push( isPrevPickPoint, isPickPoint );
        this->UpdateNormal();
        this->UpdateMargins();
        this->UpdateCursor();
    }
    else if ( this->State == vtkMultiImagePlaneWidget::Spinning )
    {
        this->Spin(isPrevPickPoint, isPickPoint);
        this->UpdateNormal();
        this->UpdateMargins();
    }
    else if ( this->State == vtkMultiImagePlaneWidget::Rotating )
    {
        camera->GetViewPlaneNormal(vpn);
        this->Rotate(isPrevPickPoint, isPickPoint, vpn);
        this->UpdateNormal();
        this->UpdateMargins();
    }
    else if ( this->State == vtkMultiImagePlaneWidget::Scaling )
    {
        this->Scale(isPrevPickPoint, isPickPoint, X, Y);
        this->UpdateNormal();
        this->UpdateMargins();
    }
    else if ( this->State == vtkMultiImagePlaneWidget::Moving )
    {
        this->Translate(isPrevPickPoint, isPickPoint);
        this->UpdateNormal();
        this->UpdateMargins();
    }
    else if ( this->State == vtkMultiImagePlaneWidget::Cursoring )
    {
        this->UpdateCursor(X,Y);
    }

    // Interact, if desired
    //
    this->Callback->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::InteractionEvent,0);

    this->RenderAll();
}

void vtkMultiImagePlaneWidget::SetSliceThickness( int nbSlices )
{
    PerVolumeObjectsVec::iterator it = Inputs.begin();
    while( it != Inputs.end() )
    {
        (*it).Reslice->SetSlabNumberOfSlices( nbSlices );
        ++it;
    }
}

void vtkMultiImagePlaneWidget::SetSliceMixMode( int imageIndex, int mode )
{
    Inputs[ imageIndex ].Reslice->SetSlabMode( mode );
}

void vtkMultiImagePlaneWidget::SetBlendingMode( int imageIndex, int mode )
{
    Inputs[ imageIndex ].Texture->SetBlendingMode( mode );
}

int vtkMultiImagePlaneWidget::GetCursorData( double xyzv[4])
{
    if ( this->State != vtkMultiImagePlaneWidget::Cursoring  || this->CurrentImageValue == VTK_FLOAT_MAX )
    {
        return 0;
    }

	if( this->Inputs.size() < 1 )
		return 0;

	// Get image value from the first volume
	vtkImageData * im = this->Inputs[0].ImageData;

    double cursorPos[3];
    this->GetPosition( cursorPos );

    xyzv[0] = cursorPos[0];
    xyzv[1] = cursorPos[1];
    xyzv[2] = cursorPos[2];
    xyzv[3] = im->GetScalarComponentAsDouble( cursorPos[0], cursorPos[1], cursorPos[2], 0 );

    return 1;
}

void vtkMultiImagePlaneWidget::SetPlaneMoveMethod( int rendererIndex, PlaneMoveMethod moveMethod )
{
	if( this->PlaneMoveMethods.size() > rendererIndex )
	{
		this->PlaneMoveMethods[ rendererIndex ] = moveMethod;
	}
}

void vtkMultiImagePlaneWidget::Push(double *p1, double *p2)
{
    // Get the motion vector
    //
    double v[3];
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];

	if( this->PlaneMoveMethods[ this->CurrentRendererIndex ] == Move3D )
	{
		this->PlaneSource->Push( vtkMath::Dot(v,this->Normal) );
	}
	else
	{
		double pointAxis2[3];
		this->PlaneSource->GetPoint2( pointAxis2 );
		double origin[3];
		this->PlaneSource->GetOrigin( origin );

        double axis[3];
        vtkMath::Subtract( pointAxis2, origin, axis );
        vtkMath::Normalize( axis );
        double ampl = vtkMath::Dot( axis, v );
        this->PlaneSource->Push( ampl );
	}

    this->PlaneSource->Update();
    this->BuildRepresentation();
}

void vtkMultiImagePlaneWidget::CreateDefaultProperties()
{
    if ( ! this->PlaneProperty )
    {
        this->PlaneProperty = vtkProperty::New();
        this->PlaneProperty->SetAmbient(1);
        this->PlaneProperty->SetColor(1,1,1);
        this->PlaneProperty->SetRepresentationToWireframe();
        this->PlaneProperty->SetInterpolationToFlat();
    }

    if ( ! this->SelectedPlaneProperty )
    {
        this->SelectedPlaneProperty = vtkProperty::New();
        this->SelectedPlaneProperty->SetAmbient(1);
        this->SelectedPlaneProperty->SetColor(0,1,0);
        this->SelectedPlaneProperty->SetRepresentationToWireframe();
        this->SelectedPlaneProperty->SetInterpolationToFlat();
    }

    if ( ! this->CursorProperty )
    {
        this->CursorProperty = vtkProperty::New();
        this->CursorProperty->SetAmbient(1);
        this->CursorProperty->SetColor( 1.0, 0.7, 0.25 );
        this->CursorProperty->SetLineWidth( .5 );
        this->CursorProperty->SetRepresentationToWireframe();
        this->CursorProperty->SetInterpolationToFlat();
    }

    if ( ! this->MarginProperty )
    {
        this->MarginProperty = vtkProperty::New();
        this->MarginProperty->SetAmbient(1);
        this->MarginProperty->SetColor(0,0,1);
        this->MarginProperty->SetRepresentationToWireframe();
        this->MarginProperty->SetInterpolationToFlat();
    }
}

void vtkMultiImagePlaneWidget::PlaceWidget(double bounds[6])
{
	double center[3];

	center[0] = (bounds[0] + bounds[1])/2.0;
	center[1] = (bounds[2] + bounds[3])/2.0;
	center[2] = (bounds[4] + bounds[5])/2.0;

    if ( this->PlaneOrientation == 1 )
    {
        this->PlaneSource->SetOrigin(bounds[0],center[1],bounds[4]);
        this->PlaneSource->SetPoint1(bounds[1],center[1],bounds[4]);
        this->PlaneSource->SetPoint2(bounds[0],center[1],bounds[5]);
    }
    else if ( this->PlaneOrientation == 2 )
    {
        this->PlaneSource->SetOrigin(bounds[0],bounds[2],center[2]);
        this->PlaneSource->SetPoint1(bounds[1],bounds[2],center[2]);
        this->PlaneSource->SetPoint2(bounds[0],bounds[3],center[2]);
    }
    else //default or x-normal
    {
        this->PlaneSource->SetOrigin(center[0],bounds[2],bounds[4]);
        this->PlaneSource->SetPoint1(center[0],bounds[3],bounds[4]);
        this->PlaneSource->SetPoint2(center[0],bounds[2],bounds[5]);
    }
    this->PlaneSource->Update();
    this->BuildRepresentation();
    this->UpdateNormal();
    this->SetCursorPosition( center );
    this->UpdateCursor();
}

void vtkMultiImagePlaneWidget::PlaceWidget()
{
	double bounds[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    ComputeBounds( bounds );

	this->PlaceWidget(bounds);
	this->InvokeEvent(vtkCommand::PlaceWidgetEvent,NULL);
	this->Placed = 1;
}


int vtkMultiImagePlaneWidget::AddInput( vtkImageData * in, vtkScalarsToColors * lut, vtkTransform * t, bool canInterpolate )
{
	PerVolumeObjects inObjects;
	inObjects.ImageData = in;
	inObjects.ImageData->Register( 0 );
    inObjects.CanInterpolate = canInterpolate;

	if ( !this->UserControlledLookupTable )
	{
		double range[2];
		in->GetScalarRange(range);
        vtkLookupTable * lut = vtkLookupTable::New();
        lut->SetTableRange(range[0],range[1]);
        lut->Build();
        inObjects.LookupTable = lut;
	}
	else if( lut )
	{
		inObjects.LookupTable = lut;
		lut->Register( 0 );
	}

	inObjects.Reslice = vtkImageReslice::New();
    inObjects.Reslice->SetInputData(in);
    vtkTransform * resliceTransform = vtkTransform::New();
    inObjects.Reslice->SetResliceTransform( resliceTransform );
    inObjects.Reslice->SetInterpolationMode( this->ResliceInterpolate );

    resliceTransform->SetInput( t );
    resliceTransform->Inverse();
    if( this->BoundingTransform )
        resliceTransform->Concatenate( this->BoundingTransform );
    resliceTransform->Delete();

	inObjects.ColorMap = vtkImageMapToColors::New();
    inObjects.ColorMap->SetLookupTable( inObjects.LookupTable );
    //inObjects.ColorMap->SetOutputFormatToRGB();
    inObjects.ColorMap->SetOutputFormatToRGBA(); //this is default
    inObjects.ColorMap->SetInputConnection(inObjects.Reslice->GetOutputPort());

	inObjects.Texture = vtkTexture::New();
    inObjects.Texture->SetInputConnection(inObjects.ColorMap->GetOutputPort());
    if( inObjects.CanInterpolate )
        inObjects.Texture->SetInterpolate(this->TextureInterpolate);
    else
        inObjects.Texture->SetInterpolate( 0 );
    inObjects.Texture->SetColorModeToDirectScalars();
    inObjects.Texture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
    inObjects.Texture->RepeatOff();

	int volIndex = this->Inputs.size();
	Inputs.push_back( inObjects );
    this->UpdateTextureUnits();

	if( Inputs.size() == 1 )
    {
		this->SetPlaneOrientation(this->PlaneOrientation);

        // Force to recompute the bounds of the widget considering the new volume and
        // reset the widget accordingly.
        this->PlaceWidget();
    }

    // Need to mark the plane source as modified to make sure the pipeline is updated properly
    this->PlaneSource->Modified();
    this->UpdateNormal();

	return volIndex;

}

void vtkMultiImagePlaneWidget::UpdateTextureUnits()
{
    for( int act = 0; act < this->TexturePlaneActors.size(); ++act )
    {
        vtkActor * actor = this->TexturePlaneActors[act];
        actor->GetProperty()->RemoveAllTextures();
        for( int in = 0; in < this->Inputs.size(); ++in )
        {
            if( !this->Inputs[in].IsHidden )
            {
                this->AddTextureToPlane( actor, this->TexturePlaneMappers[act], in );
            }
        }
    }
}

void vtkMultiImagePlaneWidget::SetImageHidden( vtkImageData * im, bool hidden )
{
    int imageIndex = GetPerVolumeIndex( im );
    if( imageIndex != -1 )
    {
        this->Inputs[ imageIndex ].IsHidden = hidden;
        UpdateTextureUnits();
    }
}

void vtkMultiImagePlaneWidget::ClearAllInputs()
{
    // remove the texture from all actors.
    for( int act = 0; act < this->TexturePlaneActors.size(); ++act )
    {
        vtkActor * actor = this->TexturePlaneActors[act];
        actor->GetProperty()->RemoveAllTextures();
    }

	for( int i = 0; i < Inputs.size(); ++i )
	{
		PerVolumeObjects & in = Inputs[i];
		if( in.ImageData )
			in.ImageData->UnRegister( 0 );
		if( in.LookupTable )
			in.LookupTable->UnRegister( 0 );
		if( in.Reslice )
			in.Reslice->Delete();
		if( in.ColorMap )
			in.ColorMap->Delete();
		if( in.Texture )
			in.Texture->Delete();
	}
	Inputs.clear();
}

void vtkMultiImagePlaneWidget::SetBoundingVolume( vtkImageData * boundingImage, vtkTransform * boundingTransform )
{
    if( boundingImage == this->BoundingImage && boundingTransform == this->BoundingTransform )
        return;

    if( this->BoundingImage )
    {
        this->BoundingImage->UnRegister( this );
        this->BoundingTransform->UnRegister( this );
    }
    this->BoundingImage = boundingImage;
    this->BoundingTransform = boundingTransform;
    if( this->BoundingImage )
        this->BoundingImage->Register( this );
    if( this->BoundingTransform )
        this->BoundingTransform->Register( this );

    // Premultiply all input volumes's transform with bounding volume's transform.
    for( int im = 0; im < this->Inputs.size(); ++ im )
    {
        PerVolumeObjects & in = this->Inputs[ im ];
        vtkTransform * resliceT = vtkTransform::SafeDownCast( in.Reslice->GetResliceTransform() );
        resliceT->Identity();
        resliceT->Concatenate( boundingTransform );
    }

    for( int act = 0; act < this->TexturePlaneActors.size(); ++act )
    {
        vtkActor * actor = this->TexturePlaneActors[act];
        actor->SetUserTransform( boundingTransform );
    }

    SetActorsTransforms();
    // If bounding image is changing, we want to reset the cursor and plane position
    this->PlaceWidget();

}

void vtkMultiImagePlaneWidget::Show( vtkRenderer * ren, int show )
{
    int index = this->GetRendererIndex( ren );
    this->Show( index, show );
}

void vtkMultiImagePlaneWidget::Show( int renIndex, int show )
{
    //this->MarginActors[renIndex]->SetVisibility( show );
    if( show && this->CursorActive )
        this->CursorActors[renIndex]->SetVisibility( 1 );
    else
        this->CursorActors[renIndex]->SetVisibility( 0 );
    this->TexturePlaneActors[renIndex]->SetVisibility( show );
    //this->PlaneOutlineActors[renIndex]->SetVisibility( show );
}

int vtkMultiImagePlaneWidget::IsShown( vtkRenderer * ren )
{
    int index = this->GetRendererIndex( ren );
    return this->TexturePlaneActors[index]->GetVisibility();
}

void vtkMultiImagePlaneWidget::ShowInAllRenderers( int show )
{
    for( int i = 0; i < this->Renderers.size(); ++ i )
        Show( i, show );
}

void vtkMultiImagePlaneWidget::SetPlaneOrientation(int i)
{
    // Generate a XY plane if i = 2, z-normal
    // or a YZ plane if i = 0, x-normal
    // or a ZX plane if i = 1, y-normal
    //
    this->PlaneOrientation = i;
    this->Modified();

    // Nothing to do if no input is defined
	if ( this->Inputs.size() < 1 )
        return;

	vtkImageData * im = this->Inputs[0].ImageData;
    int extent[6];
    im->GetExtent(extent);
    double origin[3];
	im->GetOrigin(origin);
    double spacing[3];
	im->GetSpacing(spacing);

    // Prevent obscuring voxels by offsetting the plane geometry
    //
    double xbounds[] = {origin[0] + spacing[0] * (extent[0] - 0.5),
                        origin[0] + spacing[0] * (extent[1] + 0.5)};
    double ybounds[] = {origin[1] + spacing[1] * (extent[2] - 0.5),
                        origin[1] + spacing[1] * (extent[3] + 0.5)};
    double zbounds[] = {origin[2] + spacing[2] * (extent[4] - 0.5),
                        origin[2] + spacing[2] * (extent[5] + 0.5)};

    if ( spacing[0] < 0.0f )
    {
        double t = xbounds[0];
        xbounds[0] = xbounds[1];
        xbounds[1] = t;
    }
    if ( spacing[1] < 0.0f )
    {
        double t = ybounds[0];
        ybounds[0] = ybounds[1];
        ybounds[1] = t;
    }
    if ( spacing[2] < 0.0f )
    {
        double t = zbounds[0];
        zbounds[0] = zbounds[1];
        zbounds[1] = t;
    }

    if ( i == 2 ) //XY, z-normal
    {
        this->PlaneSource->SetOrigin(xbounds[0],ybounds[0],zbounds[0]);
        this->PlaneSource->SetPoint1(xbounds[1],ybounds[0],zbounds[0]);
        this->PlaneSource->SetPoint2(xbounds[0],ybounds[1],zbounds[0]);
        this->CursorPosition[0] = ( xbounds[1] - xbounds[0] ) * .5;
        this->CursorPosition[1] = ( ybounds[1] - ybounds[0] ) * .5;
    }
    else if ( i == 0 ) //YZ, x-normal
    {
        this->PlaneSource->SetOrigin(xbounds[0],ybounds[0],zbounds[0]);
        this->PlaneSource->SetPoint1(xbounds[0],ybounds[1],zbounds[0]);
        this->PlaneSource->SetPoint2(xbounds[0],ybounds[0],zbounds[1]);
        this->CursorPosition[0] = ( ybounds[1] - ybounds[0] ) * .5;
        this->CursorPosition[1] = ( zbounds[1] - zbounds[0] ) * .5;
    }
    else  //ZX, y-normal
    {
        this->PlaneSource->SetOrigin(xbounds[0],ybounds[0],zbounds[0]);
        this->PlaneSource->SetPoint1(xbounds[0],ybounds[0],zbounds[1]);
        this->PlaneSource->SetPoint2(xbounds[1],ybounds[0],zbounds[0]);
        this->CursorPosition[0] = ( zbounds[1] - zbounds[0] ) * .5;
        this->CursorPosition[1] = ( xbounds[1] - xbounds[0] ) * .5;
    }

    this->PlaneSource->Update();
    this->BuildRepresentation();
    this->UpdateNormal();
}


void vtkMultiImagePlaneWidget::ComputeBounds( double globalBounds[] )
{		
	globalBounds[0] = 0;
	globalBounds[1] = 1;
	globalBounds[2] = 0;
	globalBounds[3] = 1;
	globalBounds[4] = 0;
	globalBounds[5] = 1;

    // Consider only first volume as we will reslice only inside that volume for now
    if( this->Inputs.size() > 0 )
    {
        vtkImageData * image = this->Inputs[0].ImageData;
        double origin[3];
        image->GetOrigin(origin);
        double spacing[3];
        image->GetSpacing(spacing);
        int extent[6];
        image->GetExtent(extent);

        globalBounds[0] = origin[0] + spacing[0]*extent[0];
        globalBounds[1] = origin[0] + spacing[0]*extent[1];
        globalBounds[2] = origin[1] + spacing[1]*extent[2];
        globalBounds[3] = origin[1] + spacing[1]*extent[3];
        globalBounds[4] = origin[2] + spacing[2]*extent[4];
        globalBounds[5] = origin[2] + spacing[2]*extent[5];

        for ( int j = 0; j <= 4; j += 2 ) // reverse bounds if necessary
        {
            if ( globalBounds[j] > globalBounds[j+1] )
            {
                double t = globalBounds[j+1];
                globalBounds[j+1] = globalBounds[j];
                globalBounds[j] = t;
            }
        }
    }
}

void vtkMultiImagePlaneWidget::EnforceRestrictPlaneToVolume()
{
    if( this->Inputs.size() < 1 )
        return;

    if ( this->RestrictPlaneToVolume )
    {
        double bounds[6];
        ComputeBounds( bounds );

        // find major axis closer to normal of the plane
        //
        double abs_normal[3];
        this->PlaneSource->GetNormal(abs_normal);
        double planeCenter[3];
        this->PlaneSource->GetCenter(planeCenter);
        double nmax = 0.0f;
        int k = 0;
        for ( int i = 0; i < 3; i++ )
        {
            abs_normal[i] = fabs(abs_normal[i]);
            if ( abs_normal[i]>nmax )
            {
                nmax = abs_normal[i];
                k = i;
            }
        }

        // Force the plane to lie within the true image bounds along its normal
        //
        if ( planeCenter[k] > bounds[2*k+1] )
        {
            planeCenter[k] = bounds[2*k+1];
            this->PlaneSource->SetCenter(planeCenter);
            this->PlaneSource->Update();
            this->BuildRepresentation();
        }
        else if ( planeCenter[k] < bounds[2*k] )
        {
            planeCenter[k] = bounds[2*k];
            this->PlaneSource->SetCenter(planeCenter);
            this->PlaneSource->Update();
            this->BuildRepresentation();
        }
    }
}

//=====================================================================
// UpdateNormal
// Use plane attributes to
void vtkMultiImagePlaneWidget::UpdateNormal()
{
    // simtodo : this might go somewhere else. Needs to go before any update to the geometry because it changes the plane source params.
    if( this->RestrictPlaneToVolume )
        this->EnforceRestrictPlaneToVolume();

    double planeAxis1[3];
    double planeAxis2[3];

    this->GetVector1(planeAxis1);
    this->GetVector2(planeAxis2);

	double planeOrigin[4];
	this->PlaneSource->GetOrigin(planeOrigin);
    planeOrigin[3] = 1.0; // w = 1

	// Set the texture coordinates to map the image to the plane
	//
    this->TexturePlaneCoords->SetOrigin( planeOrigin[0], planeOrigin[1], planeOrigin[2] );
    this->TexturePlaneCoords->SetPoint1( planeOrigin[0] + planeAxis1[0], planeOrigin[1] + planeAxis1[1], planeOrigin[2] + planeAxis1[2] );
    this->TexturePlaneCoords->SetPoint2( planeOrigin[0] + planeAxis2[0], planeOrigin[1] + planeAxis2[1], planeOrigin[2] + planeAxis2[2] );
    this->TexturePlaneCoords->ClearTCoordSets();

    // The x,y dimensions of the plane
    //
    double planeSizeX = vtkMath::Normalize(planeAxis1);
    double planeSizeY = vtkMath::Normalize(planeAxis2);

    this->PlaneSource->GetNormal(this->Normal);

    // Generate the slicing matrix
    //
    int i;
    this->ResliceAxes->Identity();
    for ( i = 0; i < 3; i++ )
    {
        this->ResliceAxes->SetElement(i,0,planeAxis1[i]);
        this->ResliceAxes->SetElement(i,1,planeAxis2[i]);
        this->ResliceAxes->SetElement(i,2,this->Normal[i]);
    }

    // Transpose is an exact way to invert a pure rotation matrix
    this->ResliceAxes->Transpose();

    double originXYZW[4];
    this->ResliceAxes->MultiplyPoint( planeOrigin, originXYZW );

    this->ResliceAxes->Transpose();
    double neworiginXYZW[4];
    double point[] = { 0.0, 0.0, originXYZW[2], 1.0 };
    this->ResliceAxes->MultiplyPoint( point, neworiginXYZW );

    this->ResliceAxes->SetElement(0,3,neworiginXYZW[0]);
    this->ResliceAxes->SetElement(1,3,neworiginXYZW[1]);
    this->ResliceAxes->SetElement(2,3,neworiginXYZW[2]);

	// Update the slicer of each of the volumes
	for( int i = 0; i < this->Inputs.size(); ++i )
	{
		PerVolumeObjects & in = this->Inputs[i];

        in.Reslice->SetResliceAxes( this->ResliceAxes );

		// Calculate appropriate pixel spacing for the reslicing
		//
		double spacing[3];
		in.ImageData->GetSpacing(spacing);

		double spacingX = fabs(planeAxis1[0]*spacing[0])+\
						fabs(planeAxis1[1]*spacing[1])+\
						fabs(planeAxis1[2]*spacing[2]);

		double spacingY = fabs(planeAxis2[0]*spacing[0])+\
						fabs(planeAxis2[1]*spacing[1])+\
						fabs(planeAxis2[2]*spacing[2]);


        double spacingZ = fabs(this->Normal[0]*spacing[0])+\
                        fabs(this->Normal[1]*spacing[1])+\
                        fabs(this->Normal[2]*spacing[2]);

        in.Reslice->SetOutputSpacing( spacingX, spacingY, spacingZ );
        in.Reslice->SetOutputOrigin( originXYZW[0], originXYZW[1], 0.0 );
        in.Reslice->Update();
		// Update the output image extent
		double extentX;
		if (spacingX == 0)
			extentX = 0;
		else
			extentX = planeSizeX / spacingX;

		double extentY;
		if( spacingY == 0 )
			extentY = 0;
		else
			extentY = planeSizeY / spacingY;

		if (extentX > 0 && extentY > 0) // Do not set negative extents
            in.Reslice->SetOutputExtent( 0, extentX, 0, extentY, 0, 0 );
		else
            in.Reslice->SetOutputExtent( 0, 1, 0, 1, 0, 0 );

        std::string tcoordName = this->ComposeTextureCoordName( i );
        double offsetX = .5 / ( extentX + 1 );
        double offsetY = .5 / ( extentY + 1 );
        this->TexturePlaneCoords->AddTCoordSet( tcoordName.c_str(), offsetX, offsetY );
        in.ColorMap->Update();
	}
}

void vtkMultiImagePlaneWidget::SetResliceInterpolate(int i)
{
    if ( this->ResliceInterpolate == i )
    {
        return;
    }
    this->ResliceInterpolate = i;

	for( int vol = 0; vol < this->Inputs.size(); ++vol )
	{
		PerVolumeObjects & in = this->Inputs[vol];

		if ( i == VTK_NEAREST_RESLICE )
		{
			in.Reslice->SetInterpolationModeToNearestNeighbor();
		}
		else if ( i == VTK_LINEAR_RESLICE)
		{
			in.Reslice->SetInterpolationModeToLinear();
		}
		else
		{
			in.Reslice->SetInterpolationModeToCubic();
		}
    }

    this->Modified();
}

void vtkMultiImagePlaneWidget::SetTextureInterpolate( int interpolate )
{
    this->TextureInterpolate = interpolate;
    for( int vol = 0; vol < this->Inputs.size(); ++vol )
    {
        PerVolumeObjects & in = this->Inputs[vol];
        if( in.CanInterpolate )
            in.Texture->SetInterpolate( this->TextureInterpolate );
        else
            in.Texture->SetInterpolate( 0 );
    }

    this->Modified();
}

void vtkMultiImagePlaneWidget::SetPicker( int index, vtkCellPicker * picker )
{
    if( index > this->PlanePickers.size() )
    {
        vtkErrorMacro( << "index > number of renderers" << endl );
    }

    if( this->PlanePickers[ index ] != picker )
    {
        // to avoid destructor recursion
        vtkCellPicker * temp = this->PlanePickers[index];
        this->PlanePickers[ index ] = picker;
        if (temp != 0)
        {
            temp->UnRegister(this);
        }
        if (this->PlanePickers[ index ] != 0)
        {
            picker->Register( this );
            picker->SetTolerance( 0.005 ); //need some fluff
            if( this->Assemblies[ index ] != 0 )
            {
                picker->AddPickList( this->Assemblies[ index ] );
            }
            else
            {
                picker->AddPickList( this->TexturePlaneActors[ index ] );
            }
            picker->PickFromListOn();
        }
    }
}

vtkLookupTable * vtkMultiImagePlaneWidget::CreateDefaultLookupTable()
{
    vtkLookupTable* lut = vtkLookupTable::New();
    lut->Register(this);
    lut->Delete();
    lut->SetNumberOfColors( 256);
    lut->SetHueRange( 0, 0);
    lut->SetSaturationRange( 0, 0);
    lut->SetValueRange( 0 ,1 );
    lut->SetAlphaRange( 1, 1 );
    lut->Build();
    return lut;
}

int vtkMultiImagePlaneWidget::GetPerVolumeIndex( vtkImageData * volume )
{
	PerVolumeObjectsVec::iterator it = this->Inputs.begin();
	PerVolumeObjectsVec::iterator itEnd = this->Inputs.end();
	int index = 0;
	while( it != itEnd )
	{
		if( (*it).ImageData == volume )
			return index;
		++it;
		++index;
	}
	return -1;
}

void vtkMultiImagePlaneWidget::SetLookupTable( vtkImageData * volume, vtkScalarsToColors* table )
{
	int volumeIndex = GetPerVolumeIndex( volume );
	if( volumeIndex == -1 )
		return;

	PerVolumeObjects & inVol = this->Inputs[volumeIndex];
	if ( inVol.LookupTable != table)
	{
		// to avoid destructor recursion
        vtkScalarsToColors *temp = inVol.LookupTable;
		inVol.LookupTable = table;
		if (temp != 0)
		{
			temp->UnRegister(this);
		}
		if (inVol.LookupTable != 0)
		{
			inVol.LookupTable->Register(this);
		}
		else  //create a default lut
		{
			inVol.LookupTable = this->CreateDefaultLookupTable();
		}
	}

    inVol.ColorMap->SetLookupTable(inVol.LookupTable);

	if( inVol.ImageData && !this->UserControlledLookupTable)
	{
        double range[2];
		inVol.ImageData->GetScalarRange(range);

        vtkLookupTable * lut = vtkLookupTable::SafeDownCast( inVol.LookupTable );
        if( lut )
        {
            lut->SetTableRange(range[0],range[1]);
            lut->Build();
        }
	}
}

vtkScalarsToColors * vtkMultiImagePlaneWidget::GetLookupTable( vtkImageData * volume )
{
	int volumeIndex = GetPerVolumeIndex( volume );
	if( volumeIndex == -1 )
		return 0;

	return Inputs[ volumeIndex ].LookupTable;
}


void vtkMultiImagePlaneWidget::SetGlobalPosition( double position[3] )
{
    vtkMatrix4x4 * invBoundingTrans = vtkMatrix4x4::New();
    this->BoundingTransform->GetInverse( invBoundingTrans );
    double transformedPosition[3];
    invBoundingTrans->MultiplyPoint( position, transformedPosition );
    this->SetPosition( transformedPosition );
    invBoundingTrans->Delete();
}

// Pushes the plane along the normal so that it intersects the point
void vtkMultiImagePlaneWidget::SetPosition( double position[3] )
{
    double planeOrigin[3];
    this->PlaneSource->GetOrigin(planeOrigin);

    double dirTarget[3];
    vtkMath::Subtract( position, planeOrigin, dirTarget );
    double amount = vtkMath::Dot( dirTarget, this->Normal );

    if( amount != 0.0 )
    {
        this->PlaneSource->Push(amount);
        this->PlaneSource->Update();
        this->BuildRepresentation();
        this->UpdateNormal();
    }

    this->SetCursorPosition( position );
}

void vtkMultiImagePlaneWidget::GetPosition( double pos[3] )
{
    double p1[3];
    this->PlaneSource->GetPoint1(p1);
    double p2[3];
    this->PlaneSource->GetPoint2(p2);
    double o[3];
    this->PlaneSource->GetOrigin(o);

    double d1[3];
    vtkMath::Subtract( p1, o, d1 );
    //vtkMath::Normalize( d1 );
    double d2[3];
    vtkMath::Subtract( p2, o, d2 );
    //vtkMath::Normalize( d2 );

    for( int i = 0; i < 3; ++i )
    {
        pos[i] = o[i] + this->CursorPosition[0] * d1[i] + this->CursorPosition[1] * d2[i];
    }
}

void vtkMultiImagePlaneWidget::MoveNSlices( int nbSlices )
{
    if( this->BoundingImage )
    {
        double spacing = this->BoundingImage->GetSpacing()[ this->PlaneOrientation ];
        double move = nbSlices * spacing;
        this->PlaneSource->Push( move );
        this->PlaneSource->Update();
        this->BuildRepresentation();
        this->UpdateNormal();
        this->UpdateMargins();
        this->UpdateCursor();
    }
}

#include "vtkPlane.h"

double vtkMultiImagePlaneWidget::DistanceToPlane( double position[3] )
{
    // Transform point to bounding volume
    vtkMatrix4x4 * invBoundingTrans = vtkMatrix4x4::New();
    this->BoundingTransform->GetInverse( invBoundingTrans );
    double transformedPosition[4];
    transformedPosition[0] = position[0];
    transformedPosition[1] = position[1];
    transformedPosition[2] = position[2];
    transformedPosition[3] = 1.0;
    invBoundingTrans->MultiplyPoint( transformedPosition, transformedPosition );
    invBoundingTrans->Delete();

    // Find distance with plane
    return vtkPlane::DistanceToPlane( transformedPosition, PlaneSource->GetNormal(), PlaneSource->GetOrigin() );
}

void vtkMultiImagePlaneWidget::ActivateCursor( int i )
{
    if( i == 0 )
    {
        this->CursorActive = 0;
        ActorVec::iterator it = this->CursorActors.begin();
        while( it != this->CursorActors.end() )
        {
            (*it)->VisibilityOff();
            ++it;
        }
    }
    else
    {
        this->CursorActive = 1;
        for( int i = 0; i < this->CursorActors.size(); ++i )
        {
            if( this->TexturePlaneActors[ i ]->GetVisibility() )
                this->CursorActors[ i ]->VisibilityOn();
        }
    }
}

void vtkMultiImagePlaneWidget::ActivateMargins(int i)
{
    if( i == 0 )
    {
        ActorVec::iterator it = this->MarginActors.begin();
        while( it != this->MarginActors.end() )
        {
            (*it)->VisibilityOff();
            ++it;
        }
    }
    else
    {
        ActorVec::iterator it = this->MarginActors.begin();
        while( it != this->MarginActors.end() )
        {
            (*it)->VisibilityOn();
            ++it;
        }
    }
}

void vtkMultiImagePlaneWidget::UpdateCursor( int X, int Y )
{
    if( this->CurrentRendererIndex == -1 )
    {
        return;
    }

    vtkRenderer * renderer = this->Renderers[ this->CurrentRendererIndex ];
    vtkAssemblyPath *path;
    this->PlanePickers[this->CurrentRendererIndex]->Pick(X,Y,0.0,renderer);
    path = this->PlanePickers[ this->CurrentRendererIndex ]->GetPath();
    this->CurrentImageValue = VTK_DOUBLE_MAX;

    int found = 0;
    int i;
    if ( path != 0 )
    {
        // Deal with the possibility that we may be using a shared picker
        //
        path->InitTraversal();
        vtkAssemblyNode *node;
        for ( i = 0; i< path->GetNumberOfItems() && !found ; i++ )
        {
            node = path->GetNextNode();
            if ( node->GetViewProp() == vtkProp::SafeDownCast( this->TexturePlaneActors[ this->CurrentRendererIndex ]) )
            {
                found = 1;
            }
        }
    }

    if( !found || path == 0 )
    {
        for( int i = 0; i < this->CursorActors.size(); ++i )
            this->CursorActors[ i ]->VisibilityOff();
        return;
    }
    else
    {
        for( int i = 0; i < this->CursorActors.size(); ++i )
            if( this->TexturePlaneActors[ i ]->GetVisibility() != 0 )
                this->CursorActors[ i ]->VisibilityOn();
    }

    double q[3];
    this->PlanePickers[ this->CurrentRendererIndex ]->GetPickPosition(q);

	if( this->Inputs.size() < 1 )
		return;
	vtkImageData * image = this->Inputs[0].ImageData;
    
    // Transform the pick position in the image space
    double qTransformed[3] = { 0, 0, 0 };
    this->TransformToImageSpace( q, qTransformed );

    // vtkImageData will find the nearest implicit point to q
	// we use the first volume for now.
	vtkIdType ptId = image->FindPoint(qTransformed);

    if ( ptId == -1 )
    {
        ActorVec::iterator it = this->CursorActors.begin();
        while( it != this->CursorActors.end() )
        {
            (*it)->VisibilityOff();
            ++it;
        }
        return;
    }

    this->SetCursorPosition( qTransformed );
}

// Assuming the plane source already intersects the point passed as a parameter (pos), compute
// the point where pos lies in the plane.
void vtkMultiImagePlaneWidget::SetCursorPosition( double pos[3] )
{
    // make sur the point is inside the bounding volume
    double bounds[6];
    double restrictedPos[3];
    restrictedPos[0] = pos[0];
    restrictedPos[1] = pos[1];
    restrictedPos[2] = pos[2];
    this->ComputeBounds( bounds );
    if( restrictedPos[0] < bounds[0] )
        restrictedPos[0] = bounds[0];
    if( restrictedPos[0] > bounds[1] )
        restrictedPos[0] = bounds[1];
    if( restrictedPos[1] < bounds[2] )
        restrictedPos[1] = bounds[2];
    if( restrictedPos[1] > bounds[3] )
        restrictedPos[1] = bounds[3];
    if( restrictedPos[2] < bounds[4] )
        restrictedPos[2] = bounds[4];
    if( restrictedPos[2] > bounds[5] )
        restrictedPos[2] = bounds[5];

    double p1[3];
    this->PlaneSource->GetPoint1(p1);
    double p2[3];
    this->PlaneSource->GetPoint2(p2);
    double o[3];
    this->PlaneSource->GetOrigin(o);

    double d1[3];
    vtkMath::Subtract( p1, o, d1 );
    double d2[3];
    vtkMath::Subtract( p2, o, d2 );
    double dPos[3];
    vtkMath::Subtract( restrictedPos, o, dPos );

    // project new pos to axes of the plane
    this->CursorPosition[0] = vtkMath::Dot(dPos,d1) / vtkMath::Dot(d1,d1);
    this->CursorPosition[1] = vtkMath::Dot(dPos,d2) / vtkMath::Dot(d2,d2);

    this->UpdateCursor();
}

// Use 2d position of cursor on the plane to update 3d data of geometry representing the cursor.
void vtkMultiImagePlaneWidget::UpdateCursor()
{
    double p1[3];
    this->PlaneSource->GetPoint1(p1);
    double p2[3];
    this->PlaneSource->GetPoint2(p2);
    double o[3];
    this->PlaneSource->GetOrigin(o);

    double d1[3];
    vtkMath::Subtract( p1, o, d1 );
    double d2[3];
    vtkMath::Subtract( p2, o, d2 );

    double a[3];
    double b[3];
    double c[3];
    double d[3];

    for( int i = 0; i < 3; i++)
    {
        a[i] = o[i]  + this->CursorPosition[1] * d2[i];   // left
        b[i] = p1[i] + this->CursorPosition[1] * d2[i];   // right
        c[i] = o[i]  + this->CursorPosition[0] * d1[i];   // bottom
        d[i] = p2[i] + this->CursorPosition[0] * d1[i];   // top
    }

    vtkPoints * cursorPts = this->CursorPolyData->GetPoints();

    cursorPts->SetPoint(0,a);
    cursorPts->SetPoint(1,b);
    cursorPts->SetPoint(2,c);
    cursorPts->SetPoint(3,d);
    cursorPts->Modified();

    this->CursorPolyData->Modified();
}

void vtkMultiImagePlaneWidget::SetOrigin(double x, double y, double z)
{
    this->PlaneSource->SetOrigin(x,y,z);
}

void vtkMultiImagePlaneWidget::SetOrigin(double xyz[3])
{
    this->PlaneSource->SetOrigin(xyz);
}

double* vtkMultiImagePlaneWidget::GetOrigin()
{
    return this->PlaneSource->GetOrigin();
}

void vtkMultiImagePlaneWidget::GetOrigin(double xyz[3])
{
    this->PlaneSource->GetOrigin(xyz);
}

void vtkMultiImagePlaneWidget::SetPoint1(double x, double y, double z)
{
    this->PlaneSource->SetPoint1(x,y,z);
}

void vtkMultiImagePlaneWidget::SetPoint1(double xyz[3])
{
    this->PlaneSource->SetPoint1(xyz);
}

double* vtkMultiImagePlaneWidget::GetPoint1()
{
    return this->PlaneSource->GetPoint1();
}

void vtkMultiImagePlaneWidget::GetPoint1(double xyz[3])
{
    this->PlaneSource->GetPoint1(xyz);
}

void vtkMultiImagePlaneWidget::SetPoint2(double x, double y, double z)
{
    this->PlaneSource->SetPoint2(x,y,z);
}

void vtkMultiImagePlaneWidget::SetPoint2(double xyz[3])
{
    this->PlaneSource->SetPoint2(xyz);
}

double* vtkMultiImagePlaneWidget::GetPoint2()
{
    return this->PlaneSource->GetPoint2();
}

void vtkMultiImagePlaneWidget::GetPoint2(double xyz[3])
{
    this->PlaneSource->GetPoint2(xyz);
}

double* vtkMultiImagePlaneWidget::GetCenter()
{
    return this->PlaneSource->GetCenter();
}

void vtkMultiImagePlaneWidget::GetCenter(double xyz[3])
{
    this->PlaneSource->GetCenter(xyz);
}

double* vtkMultiImagePlaneWidget::GetNormal()
{
    return this->PlaneSource->GetNormal();
}

void vtkMultiImagePlaneWidget::GetNormal(double xyz[3])
{
    this->PlaneSource->GetNormal(xyz);
}

void vtkMultiImagePlaneWidget::GetPolyData(vtkPolyData *pd)
{
    pd->ShallowCopy(this->PlaneSource->GetOutput());
}

void vtkMultiImagePlaneWidget::UpdatePlacement(void)
{
    this->PlaneSource->Update();
    this->BuildRepresentation();
    this->UpdateNormal();
    this->UpdateMargins();
}

void vtkMultiImagePlaneWidget::GetVector1(double v1[3])
{
    double* p1 = this->PlaneSource->GetPoint1();
    double* o =  this->PlaneSource->GetOrigin();
    v1[0] = p1[0] - o[0];
    v1[1] = p1[1] - o[1];
    v1[2] = p1[2] - o[2];
}

void vtkMultiImagePlaneWidget::GetVector2(double v2[3])
{
    double* p2 = this->PlaneSource->GetPoint2();
    double* o =  this->PlaneSource->GetOrigin();
    v2[0] = p2[0] - o[0];
    v2[1] = p2[1] - o[1];
    v2[2] = p2[2] - o[2];
}

void vtkMultiImagePlaneWidget::AdjustState()
{
    double v1[3];
    this->GetVector1(v1);
    double v2[3];
    this->GetVector2(v2);
    double planeSize1 = vtkMath::Normalize(v1);
    double planeSize2 = vtkMath::Normalize(v2);
    
    // Transform the last pick position in the space
    // of the widgets.
    double lastPickPositionTrans[ 3 ] = { 0, 0, 0 };
    this->TransformToImageSpace( this->LastPickPosition, lastPickPositionTrans );

    double * planeOrigin = this->PlaneSource->GetOrigin();
    
    double ppo[3] = {lastPickPositionTrans[0] - planeOrigin[0],
                     lastPickPositionTrans[1] - planeOrigin[1],
                     lastPickPositionTrans[2] - planeOrigin[2] };

    double x2D = vtkMath::Dot(ppo,v1);
    double y2D = vtkMath::Dot(ppo,v2);

    // Divide plane into three zones for different user interactions:
    // four corners -- spin around the plane's normal at its center
    // four edges   -- rotate around one of the plane's axes at its center
    // center area  -- push
    //
	double marginX = planeSize1 * MarginSize;
	double marginY = planeSize2 * MarginSize;

    double x0 = marginX;
    double y0 = marginY;
    double x1 = planeSize1 - marginX;
    double y1 = planeSize2 - marginY;

	if( this->DisableRotatingAndSpinning )
	{
		this->MarginSelectMode = 8;
	}
	else
	{
		if ( x2D < x0  )       // left margin
		{
			if (y2D < y0)        // bottom left corner
			{
				this->MarginSelectMode =  0;
			}
			else if (y2D > y1)   // top left corner
			{
				this->MarginSelectMode =  3;
			}
			else                 // left edge
			{
				this->MarginSelectMode =  4;
			}
		}
		else if ( x2D > x1 )   // right margin
		{
			if (y2D < y0)        // bottom right corner
			{
				this->MarginSelectMode =  1;
			}
			else if (y2D > y1)   // top right corner
			{
				this->MarginSelectMode =  2;
			}
			else                 // right edge
			{
				this->MarginSelectMode =  5;
			}
		}
		else                   // middle
		{
			if (y2D < y0)        // bottom edge
			{
				this->MarginSelectMode =  6;
			}
			else if (y2D > y1)   // top edge
			{
				this->MarginSelectMode =  7;
			}
			else                 // central area
			{
				this->MarginSelectMode =  8;
			}
		}
	}


	if (this->MarginSelectMode >= 0 && this->MarginSelectMode < 4)
	{
		this->State = vtkMultiImagePlaneWidget::Spinning;
		return;
	}
	else if (this->MarginSelectMode == 8)
	{
		this->State = vtkMultiImagePlaneWidget::Pushing;
		return;
	}
	else
	{
		this->State = vtkMultiImagePlaneWidget::Rotating;
	}

    double *raPtr = 0;
    double *rvPtr = 0;
    double rvfac = 1.0;
    double rafac = 1.0;

    switch ( this->MarginSelectMode )
    {
        // left bottom corner
    case 0:
        raPtr = v2;
        rvPtr = v1;
        rvfac = -1.0;
        rafac = -1.0;
        break;
        // right bottom corner
    case 1:
        raPtr = v2;
        rvPtr = v1;
        rafac = -1.0;
        break;
        // right top corner
    case 2:
        raPtr = v2;
        rvPtr = v1;
        break;
        // left top corner
    case 3:
        raPtr = v2;
        rvPtr = v1;
        rvfac = -1.0;
        break;
    case 4:
        raPtr = v2;
        rvPtr = v1;
        rvfac = -1.0;
        break; // left
    case 5:
        raPtr = v2;
        rvPtr = v1;
        break; // right
    case 6:
        raPtr = v1;
        rvPtr = v2;
        rvfac = -1.0;
        break; // bottom
    case 7:
        raPtr = v1;
        rvPtr = v2;
        break; // top
    default:
        raPtr = v1;
        rvPtr = v2;
        break;
    }

    for (int i = 0; i < 3; i++)
    {
        this->RotateAxis[i] = *raPtr++ * rafac;
        this->RadiusVector[i] = *rvPtr++ * rvfac;
    }
}

void vtkMultiImagePlaneWidget::Spin(double *p1, double *p2)
{
    // Disable cursor snap
    //
    this->PlaneOrientation = 3;

    // Get the motion vector, in world coords
    //
    double v[3];
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];

    // Plane center and normal before transform
    //
    double* wc = this->PlaneSource->GetCenter();
    double* wn = this->Normal;

    // Radius vector from center to cursor position
    //
    double rv[3] = {p2[0]-wc[0], p2[1]-wc[1], p2[2]-wc[2]};

    // Distance between center and cursor location
    //
    double rs = vtkMath::Normalize(rv);

    // Spin direction
    //
    double wn_cross_rv[3];
    vtkMath::Cross(wn,rv,wn_cross_rv);

    // Spin angle
    //
    double dw = vtkMath::DegreesFromRadians(vtkMath::Dot(v,wn_cross_rv) / rs);
    this->Transform->Identity();
    this->Transform->Translate(wc[0],wc[1],wc[2]);
    this->Transform->RotateWXYZ(dw,wn);
    this->Transform->Translate(-wc[0],-wc[1],-wc[2]);

    double newpt[3];
    this->Transform->TransformPoint(this->PlaneSource->GetPoint1(),newpt);
    this->PlaneSource->SetPoint1(newpt);
    this->Transform->TransformPoint(this->PlaneSource->GetPoint2(),newpt);
    this->PlaneSource->SetPoint2(newpt);
    this->Transform->TransformPoint(this->PlaneSource->GetOrigin(),newpt);
    this->PlaneSource->SetOrigin(newpt);

    this->PlaneSource->Update();
    this->BuildRepresentation();
}

void vtkMultiImagePlaneWidget::Rotate(double *p1, double *p2, double *vpn)
{
    // Disable cursor snap
    //
    this->PlaneOrientation = 3;

    // Get the motion vector, in world coords
    //
    double v[3];
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];

    // Plane center and normal
    //
    double* wc = this->PlaneSource->GetCenter();

    // Radius of the rotating circle of the picked point
    //
    double radius = fabs( this->RadiusVector[0]*(p2[0]-wc[0]) +
                          this->RadiusVector[1]*(p2[1]-wc[1]) +
                          this->RadiusVector[2]*(p2[2]-wc[2]) );

    // Rotate direction ra_cross_rv
    //
    double rd[3];
    vtkMath::Cross(this->RotateAxis,this->RadiusVector,rd);

    // Direction cosin between rotating direction and view normal
    //
    double rd_dot_vpn = rd[0]*vpn[0] + rd[1]*vpn[1] + rd[2]*vpn[2];

    // 'push' plane edge when mouse moves away from plane center
    // 'pull' plane edge when mouse moves toward plane center
    //
    double dw = vtkMath::DegreesFromRadians((vtkMath::Dot(this->RadiusVector,v))/radius * (-rd_dot_vpn));
    this->Transform->Identity();
    this->Transform->Translate(wc[0],wc[1],wc[2]);
    this->Transform->RotateWXYZ(dw,this->RotateAxis);
    this->Transform->Translate(-wc[0],-wc[1],-wc[2]);

    double newpt[3];
    this->Transform->TransformPoint(this->PlaneSource->GetPoint1(),newpt);
    this->PlaneSource->SetPoint1(newpt);
    this->Transform->TransformPoint(this->PlaneSource->GetPoint2(),newpt);
    this->PlaneSource->SetPoint2(newpt);
    this->Transform->TransformPoint(this->PlaneSource->GetOrigin(),newpt);
    this->PlaneSource->SetOrigin(newpt);

    this->PlaneSource->Update();
    this->BuildRepresentation();
}

void vtkMultiImagePlaneWidget::GeneratePlaneOutline()
{
    vtkPoints* points   = vtkPoints::New(VTK_DOUBLE);
    points->SetNumberOfPoints(4);
    int i;
    for (i = 0; i < 4; i++)
    {
        points->SetPoint(i,0.0,0.0,0.0);
    }

    vtkCellArray *cells = vtkCellArray::New();
    cells->Allocate(cells->EstimateSize(4,2));
    vtkIdType pts[2];
    pts[0] = 3;
    pts[1] = 2;       // top edge
    cells->InsertNextCell(2,pts);
    pts[0] = 0;
    pts[1] = 1;       // bottom edge
    cells->InsertNextCell(2,pts);
    pts[0] = 0;
    pts[1] = 3;       // left edge
    cells->InsertNextCell(2,pts);
    pts[0] = 1;
    pts[1] = 2;       // right edge
    cells->InsertNextCell(2,pts);

    this->PlaneOutlinePolyData->SetPoints(points);
    points->Delete();
    this->PlaneOutlinePolyData->SetLines(cells);
    cells->Delete();
}


void vtkMultiImagePlaneWidget::GenerateMargins()
{
    // Construct initial points
    vtkPoints* points = vtkPoints::New(VTK_DOUBLE);
    points->SetNumberOfPoints(8);
    int i;
    for (i = 0; i < 8; i++)
    {
        points->SetPoint(i,0.0,0.0,0.0);
    }

    vtkCellArray *cells = vtkCellArray::New();
    cells->Allocate(cells->EstimateSize(4,2));
    vtkIdType pts[2];
    pts[0] = 0;
    pts[1] = 1;       // top margin
    cells->InsertNextCell(2,pts);
    pts[0] = 2;
    pts[1] = 3;       // bottom margin
    cells->InsertNextCell(2,pts);
    pts[0] = 4;
    pts[1] = 5;       // left margin
    cells->InsertNextCell(2,pts);
    pts[0] = 6;
    pts[1] = 7;       // right margin
    cells->InsertNextCell(2,pts);

    this->MarginPolyData->SetPoints(points);
    points->Delete();
    this->MarginPolyData->SetLines(cells);
    cells->Delete();
}

void vtkMultiImagePlaneWidget::GenerateCursor()
{
    // Construct initial points
    //
    vtkPoints* points = vtkPoints::New(VTK_DOUBLE);
    points->SetNumberOfPoints(4);
    int i;
    for (i = 0; i < 4; i++)
    {
        points->SetPoint(i,0.0,0.0,0.0);
    }

    vtkCellArray *cells = vtkCellArray::New();
    cells->Allocate(cells->EstimateSize(2,2));
    vtkIdType pts[2];
    pts[0] = 0;
    pts[1] = 1;       // horizontal segment
    cells->InsertNextCell(2,pts);
    pts[0] = 2;
    pts[1] = 3;       // vertical segment
    cells->InsertNextCell(2,pts);

    this->CursorPolyData->SetPoints(points);
    points->Delete();
    this->CursorPolyData->SetLines(cells);
    cells->Delete();
}


void vtkMultiImagePlaneWidget::UpdateMargins()
{
    double v1[3];
    this->GetVector1(v1);
    double v2[3];
    this->GetVector2(v2);
    double o[3];
    this->PlaneSource->GetOrigin(o);
    double p1[3];
    this->PlaneSource->GetPoint1(p1);
    double p2[3];
    this->PlaneSource->GetPoint2(p2);

    double a[3];
    double b[3];
    double c[3];
    double d[3];

	double s = MarginSize;
	double t = MarginSize;

    int i;
    for ( i = 0; i < 3; i++)
    {
        a[i] = o[i] + v2[i]*(1-t);
        b[i] = p1[i] + v2[i]*(1-t);
        c[i] = o[i] + v2[i]*t;
        d[i] = p1[i] + v2[i]*t;
    }

    vtkPoints* marginPts = this->MarginPolyData->GetPoints();

    marginPts->SetPoint(0,a);
    marginPts->SetPoint(1,b);
    marginPts->SetPoint(2,c);
    marginPts->SetPoint(3,d);

    for ( i = 0; i < 3; i++)
    {
        a[i] = o[i] + v1[i]*s;
        b[i] = p2[i] + v1[i]*s;
        c[i] = o[i] + v1[i]*(1-s);
        d[i] = p2[i] + v1[i]*(1-s);
    }

    marginPts->SetPoint(4,a);
    marginPts->SetPoint(5,b);
    marginPts->SetPoint(6,c);
    marginPts->SetPoint(7,d);

    this->MarginPolyData->Modified();
}

void vtkMultiImagePlaneWidget::Translate(double *p1, double *p2)
{
    // Get the motion vector
    //
    double v[3];
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];

    double *o = this->PlaneSource->GetOrigin();
    double *pt1 = this->PlaneSource->GetPoint1();
    double *pt2 = this->PlaneSource->GetPoint2();
    double origin[3], point1[3], point2[3];

    double vdrv = this->RadiusVector[0]*v[0] + \
                  this->RadiusVector[1]*v[1] + \
                  this->RadiusVector[2]*v[2];
    double vdra = this->RotateAxis[0]*v[0] + \
                  this->RotateAxis[1]*v[1] + \
                  this->RotateAxis[2]*v[2];

    int i;
    if ( this->MarginSelectMode == 8 )       // everybody comes along
    {
        for (i=0; i<3; i++)
        {
            origin[i] = o[i] + v[i];
            point1[i] = pt1[i] + v[i];
            point2[i] = pt2[i] + v[i];
        }
        this->PlaneSource->SetOrigin(origin);
        this->PlaneSource->SetPoint1(point1);
        this->PlaneSource->SetPoint2(point2);
    }
    else if ( this->MarginSelectMode == 4 ) // left edge
    {
        for (i=0; i<3; i++)
        {
            origin[i] = o[i]   + vdrv*this->RadiusVector[i];
            point2[i] = pt2[i] + vdrv*this->RadiusVector[i];
        }
        this->PlaneSource->SetOrigin(origin);
        this->PlaneSource->SetPoint2(point2);
    }
    else if ( this->MarginSelectMode == 5 ) // right edge
    {
        for (i=0; i<3; i++)
        {
            point1[i] = pt1[i] + vdrv*this->RadiusVector[i];
        }
        this->PlaneSource->SetPoint1(point1);
    }
    else if ( this->MarginSelectMode == 6 ) // bottom edge
    {
        for (i=0; i<3; i++)
        {
            origin[i] = o[i]   + vdrv*this->RadiusVector[i];
            point1[i] = pt1[i] + vdrv*this->RadiusVector[i];
        }
        this->PlaneSource->SetOrigin(origin);
        this->PlaneSource->SetPoint1(point1);
    }
    else if ( this->MarginSelectMode == 7 ) // top edge
    {
        for (i=0; i<3; i++)
        {
            point2[i] = pt2[i] + vdrv*this->RadiusVector[i];
        }
        this->PlaneSource->SetPoint2(point2);
    }
    else if ( this->MarginSelectMode == 3 ) // top left corner
    {
        for (i=0; i<3; i++)
        {
            origin[i] = o[i]   + vdrv*this->RadiusVector[i];
            point2[i] = pt2[i] + vdrv*this->RadiusVector[i] +
                        vdra*this->RotateAxis[i];
        }
        this->PlaneSource->SetOrigin(origin);
        this->PlaneSource->SetPoint2(point2);
    }
    else if ( this->MarginSelectMode == 0 ) // bottom left corner
    {
        for (i=0; i<3; i++)
        {
            origin[i] = o[i]   + vdrv*this->RadiusVector[i] +
                        vdra*this->RotateAxis[i];
            point1[i] = pt1[i] + vdra*this->RotateAxis[i];
            point2[i] = pt2[i] + vdrv*this->RadiusVector[i];
        }
        this->PlaneSource->SetOrigin(origin);
        this->PlaneSource->SetPoint1(point1);
        this->PlaneSource->SetPoint2(point2);
    }
    else if ( this->MarginSelectMode == 2 ) // top right corner
    {
        for (i=0; i<3; i++)
        {
            point1[i] = pt1[i] + vdrv*this->RadiusVector[i];
            point2[i] = pt2[i] + vdra*this->RotateAxis[i];
        }
        this->PlaneSource->SetPoint1(point1);
        this->PlaneSource->SetPoint2(point2);
    }
    else                                   // bottom right corner
    {
        for (i=0; i<3; i++)
        {
            origin[i] = o[i]   + vdra*this->RotateAxis[i];
            point1[i] = pt1[i] + vdrv*this->RadiusVector[i] +
                        vdra*this->RotateAxis[i];
        }
        this->PlaneSource->SetPoint1(point1);
        this->PlaneSource->SetOrigin(origin);
    }

    this->PlaneSource->Update();
    this->BuildRepresentation();
}

void vtkMultiImagePlaneWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
    // Get the motion vector
    //
    double v[3];
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];

    double *o = this->PlaneSource->GetOrigin();
    double *pt1 = this->PlaneSource->GetPoint1();
    double *pt2 = this->PlaneSource->GetPoint2();

    double center[3];
    center[0] = o[0] + (pt1[0]-o[0])/2.0 + (pt2[0]-o[0])/2.0;
    center[1] = o[1] + (pt1[1]-o[1])/2.0 + (pt2[1]-o[1])/2.0;
    center[2] = o[2] + (pt1[2]-o[2])/2.0 + (pt2[2]-o[2])/2.0;

    // Compute the scale factor
    //
    double sf = vtkMath::Norm(v) /
                sqrt(vtkMath::Distance2BetweenPoints(pt1,pt2));
    if ( Y > this->Interactors[ this->CurrentInteractorIndex ]->GetLastEventPosition()[1] )
    {
        sf = 1.0 + sf;
    }
    else
    {
        sf = 1.0 - sf;
    }

    // Move the corner points
    //
    double origin[3], point1[3], point2[3];

    for (int i=0; i<3; i++)
    {
        origin[i] = sf * (o[i] - center[i]) + center[i];
        point1[i] = sf * (pt1[i] - center[i]) + center[i];
        point2[i] = sf * (pt2[i] - center[i]) + center[i];
    }

    this->PlaneSource->SetOrigin(origin);
    this->PlaneSource->SetPoint1(point1);
    this->PlaneSource->SetPoint2(point2);
    this->PlaneSource->Update();
    this->BuildRepresentation();
}

void vtkMultiImagePlaneWidget::TransformToImageSpace( double * in, double * out )
{
    if( this->BoundingTransform )
    {
        // Get inverse of bounding transform
        vtkMatrix4x4 * invBoundingTrans = vtkMatrix4x4::New();
        this->BoundingTransform->GetInverse( invBoundingTrans );

        // convert input to 4 components
        double untransformed[ 4 ] = { 0, 0, 0, 1 };
        untransformed[ 0 ] = in[0];
        untransformed[ 1 ] = in[1];
        untransformed[ 2 ] = in[2];

        // create temp output
        double transformed[ 4 ] = { 0, 0, 0, 1 };

        // compute transform
        invBoundingTrans->MultiplyPoint( untransformed, transformed );
        invBoundingTrans->Delete();

        // copy back result to output
        out[ 0 ] = transformed[ 0 ];
        out[ 1 ] = transformed[ 1 ];
        out[ 2 ] = transformed[ 2 ];
    }
    else
    {
        // no bounding transform, so assume volume is not transformed
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[2];
    }
}
