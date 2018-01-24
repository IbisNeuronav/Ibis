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

#include "vertebraregistrationplugininterface.h"
#include "vertebraregistrationwidget.h"
#include "application.h"
#include <QtPlugin>

#include "sceneobject.h"
#include "usacquisitionobject.h"
#include "usprobeobject.h"
#include "imageobject.h"
#include "scenemanager.h"
#include <QString>
#include <QtGui>
#include <QRect>
#include <QMessageBox>
#include <QMainWindow>

VertebraRegistrationPluginInterface::VertebraRegistrationPluginInterface()
{
    m_interfaceWidget = 0;
//    m_gpuRegistrationWidget = 0;

    m_isLive = false;
    m_isMasking = false;
    m_maskingPercent = 1.0;
    m_currentProbeObjectId = SceneManager::InvalidId;
    m_currentAcquisitionObjectId = SceneManager::InvalidId;
    m_currentVolumeObjectId = SceneManager::InvalidId;


}

VertebraRegistrationPluginInterface::~VertebraRegistrationPluginInterface()
{
}

bool VertebraRegistrationPluginInterface::CanRun()
{
    return true;
}

QWidget * VertebraRegistrationPluginInterface::CreateTab()
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
    m_interfaceWidget = new VertebraRegistrationWidget(0);
    m_interfaceWidget->SetPluginInterface( this );
    m_interfaceWidget->SetApplication( GetApplication() );
    m_interfaceWidget->setAttribute( Qt::WA_DeleteOnClose, true );

    m_gpuRegistrationWidget = new GPURigidRegistration();


    return m_interfaceWidget;
}

bool VertebraRegistrationPluginInterface::WidgetAboutToClose()
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
//    m_gpuRegistrationWidget = 0;

    return true;
}

UsProbeObject * VertebraRegistrationPluginInterface::GetCurrentUsProbe()
{
    return UsProbeObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_currentProbeObjectId ) );
}

USAcquisitionObject * VertebraRegistrationPluginInterface::GetCurrentAcquisition()
{
    return USAcquisitionObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_currentAcquisitionObjectId ) );
}

void VertebraRegistrationPluginInterface::ValidateAllSceneObjects()
{
    ValidateCurrentAcquisition();
    ValidateCurrentUsProbe();
    ValidateCurrentVolume();
}

void VertebraRegistrationPluginInterface::ValidateCurrentAcquisition()
{
    int initialAcqId = m_currentAcquisitionObjectId;
    SceneManager * man = GetSceneManager();
    if( !man->GetObjectByID( m_currentAcquisitionObjectId ) )
        m_currentAcquisitionObjectId = SceneManager::InvalidId;
    if( m_currentAcquisitionObjectId == SceneManager::InvalidId  )
    {
        QList<USAcquisitionObject*> acquisitions;
        man->GetAllUSAcquisitionObjects( acquisitions );
        if( acquisitions.size() > 0)
            m_currentAcquisitionObjectId = acquisitions[0]->GetObjectID();

    }

    if( initialAcqId != m_currentAcquisitionObjectId )
        emit ObjectsChanged();
}

void VertebraRegistrationPluginInterface::ValidateCurrentUsProbe()
{
    int initialProbeId = m_currentProbeObjectId;
    SceneManager * man = GetSceneManager();
    if( !man->GetObjectByID( m_currentProbeObjectId ) )
        m_currentProbeObjectId = SceneManager::InvalidId;
    if( m_currentProbeObjectId == SceneManager::InvalidId )
    {
        QList<UsProbeObject*> allProbes;
        man->GetAllUsProbeObjects( allProbes );
        if( allProbes.size() > 0 )
            m_currentProbeObjectId = allProbes[0]->GetObjectID();

    }

    if( initialProbeId != m_currentProbeObjectId )
        emit ObjectsChanged();
}

ImageObject * VertebraRegistrationPluginInterface::GetCurrentVolume()
{
    return ImageObject::SafeDownCast( GetSceneManager()->GetObjectByID( m_currentVolumeObjectId ) );
}

void VertebraRegistrationPluginInterface::ValidateCurrentVolume()
{
    int initialVolumeId = m_currentVolumeObjectId;

    SceneManager * man = GetSceneManager();
    if( !man->GetObjectByID( m_currentVolumeObjectId ) )
        m_currentVolumeObjectId = SceneManager::InvalidId;

    int newVolumeId = m_currentVolumeObjectId;
    if( newVolumeId == SceneManager::InvalidId )
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

void VertebraRegistrationPluginInterface::NewAcquisition()
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

bool VertebraRegistrationPluginInterface::CanCaptureTrackedVideo()
{
    return GetCurrentUsProbe() != 0;
}

void VertebraRegistrationPluginInterface::SetLive( bool l )
{
    m_isLive = l;
    UsProbeObject * p = GetCurrentUsProbe();
    Q_ASSERT( p );
    if( l )
        p->AddClient();
    else
        p->RemoveClient();
}

void VertebraRegistrationPluginInterface::SetCurrentAcquisitionObjectId( int id )
{
    m_currentAcquisitionObjectId = id;
}

void VertebraRegistrationPluginInterface::SetCurrentVolumeObjectId( int id )
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

void VertebraRegistrationPluginInterface::SceneContentChanged()
{
    ValidateAllSceneObjects();
}

void VertebraRegistrationPluginInterface::LutChanged( int vtkNotUsed(id) )
{
    emit ObjectsChanged();
}

void VertebraRegistrationPluginInterface::OnImageChanged()
{
    emit ImageChanged();
}

void VertebraRegistrationPluginInterface::MakeAcquisitionName(QString & name)
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
