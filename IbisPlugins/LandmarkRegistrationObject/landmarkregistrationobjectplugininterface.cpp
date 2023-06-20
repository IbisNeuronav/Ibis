/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <QMainWindow>
#include <QMessageBox>
#include <QRect>
#include <QString>
#include <QtGui>
#include <QtPlugin>
#include "ibisapi.h"
#include "landmarkregistrationobject.h"
#include "landmarkregistrationobjectplugininterface.h"
#include "pointsobject.h"

LandmarkRegistrationObjectPluginInterface::LandmarkRegistrationObjectPluginInterface() {}

LandmarkRegistrationObjectPluginInterface::~LandmarkRegistrationObjectPluginInterface() {}

SceneObject * LandmarkRegistrationObjectPluginInterface::CreateObject()
{
    IbisAPI * ibisAPI = GetIbisAPI();
    Q_ASSERT( ibisAPI );
    connect( ibisAPI, SIGNAL( ObjectRemoved( int ) ), this, SLOT( OnObjectRemoved( int ) ) );
    vtkSmartPointer<LandmarkRegistrationObject> regObject = vtkSmartPointer<LandmarkRegistrationObject>::New();
    regObject->SetName( "Landmark Registration" );
    regObject->SetCanEditTransformManually( false );
    vtkSmartPointer<PointsObject> sourcePoints = vtkSmartPointer<PointsObject>::New();
    sourcePoints->SetName( "RegistrationSourcePoints" );
    sourcePoints->SetListable( false );
    sourcePoints->SetCanChangeParent( false );
    double color[3] = {0.0, 1.0, 0.0};
    sourcePoints->SetSelectedColor( color );
    vtkSmartPointer<PointsObject> targetPoints = vtkSmartPointer<PointsObject>::New();
    targetPoints->SetName( "RegistrationTargetPoints" );
    targetPoints->SetListable( false );
    targetPoints->SetCanChangeParent( true );
    double color1[3] = {1.0, 0.1, 0.1};
    targetPoints->SetSelectedColor( color1 );
    double color2[3] = {0.3, 0.3, 1.0};
    targetPoints->SetDisabledColor( color2 );
    ibisAPI->AddObject( regObject );
    ibisAPI->AddObject( sourcePoints, regObject );
    ibisAPI->AddObject( targetPoints, ibisAPI->GetSceneRoot() );
    regObject->SetTargetObjectID( ibisAPI->GetSceneRoot()->GetObjectID() );
    regObject->SetSourcePoints( sourcePoints );
    regObject->SetTargetPoints( targetPoints );
    m_landmarkRegistrationObjectIds.push_back( regObject->GetObjectID() );
    return regObject;
}

void LandmarkRegistrationObjectPluginInterface::SceneAboutToLoad() { this->Clear(); }

void LandmarkRegistrationObjectPluginInterface::OnObjectRemoved( int objID )
{
    RegistrationObjectsContainer::iterator it =
        std::find( m_landmarkRegistrationObjectIds.begin(), m_landmarkRegistrationObjectIds.end(), objID );
    if( it != m_landmarkRegistrationObjectIds.end() ) m_landmarkRegistrationObjectIds.erase( it );
}

void LandmarkRegistrationObjectPluginInterface::Clear()
{
    for( unsigned i = 0; i < m_landmarkRegistrationObjectIds.size(); i++ )
    {
        SceneObject * regObj = GetIbisAPI()->GetObjectByID( m_landmarkRegistrationObjectIds[i] );
        if( regObj ) GetIbisAPI()->RemoveObject( regObj );
    }
    m_landmarkRegistrationObjectIds.clear();
}
