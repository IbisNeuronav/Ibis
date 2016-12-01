/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "landmarkregistrationobjectplugininterface.h"
#include "landmarkregistrationobject.h"
#include "application.h"
#include "scenemanager.h"
#include "pointsobject.h"
#include <QtPlugin>
#include <QString>
#include <QtGui>
#include <QRect>
#include <QMessageBox>
#include <QMainWindow>


LandmarkRegistrationObjectPluginInterface::LandmarkRegistrationObjectPluginInterface()
{
    m_landmarkRegistrationObject = 0;
    SceneManager::InConstructor( "LandmarkRegistrationObjectPluginInterface", 0 );
}

LandmarkRegistrationObjectPluginInterface::~LandmarkRegistrationObjectPluginInterface()
{
    if (m_landmarkRegistrationObject)
        m_landmarkRegistrationObject->Delete();
    SceneManager::InDestructor( "LandmarkRegistrationObjectPluginInterface", 0 );
}

SceneObject *LandmarkRegistrationObjectPluginInterface::CreateObject()
{
    SceneManager *manager = GetSceneManager();
    Q_ASSERT(manager);
    PointsObject *sourcePoints = PointsObject::New();
    sourcePoints->SetName( "RegistrationSourcePoints" );
    sourcePoints->SetListable( false );
    sourcePoints->SetCanChangeParent( false );
    double color[3] = {0.0, 1.0, 0.0};
    sourcePoints->SetSelectedColor(color);
    PointsObject *targetPoints = PointsObject::New();
    targetPoints->SetName( "RegistrationTargetPoints" );
    targetPoints->SetListable( false );
    targetPoints->SetCanChangeParent( true );
    double color1[3] = {1.0, 0.1, 0.1};
    targetPoints->SetSelectedColor(color1);
    double color2[3] = {0.3, 0.3, 1.0};
    targetPoints->SetDisabledColor(color2);
    m_landmarkRegistrationObject = LandmarkRegistrationObject::New();
    m_landmarkRegistrationObject->SetName( "Landmark Registration" );
    manager->AddObject( m_landmarkRegistrationObject );
    manager->AddObject( sourcePoints, m_landmarkRegistrationObject );
    manager->AddObject( targetPoints, manager->GetSceneRoot() );
    m_landmarkRegistrationObject->SetTargetObjectID( manager->GetSceneRoot()->GetObjectID() );
    m_landmarkRegistrationObject->SetSourcePoints( sourcePoints );
    m_landmarkRegistrationObject->SetTargetPoints( targetPoints );
    sourcePoints->Delete();
    targetPoints->Delete();
    return m_landmarkRegistrationObject;
}
