/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "triplecutplaneobject.h"
#include "view.h"
#include "imageobject.h"
#include "scenemanager.h"
#include "triplecutplaneobjectsettingswidget.h"
#include "vtkMultiImagePlaneWidget.h"
#include <vtkAssembly.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkProperty.h>

ObjectSerializationMacro( TripleCutPlaneObject );

//===== Blending modes =====
struct BlendingModeInfo
{
    BlendingModeInfo( QString n, int m ) : name(n), mode(m) {}
    QString name;
    int mode;
};

std::vector<BlendingModeInfo> FillBlendingModeInfo()
{
    std::vector<BlendingModeInfo> modes;
    modes.push_back( BlendingModeInfo( "Replace", 1 /* vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE */ ) );
    modes.push_back( BlendingModeInfo( "Modulate", 2 /* vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE */ ) );
    modes.push_back( BlendingModeInfo( "Add", 3 /* vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD */ ) );
    modes.push_back( BlendingModeInfo( "Add signed", 4 /* vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD_SIGNED */ ) );
    modes.push_back( BlendingModeInfo( "Interpolate", 5 /* vtkTexture::VTK_TEXTURE_BLENDING_MODE_INTERPOLATE */ ) );
    modes.push_back( BlendingModeInfo( "Subtract", 6 /* vtkTexture::VTK_TEXTURE_BLENDING_MODE_SUBTRACT */ ) );
    return modes;
}

std::vector< BlendingModeInfo > BlendingModes = FillBlendingModeInfo();

//=============================

TripleCutPlaneObject::TripleCutPlaneObject()
{
    this->PlaneInteractionSlotConnect = vtkEventQtSlotConnect::New();
    this->PlaneEndInteractionSlotConnect = vtkEventQtSlotConnect::New();
    m_resliceInterpolationType = 1;  // linear interpolation
    m_displayInterpolationType = 1;  // linear interpolation
    for (int i = 0; i < 3; i++)
    {
        this->Planes[i] = 0;
        this->ViewPlanes[i] = 1;
        CreatePlane( i );
        m_planePosition[i] = 0.0;
    }
    this->CursorVisible = true;   
    m_sliceThickness = 1;
}


TripleCutPlaneObject::~TripleCutPlaneObject()
{
    this->PlaneInteractionSlotConnect->Delete();
    this->PlaneEndInteractionSlotConnect->Delete();

    // Remove planes from the views
    ReleaseAllViews();

    // Delete planes
    for(int i = 0; i < 3; i++ )
    {
        this->Planes[i]->Delete();
    }

    // Release hook on images
    for( unsigned i = 0; i < Images.size(); ++i )
    {
        ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( Images[i] ) );
        im->UnRegister( 0 );
    }
}

void TripleCutPlaneObject::Serialize( Serializer * ser )
{
    //cursor visibility is a World property and it is serialized in mainwindow.cpp
    ::Serialize( ser, "ViewPlanes", this->ViewPlanes, 3 );
    double pos[3];
    if (ser->IsReader())
    {
        ::Serialize( ser, "PlanesPosition", m_planePosition, 3 );
        for(int i = 0; i < 3; ++i)
        {
            this->SetViewPlane(i, this->ViewPlanes[i]);
            this->Planes[i]->SetPosition(m_planePosition);
        }
    }
    else
    {
        this->Planes[0]->GetPosition(pos);
        ::Serialize( ser, "PlanesPosition", pos, 3 );
    }
    ::Serialize( ser, "SliceThickness", m_sliceThickness );
    ::Serialize( ser, "SliceMixMode", m_sliceMixMode );
    ::Serialize( ser, "BlendingModeIndices", m_blendingModeIndices );
}

void TripleCutPlaneObject::PostSceneRead()
{
    this->SetSliceThickness( m_sliceThickness );
    while (m_sliceMixMode.size() > Images.size()) // something really bad happened - probably file was missing
        m_sliceMixMode.pop_back();

    for( unsigned i = 0; i < m_sliceMixMode.size(); ++i )
    {
        this->SetSliceMixMode( i, m_sliceMixMode[i] );
    }
    this->SetPlanesPosition(m_planePosition);
    emit Modified();
}

bool TripleCutPlaneObject::Setup( View * view )
{
    if( this->Images.size() == 0 )
        return false;

    bool success = SceneObject::Setup( view );
    if( !success )
        return false;

    switch( view->GetType() )
    {
    case SAGITTAL_VIEW_TYPE:
        this->Setup2DRepresentation( 0, view );
        break;
    case CORONAL_VIEW_TYPE:
        this->Setup2DRepresentation( 1, view );
        break;
    case TRANSVERSE_VIEW_TYPE:
        this->Setup2DRepresentation( 2, view );
        break;
    case THREED_VIEW_TYPE:
        this->Setup3DRepresentation( view );
        break;
    }
    return true;
}

bool TripleCutPlaneObject::Release( View * view )
{
    bool success = SceneObject::Release( view );
    if( !success )
        return false;

    switch( view->GetType() )
    {
    case SAGITTAL_VIEW_TYPE:
        this->Release2DRepresentation( 0, view );
        break;
    case CORONAL_VIEW_TYPE:
        this->Release2DRepresentation( 1, view );
        break;
    case TRANSVERSE_VIEW_TYPE:
        this->Release2DRepresentation( 2, view );
        break;
    case THREED_VIEW_TYPE:
        this->Release3DRepresentation( view );
        break;
    }
    return true;
}


void TripleCutPlaneObject::ReleaseAllViews()
{
    for( int i = 0; i < 3; ++i )
        this->Planes[i]->EnabledOff();
    SceneObject::ReleaseAllViews();
}

QWidget * TripleCutPlaneObject::CreateSettingsDialog( QWidget * parent )
{
    TripleCutPlaneObjectSettingsWidget * res = new TripleCutPlaneObjectSettingsWidget( parent );
    res->setObjectName( "TripleCutPlaneObjectSettingsWidget" );
    res->setAttribute(Qt::WA_DeleteOnClose);
    res->SetTripleCutPlaneObject( this );
    return res;
}

ImageObject * TripleCutPlaneObject::GetImage( int index )
{
    Q_ASSERT( index < Images.size() );
    return ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( Images[ index ] ) );
}

void TripleCutPlaneObject::AddImage( int imageID )
{
    // Make sure the image is not already there
    ImageContainer::iterator it = std::find( Images.begin(), Images.end(), imageID );
    if( it != Images.end() )
            return;

    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( imageID ) );
    Q_ASSERT(im);
    im->Register( 0 );
    Images.push_back( imageID );
    if (m_sliceMixMode.size() < Images.size())
        m_sliceMixMode.push_back( VTK_IMAGE_SLAB_MEAN );
    if( m_blendingModeIndices.size() < Images.size() )
        m_blendingModeIndices.push_back( 2 );//this is index of vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD in BlendingModes vector

    bool refImage = false;
    if( Images.size() == 1 )
        refImage = true;

    for( int i = 0; i < 3; ++i )
    {
        if( refImage )
        {
            this->Planes[i]->SetBoundingVolume( im->GetImage(), im->GetWorldTransform() );
        }
        bool canInterpolate = !im->IsLabelImage();
        this->Planes[i]->AddInput( im->GetImage(), im->GetLut(), im->GetWorldTransform(), canInterpolate );
        this->Planes[i]->SetImageHidden( im->GetImage(), im->IsHidden() );
        this->Planes[i]->SetBlendingMode( Images.size() - 1, BlendingModes[ m_blendingModeIndices[ Images.size() - 1 ] ].mode );
    }
}

void TripleCutPlaneObject::RemoveImage( int imageID )
{
    // Make sure the images is there
    ImageContainer::iterator it = Images.begin();
    for(int i = 0 ; it != Images.end(); it++, i ++)
    {
        if ((*it) == imageID)
        {
            m_sliceMixMode.erase(m_sliceMixMode.begin()+i);
            m_blendingModeIndices.erase(m_blendingModeIndices.begin()+i);
            ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( imageID ) );
            if(im )
            {
                im->UnRegister( 0 );
            }
            break;
        }
    }

    if( it == Images.end() )
            return;

    Images.erase( it );
    if( this->GetManager()->GetReferenceDataObject()->GetObjectID() == imageID && Images.size() != 0 )
    {
        this->GetManager()->SetReferenceDataObject( this->GetManager()->GetObjectByID( Images[0] )); // emits callback that will execute AdjustAllImages()
    }
    else
        this->AdjustAllImages();

    if( Images.size() == 0 )
        ReleaseAllViews();
}

void TripleCutPlaneObject::AdjustAllImages()
{
    ImageObject *referenceObject = this->GetManager()->GetReferenceDataObject();
    if( referenceObject && Images.size() > 0 )
    {
        int refID = referenceObject->GetObjectID();
        for( int j = 0; j < 3; ++j )
        {
            // remove all inputs and re-add images.
            // (seems stupid, but it is better like that for texture unit consistency)
            this->Planes[j]->ClearAllInputs();
            // first add reference object
            bool canInterpolate = !referenceObject->IsLabelImage();
            this->Planes[j]->SetBoundingVolume( referenceObject->GetImage(), referenceObject->GetWorldTransform() );
            this->Planes[j]->AddInput( referenceObject->GetImage(), referenceObject->GetLut(), referenceObject->GetWorldTransform(), canInterpolate );
            this->Planes[j]->SetImageHidden( referenceObject->GetImage(), referenceObject->IsHidden() );
            ImageContainer::iterator it = Images.begin();
            for(int l = 0 ; it != Images.end(); it++, l++)
            {
                if ((*it) == refID)
                {
                    this->Planes[j]->SetBlendingMode( 0, BlendingModes[ m_blendingModeIndices[ l ] ].mode );
                }
            }
            if( it == Images.end() )

            for( uint i = 0; i < Images.size(); ++i )
            {
                ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( Images[i] ) );
                if( im->GetObjectID() != refID )
                {
                    bool canInterpolate = im->IsLabelImage();
                    this->Planes[j]->AddInput( im->GetImage(), im->GetLut(), im->GetWorldTransform(), canInterpolate );
                    this->Planes[j]->SetImageHidden( im->GetImage(), im->IsHidden() );
                }
            }
        }
        this->GetManager()->ResetAllCameras();
    }
}

void TripleCutPlaneObject::UpdateLut(int imageID )
{
    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( imageID ) );
    for( int i = 0; i < 3; ++i )
    {
        this->Planes[i]->SetLookupTable( im->GetImage(), im->GetLut() );
    }
}

void TripleCutPlaneObject::SetImageHidden( int imageID, bool hidden )
{
    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( imageID ) );
    for( int i = 0; i < 3; ++i )
        this->Planes[i]->SetImageHidden( im->GetImage(), hidden );
}

void TripleCutPlaneObject::PreDisplaySetup()
{
    Q_ASSERT_X( this->Planes[0], "TripleCutPlaneObject::PreDisplaySetup()", "Can't call this function before planes have been created." );
    Q_ASSERT_X( this->Planes[1], "TripleCutPlaneObject::PreDisplaySetup()", "Can't call this function before planes have been created." );
    Q_ASSERT_X( this->Planes[2], "TripleCutPlaneObject::PreDisplaySetup()", "Can't call this function before planes have been created." );
    for( int i = 0; i < 3; i++ )
    {
        this->Planes[i]->EnabledOn();
        this->Planes[i]->ActivateCursor( CursorVisible );
        SetViewPlane( i, this->ViewPlanes[ i ] );
    }
}

void TripleCutPlaneObject::ResetPlanes()
{
    Q_ASSERT( this->GetManager() );

    if (this->GetManager()->GetReferenceDataObject())
    {
        for( int i = 0; i < 3; i++ )
        {
            this->Planes[i]->PlaceWidget();
            emit PlaneMoved(i);
        }
        emit Modified();
    }
}

void TripleCutPlaneObject::SetViewPlane( int planeIndex, int isOn )
{
    this->ViewPlanes[ planeIndex ] = isOn;
    View * v = this->Get3DView();
    if( v )
    {
        this->Planes[ planeIndex ]->Show( v->GetRenderer(), isOn );
    }
    emit Modified();
}

int TripleCutPlaneObject::GetViewPlane( int planeIndex )
{
    return this->ViewPlanes[ planeIndex ];
}

void TripleCutPlaneObject::SetViewAllPlanes( int viewOn )
{
    for( int i = 0; i < 3; ++i )
        this->SetViewPlane( i, viewOn );
}

void TripleCutPlaneObject::SetCursorVisibility( bool v )
{
    if( CursorVisible == v )
        return;

    for( int i = 0; i < 3; ++i )
        this->Planes[i]->ActivateCursor( v );
    CursorVisible = v;
    emit Modified();
}

void TripleCutPlaneObject::SetCursorColor( const QColor & c )
{
    double newColorfloat[3] = { 1, 1, 1 };
    newColorfloat[0] = double( c.red() ) / 255.0;
    newColorfloat[1] = double( c.green() ) / 255.0;
    newColorfloat[2] = double( c.blue() ) / 255.0;
    for( int i = 0; i < 3; ++i )
    {
        this->Planes[i]->GetCursorProperty()->SetColor(newColorfloat);
    }
}

QColor TripleCutPlaneObject::GetCursorColor()
{
    double color[3];
    this->Planes[0]->GetCursorProperty()->GetColor(color);
    QColor ret( (int)(color[0] * 255), (int)(color[1] * 255), (int)(color[2] * 255) );
    return ret;
}

void TripleCutPlaneObject::SetResliceInterpolationType( int type )
{
    Q_ASSERT( type >= 0 && type < 3 );

    m_resliceInterpolationType = type;
    for( int i = 0; i < 3; ++i )
        this->Planes[i]->SetResliceInterpolate( m_resliceInterpolationType );
    emit Modified();
}

void TripleCutPlaneObject::SetDisplayInterpolationType( int type )
{
    Q_ASSERT( type >= 0 && type < 2 );

    m_displayInterpolationType = type;
    for( int i = 0; i < 3; ++i )
        this->Planes[i]->SetTextureInterpolate( m_displayInterpolationType );
    emit Modified();
}

void TripleCutPlaneObject::SetSliceThickness( int nbSlices )
{
    m_sliceThickness = nbSlices;
    for( int i = 0; i < 3; ++i )
        this->Planes[i]->SetSliceThickness( nbSlices );
    emit Modified();
}

void TripleCutPlaneObject::SetSliceMixMode( int imageIndex, int mode )
{
    Q_ASSERT( imageIndex < m_sliceMixMode.size() );
    m_sliceMixMode[ imageIndex ] = mode;
    for( int i = 0; i < 3; ++i )
        this->Planes[i]->SetSliceMixMode( imageIndex, mode );
    emit Modified();
}

void TripleCutPlaneObject::SetColorTable( int imageIndex, int colorTableIndex )
{
    Q_ASSERT( imageIndex < Images.size() );
    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( Images[imageIndex] ) );
    im->ChooseColorTable( colorTableIndex );
    emit Modified();
}

int TripleCutPlaneObject::GetColorTable( int imageIndex )
{
    Q_ASSERT( imageIndex < Images.size() );
    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( Images[imageIndex] ) );
    return im->GetLutIndex();
}

void TripleCutPlaneObject::SetImageIntensityFactor( int imageIndex, double factor )
{
    Q_ASSERT( imageIndex < Images.size() );
    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( Images[imageIndex] ) );
    im->SetIntensityFactor( factor );
}

double TripleCutPlaneObject::GetImageIntensityFactor( int imageIndex )
{
    Q_ASSERT( imageIndex < Images.size() );
    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( Images[imageIndex] ) );
    return im->GetIntensityFactor();
}

int TripleCutPlaneObject::GetNumberOfBlendingModes()
{
    return BlendingModes.size();
}

QString TripleCutPlaneObject::GetBlendingModeName( int blendingModeIndex )
{
    Q_ASSERT( blendingModeIndex < BlendingModes.size() );
    return BlendingModes[ blendingModeIndex ].name;
}

void TripleCutPlaneObject::SetBlendingModeIndex( int imageIndex, int blendingModeIndex )
{
    Q_ASSERT( imageIndex < m_blendingModeIndices.size() );
    Q_ASSERT( blendingModeIndex < BlendingModes.size() );
    m_blendingModeIndices[ imageIndex ] = blendingModeIndex;
    for( int i = 0; i < 3; ++i )
        this->Planes[i]->SetBlendingMode( imageIndex, BlendingModes[ blendingModeIndex ].mode );
    emit Modified();
}

void TripleCutPlaneObject::SetWorldPlanesPosition( double * pos )
{
    Q_ASSERT( this->GetManager() );
    ImageObject *ref = this->GetManager()->GetReferenceDataObject();
    if (ref && pos)
    {
        vtkLinearTransform *inverseReg = ref->GetWorldTransform()->GetLinearInverse();
        double *newPos = inverseReg->TransformDoublePoint(pos);
        this->SetPlanesPosition(newPos);
    }
}

void TripleCutPlaneObject::GetPlanesPosition( double pos[3] )
{
    this->Planes[0]->GetPosition( pos );
}

bool TripleCutPlaneObject::IsInPlane( VIEWTYPES planeType, double pos[3] )
{
    double dist = this->Planes[ planeType ]->DistanceToPlane( pos );
    ImageObject *ref = this->GetManager()->GetReferenceDataObject();
    if( ref )
    {
        double * spacing = ref->GetSpacing();
        return dist < spacing[planeType] * 0.5;
    }
    return dist < 0.5; // no ref volume, assume voxel spacing = 1mm
}

void TripleCutPlaneObject::SetPlanesPosition( double * pos )
{
    for( int i = 0; i < 3; ++i )
    {
        this->Planes[i]->SetPosition( pos );
        emit PlaneMoved(i);
    }
    emit Modified();
}

void TripleCutPlaneObject::PlaneStartInteractionEvent( vtkObject * caller, unsigned long event )
{
    Q_ASSERT( this->GetManager() );
    int whichPlane = GetPlaneIndex( caller );
    Q_ASSERT( whichPlane != -1 );
    emit StartPlaneMoved( whichPlane );
    UpdateOtherPlanesPosition( whichPlane );
}

void TripleCutPlaneObject::PlaneInteractionEvent( vtkObject * caller, unsigned long /*event*/ )
{
    int whichPlane = GetPlaneIndex( caller );
    Q_ASSERT( whichPlane != -1 );
    UpdateOtherPlanesPosition( whichPlane );
    emit Modified();
}

void TripleCutPlaneObject::PlaneEndInteractionEvent( vtkObject * caller, unsigned long /*event*/ )
{
    Q_ASSERT( this->GetManager() );

    // Find which plane generated the event
    int whichPlane = GetPlaneIndex( caller );
    Q_ASSERT( whichPlane != -1 );

    this->UpdateOtherPlanesPosition( whichPlane );

    if (this->GetManager()->GetCurrentView() == THREED_VIEW_TYPE)
        emit EndPlaneMove(whichPlane);
    else
    {
        // if we use left mouse button, then only the "whichPlane" moves, if it is a right button, 2 other planes move
        // at this point, we do not know which button was pressed, so we adjust all planes
        for( int i = 0; i < 3; ++i )
        {
            emit EndPlaneMove(i);
        }
    }

    emit Modified();
}

void TripleCutPlaneObject::MarkModified()
{
    for( int i = 0; i < 3; i++ )
    {
        this->Planes[i]->UpdateNormal();
    }
    SceneObject::MarkModified();
}

View * TripleCutPlaneObject::Get3DView()
{
    for( ViewContainer::iterator it = Views.begin(); it != Views.end(); ++it )
    {
        if( (*it)->GetType() == THREED_VIEW_TYPE )
        {
            return *it;
        }
    }
    return 0;
}

void TripleCutPlaneObject::Setup3DRepresentation( View * view )
{
    for( int i = 0; i < 3; i++ )
    {
        this->Planes[i]->AddInteractor( view->GetInteractor() );
        int renIndex = this->Planes[i]->AddRenderer( view->GetRenderer());
        this->Planes[i]->SetPlaneMoveMethod( renIndex, vtkMultiImagePlaneWidget::Move3D );
        this->Planes[i]->SetPicker( this->Planes[i]->GetNumberOfRenderers() - 1, view->GetPicker() );
        SetViewPlane( i, this->ViewPlanes[i] );
    }
}


void TripleCutPlaneObject::Release3DRepresentation( View * view )
{
    // release the planes
    for( int i = 0; i < 3; i++ )
    {
        this->Planes[i]->RemoveInteractor( view->GetInteractor() );
        this->Planes[i]->RemoveRenderer( view->GetRenderer() );
    }
}

void TripleCutPlaneObject::Setup2DRepresentation( int viewType, View * view )
{
    this->Planes[viewType]->AddInteractor( view->GetInteractor() );
    int renIndex = this->Planes[viewType]->AddRenderer( view->GetRenderer() );
    this->Planes[viewType]->SetPlaneMoveMethod( renIndex, vtkMultiImagePlaneWidget::Move2D );
    this->Planes[viewType]->SetPicker( this->Planes[viewType]->GetNumberOfRenderers() - 1, view->GetPicker() );
}

void TripleCutPlaneObject::Release2DRepresentation( int viewType, View * view )
{
	// release the planes
    this->Planes[viewType]->RemoveInteractor( view->GetInteractor() );
    this->Planes[viewType]->RemoveRenderer( view->GetRenderer() );
}

void TripleCutPlaneObject::CreatePlane( int viewType )
{
    if( this->Planes[ viewType ] == 0 )
    {
        this->Planes[viewType] = vtkMultiImagePlaneWidget::New();
        this->Planes[viewType]->SetPlaneOrientation( viewType );
        this->Planes[viewType]->SetLeftButtonAction( vtkMultiImagePlaneWidget::NO_ACTION );
        this->Planes[viewType]->SetMiddleButtonAction( vtkMultiImagePlaneWidget::NO_ACTION );
        this->Planes[viewType]->SetRightButtonAction( vtkMultiImagePlaneWidget::NO_ACTION );
        this->Planes[viewType]->SetLeftButton2DAction( vtkMultiImagePlaneWidget::CURSOR_ACTION );
        this->Planes[viewType]->SetLeftButtonCtrlAction( vtkMultiImagePlaneWidget::SLICE_MOTION_ACTION );
        this->Planes[viewType]->SetRightButtonCtrlAction( vtkMultiImagePlaneWidget::NO_ACTION );
        this->Planes[viewType]->SetMiddleButtonCtrlAction( vtkMultiImagePlaneWidget::NO_ACTION );
        this->Planes[viewType]->UserControlledLookupTableOn();
        this->Planes[viewType]->DisableRotatingAndSpinningOn();
        this->Planes[viewType]->SetResliceInterpolate( m_resliceInterpolationType );
        this->Planes[viewType]->SetTextureInterpolate( m_displayInterpolationType );

        this->PlaneInteractionSlotConnect->Connect( this->Planes[viewType], vtkCommand::StartInteractionEvent, this, SLOT(PlaneStartInteractionEvent( vtkObject*, unsigned long) ) );
        this->PlaneInteractionSlotConnect->Connect( this->Planes[viewType], vtkCommand::InteractionEvent, this, SLOT(PlaneInteractionEvent( vtkObject*, unsigned long) ) );
        this->PlaneInteractionSlotConnect->Connect( this->Planes[viewType], vtkCommand::PlaceWidgetEvent, this, SLOT(PlaneInteractionEvent( vtkObject*, unsigned long) ) );
        this->PlaneEndInteractionSlotConnect->Connect( this->Planes[viewType], vtkCommand::EndInteractionEvent, this, SLOT(PlaneEndInteractionEvent( vtkObject*, unsigned long) ) );
    }
}

int TripleCutPlaneObject::GetPlaneIndex( vtkObject * caller )
{
    for( int i = 0; i < 3; ++i )
    {
        if( caller == this->Planes[i] )
        {
            return i;
        }
    }
    return -1;
}

void TripleCutPlaneObject::UpdateOtherPlanesPosition( int planeIndex )
{
    // Set other planes position according to the current cursor's position
    double cursorPos[3];
    this->Planes[ planeIndex ]->GetPosition( cursorPos );
    for( int i = 0; i < 3; ++i )
    {
        if( i != planeIndex )
        {
            this->Planes[i]->SetPosition( cursorPos );
        }
        emit PlaneMoved(i);
    }
}

