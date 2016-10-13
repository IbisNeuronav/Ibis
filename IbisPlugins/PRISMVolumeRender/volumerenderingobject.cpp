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

#include "volumerenderingobject.h"
#include "imageobject.h"
#include "scenemanager.h"
#include "view.h"
#include "volumerenderingobjectsettingswidget.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkPRISMVolumeMapper.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkTransform.h"
#include "vtkHandleWidget.h"
#include "vtkHandleRepresentation.h"
#include "vtkLineWidget2.h"
#include "vtkLineRepresentation.h"
#include "application.h"
#include <QTime>

ObjectSerializationMacro( VolumeRenderingObject );
ObjectSerializationMacro( VolumeRenderingObject::PerImage );

// ======== Builtin shader contrib code ===================
const char shaderContributionAdd[] = "vec4 sample = SampleVolumeWithTransferFunction( volIndex, pos ); \n\
fullSample += sample;";

const char shaderContributionAddSmooth[] = "vec4 sample = SampleVolumeSmoothWithTransferFunction( volIndex, pos, 0.002 ); \n\
fullSample += sample;";

const char shaderContributionMultiply[] = "vec4 sample = SampleVolumeWithTransferFunction( volIndex, pos ); \n\
fullSample *= sample;";

const char shaderContributionDiffuseShade[] = "// param initialization\n\
float gradStep = 0.01;\n\
vec3 lightPos = interactionPoint1TSpace;\n\
float diffuseK = 0.5;\n\
vec3 diffuseColor = vec3( 1.0, 1.0, 1.0 );\n\
\n\
// shading \n\
vec4 sample = SampleVolumeWithTransferFunction( volIndex, pos ); \n\
fullSample += sample; \n\
if( sample.a > 0.1 ) \n\
{ \n\
    vec3 n = ComputeGradient( volIndex, pos, gradStep ).rgb; \n\
    vec3 l = normalize( lightPos - pos ); \n\
    vec3 v = -1.0 * rayDir; \n\
    float dotNL = dot( n, l ); \n\
    vec3 diffuseContrib = clamp(dotNL, 0.0, 1.0 ) * diffuseColor * diffuseK; \n\
    fullSample.rgb += diffuseContrib; \n\
}";

const char shaderContributionDiffuseShadeFalloff[] = "vec4 sample = SampleVolumeWithTransferFunction( volIndex, pos ); \n\
fullSample += sample; \n\
\n\
if( sample.a > 0.1 ) \n\
{ \n\
    float lightDist = length( interactionPoint1TSpace - pos ); \n\
    float maxLightDist = 0.7; \n\
    if( lightDist < maxLightDist ) \n\
    { \n\
        float gradStep = 0.01; \n\
        vec4 n = ComputeGradient( volIndex, pos, gradStep ); \n\
        vec3 l = normalize( interactionPoint1TSpace - pos ); \n\
        vec4 diffuseContrib = dot( n.rgb, l ) * vec4( 1.0, 1.0, 1.0, 0.0 ) * 0.5; \n\
        diffuseContrib *= 1.0 - lightDist / maxLightDist; \n\
        fullSample += diffuseContrib; \n\
    } \n\
}";

const char shaderContributionAddGradientOpacity[] = "vec4 g = ComputeGradient( volIndex, pos, 0.004 ); \n\
        float alp = texture1D( transferFunctions[volIndex], g.a ).a; \n\
        vec4 col = SampleVolumeWithTransferFunction( volIndex, pos ); \n\
        col.a = alp; \n\
        fullSample += col;";

const char shaderContributionNone[] = "";

// ======== Builtin Init Ray shader contrib code =============

const char initShaderContribClipFront[] = "vec3 dp = interactionPoint1TSpace - rayStart; \n\
float dplane = dot( dp, rayDir ); \n\
if( dplane > 0 ) \n\
{ \n\
    // make ray integration start there \n\
    curDist = dplane; \n\
    \n\
    // compute color contribution on this plane \n\
    vec3 pos = rayStart + rayDir * curDist; \n\
    vec4 sample = SampleVolumeWithTransferFunction( 0, pos ); \n\
    if( sample.a > 0.1 ) \n\
    { \n\
        vec3 n = -1.0 * rayDir; \n\
        vec3 l = normalize( interactionPoint2TSpace - pos ); \n\
        vec4 diffuseContrib = dot( n, l ) * vec4( 1.0, 1.0, 1.0, 0.0 ) * 0.2; \n\
        sample += diffuseContrib; \n\
    } \n\
    sample *= vec4( 0.0, 1.0, 0.0, 1.0 ); \n\
    colorAccumulator += sample; \n\
}";

const char initShaderContribClipBack[] = "vec3 dp = interactionPoint1TSpace - rayStart; \n\
float dplane = dot( dp, rayDir ); \n\
if( dplane > 0.0 ) \n\
{	\n\
    stopDist = dplane; \n\
} \n\
else \n\
    stopDist = 0.0;";

const char initShaderContribNone[] = "";



VolumeRenderingObject::PerImage::PerImage()
{
    lastImageObjectId = InvalidObjectId;
    image = 0;
    imageCast = 0;

    // default volume properties
    volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetInterpolationTypeToLinear();
    vtkPiecewiseFunction * scalarOpacity = vtkPiecewiseFunction::New();
    scalarOpacity->AddPoint( 0.0, 0.0 );
    scalarOpacity->AddPoint( 255.0, 1.0 );
    volumeProperty->SetScalarOpacity( scalarOpacity );
    scalarOpacity->Delete();
    vtkColorTransferFunction * transferFunction = vtkColorTransferFunction::New();
    transferFunction->AddRGBPoint( 0.0 * 255, 0.0, 0.0, 0.0 );
    transferFunction->AddRGBPoint( 1.0 * 255, 1.0, 1.0, 1.0 );
    volumeProperty->SetColor( transferFunction );
    transferFunction->Delete();

    volumeEnabled = true;
    volumeIs16Bits = false;
    linearSampling = true;
    shaderContributionType = 0;
    shaderContributionTypeName = "Add";
}

VolumeRenderingObject::PerImage::~PerImage()
{
    volumeProperty->Delete();
}

#include "serializerhelper.h"

void VolumeRenderingObject::PerImage::Serialize( Serializer * ser )
{
    ::Serialize( ser, "VolumeEnabled", volumeEnabled );
    ::Serialize( ser, "VolumeIs16Bits", volumeIs16Bits );
    ::Serialize( ser, "LinearSampling", linearSampling );
    ::Serialize( ser, "ShaderContributionTypeName", shaderContributionTypeName );
    if( !ser->IsReader() )
    {
        if( image )
            lastImageObjectId = image->GetObjectID();
        else
            lastImageObjectId = InvalidObjectId;
    }
    ::Serialize( ser, "LastImageObjectId", lastImageObjectId );
    ::Serialize( ser, "ColorTransferFunction", volumeProperty->GetRGBTransferFunction() );
    ::Serialize( ser, "OpacityTransferFunction", volumeProperty->GetScalarOpacity() );
}

VolumeRenderingObject::VolumeRenderingObject()
{
    m_isAnimating = false;
    m_multFactor = 1.0;
    m_samplingDistance = 1.0;
    m_time = new QTime;
    m_timerId = -1;
    m_transferFunctionModifiedCallback = vtkEventQtSlotConnect::New();

    m_showInteractionWidget = false;
    m_pointerTracksInteractionPoints = false;
    m_interactionWidgetLine = false;
    m_interactionWidgetModifiedCallback = vtkEventQtSlotConnect::New();
    m_interactionPoint1[0] = 0.0;
    m_interactionPoint1[1] = 0.0;
    m_interactionPoint1[2] = 0.0;
    m_interactionPoint2[0] = 200.0;
    m_interactionPoint2[1] = 0.0;
    m_interactionPoint2[2] = 0.0;
    m_pickPos = false;
    m_pickValue = 0.5;

    // Add shader contrib types
    ShaderContrib contribAdd;
    contribAdd.name = "Add";
    contribAdd.code = shaderContributionAdd;
    m_shaderContribs.push_back( contribAdd );

    ShaderContrib contribAddSmooth;
    contribAddSmooth.name = "Add Smooth";
    contribAddSmooth.code = shaderContributionAddSmooth;
    m_shaderContribs.push_back( contribAddSmooth );

    ShaderContrib contribMult;
    contribMult.name = "Multiply";
    contribMult.code = shaderContributionMultiply;
    m_shaderContribs.push_back( contribMult );

    ShaderContrib contribDiffuseShade;
    contribDiffuseShade.name = "Diffuse Shade";
    contribDiffuseShade.code = shaderContributionDiffuseShade;
    m_shaderContribs.push_back( contribDiffuseShade );

    ShaderContrib contribDiffuseShadeFalloff;
    contribDiffuseShadeFalloff.name = "Diffuse Shade Falloff";
    contribDiffuseShadeFalloff.code = shaderContributionDiffuseShadeFalloff;
    m_shaderContribs.push_back( contribDiffuseShadeFalloff );

    ShaderContrib contribAddGradientOpacity;
    contribAddGradientOpacity.name = "Add Gradient Opacity";
    contribAddGradientOpacity.code = shaderContributionAddGradientOpacity;
    m_shaderContribs.push_back( contribAddGradientOpacity );

    ShaderContrib contribNone;
    contribNone.name = "None";
    contribNone.code = shaderContributionNone;
    m_shaderContribs.push_back( contribNone );

    // Register Init ray shader types
    ShaderContrib initContribClipFront;
    initContribClipFront.name = "Clip Front";
    initContribClipFront.code = initShaderContribClipFront;
    m_initShaderContribs.push_back( initContribClipFront );

    ShaderContrib initContribClipBack;
    initContribClipBack.name = "Clip Back";
    initContribClipBack.code = initShaderContribClipBack;
    m_initShaderContribs.push_back( initContribClipBack );

    ShaderContrib initContribNone;
    initContribNone.name = "None";
    initContribNone.code = initShaderContribNone;
    m_initShaderContribs.push_back( initContribNone );

    m_initShaderContributionType = 2;

    // Make one slot
    this->AddImageSlot();
}

VolumeRenderingObject::~VolumeRenderingObject()
{
    Clear();

    delete m_time;
    if( m_timerId != -1 )
        killTimer( m_timerId );

    m_interactionWidgetModifiedCallback->Delete();
    m_transferFunctionModifiedCallback->Delete();
}

void VolumeRenderingObject::Serialize( Serializer * ser )
{
    // parent class
    SceneObject::Serialize( ser );

    if( ser->IsReader() )
        Clear();

    ser->Serialize( "IsAnimating", m_isAnimating );
    ser->Serialize( "SamplingDistance", m_samplingDistance );
    ser->Serialize( "ShowInteractionWidget", m_showInteractionWidget );
    ser->Serialize( "InteractionWidgetLine", m_interactionWidgetLine );
    ser->Serialize( "InteractionPoint1", m_interactionPoint1, 3 );
    ser->Serialize( "InteractionPoint2", m_interactionPoint2, 3 );
    QString rayInitShaderTypeName = GetRayInitShaderName();
    ser->Serialize( "RayInitShaderTypeName", rayInitShaderTypeName );
    ::Serialize( ser, "ImageSlots", m_perImage );
    if( ser->IsReader() )
    {
        SetRayInitShaderTypeByName( rayInitShaderTypeName );
        UpdateShaderContributionTypeIndices();
        UpdateInteractionPointPos();
        UpdateInteractionWidgetVisibility();
    }
}

void VolumeRenderingObject::Clear()
{
    while( GetNumberOfImageSlots() > 0 )
        RemoveImageSlot();
}

void VolumeRenderingObject::ObjectAddedToScene()
{
    Q_ASSERT( GetManager() );
    connect( GetManager(), SIGNAL(ObjectAdded(int)), this, SLOT(ObjectAddedSlot(int)) );
    connect( GetManager(), SIGNAL(ObjectRemoved(int)), this, SLOT(ObjectRemovedSlot(int)) );
}

void VolumeRenderingObject::ObjectRemovedFromScene()
{
    Q_ASSERT( GetManager() );
    disconnect( GetManager(), SIGNAL(ObjectAdded(int)), this, SLOT(ObjectAddedSlot(int)) );
    disconnect( GetManager(), SIGNAL(ObjectRemoved(int)), this, SLOT(ObjectRemovedSlot(int)) );
}

#include "vtkProperty.h"

void VolumeRenderingObject::Setup( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        // make sure we haven't setup already
        PerViewContainer::iterator it = m_perView.find( view );
        if( it != m_perView.end() )
            return;

        PerView perView;
        perView.volumeMapper = vtkPRISMVolumeMapper::New();

        perView.volumeActor = vtkVolume::New();
        perView.volumeActor->SetMapper( perView.volumeMapper );
        perView.volumeActor->SetUserTransform( this->WorldTransform );
        UpdateMapper( perView );

        perView.sphereWidget = vtkHandleWidget::New();
        perView.sphereWidget->SetInteractor( view->GetInteractor() );
        perView.sphereWidget->AllowHandleResizeOff();
        perView.sphereWidget->CreateDefaultRepresentation();
        m_interactionWidgetModifiedCallback->Connect( perView.sphereWidget, vtkCommand::InteractionEvent, this, SLOT(InteractionWidgetMoved(vtkObject*)) );

        perView.lineWidget = vtkLineWidget2::New();
        perView.lineWidget->SetInteractor( view->GetInteractor() );
        vtkLineRepresentation * lineRep = vtkLineRepresentation::New();
        lineRep->SetPoint1WorldPosition( m_interactionPoint1 );
        lineRep->SetPoint2WorldPosition( m_interactionPoint2 );
        perView.lineWidget->SetRepresentation( lineRep );
        lineRep->Delete();
        m_interactionWidgetModifiedCallback->Connect( perView.lineWidget, vtkCommand::InteractionEvent, this, SLOT(InteractionWidgetMoved(vtkObject*)) );

        if( this->IsHidden() )
        {
            perView.volumeActor->VisibilityOff();
        }
        else
        {
            perView.volumeActor->VisibilityOn();
        }

        view->GetRenderer()->AddViewProp( perView.volumeActor );

        connect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );

        // register to receive mouse interaction event from this view
        view->AddInteractionObject( this, 0.5 );

        m_perView[ view ] = perView;
    }
}

void VolumeRenderingObject::PreDisplaySetup()
{
    SceneObject::PreDisplaySetup();
    UpdateInteractionWidgetVisibility();
}

void VolumeRenderingObject::Release( View * view )
{
    // Make sure the view has been setup
    PerViewContainer::iterator it = m_perView.find( view );
    if( it == m_perView.end() )
        return;

    PerView & perView = (*it).second;
    view->GetRenderer()->RemoveViewProp( perView.volumeActor );
    perView.volumeActor->Delete();
    perView.volumeMapper->Delete();
    perView.sphereWidget->EnabledOff();
    perView.sphereWidget->SetInteractor( 0 );
    perView.sphereWidget->Delete();
    m_perView.erase( view );
}

QWidget * VolumeRenderingObject::CreateSettingsDialog( QWidget * parent )
{
    VolumeRenderingObjectSettingsWidget * widget = new VolumeRenderingObjectSettingsWidget( parent );
    widget->SetVolumeRenderingObject( this );
    return widget;
}

void VolumeRenderingObject::Hide()
{
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeActor->VisibilityOff();
        perView.sphereWidget->EnabledOff();
        perView.sphereWidget->Off();
        ++itView;
    }
    UpdateInteractionWidgetVisibility();
    emit Modified();
}

void VolumeRenderingObject::Show()
{
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeActor->VisibilityOn();
        perView.sphereWidget->EnabledOn();
        perView.sphereWidget->On();
        ++itView;
    }
    UpdateInteractionWidgetVisibility();
    emit Modified();
}

#include "vtkVolumePicker.h"
#include "vtkPoints.h"

bool VolumeRenderingObject::Pick( View * v, int x, int y, double pickedPos[3] )
{
    vtkVolumePicker * volPicker = vtkVolumePicker::New();
    volPicker->SetVolumeOpacityIsovalue( m_pickValue );
    volPicker->PickFromListOn();
    PerViewContainer::iterator it = m_perView.find( v );
    Q_ASSERT( it != m_perView.end() );
    volPicker->AddPickList( (*it).second.volumeActor );
    bool didPick = volPicker->Pick( x, y, 0, v->GetRenderer() );
    if( didPick )
    {
        vtkPoints * pickedPoints = volPicker->GetPickedPositions();
        double * p = pickedPoints->GetPoint( 0 );
        pickedPos[0] = p[0];
        pickedPos[1] = p[1];
        pickedPos[2] = p[2];
    }
    volPicker->Delete();
    return didPick;
}

bool VolumeRenderingObject::OnLeftButtonPressed( View * v, int x, int y, unsigned modifiers )
{
    if( !m_pickPos )
        return false;

    bool swallow = false;
    if( modifiers && ShiftModifier > 0 )
    {
        double pickedPos[3];
        if( Pick( v, x, y, pickedPos ) )
        {
            SetInteractionPoint1( pickedPos[0], pickedPos[1], pickedPos[2] );
            UpdateInteractionWidgetVisibility();
            swallow = true;
        }
    }
    return swallow;
}

void VolumeRenderingObject::SetPickPos( bool set )
{
    m_pickPos = set;
}

void VolumeRenderingObject::SetPickValue( double val )
{
    m_pickValue = val;
}

ImageObject * VolumeRenderingObject::GetImage( int index )
{
    Q_ASSERT_X( m_perImage.size() > (unsigned)index, "VolumeRenderingObject::GetImage()", "Image index out of bounds" );

    return m_perImage[ index ]->image;
}

#include "vtkMatrix4x4.h"

void VolumeRenderingObject::SetImage( int index, ImageObject * im )
{
    Q_ASSERT_X( m_perImage.size() > (unsigned)index, "VolumeRenderingObject::SetImage()", "Image index out of bounds" );

    PerImage * pi = m_perImage[ index ];

    // Clear old image if there was one
    if( pi->image )
    {
        pi->imageCast->Delete();
        pi->imageCast = 0;
        pi->image->UnRegister( this );
    }

    // Setup new image
    pi->image = im;
    if( pi->image )
    {
        pi->lastImageObjectId = im->GetObjectID();
        pi->imageCast = vtkImageShiftScale::New();
        pi->imageCast->ClampOverflowOn();
        pi->imageCast->SetInputData( pi->image->GetImage() );
        UpdateShiftScale( index );
        pi->image->Register( this );
    }
    else
        pi->lastImageObjectId = SceneObject::InvalidObjectId;

    // Grab transform if it is first volume
    if( index == 0 )
    {
        if( im )
            this->SetLocalTransform( im->GetWorldTransform() );
        else
        {
            this->SetLocalTransform( 0 );
        }
    }

    // Update all mappers with new situation
    UpdateAllMappers();

    emit Modified();
}

int VolumeRenderingObject::GetNumberOfImageSlots()
{
    return m_perImage.size();
}

void VolumeRenderingObject::AddImageSlot()
{
    PerImage * pi = new PerImage;
    ConnectImageSlot( pi );
    m_perImage.push_back( pi );
    emit VolumeSlotModified();
}

void VolumeRenderingObject::RemoveImageSlot()
{
    Q_ASSERT( m_perImage.size() > 0 );
    SetImage( m_perImage.size() - 1, 0 );
    PerImage * pi = m_perImage.back();
    delete pi;
    m_perImage.pop_back();
    emit VolumeSlotModified();
}

bool VolumeRenderingObject::IsVolumeEnabled( int index )
{
    Q_ASSERT( (unsigned)index < m_perImage.size() );
    PerImage * pi = m_perImage[index];
    return pi->volumeEnabled;
}

void VolumeRenderingObject::EnableVolume( int index, bool enable )
{
    Q_ASSERT( (unsigned)index < m_perImage.size() );

    PerImage * pi = m_perImage[index];

    if( pi->volumeEnabled == enable )
        return;
    pi->volumeEnabled = enable;

    // Tell all mappers about the change
    if( pi->image )
    {
        PerViewContainer::iterator itView = m_perView.begin();
        while( itView != m_perView.end() )
        {
            PerView & perView = (*itView).second;
            perView.volumeMapper->EnableInput( index, enable );
            ++itView;
        }
        emit Modified();
    }
}

void VolumeRenderingObject::Animate( bool on )
{
    if( m_isAnimating == on )
        return;
    m_isAnimating = on;
    if( on )
    {
        m_time->restart();
        m_timerId = startTimer( 0 );
    }
    else
    {
        killTimer( m_timerId );
        m_timerId = -1;
    }
}

void VolumeRenderingObject::SetMultFactor( double val )
{
    m_multFactor = val;
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeMapper->SetMultFactor( m_multFactor );
        ++itView;
    }
    emit Modified();
}

void VolumeRenderingObject::SetSamplingDistance( double samplingDistance )
{
    Q_ASSERT( samplingDistance > 0.0 );
    m_samplingDistance = samplingDistance;
    UpdateShaderParams();
    emit Modified();
}

void VolumeRenderingObject::SetShowInteractionWidget( bool s )
{
    m_showInteractionWidget = s;
    UpdateInteractionWidgetVisibility();
}

void VolumeRenderingObject::SetPointerTracksInteractionPoints( bool t )
{
    Application * app = &Application::GetInstance();
    Q_ASSERT( app );

    if( m_pointerTracksInteractionPoints == t )
        return;
    m_pointerTracksInteractionPoints = t;
    if( m_pointerTracksInteractionPoints )
    {
        connect( app, SIGNAL(IbisClockTick()), this, SLOT(TrackingUpdatedSlot()) );
    }
    else
    {
        disconnect( app, SIGNAL(IbisClockTick()), this, SLOT(TrackingUpdatedSlot()) );
    }
}

void VolumeRenderingObject::SetInteractionWidgetAsLine( bool l )
{
    m_interactionWidgetLine = l;
    UpdateInteractionWidgetVisibility();
}

void VolumeRenderingObject::SetInteractionPoint1( double x, double y, double z )
{
    m_interactionPoint1[0] = x;
    m_interactionPoint1[1] = y;
    m_interactionPoint1[2] = z;
    UpdateInteractionPointPos();
}

void VolumeRenderingObject::GetInteractionPoint1( double & x, double & y, double & z )
{
    x = m_interactionPoint1[0];
    y = m_interactionPoint1[1];
    z = m_interactionPoint1[2];
}

void VolumeRenderingObject::SetInteractionPoint2( double x, double y, double z )
{
    m_interactionPoint2[0] = x;
    m_interactionPoint2[1] = y;
    m_interactionPoint2[2] = z;
    UpdateInteractionPointPos();
}

void VolumeRenderingObject::GetInteractionPoint2( double & x, double & y, double & z )
{
    x = m_interactionPoint2[0];
    y = m_interactionPoint2[1];
    z = m_interactionPoint2[2];
}

int VolumeRenderingObject::GetNumberOfRayInitShaderTypes()
{
    return m_initShaderContribs.size();
}

QString VolumeRenderingObject::GetRayInitShaderTypeName( int index )
{
    Q_ASSERT( index < GetNumberOfRayInitShaderTypes() && index >= 0 );
    return m_initShaderContribs[ index ].name;
}

bool VolumeRenderingObject::IsRayInitShaderTypeCustom( int index )
{
    Q_ASSERT( index < GetNumberOfRayInitShaderTypes() && index >= 0 );
    return m_initShaderContribs[ index ].custom;
}

bool VolumeRenderingObject::DoesRayInitShaderExist( QString name )
{
    for( int i = 0; i < GetNumberOfRayInitShaderTypes(); ++i )
    {
        if( m_initShaderContribs[i].name == name )
        {
            return true;
        }
    }
    return false;
}

void VolumeRenderingObject::SetRayInitShaderType( int typeIndex )
{
    Q_ASSERT( typeIndex < GetNumberOfRayInitShaderTypes() && typeIndex >= 0 );
    m_initShaderContributionType = typeIndex;
    UpdateAllMappers();
    emit Modified();
}

void VolumeRenderingObject::SetRayInitShaderTypeByName( QString name )
{
    int index = -1;
    for( int i = 0; i < GetNumberOfRayInitShaderTypes(); ++i )
    {
        if( m_initShaderContribs[i].name == name )
        {
            index = i;
            break;
        }
    }

    if( index != -1 )
        SetRayInitShaderType( index );
}

int VolumeRenderingObject::GetRayInitShaderType()
{
    return m_initShaderContributionType;
}

QString VolumeRenderingObject::GetRayInitShaderName()
{
    Q_ASSERT( m_initShaderContributionType >= 0 && m_initShaderContributionType < m_initShaderContribs.size() );
    return m_initShaderContribs[ m_initShaderContributionType ].name;
}

void VolumeRenderingObject::AddRayInitShaderType( QString name, QString code, bool custom )
{
    ShaderContrib newContrib;
    newContrib.name = name;
    newContrib.code = code;
    newContrib.custom = custom;
    m_initShaderContribs.push_back( newContrib );
    m_initShaderContributionType = m_initShaderContribs.size() - 1;
    emit Modified();
}

void VolumeRenderingObject::DuplicateRayInitShaderType()
{
    ShaderContrib newContrib = m_initShaderContribs[ m_initShaderContributionType ];

    // Find unique name
    QStringList allNames;
    for( int i = 0; i < m_initShaderContribs.size(); ++i )
        allNames.push_back( m_initShaderContribs[i].name );
    newContrib.name = SceneManager::FindUniqueName( newContrib.name, allNames );

    newContrib.custom = true;
    m_initShaderContribs.push_back( newContrib );
    m_initShaderContributionType = m_initShaderContribs.size() - 1;
    emit Modified();
}

void VolumeRenderingObject::DeleteRayInitShaderType()
{
    //Q_ASSERT( m_initShaderContribs[ m_initShaderContributionType ].custom == true );
    m_initShaderContribs.removeAt( m_initShaderContributionType );
    int newInitShaderType = m_initShaderContributionType;
    if( newInitShaderType > 0 )
        --newInitShaderType;
    SetRayInitShaderType( newInitShaderType );
}

QString VolumeRenderingObject::GetRayInitShaderCode()
{
    Q_ASSERT( m_initShaderContributionType >= 0 && m_initShaderContributionType < m_initShaderContribs.size() );
    return m_initShaderContribs[ m_initShaderContributionType ].code;
}

void VolumeRenderingObject::SetRayInitShaderCode( QString code )
{
    //Q_ASSERT( m_initShaderContribs[ m_initShaderContributionType ].custom == true );
    Q_ASSERT( m_initShaderContributionType >= 0 && m_initShaderContributionType < m_initShaderContribs.size() );
    m_initShaderContribs[ m_initShaderContributionType ].code = code;
    UpdateAllMappers();
    emit Modified();
}

int VolumeRenderingObject::GetNumberOfShaderContributionTypes()
{
    return m_shaderContribs.size();
}

int VolumeRenderingObject::GetShaderContributionTypeIndex( QString shaderName )
{
    for( int i = 0; i < m_shaderContribs.size(); ++i )
    {
        if( m_shaderContribs[i].name == shaderName )
            return i;
    }
    return -1;
}

QString VolumeRenderingObject::GetShaderContributionTypeName( int typeIndex )
{
    Q_ASSERT( typeIndex < GetNumberOfShaderContributionTypes() );
    return m_shaderContribs[typeIndex].name;
}

bool VolumeRenderingObject::DoesShaderContributionTypeExist( QString name )
{
    for( int i = 0; i < GetNumberOfShaderContributionTypes(); ++i )
    {
        if( m_shaderContribs[i].name == name )
        {
            return true;
        }
    }
    return false;
}

void VolumeRenderingObject::AddShaderContributionType( QString shaderName, QString code, bool custom )
{
    ShaderContrib contrib;
    contrib.custom = custom;
    contrib.code = code;
    contrib.name = shaderName;
    m_shaderContribs.push_back( contrib );
    emit Modified();
}

void VolumeRenderingObject::DeleteShaderContributionType( QString shaderName )
{
    // Set contribution type of volume using this shader to 0
    int shaderType = GetShaderContributionTypeIndex( shaderName );
    for( int i = 0; i < m_perImage.size(); ++i )
    {
        PerImage * pi = m_perImage[i];
        if( pi->shaderContributionType == shaderType )
            SetShaderContributionType( i, 0 );
    }

    m_shaderContribs.removeAt( shaderType );

    emit Modified();
}

QString VolumeRenderingObject::GetUniqueCustomShaderName( QString name )
{
    QStringList allNames;
    for( int i = 0; i < m_shaderContribs.size(); ++i )
        allNames.push_back( m_shaderContribs[i].name );
    return SceneManager::FindUniqueName( name, allNames );
}

void VolumeRenderingObject::DuplicateShaderContribType( int typeIndex )
{
    Q_ASSERT( typeIndex < m_shaderContribs.size() );

    ShaderContrib contrib = m_shaderContribs[ typeIndex ];
    contrib.custom = true;

    // Make sure we have a unique name
    contrib.name = GetUniqueCustomShaderName( contrib.name );

    m_shaderContribs.push_back( contrib );

    emit Modified();
}

void VolumeRenderingObject::SetShaderContributionType( int volumeIndex, int typeIndex )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    Q_ASSERT( typeIndex < GetNumberOfShaderContributionTypes() );
    m_perImage[ volumeIndex ]->shaderContributionType = typeIndex;
    m_perImage[ volumeIndex ]->shaderContributionTypeName = m_shaderContribs[ typeIndex ].name;
    UpdateAllMappers();
    emit Modified();
}

void VolumeRenderingObject::SetShaderContributionTypeByName( int volumeIndex, QString name )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    for( int i = 0; i < GetNumberOfShaderContributionTypes(); ++i )
    {
        if( m_shaderContribs[ i ].name == name )
        {
            SetShaderContributionType( volumeIndex, i );
            break;
        }
    }
}

int VolumeRenderingObject::GetShaderContributionType( int volumeIndex )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    return m_perImage[ volumeIndex ]->shaderContributionType;
}

bool VolumeRenderingObject::IsShaderTypeCustom( int typeIndex )
{
    Q_ASSERT( typeIndex < m_shaderContribs.size() );
    return m_shaderContribs[ typeIndex ].custom;
}

QString VolumeRenderingObject::GetCustomShaderContribution( int volumeIndex )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    int contributionType = m_perImage[ volumeIndex ]->shaderContributionType;
    Q_ASSERT( contributionType < m_shaderContribs.size() );
    return m_shaderContribs[ contributionType ].code;
}

void VolumeRenderingObject::SetCustomShaderCode( int volumeIndex, QString code )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    int contributionType = m_perImage[ volumeIndex ]->shaderContributionType;
    Q_ASSERT( contributionType < m_shaderContribs.size() );
    m_shaderContribs[ contributionType ].code = code;
    UpdateAllMappers();
    emit Modified();
}

void VolumeRenderingObject::SetUse16BitsVolume( int volumeIndex, bool use )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    m_perImage[ volumeIndex ]->volumeIs16Bits = use;
    UpdateShiftScale( volumeIndex );
    emit Modified();
}

bool VolumeRenderingObject::GetUse16BitsVolume( int volumeIndex )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    return m_perImage[ volumeIndex ]->volumeIs16Bits;
}

void VolumeRenderingObject::SetUseLinearSampling( int volumeIndex, bool isLinear )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    m_perImage[ volumeIndex ]->linearSampling = isLinear;
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeMapper->SetUseLinearSampling( volumeIndex, isLinear );
        ++itView;
    }
    emit Modified();
}

bool VolumeRenderingObject::GetUseLinearSampling( int volumeIndex )
{
    Q_ASSERT( (unsigned)volumeIndex < m_perImage.size() );
    return m_perImage[ volumeIndex ]->linearSampling;
}

vtkVolumeProperty * VolumeRenderingObject::GetVolumeProperty( int index )
{
    Q_ASSERT( (unsigned)index < m_perImage.size() );
    return m_perImage[index]->volumeProperty;
}

void VolumeRenderingObject::SetRenderState( vtkIbisRenderState * state )
{
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeMapper->SetRenderState( state );
        ++itView;
    }
}

void VolumeRenderingObject::TransferFunctionModifiedSlot()
{
    // Tell views to re-render
    emit Modified();
}

void VolumeRenderingObject::ObjectAddedSlot( int objectId )
{
    Q_ASSERT( this->GetManager() );
    ImageObject * im = ImageObject::SafeDownCast( this->GetManager()->GetObjectByID( objectId ) );

    if( im && !this->GetImage( 0 ) && !GetManager()->IsLoadingScene() )
    {
        this->SetImage( 0, im );
    }
}

void VolumeRenderingObject::ObjectRemovedSlot( int objectId )
{
    for( int i = 0; i < m_perImage.size(); ++i )
    {
        if( m_perImage[i]->lastImageObjectId == objectId )
            SetImage( i, 0 );
    }
}

#include "pointerobject.h"

void VolumeRenderingObject::TrackingUpdatedSlot()
{
    Q_ASSERT( this->GetManager() );
    PointerObject * navPointer = this->GetManager()->GetNavigationPointerObject();
    if( navPointer )
    {
        double * tip = navPointer->GetTipPosition();
        m_interactionPoint1[0] = tip[0];
        m_interactionPoint1[1] = tip[1];
        m_interactionPoint1[2] = tip[2];

        navPointer->GetMainAxisPosition( m_interactionPoint2 );

        UpdateInteractionPointPos();
        UpdateInteractionWidgetVisibility();
    }
}

void VolumeRenderingObject::InteractionWidgetMoved( vtkObject * caller )
{
    // Get the widget that was modified
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        if( perView.sphereWidget == vtkHandleWidget::SafeDownCast( caller ) )
            break;
        if( perView.lineWidget == vtkLineWidget2::SafeDownCast( caller ) )
            break;
        ++itView;
    }

    if( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        if( m_interactionWidgetLine )
        {
            perView.lineWidget->GetLineRepresentation()->GetPoint1WorldPosition( m_interactionPoint1 );
            perView.lineWidget->GetLineRepresentation()->GetPoint2WorldPosition( m_interactionPoint2 );
        }
        else
        {
            perView.sphereWidget->GetHandleRepresentation()->GetWorldPosition( m_interactionPoint1 );
        }
        UpdateInteractionPointPos();
    }
}

void VolumeRenderingObject::UpdateInteractionWidgetVisibility()
{
    int lineWidgetOn = 0;
    int sphereWidgetOn = 0;

    if( !IsHidden() && m_showInteractionWidget )
    {
        if( m_interactionWidgetLine )
            lineWidgetOn = 1;
        else
            sphereWidgetOn = 1;
    }

    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.lineWidget->SetEnabled( lineWidgetOn );
        perView.lineWidget->GetLineRepresentation()->SetPoint1WorldPosition( m_interactionPoint1 );
        perView.lineWidget->GetLineRepresentation()->SetPoint2WorldPosition( m_interactionPoint2 );
        perView.sphereWidget->SetEnabled( sphereWidgetOn );
        perView.sphereWidget->GetHandleRepresentation()->SetWorldPosition( m_interactionPoint1 );
        ++itView;
    }
    emit Modified();
}

void VolumeRenderingObject::UpdateInteractionPointPos()
{
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeMapper->SetInteractionPoint1(  m_interactionPoint1 );
        perView.volumeMapper->SetInteractionPoint2(  m_interactionPoint2 );
        ++itView;
    }
    emit Modified();
}

void VolumeRenderingObject::timerEvent( QTimerEvent * e )
{
    // Get Current time.
    float t = ((float)(m_time->elapsed())) / 1000.0;

    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeMapper->SetTime( t );
        ++itView;
    }
    emit Modified();
}

void VolumeRenderingObject::UpdateMapper( PerView & pv )
{
    // Set volume property of the first image in the volume actor (needed for picking)
    if( m_perImage.size() > 0 )
        pv.volumeActor->SetProperty( m_perImage[0]->volumeProperty );

    vtkPRISMVolumeMapper * mapper = pv.volumeMapper;
    mapper->ClearAllInputs();
    for( unsigned i = 0; i < m_perImage.size(); ++i )
    {
        PerImage * pi = m_perImage[i];
        if( pi->imageCast )
        {
            QString shaderContribution = m_shaderContribs[ pi->shaderContributionType ].code;
            mapper->AddInput( pi->imageCast->GetOutputPort(), pi->volumeProperty, shaderContribution.toUtf8().data() );
            mapper->EnableInput( i, pi->volumeEnabled );
            mapper->SetUseLinearSampling( i, pi->linearSampling );
        }
    }
    mapper->SetSampleDistance( m_samplingDistance );
    QString initShaderCode = m_initShaderContribs[ m_initShaderContributionType ].code;
    mapper->SetShaderInitCode( initShaderCode.toUtf8().data() );
}

void VolumeRenderingObject::UpdateAllMappers()
{
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        UpdateMapper( perView );
        ++itView;
    }
}

void VolumeRenderingObject::UpdateShiftScale( int index )
{
    Q_ASSERT( (unsigned)index < m_perImage.size() );
    PerImage * pi = m_perImage[ index ];

    double rangeMax = 255.0;
    if( pi->volumeIs16Bits )
    {
        rangeMax = 65535.0;
        pi->imageCast->SetOutputScalarTypeToUnsignedShort();
    }
    else
        pi->imageCast->SetOutputScalarTypeToUnsignedChar();

    double imageScalarRange[2];
    pi->image->GetImage()->GetScalarRange( imageScalarRange );
    pi->imageCast->SetShift( -imageScalarRange[0] );
    double scale = rangeMax / ( imageScalarRange[1] - imageScalarRange[0] );
    pi->imageCast->SetScale( scale );
}

void VolumeRenderingObject::UpdateShaderParams()
{
    PerViewContainer::iterator itView = m_perView.begin();
    while( itView != m_perView.end() )
    {
        PerView & perView = (*itView).second;
        perView.volumeMapper->SetSampleDistance( m_samplingDistance );
        ++itView;
    }
}

void VolumeRenderingObject::ConnectImageSlot( PerImage * pi )
{
    vtkPiecewiseFunction * scalarOpacity = pi->volumeProperty->GetScalarOpacity();
    m_transferFunctionModifiedCallback->Connect( scalarOpacity, vtkCommand::ModifiedEvent, this, SLOT(TransferFunctionModifiedSlot()) );
    vtkColorTransferFunction * transferFunction = pi->volumeProperty->GetRGBTransferFunction();
    m_transferFunctionModifiedCallback->Connect( transferFunction, vtkCommand::ModifiedEvent, this, SLOT(TransferFunctionModifiedSlot()) );
}

void VolumeRenderingObject::InternalPostSceneRead()
{
    Q_ASSERT( this->GetManager() );

    // At this point the scene objects are available, we can get them by Id.
    for( unsigned i = 0; i < m_perImage.size(); ++i )
    {
        PerImage * pi = m_perImage[i];
        ConnectImageSlot( pi );
        SceneObject * obj = GetManager()->GetObjectByID( pi->lastImageObjectId );
        if( obj )
        {
            ImageObject * image = ImageObject::SafeDownCast( obj );
            Q_ASSERT( image );
            this->SetImage( (int)i, image );
        }
    }
    emit Modified();
}

#include <QDir>

void VolumeRenderingObject::LoadCustomShaderContribs()
{
    QString configDir = Application::GetConfigDirectory();
    QString shaderContribDir = configDir + "VolumeRenderer/ShaderContribs/";
    if( QFile::exists( shaderContribDir ) )
    {
        // list all .glsl files
        QDir dir( shaderContribDir );
        QStringList filter;
        filter.push_back( QString("*.glsl") );
        QStringList allFiles = dir.entryList( filter );

        // for each file, read and create a shader contrib.
        for( int i = 0; i < allFiles.size(); ++i )
        {
            QString shaderFileName = shaderContribDir + allFiles[i];
            QFile shaderFile( shaderFileName );
            if( shaderFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
            {
                ShaderContrib contrib;
                contrib.custom = true;
                QFileInfo info( allFiles[i] );
                contrib.name = info.baseName();
                QTextStream shaderStream( &shaderFile );
                contrib.code = shaderStream.readAll();
                m_shaderContribs.push_back( contrib );
            }
            shaderFile.close();
        }
    }
}

void VolumeRenderingObject::SaveCustomShaderContribs()
{
    // Make sure dir exists
    QString configDir = Application::GetInstance().GetConfigDirectory();
    QString shaderContribDir = configDir + "VolumeRenderer/ShaderContribs/";
    if( !QFile::exists( shaderContribDir ) )
    {
        QDir dir;
        dir.mkpath( shaderContribDir );
    }

    for( int i = 0; i < m_shaderContribs.size(); ++i )
    {
        if( m_shaderContribs[i].custom )
        {
            QString filename = shaderContribDir + m_shaderContribs[i].name + ".glsl";
            QFile shaderFile( filename );
            if( shaderFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
            {
                QTextStream shaderStream( &shaderFile );
                shaderStream << m_shaderContribs[i].code;
            }
            shaderFile.close();
        }
    }
}

void VolumeRenderingObject::LoadCustomRayInitShaders()
{
    QString configDir = Application::GetConfigDirectory();
    QString shadersDir = configDir + "VolumeRenderer/CustomRayInitShaders/";
    if( QFile::exists( shadersDir ) )
    {
        // list all .glsl files
        QDir dir( shadersDir );
        QStringList filter;
        filter.push_back( QString("*.glsl") );
        QStringList allFiles = dir.entryList( filter );

        // for each file, read and create a shader contrib.
        for( int i = 0; i < allFiles.size(); ++i )
        {
            QString shaderFileName = shadersDir + allFiles[i];
            QFile shaderFile( shaderFileName );
            if( shaderFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
            {
                ShaderContrib contrib;
                contrib.custom = true;
                QFileInfo info( allFiles[i] );
                contrib.name = info.baseName();
                QTextStream shaderStream( &shaderFile );
                contrib.code = shaderStream.readAll();
                m_initShaderContribs.push_back( contrib );
            }
            shaderFile.close();
        }
    }
}

void VolumeRenderingObject::SaveCustomRayInitShaders()
{
    // Make sure dir exists
    QString configDir = Application::GetInstance().GetConfigDirectory();
    QString shadersDir = configDir + "VolumeRenderer/CustomRayInitShaders/";
    if( !QFile::exists( shadersDir ) )
    {
        QDir dir;
        dir.mkpath( shadersDir );
    }

    for( int i = 0; i < m_initShaderContribs.size(); ++i )
    {
        if( m_initShaderContribs[i].custom )
        {
            QString filename = shadersDir + m_initShaderContribs[i].name + ".glsl";
            QFile shaderFile( filename );
            if( shaderFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
            {
                QTextStream shaderStream( &shaderFile );
                shaderStream << m_initShaderContribs[i].code;
            }
            shaderFile.close();
        }
    }
}

void VolumeRenderingObject::UpdateShaderContributionTypeIndices()
{
    // update indexes according to names
    for( int i = 0; i < m_perImage.size(); ++i )
    {
        QString name = m_perImage[i]->shaderContributionTypeName;
        for( int t = 0; t < m_shaderContribs.size(); ++t )
        {
            if( m_shaderContribs[t].name == name )
            {
                m_perImage[i]->shaderContributionType = t;
                break;
            }
        }
    }
}
