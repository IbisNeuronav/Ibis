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

#include <QWidget>
#include "serializer.h"
#include "toolplugininterface.h"

class USAcquisitionObject;
class UsProbeObject;
class DoubleViewWidget;
class ImageObject;

/**
 * @class   USAcquisitionPluginInterface
 * @brief   This is a plugin used to handle US acquisitions, live or saved.
 *
 * Using IbisAPI the plugin communicates with the application in order to start and stop acquisition
 * and set acquisition's parameters. The acquisition is shown in the DoubleViewWindow together with the correxponding
 * volume. The acquisition and the volume can be blended in order to see the difference between both.
 *
 * @sa USAcquisitionObject UsProbeObject DoubleViewWidget ImageObject IbisAPI
 * */
class USAcquisitionPluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.USAcquisitionPluginInterface" )

public:
    vtkTypeMacro( USAcquisitionPluginInterface, ToolPluginInterface );

    USAcquisitionPluginInterface();
    virtual ~USAcquisitionPluginInterface();
    virtual QString GetMenuEntryString() override { return QString( "US Acquisition Double View" ); }
    virtual QString GetPluginName() override { return QString( "USAcquisitionDoubleView" ); }
    virtual bool CanRun() override;
    virtual QWidget * CreateFloatingWidget() override;
    virtual bool WidgetAboutToClose() override;

    virtual QString GetPluginDescription() override
    {
        QString description;
        description =
            "US Acquisition Double View\n"
            "This plugin is used to collect and review an ultrasound sweep.\n"
            "The double window allows user to show\n"
            "image and acquisition data side by side.\n"
            "Both images may be blended to show better the differences."
            "\n";
        return description;
    }

    /** Get the pointer to the currently used US probe.*/
    UsProbeObject * GetCurrentUsProbe();
    /** Get the pointer to the current acquisition object.*/
    USAcquisitionObject * GetCurrentAcquisition();
    /** Start acquisition.*/
    void NewAcquisition();
    /** Get pointer to the primary volume. displayed in the right window.*/
    ImageObject * GetCurrentVolume();
    /** Get pointer to the secondary volume. displayed in the right window.*/
    ImageObject * GetAddedVolume();
    /** Check if there is a US probe.*/
    bool CanCaptureTrackedVideo();
    /** Check if trackless capture is allowede.*/
    bool IsTrackerlessCaptureAllowed() { return m_allowTrackerlessCapture; }
    /** Check if the acquisition is live.*/
    bool IsLive() { return m_isLive; }
    /** Start/stop live acquisition.*/
    void SetLive( bool l );

    /** Check if the acquisition is blended with the volume.*/
    bool IsBlending() { return m_isBlending; }
    /** Blend or remove blending.*/
    void SetBlending( bool b ) { m_isBlending = b; }
    /** What is blending percent?*/
    double GetBlendingPercent() { return m_blendingPercent; }
    /** Set blending opacity.*/
    void SetBlendingPercent( double p ) { m_blendingPercent = p; }
    /** Is the acquisition masked?*/
    bool IsMasking() { return m_isMasking; }
    /** Set the acquisition's mask.*/
    void SetMasking( bool m ) { m_isMasking = m; }
    /** Get masking percentage.*/
    double GetMaskingPercent() { return m_maskingPercent; }
    /** Set masking percentage.*/
    void SetMaskingPercent( double p ) { m_maskingPercent = p; }
    /** Get the object id of the current acquisition.*/
    int GetCurrentAcquisitionObjectId() { return m_currentAcquisitionObjectId; }
    /** Set the object id of the current acquisition.*/
    void SetCurrentAcquisitionObjectId( int id );
    /** Get the object id of the current volume.*/
    int GetCurrentVolumeObjectId() { return m_currentVolumeObjectId; }
    /** Set the object id of the current volume.*/
    void SetCurrentVolumeObjectId( int id );
    /** Are the two volumes in the right window blended?*/
    bool IsBlendingVolumes() { return m_isBlendingVolumes; }
    /** Blend the two volumes in the right window or remove blending.*/
    void SetBlendingVolumes( bool b ) { m_isBlendingVolumes = b; }
    /** Get blending percentage of the two volumes in the right window.*/
    double GetBlendingVolumesPercent() { return m_blendingVolumesPercent; }
    /** Set blending percentage of the two volumes in the right window.*/
    void SetBlendingVolumesPercent( double p ) { m_blendingVolumesPercent = p; }
    /** Get the object id of the secondary volume.*/
    int GetAddedVolumeObjectId() { return m_addedVolumeObjectId; }
    /** Set the object id of the secondary volume.*/
    void SetAddedVolumeObjectId( int id );

signals:

    void ObjectsChanged();
    void ImageChanged();

private slots:

    void SceneContentChanged();
    void LutChanged( int );
    void OnImageChanged();

protected:
    void Init();
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
    int m_currentVolumeObjectId;  // main modality MRI

    // added content May 13, 2015 by Xiao
    bool m_isBlendingVolumes;
    double m_blendingVolumesPercent;
    int m_addedVolumeObjectId;

    QString m_baseDir;
    void MakeAcquisitionName( QString & name );
};

#endif
