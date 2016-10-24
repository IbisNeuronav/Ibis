/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef USACQUISITIONPLUGININTERFACE_H
#define USACQUISITIONPLUGININTERFACE_H

#include <QObject>
#include <QWidget>
#include "toolplugininterface.h"
#include "serializer.h"

class USAcquisitionObject;
class UsProbeObject;
class DoubleViewWidget;
class ImageObject;

class USAcquisitionPluginInterface : public QObject, public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.USAcquisitionPluginInterface" )

public:

    vtkTypeMacro( USAcquisitionPluginInterface, ToolPluginInterface );

    USAcquisitionPluginInterface();
    ~USAcquisitionPluginInterface();
    QString GetMenuEntryString() { return QString("US Acquisition Double View"); }
    virtual QString GetPluginName() { return QString("USAcquisitionDoubleView"); }
    bool CanRun();
    QWidget * CreateFloatingWidget();
    virtual bool WidgetAboutToClose();

    UsProbeObject * GetCurrentUsProbe();
    USAcquisitionObject * GetCurrentAcquisition();
    void NewAcquisition();
    ImageObject * GetCurrentVolume();
    ImageObject * GetAddedVolume();

    bool CanCaptureTrackedVideo();
    bool IsTrackerlessCaptureAllowed() { return m_allowTrackerlessCapture; }
    bool IsLive() { return m_isLive; }
    void SetLive( bool l );

    bool IsBlending() { return m_isBlending; }
    void SetBlending( bool b ) { m_isBlending = b; }
    double GetBlendingPercent() { return m_blendingPercent; }
    void SetBlendingPercent( double p ) { m_blendingPercent = p; }
    bool IsMasking() { return m_isMasking; }
    void SetMasking( bool m ) { m_isMasking = m; }
    double GetMaskingPercent() { return m_maskingPercent; }
    void SetMaskingPercent( double p ) { m_maskingPercent = p; }

    int GetCurrentAcquisitionObjectId() { return m_currentAcquisitionObjectId; }
    void SetCurrentAcquisitionObjectId( int id );

    int GetCurrentVolumeObjectId() { return m_currentVolumeObjectId; }
    void SetCurrentVolumeObjectId( int id );

    bool IsBlendingVolumes() { return m_isBlendingVolumes; }
    void SetBlendingVolumes( bool b ) { m_isBlendingVolumes = b; }

    double GetBlendingVolumesPercent() { return m_blendingVolumesPercent; }
    void SetBlendingVolumesPercent( double p ) { m_blendingVolumesPercent = p; }

    int GetAddedVolumeObjectId() { return m_addedVolumeObjectId; }
    void SetAddedVolumeObjectId( int id );

signals:

    void ObjectsChanged();
    void ImageChanged();

private slots:

    void SceneContentChanged();
    void LutChanged();
    void OnImageChanged();

protected:

    void ValidateAllSceneObjects();
    void ValidateCurrentUsProbe();
    void ValidateCurrentAcquisition();
    void ValidateCurrentVolume();
    void ValidateAddedVolume();

    DoubleViewWidget * m_interfaceWidget;

    const bool m_allowTrackerlessCapture;  // this is for debugging without tracking, not for regular use
    bool m_isLive;
    bool m_isBlending;
    double m_blendingPercent;
    bool m_isMasking;
    double m_maskingPercent;
    int m_currentProbeObjectId;
    int m_currentAcquisitionObjectId;
    int m_currentVolumeObjectId;  //main modality MRI

    // added content May 13, 2015 by Xiao
    bool m_isBlendingVolumes;
    double m_blendingVolumesPercent;
    int m_addedVolumeObjectId;


    QString m_baseDir;
    void MakeAcquisitionName(QString & name);
};

#endif
