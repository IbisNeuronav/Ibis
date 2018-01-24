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

#ifndef __VertebraRegistrationPluginInterface_h_
#define __VertebraRegistrationPluginInterface_h_

#include <QObject>
#include <QWidget>
#include "toolplugininterface.h"
#include "serializer.h"

#include "itkgpurigidregistration.h"

class VertebraRegistrationWidget;
class USAcquisitionObject;
class UsProbeObject;
class ImageObject;

class GPURigidRegistration;

class VertebraRegistrationPluginInterface : public QObject, public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.VertebraRegistrationPluginInterface" )

public:\

    vtkTypeMacro( VertebraRegistrationPluginInterface, ToolPluginInterface );

    VertebraRegistrationPluginInterface();
    ~VertebraRegistrationPluginInterface();
    virtual QString GetPluginName() { return QString("VertebraRegistration"); }
    bool CanRun();
    QString GetMenuEntryString() { return QString("Single Vertebra Registration"); }

    QWidget * CreateTab();


    virtual bool WidgetAboutToClose();

    UsProbeObject * GetCurrentUsProbe();
    USAcquisitionObject * GetCurrentAcquisition();
    void NewAcquisition();
    ImageObject * GetCurrentVolume();

    bool CanCaptureTrackedVideo();
    bool IsLive() { return m_isLive; }
    void SetLive( bool l );

    bool IsMasking() { return m_isMasking; }
    void SetMasking( bool m ) { m_isMasking = m; }
    double GetMaskingPercent() { return m_maskingPercent; }
    void SetMaskingPercent( double p ) { m_maskingPercent = p; }

    int GetCurrentAcquisitionObjectId() { return m_currentAcquisitionObjectId; }
    void SetCurrentAcquisitionObjectId( int id );

    int GetCurrentVolumeObjectId() { return m_currentVolumeObjectId; }
    void SetCurrentVolumeObjectId( int id );

    GPURigidRegistration * getGPURegistrationWidget() { return m_gpuRegistrationWidget; }

signals:

    void ObjectsChanged();
    void ImageChanged();

private slots:

    void SceneContentChanged();
    void LutChanged( int );
    void OnImageChanged();

protected:

    void ValidateAllSceneObjects();
    void ValidateCurrentUsProbe();
    void ValidateCurrentAcquisition();
    void ValidateCurrentVolume();

    VertebraRegistrationWidget * m_interfaceWidget;
    GPURigidRegistration * m_gpuRegistrationWidget;

    bool m_isLive;
    bool m_isMasking;
    double m_maskingPercent;
    int m_currentProbeObjectId;
    int m_currentAcquisitionObjectId;
    int m_currentVolumeObjectId;  //main modality MRI

    QString m_baseDir;
    void MakeAcquisitionName(QString & name);

};

#endif
