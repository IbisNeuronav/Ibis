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
#include "application.h"
#include "sceneobject.h"
#include "usacquisitionobject.h"
#include "usprobeobject.h"
#include "imageobject.h"
#include "doubleviewwidget.h"
#include "scenemanager.h"
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
    m_currentProbeObjectId = SceneObject::InvalidObjectId;
    m_currentAcquisitionObjectId = SceneObject::InvalidObjectId;
    m_currentVolumeObjectId = SceneObject::InvalidObjectId;

    // added content for adding an additional MRI, May 13, 2015 by Xiao
    m_isBlendingVolumes = false;
    m_blendingVolumesPercent = 0.5;
    m_addedVolumeObjectId = SceneObject::InvalidObjectId;
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
    m_baseDir = GetSceneManager()->GetSceneDirectory();
    m_baseDir.append("/");
    m_baseDir.append(ACQ_BASE_DIR);

    // Watch for objects added and removed from the scene
    SceneManager * man = GetSceneManager();
    Q_ASSERT(man);
    connect( man, SIGNAL(ObjectAdded(int)), this, SLOT(SceneContentChanged()) );
    connect( man, SIGNAL(ObjectRemoved(int)), this, SLOT(SceneContentChanged()) );

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
    SceneManager * man = GetSceneManager();
    disconnect( man, SIGNAL(ObjectAdded(int)), this, SLOT(SceneContentChanged()) );
    disconnect( man, SIGNAL(ObjectRemoved(int)), this, SLOT(SceneContentChanged()) );

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
    return true;
}

UsProbeObject * USAcquisitionPluginInterface::GetCurrentUsProbe()
{
    return UsProbeObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_currentProbeObjectId ) );
}

USAcquisitionObject * USAcquisitionPluginInterface::GetCurrentAcquisition()
{
    return USAcquisitionObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_currentAcquisitionObjectId ) );
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
    SceneManager * man = GetSceneManager();
    if( !man->GetObjectByID( m_currentAcquisitionObjectId ) )
        m_currentAcquisitionObjectId = SceneObject::InvalidObjectId;
    if( m_currentAcquisitionObjectId == SceneObject::InvalidObjectId  )
    {
        QList<USAcquisitionObject*> acquisitions;
        man->GetAllUSAcquisitionObjects( acquisitions );
        if( acquisitions.size() > 0)
            m_currentAcquisitionObjectId = acquisitions[0]->GetObjectID();
    }

    if( initialAcqId != m_currentAcquisitionObjectId )
        emit ObjectsChanged();
}

void USAcquisitionPluginInterface::ValidateCurrentUsProbe()
{
    int initialProbeId = m_currentProbeObjectId;
    SceneManager * man = GetSceneManager();
    if( !man->GetObjectByID( m_currentProbeObjectId ) )
        m_currentProbeObjectId = SceneObject::InvalidObjectId;
    if( m_currentProbeObjectId == SceneObject::InvalidObjectId )
    {
        QList<UsProbeObject*> allProbes;
        man->GetAllUsProbeObjects( allProbes );
        if( allProbes.size() > 0 )
            m_currentProbeObjectId = allProbes[0]->GetObjectID();
    }

    if( initialProbeId != m_currentProbeObjectId )
        emit ObjectsChanged();
}

ImageObject * USAcquisitionPluginInterface::GetCurrentVolume()
{
    return ImageObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_currentVolumeObjectId ) );
}

ImageObject * USAcquisitionPluginInterface::GetAddedVolume()
{
    return ImageObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_addedVolumeObjectId ) );
}

void USAcquisitionPluginInterface::ValidateCurrentVolume()
{
    int initialVolumeId = m_currentVolumeObjectId;
    int newVolumeId = m_currentVolumeObjectId;
    SceneManager * man = GetSceneManager();

    if( !man->GetObjectByID( m_currentVolumeObjectId ) )
        newVolumeId = SceneObject::InvalidObjectId;
    if( newVolumeId == SceneObject::InvalidObjectId )
    {
        QList<ImageObject*> images;
        man->GetAllImageObjects( images );
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
    int newVolumeId = m_addedVolumeObjectId;
    SceneManager * man = GetSceneManager();

    if( !man->GetObjectByID( newVolumeId ) )
        newVolumeId = SceneObject::InvalidObjectId;

    if( newVolumeId == SceneObject::InvalidObjectId )
    {
        QList<ImageObject*> images;
        man->GetAllImageObjects( images );
        if( images.size() > 1 )
            newVolumeId = images[1]->GetObjectID();
        else if( images.size() > 0 )
            newVolumeId = images[0]->GetObjectID();
        else
            newVolumeId = SceneObject::InvalidObjectId;
    }

    if( initialVolumeId != newVolumeId )
    {
        SetAddedVolumeObjectId( newVolumeId );
        emit ObjectsChanged();
    }
}

void USAcquisitionPluginInterface::NewAcquisition()
{
    SceneManager * manager = GetSceneManager();
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
    manager->AddObject(newAcquisition);
    manager->SetCurrentObject(newAcquisition);
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
    ImageObject * prev = ImageObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_currentVolumeObjectId ) );
    if( prev )
    {
        disconnect( prev, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        disconnect( prev, SIGNAL(Modified()), this, SLOT(OnImageChanged()));
    }
    m_currentVolumeObjectId = id;
    ImageObject * im = ImageObject::SafeDownCast( GetSceneManager()->GetObjectByID( id ) );
    if( im )
    {
        connect( im, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        connect( im, SIGNAL(Modified()), this, SLOT(OnImageChanged()));
    }
}

void USAcquisitionPluginInterface::SetAddedVolumeObjectId( int id )
{
    ImageObject * prev = ImageObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_addedVolumeObjectId ) );
    if( prev )
    {
        disconnect( prev, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        disconnect( prev, SIGNAL(Modified()), this, SLOT(OnImageChanged()));
    }
    m_addedVolumeObjectId = id;
    ImageObject * im = ImageObject::SafeDownCast( GetSceneManager()->GetObjectByID( id ) );
    if( im )
    {
        connect( im, SIGNAL(LutChanged(int)), this, SLOT(LutChanged(int)) );
        connect( im, SIGNAL(Modified()), this, SLOT(OnImageChanged()));
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
    SceneManager *manager = GetSceneManager();
    QList< USAcquisitionObject* > acquisitions;
    manager->GetAllUSAcquisitionObjects(acquisitions);
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
