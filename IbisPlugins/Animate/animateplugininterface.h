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

#ifndef __AnimatePluginInterface_h_
#define __AnimatePluginInterface_h_

#include "toolplugininterface.h"
#include "vtkPRISMVolumeMapper.h"

class VolumeRenderingObject;
class DomeRenderer;
class TimelineWidget;
class CameraAnimation;
class TFAnimation;
class DoubleValueAnimation;
class vtkOpenGLRenderWindow;
class QTimer;
class QTime;

// This is a hack that allows to tell the volume renderer that
// the size of the render surface is not that of the window
// because we render to a texture.
class DomeRenderState : public vtkIbisRenderState
{
public:
    virtual ~DomeRenderState() {}
    virtual void GetRenderSize( vtkRenderer * ren, int size[2] );
};


class AnimatePluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.AnimatePluginInterface" )

public:

    AnimatePluginInterface();
    virtual ~AnimatePluginInterface();
    virtual QString GetPluginName() override { return QString("Animate"); }
    virtual bool CanRun() override;
    virtual QString GetMenuEntryString() override{ return QString("Animate"); }

    virtual void Serialize( Serializer * serializer ) override;

    virtual QWidget * CreateTab() override;
    virtual bool WidgetAboutToClose() override;

    VolumeRenderingObject * GetVolumeRenderer();

    void SetRenderDome( bool r );
    bool GetRenderDome();
    void SetDomeCubeTextureSize( int size );
    int GetDomeCubeTextureSize();

    double GetCameraAngle();
    void SetCameraAngle( double angle );

    double GetDomeViewAngle();
    void SetDomeViewAngle( double angle );

    void SetRenderSize( int w, int h );
    int * GetRenderSize() { return m_renderSize; }

    int GetNumberOfFrames() { return m_numberOfFrames; }
    void SetNumberOfFrames( int nbFrames );

    int GetCurrentFrame() { return m_currentFrame; }
    void SetCurrentFrame( int f );

    bool IsUsingMinCamDistance() { return m_useMinCamDistance; }
    void SetUseMinCamDistance( bool use );
    double GetMinCamDistance() { return m_minCamDistance; }
    void SetMinCamDistance( double dist );

    void SetCameraKey( bool set );
    bool HasCameraKey();

    void SetTFKey( bool set );
    bool HasTFKey();

    void SetCamMinDistKey( bool set );
    bool HasCamMinDistKey();

    void RenderCurrentFrame();
    void RenderAnimation();

    void NextKeyframe();
    void PrevKeyframe();

    bool IsPlaying();
    void SetPlaying( bool p );

    // Funcs to manipulate keyframes
    bool HasKey( int paramIndex, int frame );
    int FindClosestKey( int paramIndex, int frame );
    void MoveKey( int paramIndex, int keyIndex, int newFrame );
    void RemoveKey( int row, int keyIndex );
    int GetNumberOfAnimatedParams();
    QString GetAnimatedParamName( int paramIndex );
    int GetNumberOfKeys( int paramIndex );
    int GetFrameForKey( int paramIndex, int keyIndex );
    int GetKeyForFrame( int paramIndex, int frame );

public slots:

    void TimerCallback();

signals:

    void CurrentFrameChanged();
    void KeyframesChanged();

protected:

    void UpdateVolumeInitShader();

    bool m_renderDome;
    DomeRenderer * m_domeRenderDelegate;
    DomeRenderState * m_renderState;
    TimelineWidget * m_timelineWidget;

    int m_renderSize[2];

    int m_numberOfFrames;
    int m_currentFrame;

    bool m_useMinCamDistance;
    double m_minCamDistance;

    CameraAnimation * m_cameraAnim;
    TFAnimation * m_tfAnim;
    DoubleValueAnimation * m_minCamDistanceAnim;

    // playback
    QTimer * m_playbackTimer;
    QTime * m_playbackTime;
    int m_playbackInitialFrame;

};

#endif
