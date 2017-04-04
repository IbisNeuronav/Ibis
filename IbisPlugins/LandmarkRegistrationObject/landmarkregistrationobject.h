/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef LANDMARKREGISTRATIONOBJECT_H
#define LANDMARKREGISTRATIONOBJECT_H

#include <QVector>
#include "sceneobject.h"
#include "pointsobject.h"
#include "vtkSmartPointer.h"

class LandmarkTransform;
class vtkPoints;
class SceneManager;

class LandmarkRegistrationObject : public SceneObject
{
    Q_OBJECT

public:
    static LandmarkRegistrationObject * New() { return new LandmarkRegistrationObject; }
    vtkTypeMacro(LandmarkRegistrationObject,SceneObject);

    LandmarkRegistrationObject();
    virtual ~LandmarkRegistrationObject();

    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets );
    virtual void Export();
    virtual bool IsExportable()  { return true; }
    virtual void Serialize( Serializer * ser );
    virtual void PostSceneRead();

    bool ReadTagFile( );

    void SetSourcePoints( PointsObject *pts );
    void SetTargetPoints( PointsObject *pts );
    PointsObject *GetSourcePoints() { return m_sourcePoints; }
    PointsObject *GetTargetPoints() { return m_targetPoints; }
    QStringList GetPointNames( );
    int  GetNumberOfPoints();
    int  GetNumberOfActivePoints();
    vtkSmartPointer<LandmarkTransform> GetLandmarkTransform() { return m_registrationTransform; }
    void UpdateLandmarkTransform();
    void RegisterObject( bool on );
    bool IsRegistered();
    void SetAllowScaling( bool on );
    bool IsScalingAllowed();
    int  GetTargetObjectID() { return m_targetObjectID; }
    void SetTargetObjectID( int id );
    int  GetPointEnabledStatus( int index );
    void SetPointEnabledStatus (int index, int stat );
    void DeletePoint( int index );
    void SelectPoint( int index );
    void SetPointLabel( int index, const QString & label );
    void SetTargetPointCoordinates( int index, double coords[3] );
    void SetTargetPointTimeStamp( int index, const QString &stamp );
    void SetTagSize( int tagSize );

public slots:
    void PointAdded( );
    void PointRemoved( int );
    void Update();

protected slots:

    void CurrentObjectChanged();

protected:

    // SceneObject protected overloads
    virtual void ObjectAddedToScene();
    virtual void ObjectAboutToBeRemovedFromScene();

    virtual void InternalPostSceneRead();
    virtual void Hide();
    virtual void Show();

    void WriteTagFile( const QString & filename, bool saveEnabledOnly = false );
    void WriteXFMFile( const QString & filename );
    void UpdateActivePoints();
    void UpdateTargetPoints();

    void EnablePicking( bool e );

    vtkSmartPointer<LandmarkTransform> m_registrationTransform;
    vtkSmartPointer<vtkTransform> m_backUpTransform;
    PointsObject *m_sourcePoints;
    vtkSmartPointer<vtkPoints> m_activeSourcePoints;
    PointsObject *m_targetPoints;
    vtkSmartPointer<vtkPoints> m_activeTargetPoints;
    QStringList m_activePointNames;
    QVector<int> m_pointEnabledStatus;
    int m_targetObjectID;
    bool m_loadingPointStatus;
    bool m_registerRequested;  // this is used only for serialization

private:
    int m_sourcePointsID; // needed for saving/loading scene
    int m_targetPointsID;
};

ObjectSerializationHeaderMacro( LandmarkRegistrationObject );

#endif // LANDMARKREGISTRATIONOBJECT_H
