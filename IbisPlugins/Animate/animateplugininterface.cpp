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

#include "animateplugininterface.h"
#include "animatewidget.h"
#include "timelinewidget.h"
#include <QtPlugin>
#include "application.h"
#include "scenemanager.h"
#include "DomeRenderer.h"
#include "view.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "volumerenderingobject.h"
#include "transferfunctionkey.h"
#include "CameraAnimation.h"
#include "doublevalueanimation.h"
#include "vtkVolumeProperty.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include <QTimer>
#include <QTime>

void DomeRenderState::GetRenderSize( vtkRenderer * ren, int size[2] )
{
    DomeRenderer * dr = DomeRenderer::SafeDownCast( ren->GetDelegate() );
    Q_ASSERT(dr);
    size[0] = dr->GetCubeTextureSize();
    size[1] = size[0];
}


AnimatePluginInterface::AnimatePluginInterface()
{
    m_renderDome = false;
    m_domeRenderDelegate = DomeRenderer::New();
    m_domeRenderDelegate->UsedOn();
    m_renderState = new DomeRenderState;
    m_renderSize[0] = 1024;
    m_renderSize[1] = 1024;
    m_useMinCamDistance = false;
    m_minCamDistance = 0.1;
    m_currentFrame = 0;
    m_numberOfFrames = 100;
    m_timelineWidget = 0;
    m_cameraAnim = new CameraAnimation;
    m_tfAnim = new TFAnimation;
    m_minCamDistanceAnim = new DoubleValueAnimation;
    m_playbackTimer = new QTimer;
    connect( m_playbackTimer, SIGNAL(timeout()), this, SLOT(TimerCallback()) );
    m_playbackTime = new QTime;
    m_playbackInitialFrame = 0;
}

AnimatePluginInterface::~AnimatePluginInterface()
{
    m_domeRenderDelegate->Delete();
    delete m_renderState;
    delete m_cameraAnim;
    delete m_playbackTimer;
}

bool AnimatePluginInterface::CanRun()
{
    return true;
}

void AnimatePluginInterface::Serialize( Serializer * serializer )
{
    ::Serialize( serializer, "RenderSize", m_renderSize, 2 );
    ::Serialize( serializer, "CurrentFrame", m_currentFrame );
    ::Serialize( serializer, "NumberOfFrames", m_numberOfFrames );
    ::Serialize( serializer, "CameraAnimation", m_cameraAnim );
    ::Serialize( serializer, "TFAnimation", m_tfAnim );
    ::Serialize( serializer, "CamMinDistanceAnim", m_minCamDistanceAnim );
}

QWidget * AnimatePluginInterface::CreateTab()
{
    AnimateWidget * widget = new AnimateWidget;
    widget->SetPluginInterface( this );

    m_timelineWidget = new TimelineWidget;
    m_timelineWidget->SetPluginInterface( this );
    m_timelineWidget->setAttribute( Qt::WA_DeleteOnClose );
    GetApplication()->AddBottomWidget( m_timelineWidget );

    return widget;
}

bool AnimatePluginInterface::WidgetAboutToClose()
{
    if( GetRenderDome() )
        SetRenderDome( false );
    GetApplication()->RemoveBottomWidget( m_timelineWidget );
    m_timelineWidget = 0;
    return true;
}

VolumeRenderingObject * AnimatePluginInterface::GetVolumeRenderer()
{
    VolumeRenderingObject * vr = VolumeRenderingObject::SafeDownCast( GetApplication()->GetGlobalObjectInstance("VolumeRenderingObject") );
    return vr;
}

void AnimatePluginInterface::SetRenderDome( bool r )
{
    m_renderDome = r;
    if( r )
    {
        GetSceneManager()->GetMain3DView()->GetRenderer()->SetDelegate( m_domeRenderDelegate );
        GetVolumeRenderer()->SetRenderState( m_renderState );
    }
    else
    {
        GetSceneManager()->GetMain3DView()->GetRenderer()->SetDelegate( 0 );
        GetVolumeRenderer()->SetRenderState( 0 );
    }
    GetSceneManager()->GetMain3DView()->NotifyNeedRender();
}

bool AnimatePluginInterface::GetRenderDome()
{
    return m_renderDome;
}

void AnimatePluginInterface::SetDomeCubeTextureSize( int size )
{
    m_domeRenderDelegate->SetCubeTextureSize( size );
    GetSceneManager()->GetMain3DView()->NotifyNeedRender();
}

int AnimatePluginInterface::GetDomeCubeTextureSize()
{
    return m_domeRenderDelegate->GetCubeTextureSize();
}

double AnimatePluginInterface::GetCameraAngle()
{
    return GetSceneManager()->GetMain3DView()->GetViewAngle();
}

void AnimatePluginInterface::SetCameraAngle( double angle )
{
    GetSceneManager()->GetMain3DView()->SetViewAngle( angle );
}

double AnimatePluginInterface::GetDomeViewAngle()
{
    return m_domeRenderDelegate->GetDomeViewAngle();
}

void AnimatePluginInterface::SetDomeViewAngle( double angle )
{
    m_domeRenderDelegate->SetDomeViewAngle( angle );
    GetSceneManager()->GetMain3DView()->NotifyNeedRender();
}

void AnimatePluginInterface::SetRenderSize( int w, int h )
{
    m_renderSize[0] = w;
    m_renderSize[1] = h;
}

void AnimatePluginInterface::SetNumberOfFrames( int nbFrames )
{
    m_numberOfFrames = nbFrames;
    emit CurrentFrameChanged();
}

void AnimatePluginInterface::SetCurrentFrame( int f )
{
    m_currentFrame = f;

    // Adjust camera
    vtkCamera * cam = GetSceneManager()->GetMain3DView()->GetRenderer()->GetActiveCamera();
    m_cameraAnim->ComputeFrame( f, cam );

    // Adjust transfer function
    vtkVolumeProperty * prop = GetVolumeRenderer()->GetVolumeProperty( 0 );
    vtkColorTransferFunction * color = prop->GetRGBTransferFunction();
    vtkPiecewiseFunction * opacity = prop->GetScalarOpacity();
    TransferFunctionKey key;
    bool isFrame = m_tfAnim->ComputeFrame( f, key );
    if( isFrame )
    {
        color->DeepCopy( key.GetColorFunc() );
        opacity->DeepCopy( key.GetOpacityFunc() );
    }

    // Adjust camera min distance
    DoubleValueKey camDistKey;
    m_minCamDistanceAnim->ComputeFrame( m_currentFrame, camDistKey );
    this->SetMinCamDistance( camDistKey.GetValue() );

    // Broadcast the message
    GetSceneManager()->GetMain3DView()->NotifyNeedRender();
    emit CurrentFrameChanged();
}

void AnimatePluginInterface::SetUseMinCamDistance( bool use )
{
    m_useMinCamDistance = use;
    UpdateVolumeInitShader();
}

void AnimatePluginInterface::SetMinCamDistance( double dist )
{
    m_minCamDistance = dist;
    UpdateVolumeInitShader();
}

static const char * shaderInitName = "Camera Min Distance";
static const char * shaderInitCode = "float d = length( rayStart - cameraPosTSpace ); \
        if( d < @min-distance@ ) \
        { \
            curDist = @min-distance@ - d; \
        }";

void AnimatePluginInterface::UpdateVolumeInitShader()
{
    VolumeRenderingObject * vr = GetVolumeRenderer();
    if( m_useMinCamDistance )
    {
        if( !vr->DoesRayInitShaderExist(shaderInitName) )
        {
            vr->AddRayInitShaderType( shaderInitName, QString(""), false );
        }
        QString code( shaderInitCode );
        code.replace( QString("@min-distance@"), QString::number( m_minCamDistance ) );
        vr->SetRayInitShaderCode( code );
    }
    else
    {
        if( vr->DoesRayInitShaderExist( shaderInitName ) )
        {
            vr->SetRayInitShaderTypeByName( shaderInitName );
            vr->DeleteRayInitShaderType();
            vr->SetRayInitShaderTypeByName("None");
        }
    }
}

void AnimatePluginInterface::SetCameraKey( bool set )
{
    bool hasKey = HasCameraKey();
    Q_ASSERT( hasKey != set );

    vtkCamera * cam = GetSceneManager()->GetMain3DView()->GetRenderer()->GetActiveCamera();
    if( set )
        m_cameraAnim->AddKeyframe( m_currentFrame, cam );
    else
        m_cameraAnim->RemoveKey( m_currentFrame );

    emit KeyframesChanged();
}

bool AnimatePluginInterface::HasCameraKey()
{
    return m_cameraAnim->HasKey( m_currentFrame );
}

void AnimatePluginInterface::SetTFKey( bool set )
{
    bool hasKey = HasTFKey();
    Q_ASSERT( hasKey != set );

    if( set )
    {
        vtkVolumeProperty * prop = GetVolumeRenderer()->GetVolumeProperty( 0 );
        vtkColorTransferFunction * color = prop->GetRGBTransferFunction();
        vtkPiecewiseFunction * opacity = prop->GetScalarOpacity();
        TransferFunctionKey tfKey;
        tfKey.SetFunctions( color, opacity );
        tfKey.frame = m_currentFrame;
        m_tfAnim->AddKey( tfKey );
    }
    else
        m_tfAnim->RemoveKey( m_currentFrame );

    emit KeyframesChanged();
}

bool AnimatePluginInterface::HasTFKey()
{
    return m_tfAnim->HasKey( m_currentFrame );
}

void AnimatePluginInterface::SetCamMinDistKey( bool set )
{
    bool hasKey = HasCamMinDistKey();
    Q_ASSERT( hasKey != set );
    if( set )
    {
        DoubleValueKey key;
        key.SetValue( m_minCamDistance );
        key.frame = m_currentFrame;
        m_minCamDistanceAnim->AddKey( key );
    }
    else
        m_minCamDistanceAnim->RemoveKey( m_currentFrame );

    emit KeyframesChanged();
}

bool AnimatePluginInterface::HasCamMinDistKey()
{
    return m_minCamDistanceAnim->HasKey( m_currentFrame );
}

#include "offscreenrenderer.h"

// This is a hack that allows to tell the volume renderer that
// the size of the render surface is not that of the window
// because we render to a texture.
class OffscreenRenderState : public vtkIbisRenderState
{
public:
    OffscreenRenderState( int width, int height ) : m_width( width ), m_height( height ) {}
    virtual void GetRenderSize( vtkRenderer * ren, int size[2] ) { size[0] = m_width; size[1] = m_height; }
private:
    int m_width;
    int m_height;
};

void AnimatePluginInterface::RenderCurrentFrame()
{
    // Hack to tell volume renderer to render to a different size than the window
    OffscreenRenderState rs( m_renderSize[0], m_renderSize[1] );
    VolumeRenderingObject * vr = GetVolumeRenderer();
    if( !GetRenderDome() && vr )
        vr->SetRenderState( &rs );

    OffscreenRenderer ren( this );
    ren.SetRenderSize( m_renderSize[0], m_renderSize[1] );
    ren.RenderCurrent();

    if( !GetRenderDome() && vr )
        vr->SetRenderState( 0 );
}

void AnimatePluginInterface::RenderAnimation()
{
    // Hack to tell volume renderer to render to a different size than the window
    OffscreenRenderState rs( m_renderSize[0], m_renderSize[1] );
    VolumeRenderingObject * vr = GetVolumeRenderer();
    if( !GetRenderDome() && vr )
        vr->SetRenderState( &rs );

    OffscreenRenderer ren( this );
    ren.SetRenderSize( m_renderSize[0], m_renderSize[1] );
    ren.RenderAnimation();

    if( !GetRenderDome() && vr )
        vr->SetRenderState( 0 );
}

void AnimatePluginInterface::NextKeyframe()
{
    int nextCamKey = m_cameraAnim->NextKeyframe( m_currentFrame );
    int nextTFKey = m_tfAnim->NextKeyframe( m_currentFrame );
    int nextCamDistKey = m_minCamDistanceAnim->NextKeyframe( m_currentFrame );

    int nextKey = -1;
    if( nextCamKey != -1 )
        nextKey = nextCamKey;
    if( nextTFKey != -1 && nextTFKey < nextKey )
        nextKey = nextTFKey;
    if( nextCamDistKey != -1 && nextCamDistKey < nextKey )
        nextKey = nextCamDistKey;

    if( nextKey != -1 )
        SetCurrentFrame( nextKey );
}

void AnimatePluginInterface::PrevKeyframe()
{
    int prevCamKey = m_cameraAnim->PrevKeyframe( m_currentFrame );
    int prevTFKey = m_tfAnim->PrevKeyframe( m_currentFrame );
    int prevCamDistKey = m_minCamDistanceAnim->PrevKeyframe( m_currentFrame );

    int prevKey = -1;
    if( prevCamKey != -1 )
        prevKey = prevCamKey;
    if( prevTFKey != -1 && prevTFKey > prevKey )
        prevKey = prevTFKey;
    if( prevCamDistKey != -1 && prevCamDistKey > prevKey )
        prevKey = prevCamDistKey;

    if( prevKey != -1 )
        SetCurrentFrame( prevKey );
}

bool AnimatePluginInterface::IsPlaying()
{
    return m_playbackTimer->isActive();
}

void AnimatePluginInterface::SetPlaying( bool p )
{
    if( p )
    {
        m_playbackTimer->start( 0 );
        m_playbackTime->restart();
        m_playbackInitialFrame = m_currentFrame;
    }
    else
        m_playbackTimer->stop();
}

#include "quadviewwindow.h"

void AnimatePluginInterface::ToggleDetachWindow( bool checked )
{
    if( checked )
        GetApplication()->GetQuadViewWidget()->Detach3DView( GetApplication()->GetMainWindow() );
    else
        GetApplication()->GetQuadViewWidget()->Attach3DView();
}

//=============================
// For now:
// 0 - Camera anim
// 1 - TF anim
// 2 - Min dist anim

bool AnimatePluginInterface::HasKey( int paramIndex, int frame )
{
    if( paramIndex == 0 )
        return m_cameraAnim->HasKey( frame );
    else if( paramIndex == 1 )
        return m_tfAnim->HasKey( frame );
    else
        return m_minCamDistanceAnim->HasKey( frame );
    return false;
}

int AnimatePluginInterface::FindClosestKey( int paramIndex, int frame )
{
    if( paramIndex == 0 )
        return m_cameraAnim->FindClosestKey( frame );
    else if( paramIndex == 1 )
        return m_tfAnim->FindClosestKey( frame );
    else
        return m_minCamDistanceAnim->FindClosestKey( frame );
    return -1;
}

void AnimatePluginInterface::MoveKey( int paramIndex, int oldFrame, int newFrame )
{
    if( paramIndex == 0 )
        m_cameraAnim->MoveKey( oldFrame, newFrame );
    else if( paramIndex == 1 )
        m_tfAnim->MoveKey( oldFrame, newFrame );
    else
        m_minCamDistanceAnim->MoveKey( oldFrame, newFrame );
    SetCurrentFrame( m_currentFrame );
    emit KeyframesChanged();
}

void AnimatePluginInterface::RemoveKey( int paramIndex, int frame )
{
    if( paramIndex == 0 )
        m_cameraAnim->RemoveKey( frame );
    else if( paramIndex == 1 )
        m_tfAnim->RemoveKey( frame );
    else
        m_minCamDistanceAnim->RemoveKey( frame );
    emit KeyframesChanged();
}

int AnimatePluginInterface::GetNumberOfAnimatedParams()
{
    return 3;
}

QString AnimatePluginInterface::GetAnimatedParamName( int paramIndex )
{
    if( paramIndex == 0 )
        return QString("Camera");
    if( paramIndex == 1 )
        return QString("Transfer func");
    if( paramIndex == 2 )
        return QString("Min Cam Dist");
}

int AnimatePluginInterface::GetNumberOfKeys( int paramIndex )
{
    if( paramIndex == 0 )
        return m_cameraAnim->GetNumberOfKeys();
    if( paramIndex == 1 )
        return m_tfAnim->GetNumberOfKeys();
    if( paramIndex == 2 )
        return m_minCamDistanceAnim->GetNumberOfKeys();
}

int AnimatePluginInterface::GetFrameForKey( int paramIndex, int keyIndex )
{
    if( paramIndex == 0 )
        return m_cameraAnim->GetFrameForKey( keyIndex );
    if( paramIndex == 1 )
        return m_tfAnim->GetFrameForKey( keyIndex );
    if( paramIndex == 2 )
        return m_minCamDistanceAnim->GetFrameForKey( keyIndex );
}

int AnimatePluginInterface::GetKeyForFrame( int paramIndex, int frame )
{
    if( paramIndex == 0 )
        return m_cameraAnim->GetKeyForFrame( frame );
    if( paramIndex == 1 )
        return m_tfAnim->GetKeyForFrame( frame );
    if( paramIndex == 2 )
        return m_minCamDistanceAnim->GetKeyForFrame( frame );
}

void AnimatePluginInterface::TimerCallback()
{
    int ms = m_playbackTime->elapsed();
    int nbFrames = ms / 33.33;  // 30 fps
    int newFrame = ( m_playbackInitialFrame + nbFrames ) % m_numberOfFrames;
    SetCurrentFrame( newFrame );
}
