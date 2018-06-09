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
#include "ibisapi.h"
#include "pointsobject.h"
#include <QtPlugin>
#include <QString>
#include <QtGui>
#include <QRect>
#include <QMessageBox>
#include <QMainWindow>


LandmarkRegistrationObjectPluginInterface::LandmarkRegistrationObjectPluginInterface()
{
}

LandmarkRegistrationObjectPluginInterface::~LandmarkRegistrationObjectPluginInterface()
{
}

SceneObject *LandmarkRegistrationObjectPluginInterface::CreateObject()
{
    m_landmarkRegistrationObject = 0;
    IbisAPI *ibisAPI = GetIbisAPI();
    Q_ASSERT(ibisAPI);
    vtkSmartPointer<PointsObject>sourcePoints = vtkSmartPointer<PointsObject>::New();
    sourcePoints->SetName( "RegistrationSourcePoints" );
    sourcePoints->SetListable( false );
    sourcePoints->SetCanChangeParent( false );
    double color[3] = {0.0, 1.0, 0.0};
    sourcePoints->SetSelectedColor(color);
    vtkSmartPointer<PointsObject> targetPoints = vtkSmartPointer<PointsObject>::New();
    targetPoints->SetName( "RegistrationTargetPoints" );
    targetPoints->SetListable( false );
    targetPoints->SetCanChangeParent( true );
    double color1[3] = {1.0, 0.1, 0.1};
    targetPoints->SetSelectedColor(color1);
    double color2[3] = {0.3, 0.3, 1.0};
    targetPoints->SetDisabledColor(color2);
    m_landmarkRegistrationObject = vtkSmartPointer<LandmarkRegistrationObject>::New();
    m_landmarkRegistrationObject->SetName( "Landmark Registration" );
    m_landmarkRegistrationObject->SetCanEditTransformManually( false );
    ibisAPI->AddObject( m_landmarkRegistrationObject );
    ibisAPI->AddObject( sourcePoints, m_landmarkRegistrationObject );
    ibisAPI->AddObject( targetPoints, ibisAPI->GetSceneRoot() );
    m_landmarkRegistrationObject->SetTargetObjectID( ibisAPI->GetSceneRoot()->GetObjectID() );
    m_landmarkRegistrationObject->SetSourcePoints( sourcePoints );
    m_landmarkRegistrationObject->SetTargetPoints( targetPoints );
    return m_landmarkRegistrationObject;
}
