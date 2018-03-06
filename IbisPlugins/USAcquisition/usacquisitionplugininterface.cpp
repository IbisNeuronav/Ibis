/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "usacquisitionplugininterface.h"
#include "sceneobject.h"
#include "usacquisitionobject.h"
#include "usprobeobject.h"
#include "imageobject.h"
#include "doubleviewwidget.h"
#include "ibisapi.h"
#include <QtPlugin>
#include <QString>
#include <QtGui>
#include <QRect>
#include <QMessageBox>
#include <QMainWindow>


USAcquisitionPluginInterface::USAcquisitionPluginInterface() : m_allowTrackerlessCapture(false)
{
    m_interfaceWidget = 0;

    m_isLive = false;
    m_isBlending = false;
    m_blendingPercent = 0.5;
    m_isMasking = false;
    m_maskingPercent = 1.0;
    m_currentProbeObjectId = SceneManager::InvalidId;
    m_currentAcquisitionObjectId = SceneManager::InvalidId;
    m_currentVolumeObjectId = SceneManager::InvalidId;

    // added content for adding an additional MRI, May 13, 2015 by Xiao
    m_isBlendingVolumes = false;
    m_blendingVolumesPercent = 0.5;
    m_addedVolumeObjectId = SceneManager::InvalidId;
}

USAcquisitionPluginInterface::~USAcquisitionPluginInterface()
{
}

bool USAcquisitionPluginInterface::CanRun()
{
    return true;
}

QWidget *USAcquisitionPluginInterface::CreateFloatingWidget()
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    m_baseDir = ibisAPI->GetSceneDirectory();
    m_baseDir.append("/");
    m_baseDir.append(ACQ_BASE_DIR);

    // Watch for objects added and removed from the scene
    connect( ibisAPI, SIGNAL(ObjectAdded(int)), this, SLOT(SceneContentChanged()) );
    connect( ibisAPI, SIGNAL(ObjectRemoved(int)), this, SLOT(SceneContentChanged()) );

    ValidateAllSceneObjects();

    // Start the window
    m_interfaceWidget = new DoubleViewWidget( 0, Qt::WindowStaysOnTopHint );
    m_interfaceWidget->SetPluginInterface( this );
    m_interfaceWidget->setAttribute( Qt::WA_DeleteOnClose, true );
    m_interfaceWidget->setWindowTitle( "Ultrasound View" );

    return m_interfaceWidget;
}

bool USAcquisitionPluginInterface::WidgetAboutToClose()
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    disconnect( ibisAPI, SIGNAL(ObjectAdded(int)), this, SLOT(SceneContentChanged()) );
    disconnect( ibisAPI, SIGNAL(ObjectRemoved(int)), this, SLOT(SceneContentChanged()) );

    USAcquisitionObject * acq = GetCurrentAcquisition();
    if( acq )
        acq->Stop();
    if( this->IsLive() )
    {
        UsProbeObject * probe = GetCurrentUsProbe();
        Q_ASSERT( probe );
        probe->RemoveClient();
        this->SetLive(false);
    }
    m_interfaceWidget = 0;
    m_isBlending = false;
    m_isBlendingVolumes = false;
    return true;
}

UsProbeObject * USAcquisitionPluginInterface::GetCurrentUsProbe()
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    return UsProbeObject::SafeDownCast( ibisAPI->GetObjectByID( m_currentProbeObjectId ) );
}

USAcquisitionObject * USAcquisitionPluginInterface::GetCurrentAcquisition()
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    return USAcquisitionObject::SafeDownCast( ibisAPI->GetObjectByID( m_currentAcquisitionObjectId ) );
}

void USAcquisitionPluginInterface::ValidateAllSceneObjects()
{
    ValidateCurrentAcquisition();
    ValidateCurrentUsProbe();
    ValidateCurrentVolume();
    ValidateAddedVolume();
}

void USAcquisitionPluginInterface::ValidateCurrentAcquisition()
{
    int initialAcqId = m_currentAcquisitionObjectId;
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    if( !ibisAPI->GetObjectByID( m_currentAcquisitionObjectId ) )
        m_currentAcquisitionObjectId = SceneManager::InvalidId;
    if( m_currentAcquisitionObjectId == SceneManager::InvalidId  )
    {
        QList<USAcquisitionObject*> acquisitions;
        ibisAPI->GetAllUSAcquisitionObjects( acquisitions );
        if( acquisitions.size() > 0)
            m_currentAcquisitionObjectId = acquisitions[0]->GetObjectID();
    }

    if( initialAcqId != m_currentAcquisitionObjectId )
        emit ObjectsChanged();
}

void USAcquisitionPluginInterface::ValidateCurrentUsProbe()
{
    int initialProbeId = m_currentProbeObjectId;
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    if( !ibisAPI->GetObjectByID( m_currentProbeObjectId ) )
        m_currentProbeObjectId = SceneManager::InvalidId;
    if( m_currentProbeObjectId == SceneManager::InvalidId )
    {
        QList<UsProbeObject*> allProbes;
        ibisAPI->GetAllUsProbeObjects( allProbes );
        if( allProbes.size() > 0 )
            m_currentProbeObjectId = allProbes[0]->GetObjectID();
    }

    if( initialProbeId != m_currentProbeObjectId )
        emit ObjectsChanged();
}

ImageObject * USAcquisitionPluginInterface::GetCurrentVolume()
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    return ImageObject::SafeDownCast( ibisAPI->GetObjectByID( m_currentVolumeObjectId ) );
}

ImageObject * USAcquisitionPluginInterface::GetAddedVolume()
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    return ImageObject::SafeDownCast( ibisAPI->GetObjectByID( m_addedVolumeObjectId ) );
}

void USAcquisitionPluginInterface::ValidateCurrentVolume()
{
    int initialVolumeId = m_currentVolumeObjectId;

    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    if( !ibisAPI->GetObjectByID( m_currentVolumeObjectId ) )
        m_currentVolumeObjectId = SceneManager::InvalidId;

    int newVolumeId = m_currentVolumeObjectId;
    if( newVolumeId == SceneManager::InvalidId )
    {
        QList<ImageObject*> images;
        ibisAPI->GetAllImageObjects( images );
        if( images.size() > 0 )
            newVolumeId = images[0]->GetObjectID();
    }

    if( initialVolumeId != newVolumeId )
    {
        SetCurrentVolumeObjectId( newVolumeId );
        emit ObjectsChanged();
    }
}

void USAcquisitionPluginInterface::ValidateAddedVolume()
{
    int initialVolumeId = m_addedVolumeObjectId;

    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    if( !ibisAPI->GetObjectByID( m_addedVolumeObjectId ) )
        m_addedVolumeObjectId = SceneManager::InvalidId;

    int newVolumeId = m_addedVolumeObjectId;
    if( newVolumeId == SceneManager::InvalidId )
    {
        QList<ImageObject*> images;
        ibisAPI->GetAllImageObjects( images );
        if( images.size() > 1 )
            newVolumeId = images[1]->GetObjectID();
        else if( images.size() > 0 )
            newVolumeId = images[0]->GetObjectID();
        else
            newVolumeId = SceneManager::InvalidId;
    }

    if( initialVolumeId != newVolumeId )
    {
        SetAddedVolumeObjectId( newVolumeId );
        emit ObjectsChanged();
    }
}

void USAcquisitionPluginInterface::NewAcquisition()
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    USAcquisitionObject *newAcquisition = USAcquisitionObject::New();
    newAcquisition->SetCanAppendChildren(true);
    newAcquisition->SetNameChangeable( true );
    newAcquisition->SetObjectDeletable( true );
    newAcquisition->SetCanChangeParent( false );
    newAcquisition->SetCanEditTransformManually( true );
    newAcquisition->SetObjectManagedBySystem(false);
    newAcquisition->SetBaseDirectory(m_baseDir);
    QString name;
    this->MakeAcquisitionName(name);
    newAcquisition->SetName(name);
    newAcquisition->SetUsProbe( GetCurrentUsProbe() );
    newAcquisition->SetHidden( true );
    ibisAPI->AddObject(newAcquisition);
    ibisAPI->SetCurrentObject(newAcquisition);
    m_currentAcquisitionObjectId = newAcquisition->GetObjectID();
    newAcquisition->Delete();

    emit ObjectsChanged();
}

bool USAcquisitionPluginInterface::CanCaptureTrackedVideo()
{
    return GetCurrentUsProbe() != 0;
}

void USAcquisitionPluginInterface::SetLive( bool l )
{
    m_isLive = l;
    UsProbeObject * p = GetCurrentUsProbe();
    Q_ASSERT( p );
    if( l )
        p->AddClient();
    else
        p->RemoveClient();
}

void USAcquisitionPluginInterface::SetCurrentAcquisitionObjectId( int id )
{
    m_currentAcquisitionObjectId = id;
}

void USAcquisitionPluginInterface::SetCurrentVolumeObjectId( int id )
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    ImageObject * prev = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( m_currentVolumeObjectId ) );
    if( prev )
    {
        disconnect( prev, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        disconnect( prev, SIGNAL(ObjectModified()), this, SLOT(OnImageChanged()));
    }
    m_currentVolumeObjectId = id;
    ImageObject * im = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( id ) );
    if( im )
    {
        connect( im, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        connect( im, SIGNAL(ObjectModified()), this, SLOT(OnImageChanged()));
    }
}

void USAcquisitionPluginInterface::SetAddedVolumeObjectId( int id )
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    ImageObject * prev = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( m_addedVolumeObjectId ) );
    if( prev )
    {
        disconnect( prev, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        disconnect( prev, SIGNAL(ObjectModified()), this, SLOT(OnImageChanged()));
    }
    m_addedVolumeObjectId = id;
    ImageObject * im = ImageObject::SafeDownCast( ibisAPI->GetObjectByID( id ) );
    if( im )
    {
        connect( im, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        connect( im, SIGNAL(ObjectModified()), this, SLOT(OnImageChanged()));
    }
}

void USAcquisitionPluginInterface::SceneContentChanged()
{
    ValidateAllSceneObjects();
}

void USAcquisitionPluginInterface::LutChanged( int vtkNotUsed(id) )
{
    emit ObjectsChanged();
}

void USAcquisitionPluginInterface::OnImageChanged()
{
    emit ImageChanged();
}

void USAcquisitionPluginInterface::MakeAcquisitionName(QString & name)
{
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    QList< USAcquisitionObject* > acquisitions;
    ibisAPI->GetAllUSAcquisitionObjects(acquisitions);
    int index = acquisitions.count();
    QString namePrefix(ACQ_ACQUISITION_PREFIX);
    name = namePrefix;
    name.append(QString::number(index));
    //check if there is an acuisition with that name
    bool found;
    do
    {
        found = false;
        for( int i = 0; i < acquisitions.count(); ++i )
        {
            USAcquisitionObject * acq = acquisitions.at(i);
            if (QString::compare(acq->GetName(), name) == 0)
            {
                found = true;
                index++;
                name = namePrefix;
                name.append(QString::number(index));
                break;
            }
        }
        if (!found)
            break;
     } while (1);
}
