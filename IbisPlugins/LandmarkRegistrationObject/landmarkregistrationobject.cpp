/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <vtkLandmarkTransform.h>
#include <vtkPoints.h>
#include <vtkTransform.h>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include "application.h"
#include "ibisconfig.h"
#include "imageobject.h"
#include "landmarkregistrationobject.h"
#include "landmarkregistrationobjectplugininterface.h"
#include "landmarkregistrationobjectsettingswidget.h"
#include "landmarktransform.h"
#include "scenemanager.h"
#include "view.h"
#include "vtkTagWriter.h"
#include "vtkXFMWriter.h"

ObjectSerializationMacro( LandmarkRegistrationObject );

LandmarkRegistrationObject::LandmarkRegistrationObject() : SceneObject()
{
    m_registrationTransform = vtkSmartPointer<LandmarkTransform>::New();
    m_activeSourcePoints    = vtkSmartPointer<vtkPoints>::New();
    m_activeTargetPoints    = vtkSmartPointer<vtkPoints>::New();
    m_registrationTransform->SetSourcePoints( m_activeSourcePoints );
    m_registrationTransform->SetTargetPoints( m_activeTargetPoints );
    m_backUpTransform = vtkSmartPointer<vtkTransform>::New();
    m_backUpTransform->Identity();
    m_sourcePointsID     = SceneManager::InvalidId;
    m_targetPointsID     = SceneManager::InvalidId;
    m_targetObjectID     = SceneManager::InvalidId;
    m_loadingPointStatus = false;
    m_registerRequested  = false;
    m_isRegistered       = false;
}

LandmarkRegistrationObject::~LandmarkRegistrationObject() {}

void LandmarkRegistrationObject::CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets )
{
    LandmarkRegistrationObjectSettingsWidget * props = new LandmarkRegistrationObjectSettingsWidget( parent );
    props->setAttribute( Qt::WA_DeleteOnClose );
    props->SetLandmarkRegistrationObject( this );
    props->SetApplication( &Application::GetInstance() );
    props->setObjectName( "Properties" );
    if( m_sourcePoints )
    {
        connect( m_sourcePoints, SIGNAL( ObjectModified() ), props, SLOT( UpdateUI() ) );
        connect( this, SIGNAL( UpdateSettings() ), props, SLOT( UpdateUI() ) );
    }
    connect( this, SIGNAL( ObjectModified() ), props, SLOT( UpdateUI() ) );
    widgets->append( props );
    if( !this->IsRegistered() ) m_backUpTransform->DeepCopy( this->GetLocalTransform() );
}

void LandmarkRegistrationObject::Serialize( Serializer * ser )
{
    m_registerRequested = this->IsRegistered();
    SceneObject::Serialize( ser );
    int sourceId = SceneManager::InvalidId;
    int targetId = SceneManager::InvalidId;
    if( !ser->IsReader() )
    {
        sourceId = m_sourcePoints->GetObjectID();
        targetId = m_targetPoints->GetObjectID();
    }
    ::Serialize( ser, "Registered", m_registerRequested );
    ::Serialize( ser, "SourcePointsObjectId", sourceId );
    ::Serialize( ser, "TargetPointsObjectId", targetId );
    ::Serialize( ser, "RegistrationTargetObjectId", m_targetObjectID );
    int numberOfPoints = m_pointEnabledStatus.count();
    ::Serialize( ser, "NumberOfPoints", numberOfPoints );
    int * enabledPoints = 0;
    if( numberOfPoints > 0 )
    {
        enabledPoints = new int[numberOfPoints];
        if( ser->IsReader() )
        {
            m_loadingPointStatus = true;
        }
        else
            for( int i = 0; i < numberOfPoints; i++ ) enabledPoints[i] = m_pointEnabledStatus.at( i );
        ::Serialize( ser, "PointsEnabledStatus", enabledPoints, numberOfPoints );
    }

    // Scaling allowed?
    bool scalingAllowed = m_registrationTransform->IsScalingAllowed();
    ::Serialize( ser, "ScalingAllowed", scalingAllowed );
    if( ser->IsReader() ) m_registrationTransform->SetScalingAllowed( scalingAllowed );

    if( ser->IsReader() )
    {
        m_sourcePointsID = sourceId;
        m_targetPointsID = targetId;
        for( int i = 0; i < numberOfPoints; i++ ) m_pointEnabledStatus.push_back( enabledPoints[i] );
        delete[] enabledPoints;
    }
    if( !ser->IsReader() )
    {
        QString filename( ser->GetSerializationDirectory() );
        filename.append( "/LandmarkRegistration.tag" );
        this->WriteTagFile( filename, true );
        QString filename1( ser->GetSerializationDirectory() );
        filename1.append( "/LandmarkRegistration.xfm" );
        this->WriteXFMFile( filename1 );
    }
    // LandmarkRegistrationObject cannot be manually trensformed, previously, the flag AllowManualTransformEdit
    // was not used and in the old scenes it was set to default value true
    // now, we use that flag to disallow manual changes in registration transform
    this->SetCanEditTransformManually( false );
}

void LandmarkRegistrationObject::PostSceneRead()
{
    this->InternalPostSceneRead();
    for( int i = 0; i < Children.size(); ++i ) Children[i]->PostSceneRead();
    this->SelectPoint( m_sourcePoints->GetSelectedPointIndex() );
}

void LandmarkRegistrationObject::CurrentObjectChanged()
{
    if( m_sourcePoints != nullptr )  // m_sourcePoints is set to nullptr when the points are deleted e.g. on app exit
    {
        if( GetManager()->GetCurrentObject() == SceneObject::SafeDownCast( this ) )
        {
            EnablePicking( true );
            m_sourcePoints->ValidateSelectedPoint();
            emit UpdateSettings();
        }
        else
            EnablePicking( false );
    }
}

void LandmarkRegistrationObject::InternalPostSceneRead()
{
    Q_ASSERT( GetManager() );

    vtkSmartPointer<PointsObject> source =
        PointsObject::SafeDownCast( this->GetManager()->GetObjectByID( m_sourcePointsID ) );
    vtkSmartPointer<PointsObject> target =
        PointsObject::SafeDownCast( this->GetManager()->GetObjectByID( m_targetPointsID ) );

    Q_ASSERT( source && target );

    this->SetSourcePoints( source );
    this->SetTargetPoints( target );
    this->SetTargetObjectID( m_targetObjectID );
    for( int i = 0; i < m_pointEnabledStatus.count(); i++ )
        this->SetPointEnabledStatus( i, m_pointEnabledStatus.at( i ) );
    this->UpdateLandmarkTransform();
    if( m_registerRequested )
    {
        this->RegisterObject( true );
    }
    m_sourcePoints->SetHidden( this->IsHidden() );
    m_targetPoints->SetHidden( this->IsHidden() );

    m_loadingPointStatus = false;
}

void LandmarkRegistrationObject::ObjectAddedToScene()
{
    connect( GetManager(), SIGNAL( CurrentObjectChanged() ), this, SLOT( CurrentObjectChanged() ) );
    connect( GetManager(), SIGNAL( CursorPositionChanged() ), this, SLOT( CurrentObjectChanged() ) );
}

void LandmarkRegistrationObject::ObjectAboutToBeRemovedFromScene()
{
    disconnect( GetManager(), SIGNAL( CurrentObjectChanged() ), this, SLOT( CurrentObjectChanged() ) );
    disconnect( GetManager(), SIGNAL( CursorPositionChanged() ), this, SLOT( CurrentObjectChanged() ) );
    // m_targetPoints is not a child of LandmarkRegistrationObject, it is removed as a World child
    m_targetPoints->UnRegister( this );
    m_targetPoints = nullptr;
}

void LandmarkRegistrationObject::Export()
{
    Q_ASSERT( GetManager() );

    bool saveEnabledOnly = false;
    QString question     = QString( tr( "Save disabled points?" ) );
    QMessageBox::StandardButton reply;
    reply = (QMessageBox::StandardButton)QMessageBox::question( 0, QString( tr( "Export" ) ), question,
                                                                QMessageBox::Yes, QMessageBox::No );
    if( reply == QMessageBox::No )
    {
        saveEnabledOnly = true;
    }
    QString tag_directory = this->GetManager()->GetSceneDirectory();
    if( !QFile::exists( tag_directory ) )
    {
        tag_directory = QDir::homePath() + "/" + IBIS_CONFIGURATION_SUBDIRECTORY;
    }
    QString filename = Application::GetInstance().GetFileNameSave( "Save tag file", tag_directory, "Tag file (*.tag)" );

    if( !filename.isEmpty() )
    {
        QFileInfo info( filename );
        QString dirPath = info.dir().absolutePath();
        QFileInfo info1( dirPath );
        if( !info1.isWritable() )
        {
            QString accessError = tr( "No write permission:\n" ) + dirPath;
            QMessageBox::warning( 0, tr( "Error: " ), accessError, 1, 0 );
            return;
        }
        QString filename1( filename );
        if( info.suffix() != "tag" ) filename1.append( ".tag" );
        info.setFile( filename1 );
        this->WriteTagFile( info.absoluteFilePath(), saveEnabledOnly );
        info.setFile( filename );
        if( info.suffix() != "xfm" ) filename.append( ".xfm" );
        info.setFile( filename );
        this->WriteXFMFile( info.absoluteFilePath() );
        return;
    }
}

void LandmarkRegistrationObject::WriteTagFile( const QString & filename, bool saveEnabledOnly )
{
    std::vector<std::string> pointNames;
    std::vector<std::string> timeStamps;
    QStringList::iterator it;
    if( saveEnabledOnly )
    {
        for( int i = 0; i < m_activePointNames.count(); i++ )
            pointNames.push_back( m_activePointNames.value( i ).toUtf8().data() );
        for( int i = 0; i < m_targetPoints->GetTimeStamps()->count(); i++ )
        {
            if( m_pointEnabledStatus[i] == 1 )
            {
                timeStamps.push_back( m_targetPoints->GetTimeStamps()->value( i ).toUtf8().data() );
            }
        }
    }
    else
    {
        for( int i = 0; i < m_sourcePoints->GetPointsNames()->count(); i++ )
            pointNames.push_back( m_sourcePoints->GetPointsNames()->value( i ).toUtf8().data() );
        for( int i = 0; i < m_targetPoints->GetTimeStamps()->count(); i++ )
            timeStamps.push_back( m_targetPoints->GetTimeStamps()->value( i ).toUtf8().data() );
    }

    vtkSmartPointer<vtkTagWriter> writer = vtkSmartPointer<vtkTagWriter>::New();
    writer->SetFileName( filename.toUtf8().data() );
    writer->SetPointNames( pointNames );
    if( saveEnabledOnly )
    {
        writer->AddVolume( m_activeSourcePoints, m_sourcePoints->GetName().toUtf8().data() );
        writer->AddVolume( m_activeTargetPoints, m_targetPoints->GetName().toUtf8().data() );
    }
    else
    {
        vtkPoints * ptSource = m_sourcePoints->GetPoints();
        vtkPoints * ptTarget = m_targetPoints->GetPoints();
        writer->AddVolume( ptSource, m_sourcePoints->GetName().toUtf8().data() );
        writer->AddVolume( ptTarget, m_targetPoints->GetName().toUtf8().data() );
    }
    writer->SetTimeStamps( timeStamps );
    writer->Write();
}

void LandmarkRegistrationObject::WriteXFMFile( const QString & filename )
{
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    mat->Identity();
    vtkSmartPointer<vtkXFMWriter> writer = vtkSmartPointer<vtkXFMWriter>::New();
    writer->SetFileName( filename.toUtf8().data() );
    m_registrationTransform->GetRegistrationTransform()->GetMatrix( mat );
    writer->SetMatrix( mat );
    writer->Write();
}

void LandmarkRegistrationObject::Hide()
{
    m_sourcePoints->SetHidden( true );
    m_targetPoints->SetHidden( true );
}

void LandmarkRegistrationObject::Show()
{
    m_sourcePoints->SetHidden( false );
    m_sourcePoints->ValidateSelectedPoint();
    m_targetPoints->SetHidden( false );
    m_sourcePoints->UpdatePointsVisibility();
    m_targetPoints->UpdatePointsVisibility();
}

void LandmarkRegistrationObject::SetHiddenChildren( SceneObject * parent, bool hide )
{
    // LandmarkRegistrationObject manages two PointsObjects, we just show/hide both.
    m_sourcePoints->SetHidden( hide );
    if( !hide ) m_sourcePoints->ValidateSelectedPoint();
    m_targetPoints->SetHidden( hide );
    m_sourcePoints->UpdatePointsVisibility();
    m_targetPoints->UpdatePointsVisibility();
}

void LandmarkRegistrationObject::SetSourcePoints( vtkSmartPointer<PointsObject> pts )
{
    Q_ASSERT( GetManager() );

    if( m_sourcePoints == pts ) return;
    if( m_sourcePoints )
    {
        m_sourcePoints->disconnect( this );
        this->GetManager()->RemoveObject( m_sourcePoints );
        m_sourcePoints->UnRegister( this );
    }
    m_sourcePoints = pts;
    if( m_sourcePoints )
    {
        m_sourcePoints->Register( this );
        m_sourcePoints->SetListable( false );
        if( m_sourcePoints->GetObjectID() == SceneManager::InvalidId )
            this->GetManager()->AddObject( m_sourcePoints, this );
        else if( m_sourcePoints->GetParent() != this )
            this->GetManager()->ChangeParent( m_sourcePoints, this, 0 );
        if( !m_loadingPointStatus )
        {
            m_pointEnabledStatus.clear();
            for( int i = 0; i < m_sourcePoints->GetNumberOfPoints(); i++ ) m_pointEnabledStatus.push_back( 1 );
        }
        m_sourcePoints->SetHidden( this->IsHidden() );
        if( this->GetManager()->GetCurrentObject() == this ) m_sourcePoints->SetPickable( true );
        m_activeSourcePoints->Reset();
        m_activeSourcePoints->DeepCopy( m_sourcePoints->GetPoints() );
        connect( m_sourcePoints, SIGNAL( PointAdded() ), this, SLOT( PointAdded() ) );
        connect( m_sourcePoints, SIGNAL( PointRemoved( int ) ), this, SLOT( PointRemoved( int ) ) );
        connect( m_sourcePoints, SIGNAL( PointsChanged() ), this, SLOT( Update() ) );
        connect( m_sourcePoints, SIGNAL( RemovingFromScene() ), this, SLOT( OnSourcePointsRemoved() ) );
        disconnect( this->GetManager(), SIGNAL( CurrentObjectChanged() ), m_sourcePoints,
                    SLOT( OnCurrentObjectChanged() ) );
        m_sourcePointsID = m_sourcePoints->GetObjectID();
    }
}

void LandmarkRegistrationObject::SetTargetPoints( vtkSmartPointer<PointsObject> pts )
{
    Q_ASSERT( GetManager() );

    if( m_targetPoints )
    {
        this->GetManager()->RemoveObject( m_targetPoints );
        m_targetPoints->UnRegister( this );
    }
    m_targetPoints = pts;
    if( m_targetPoints )
    {
        m_targetPoints->Register( this );
        m_targetPoints->SetListable( false );
        if( m_targetObjectID == SceneManager::InvalidId )
            m_targetObjectID = this->GetManager()->GetSceneRoot()->GetObjectID();
        if( m_targetPoints->GetObjectID() == SceneManager::InvalidId )
            this->GetManager()->AddObject( m_targetPoints, this->GetManager()->GetObjectByID( m_targetObjectID ) );
        else if( m_targetPoints->GetParent() != this->GetManager()->GetObjectByID( m_targetObjectID ) )
            this->GetManager()->ChangeParent( m_targetPoints, this->GetManager()->GetObjectByID( m_targetObjectID ),
                                              0 );
        m_targetPoints->SetHidden( this->IsHidden() );
        m_targetPoints->SetPickabilityLocked( true );
        m_activeTargetPoints->Reset();
        m_activeTargetPoints->DeepCopy( m_targetPoints->GetPoints() );
        m_targetPointsID = m_targetPoints->GetObjectID();
    }
}

vtkSmartPointer<PointsObject> LandmarkRegistrationObject::GetSourcePoints() { return m_sourcePoints; }

vtkSmartPointer<PointsObject> LandmarkRegistrationObject::GetTargetPoints() { return m_targetPoints; }

int LandmarkRegistrationObject::GetNumberOfPoints()
{
    if( m_sourcePoints ) return m_sourcePoints->GetNumberOfPoints();
    return 0;
}

QStringList LandmarkRegistrationObject::GetPointNames()
{
    Q_ASSERT( m_sourcePoints );
    return *( m_sourcePoints->GetPointsNames() );
}

int LandmarkRegistrationObject::GetPointEnabledStatus( int index )
{
    Q_ASSERT( index >= 0 && index < m_pointEnabledStatus.size() );
    return m_pointEnabledStatus[index];
}

void LandmarkRegistrationObject::SetPointEnabledStatus( int index, int stat )
{
    Q_ASSERT( index >= 0 && index < m_pointEnabledStatus.size() );
    m_sourcePoints->EnableDisablePoint( index, ( stat == 1 ) ? true : false );
    m_pointEnabledStatus[index] = stat;
    this->UpdateLandmarkTransform();
}

void LandmarkRegistrationObject::DeletePoint( int index )
{
    m_sourcePoints->RemovePoint( index );
    // no need to remove target points, it is done on signal PointRemoved() sent by source points
    m_activeSourcePoints->Reset();
    m_activeTargetPoints->Reset();
    m_activePointNames.clear();
    m_activeSourcePoints->DeepCopy( m_sourcePoints->GetPoints() );
    m_activeTargetPoints->DeepCopy( m_targetPoints->GetPoints() );
    QStringList * names = m_sourcePoints->GetPointsNames();
    for( int i = 0; i < m_sourcePoints->GetNumberOfPoints(); i++ )
    {
        m_activePointNames.append( names->at( i ) );
    }
    if( this->GetNumberOfActivePoints() > 2 ) this->UpdateLandmarkTransform();
}

void LandmarkRegistrationObject::SelectPoint( int index )
{
    if( index != PointsObject::InvalidPointIndex )
    {
        m_sourcePoints->SetSelectedPoint( index );
        m_sourcePoints->MoveCursorToPoint( index );
        m_targetPoints->SetSelectedPoint( index );
    }
}

void LandmarkRegistrationObject::SetPointLabel( int index, const QString & label )
{
    m_sourcePoints->SetPointLabel( index, label.toUtf8().data() );
    m_targetPoints->SetPointLabel( index, label.toUtf8().data() );
}

void LandmarkRegistrationObject::SetTargetPointCoordinates( int index, double coords[3] )
{
    m_targetPoints->SetPointCoordinates( index, coords );
    m_activeTargetPoints->SetPoint( index, coords );
    this->UpdateLandmarkTransform();
}

void LandmarkRegistrationObject::SetTargetPointTimeStamp( int index, const QString & stamp )
{
    m_targetPoints->SetPointTimeStamp( index, stamp );
}

void LandmarkRegistrationObject::SetTagSize( int tagSize )
{
    m_sourcePoints->Set3DRadius( tagSize );
    m_targetPoints->Set3DRadius( tagSize );
}

void LandmarkRegistrationObject::PointAdded()
{
    m_pointEnabledStatus.push_back( 1 );  // new point is enabled by definition
    m_activePointNames.append( m_sourcePoints->GetPointsNames()->at( m_sourcePoints->GetNumberOfPoints() - 1 ) );
    m_activeSourcePoints->InsertNextPoint(
        m_sourcePoints->GetPointCoordinates( m_sourcePoints->GetNumberOfPoints() - 1 ) );
    // we may have to add a target point
    if( m_targetPoints->GetNumberOfPoints() < m_sourcePoints->GetNumberOfPoints() )
    {
        double coords[3] = {0.0, 0.0, 0.0};
        m_targetPoints->AddPoint( QString::number( m_targetPoints->GetNumberOfPoints() + 1 ), coords );
        m_activeTargetPoints->InsertNextPoint(
            m_targetPoints->GetPointCoordinates( m_targetPoints->GetNumberOfPoints() - 1 ) );
    }
    emit ObjectModified();
}

void LandmarkRegistrationObject::PointRemoved( int index )
{
    if( m_sourcePoints->GetNumberOfPoints() == m_targetPoints->GetNumberOfPoints() ) return;
    if( m_sourcePoints->GetNumberOfPoints() > m_targetPoints->GetNumberOfPoints() )
        m_sourcePoints->RemovePoint( index );
    else
        m_targetPoints->RemovePoint( index );
    m_activeSourcePoints->Reset();
    m_activeTargetPoints->Reset();
    m_activePointNames.clear();
    m_activeSourcePoints->DeepCopy( m_sourcePoints->GetPoints() );
    m_activeTargetPoints->DeepCopy( m_targetPoints->GetPoints() );
    QStringList * names = m_sourcePoints->GetPointsNames();
    for( int i = 0; i < m_sourcePoints->GetNumberOfPoints(); i++ )
    {
        m_activePointNames.append( names->at( i ) );
    }
    m_pointEnabledStatus.erase( m_pointEnabledStatus.begin() + index );
    this->UpdateLandmarkTransform();
    if( m_sourcePoints->GetNumberOfPoints() > 0 ) this->SelectPoint( 0 );
    emit ObjectModified();
}

void LandmarkRegistrationObject::EnablePicking( bool enable )
{
    Q_ASSERT( m_sourcePoints );
    m_sourcePoints->SetPickable( enable );
}

void LandmarkRegistrationObject::UpdateActivePoints()
{
    m_activeSourcePoints->Reset();
    m_activeTargetPoints->Reset();
    m_activePointNames.clear();
    vtkPoints * source  = m_sourcePoints->GetPoints();
    vtkPoints * target  = m_targetPoints->GetPoints();
    QStringList * names = m_sourcePoints->GetPointsNames();
    for( int i = 0; i < m_sourcePoints->GetNumberOfPoints(); i++ )
    {
        if( m_pointEnabledStatus[i] == 1 )
        {
            m_activeSourcePoints->InsertNextPoint( source->GetPoint( i ) );
            m_activeTargetPoints->InsertNextPoint( target->GetPoint( i ) );
            m_activePointNames.append( names->at( i ) );
        }
    }
    m_activeSourcePoints->Modified();
    m_activeTargetPoints->Modified();
}

void LandmarkRegistrationObject::UpdateTargetPoints()
{
    double coords[3] = {0.0, 0.0, 0.0};
    m_targetPoints->SetListable( false );
    int numSrcPts = m_sourcePoints->GetNumberOfPoints();
    int numTrgPts = m_targetPoints->GetNumberOfPoints();
    if( numTrgPts > numSrcPts )
    {
        int i = numTrgPts - 1;
        while( ( i = m_targetPoints->GetNumberOfPoints() - 1 ) > numSrcPts - 1 ) m_targetPoints->RemovePoint( i );
        for( int i = 0; i < numTrgPts; i++ ) m_targetPoints->SetPointCoordinates( i, coords );
    }
    else
    {
        for( int i = 0; i < m_targetPoints->GetNumberOfPoints(); i++ ) m_targetPoints->SetPointCoordinates( i, coords );
        for( int i = m_targetPoints->GetNumberOfPoints(); i < numSrcPts; i++ )
            m_targetPoints->AddPoint( m_sourcePoints->GetPointLabel( i ), coords );
    }
    m_targetPoints->SetHidden( this->IsHidden() );
}

void LandmarkRegistrationObject::UpdateLandmarkTransform()
{
    this->UpdateActivePoints();
    m_registrationTransform->UpdateRegistrationTransform();
    this->WorldTransformChanged();
}

void LandmarkRegistrationObject::OnSourcePointsRemoved()
{
    // No callback operation can be done when source points are removed/
    disconnect( m_sourcePoints );
    disconnect( this, SIGNAL( UpdateSettings() ) );
    disconnect( this, SIGNAL( ObjectModified() ) );
    m_sourcePoints->UnRegister( this );
    m_sourcePoints   = nullptr;
    m_sourcePointsID = SceneManager::InvalidId;
}

void LandmarkRegistrationObject::Update()
{
    this->UpdateLandmarkTransform();
    m_targetPoints->SetSelectedPoint( m_sourcePoints->GetSelectedPointIndex() );

    emit ObjectModified();
}

void LandmarkRegistrationObject::RegisterObject( bool on )
{
    Q_ASSERT( on != IsRegistered() );
    if( on )
    {
        m_registrationTransform->UpdateRegistrationTransform();
        this->GetLocalTransform()->SetInput( m_registrationTransform->GetRegistrationTransform() );
        m_isRegistered = true;
    }
    else
    {
        this->GetLocalTransform()->SetInput( m_backUpTransform );
        m_isRegistered = false;
    }
}

void LandmarkRegistrationObject::SetTargetObjectID( int id )
{
    Q_ASSERT( GetManager() );

    if( m_targetObjectID == id ) return;
    SceneObject * newTarget = this->GetManager()->GetObjectByID( id );
    if( newTarget )
    {
        m_targetObjectID = id;
        if( m_targetPoints && m_targetPoints->GetParent() != newTarget )
            this->GetManager()->ChangeParent( m_targetPoints, newTarget, 0 );
    }
}

bool LandmarkRegistrationObject::ReadTagFile()
{
    Q_ASSERT( GetManager() );

    QString tag_directory = this->GetManager()->GetSceneDirectory();
    if( !QFile::exists( tag_directory ) )
    {
        tag_directory = QDir::homePath() + "/" + IBIS_CONFIGURATION_SUBDIRECTORY;
    }
    if( !QFile::exists( tag_directory ) ) tag_directory = QDir::homePath();
    QString filename = Application::GetInstance().GetFileNameOpen( "Load tag file", tag_directory, "Tag file (*.tag)" );
    if( !filename.isEmpty() )
    {
        QFileInfo fi( filename );
        if( !fi.exists() )
        {
            QString accessError = tr( "File does not exist:\n" ) + filename;
            QMessageBox::warning( 0, tr( "Error: " ), accessError, 1, 0 );
            return false;
        }
        if( !fi.isReadable() )
        {
            QString accessError = tr( "No read permission:\n" ) + filename;
            QMessageBox::warning( 0, tr( "Error: " ), accessError, 1, 0 );
            return false;
        }
    }
    if( filename.isNull() || filename.isEmpty() ) return false;
    vtkSmartPointer<PointsObject> src  = vtkSmartPointer<PointsObject>::New();
    vtkSmartPointer<PointsObject> trgt = vtkSmartPointer<PointsObject>::New();
    bool ok                            = Application::GetInstance().GetPointsFromTagFile( filename, src, trgt );
    if( !ok ) return false;
    if( src->GetNumberOfPoints() == 0 )
    {
        QString accessError = tr( "File " ) + filename + tr( " contains no points." );
        QMessageBox::critical( 0, tr( "Error: " ), accessError, 1, 0 );
        return false;
    }
    this->SetSourcePoints( src );
    if( trgt->GetNumberOfPoints() > 0 )
    {
        this->SetTargetPoints( trgt );
    }
    else
    {
        this->UpdateTargetPoints();
    }
    this->SelectPoint( 0 );
    this->UpdateLandmarkTransform();
    return true;
}

int LandmarkRegistrationObject::GetNumberOfActivePoints() { return m_activeSourcePoints->GetNumberOfPoints(); }
