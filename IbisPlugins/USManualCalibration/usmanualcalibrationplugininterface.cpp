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

#include "usmanualcalibrationplugininterface.h"
#include "usmanualcalibrationwidget.h"
#include "ibisapi.h"
#include "sceneobject.h"
#include "usprobeobject.h"
#include <QtPlugin>
#include <QSettings>
#include <QMessageBox>

const double phantomPoints[2][16][3] = {
    // medium depth 
  { { 0, 5, 50 },
    { 50, 5, 50 },
    { 0, 45, 50 },
    { 50, 45, 50 },
    { 0, 50, 45 },
    { 50, 50, 45 },
    { 0, 50, 5 },
    { 50, 50, 5 },
    { 0, 45, 0 },
    { 50, 45, 0 },
    {  0, 5, 0 },
    {  50, 5, 0 },
    {  0, 0, 45 },
    {  50, 0, 45 },
    {  0, 0, 5 },
    {  50, 0, 5 } 
  },
// shallow depth
  { { 0, 15, 40 },
    { 50, 15, 40 },
    {  0, 35, 40 },
    { 50, 35, 40 },
    {  0, 40, 35 },
    { 50, 40, 35 },
    {  0, 40, 15 },
    { 50, 40, 15 },
    {  0, 35, 10 },
    { 50, 35, 10 },
    {  0, 15, 10 },
    { 50, 15, 10 },
    {  0, 10, 35 },
    { 50, 10, 35 },
    {  0, 10, 15 },
    { 50, 10, 15 }
  }
};

USManualCalibrationPluginInterface::USManualCalibrationPluginInterface()
{
    m_phantomWiresObjectId = IbisAPI::InvalidId;
    m_calibrationPhantomObjectId = IbisAPI::InvalidId;
    m_phantomRegSourcePointsId = IbisAPI::InvalidId;
    m_phantomRegTargetPointsId = IbisAPI::InvalidId;
    m_landmarkRegistrationObjectId = IbisAPI::InvalidId;
    m_usProbeObjectId = IbisAPI::InvalidId;
    m_currentPhantomSize = PhantomSize::MEDIUMDEPTH;
}

USManualCalibrationPluginInterface::~USManualCalibrationPluginInterface()
{
}

bool USManualCalibrationPluginInterface::CanRun()
{
    return true;
}

QWidget * USManualCalibrationPluginInterface::CreateFloatingWidget()
{
    // Make sure the us probe is still available and find another one if it is not
    ValidateUsProbe();

    if( m_usProbeObjectId == IbisAPI::InvalidId )
    {
        QString message( "No valid probe detected." );
        QMessageBox::critical( 0, "Error", message, 1, 0 );
        return 0;
    }

    BuildWiresRepresentation();

    USManualCalibrationWidget * calibrationWidget = new USManualCalibrationWidget;
    calibrationWidget->SetPluginInterface( this );

    if( m_landmarkRegistrationObjectId == IbisAPI::InvalidId )
        this->StartPhantomRegistration();

    return calibrationWidget;
}

bool USManualCalibrationPluginInterface::WidgetAboutToClose()
{
    if( m_landmarkRegistrationObjectId != IbisAPI::InvalidId )
    {
        SceneObject *phantom = this->GetIbisAPI()->GetObjectByID(m_phantomWiresObjectId);
        if( phantom )
            this->GetIbisAPI()->RemoveObject( phantom );
        SceneObject * model = this->GetIbisAPI()->GetObjectByID(m_calibrationPhantomObjectId);
        if( model )
            this->GetIbisAPI()->RemoveObject(model);
        this->GetIbisAPI()->RemoveObject( this->GetIbisAPI()->GetObjectByID( m_landmarkRegistrationObjectId ) );
        m_phantomWiresObjectId = IbisAPI::InvalidId;
        m_calibrationPhantomObjectId = IbisAPI::InvalidId;
        m_phantomRegSourcePointsId = IbisAPI::InvalidId;
        m_phantomRegTargetPointsId = IbisAPI::InvalidId;
        m_landmarkRegistrationObjectId = IbisAPI::InvalidId;
    }
    return true;
}

void USManualCalibrationPluginInterface::LoadSettings( QSettings & s )
{
    // simtodo : implement this
    //m_calibrationGridWidth = s.value( "CalibrationGridWidth", 6 ).toInt();
}

void USManualCalibrationPluginInterface::SaveSettings( QSettings & s )
{
    // simtodo : implement this
    //s.setValue( "CalibrationGridWidth", m_calibrationGridWidth );
}

UsProbeObject * USManualCalibrationPluginInterface::GetCurrentUsProbe()
{
    return UsProbeObject::SafeDownCast( GetIbisAPI()->GetObjectByID( m_usProbeObjectId ) );
}

void USManualCalibrationPluginInterface::ValidateUsProbe()
{
    UsProbeObject * probe = UsProbeObject::SafeDownCast( GetIbisAPI()->GetObjectByID( m_usProbeObjectId ) );
    if( !probe )
        m_usProbeObjectId = IbisAPI::InvalidId;
    if( m_usProbeObjectId == IbisAPI::InvalidId )
    {
        QList<UsProbeObject*> allProbes;
        GetIbisAPI()->GetAllUsProbeObjects( allProbes );
        if( allProbes.size() > 0 )
            m_usProbeObjectId = allProbes[0]->GetObjectID();
    }
}

#include "polydataobject.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"

void USManualCalibrationPluginInterface::BuildWiresRepresentation()
{
    // Build wire representation
    bool needNewRepresentation = m_phantomWiresObjectId == IbisAPI::InvalidId;
    needNewRepresentation |= GetIbisAPI()->GetObjectByID(m_phantomWiresObjectId) == 0;
    if( !needNewRepresentation )
        return;

    vtkPolyData * phantomLinesPoly = vtkPolyData::New();
    vtkPoints * phantomLinesPoints = vtkPoints::New();

    phantomLinesPoints->SetNumberOfPoints(16);
    for( int i = 0; i < 16; ++i )
        phantomLinesPoints->SetPoint(i, phantomPoints[m_currentPhantomSize][i]);

    vtkIdType pts[4][4] = {{0,1,2,3},{4,5,6,7},{8,9,10,11},{12,13,14,15}};
    vtkCellArray * lines = vtkCellArray::New();
    for( int i = 0; i < 4; i++ ) lines->InsertNextCell(4, pts[i]);
    phantomLinesPoly->SetPoints(phantomLinesPoints);
    phantomLinesPoly->SetLines(lines);

    PolyDataObject * phantomObject = PolyDataObject::New();
    phantomObject->SetName("Phantom Wires");
    phantomObject->SetCanChangeParent(false);
    phantomObject->SetCanEditTransformManually(false);
    phantomObject->SetNameChangeable(false);
    //phantomObject->SetObjectDeletable( false );
    phantomObject->SetPolyData(phantomLinesPoly);
    GetIbisAPI()->AddObject(phantomObject);
    m_phantomWiresObjectId = phantomObject->GetObjectID();

    // Cleanup
    phantomObject->Delete();
    lines->Delete();
    phantomLinesPoints->Delete();
    phantomLinesPoly->Delete();
}

void USManualCalibrationPluginInterface::BuildCalibrationPhantomRepresentation()
{
    // Build model representation
    bool needNewRepresentation = m_calibrationPhantomObjectId == IbisAPI::InvalidId;
    needNewRepresentation |= GetIbisAPI()->GetObjectByID(m_calibrationPhantomObjectId) == 0;
    if( !needNewRepresentation )
        return;

    // TODO: get file name from CMake
    QString filePath( tr("../IbisPlugins/") + this->GetPluginName() + tr("/calibrationPhantomModel.ply") );
    if( QFile(filePath).exists() )
    {
        OpenFileParams * params = new OpenFileParams;
        params->AddInputFile(filePath, tr("Phantom Model"));
        this->GetIbisAPI()->OpenFiles(params);
        if( params->filesParams[0].loadedObject )
        {
            params->filesParams[0].loadedObject->SetCanChangeParent(false);
            params->filesParams[0].loadedObject->SetCanEditTransformManually(false);
            params->filesParams[0].loadedObject->SetNameChangeable(false);
            m_calibrationPhantomObjectId = params->filesParams[0].loadedObject->GetObjectID();
        }
    }
}

void USManualCalibrationPluginInterface::UpdateWiresRepresentation()
{
    bool needNewRepresentation = m_phantomWiresObjectId == IbisAPI::InvalidId;
    PolyDataObject * phantomObject = PolyDataObject::SafeDownCast(GetIbisAPI()->GetObjectByID(m_phantomWiresObjectId));
    if( needNewRepresentation | (phantomObject == 0) )
    {
        this->BuildWiresRepresentation();
        return;
    }

    vtkPolyData * phantomLinesPoly = phantomObject->GetPolyData();
    Q_ASSERT(phantomLinesPoly->GetNumberOfPoints() == 16);

    vtkPoints * phantomLinesPoints = vtkPoints::New();
    vtkCellArray * lines = vtkCellArray::New();

    phantomLinesPoints->SetNumberOfPoints(16);
    for( int i = 0; i < 16; ++i )
        phantomLinesPoints->SetPoint(i, phantomPoints[m_currentPhantomSize][i]);

    vtkIdType pts[4][4] = {{0,1,2,3},{4,5,6,7},{8,9,10,11},{12,13,14,15}};    
    for( int i = 0; i < 4; i++ ) lines->InsertNextCell(4, pts[i]);
    
    phantomLinesPoly->DeleteCells();
    phantomLinesPoly->SetPoints(phantomLinesPoints);
    phantomLinesPoly->SetLines(lines);
    phantomObject->Modified();

    // Cleanup
    lines->Delete();
}

#include "pointsobject.h"
#include "landmarkregistrationobjectplugininterface.h"
#include "landmarkregistrationobject.h"

void USManualCalibrationPluginInterface::StartPhantomRegistration()
{
    const char * pointNames[4] = { "One", "Two", "Three", "Four" };
    double pointCoords[4][3] = { { 0, -17.5, 53 }, { 50, -17.5, 53 }, { 42, 112.5, 53 }, {  10, 80, 53 } };

    if( m_phantomWiresObjectId == IbisAPI::InvalidId )
        BuildWiresRepresentation();
    SceneObject * wiresObject = GetIbisAPI()->GetObjectByID(m_phantomWiresObjectId);

    if( m_calibrationPhantomObjectId == IbisAPI::InvalidId )
        BuildCalibrationPhantomRepresentation();
    SceneObject * phantomObject = GetIbisAPI()->GetObjectByID(m_calibrationPhantomObjectId);

    // Add source and target points to scene
    PointsObject * sourcePoints = PointsObject::SafeDownCast( GetIbisAPI()->GetObjectByID( m_phantomRegSourcePointsId ) );
    bool allocSource = false;
    if( !sourcePoints )
    {
        allocSource = true;
        sourcePoints = PointsObject::New();
        sourcePoints->SetName( "Phantom Source Points" );
        for( int i = 0; i < 4; ++i )
            sourcePoints->AddPoint( pointNames[i], pointCoords[i] );

    }

    PointsObject * targetPoints = PointsObject::SafeDownCast( GetIbisAPI()->GetObjectByID( m_phantomRegTargetPointsId ) );
    bool allocTarget = false;
    if( !targetPoints )
    {
        allocTarget = true;
        targetPoints = PointsObject::New();
        targetPoints->SetName( "Phantom Target Points" );
        for( int i = 0; i < 4; ++i )
            targetPoints->AddPoint( pointNames[i], pointCoords[i] );
    }

    // Setup data in landmark registration plugin
    ObjectPluginInterface * basePlugin = GetIbisAPI()->GetObjectPluginByName( "LandmarkRegistrationObject" );
    LandmarkRegistrationObjectPluginInterface * landmarkRegPlugin = LandmarkRegistrationObjectPluginInterface::SafeDownCast( basePlugin );
    Q_ASSERT( landmarkRegPlugin );
    LandmarkRegistrationObject *regObj = LandmarkRegistrationObject::SafeDownCast( landmarkRegPlugin->CreateObject() );
    m_landmarkRegistrationObjectId = regObj->GetObjectID();
    regObj->SetName("US Phantom Registration");
    Q_ASSERT( regObj );
    regObj->SetSourcePoints( sourcePoints );
    regObj->SetTargetPoints( targetPoints );
    m_phantomRegSourcePointsId = sourcePoints->GetObjectID();
    m_phantomRegTargetPointsId = targetPoints->GetObjectID();

    GetIbisAPI()->ChangeParent( wiresObject, regObj, 0 );
    GetIbisAPI()->ChangeParent( phantomObject, regObj, 0 );
    regObj->SelectPoint( 0 );
    GetIbisAPI()->SetCurrentObject( regObj );
    // cleanup
    if( allocSource )
        sourcePoints->Delete();
    if( allocTarget )
        targetPoints->Delete();
}

 const double * USManualCalibrationPluginInterface::GetPhantomPoint( int nIndex, int pointIndex )
 {
     return phantomPoints[m_currentPhantomSize][ nIndex * 4 + pointIndex ];
 }

 SceneObject * USManualCalibrationPluginInterface::GetPhantomWiresObject()
 {
     SceneObject * obj = GetIbisAPI()->GetObjectByID(m_phantomWiresObjectId);
     return obj;
 }

 void USManualCalibrationPluginInterface::SetPhatonSize(int index)
 {
     switch( index )
     {
        case 0:
            m_currentPhantomSize = PhantomSize::MEDIUMDEPTH;
            break;
        case 1:
            m_currentPhantomSize = PhantomSize::SHALLOWDEPTH;
            break;
        default:
            m_currentPhantomSize = PhantomSize::MEDIUMDEPTH;
     }
     UpdateWiresRepresentation();
 }