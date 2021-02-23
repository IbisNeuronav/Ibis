/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <QFileDialog>
#include <QMessageBox>
#include "polydataobject.h"
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkTransform.h>
#include <vtkAssembly.h>
#include <vtkPolyDataWriter.h>
#include <vtkImageData.h>
#include <vtkPNGReader.h>
#include <vtkDoubleArray.h>
#include <vtkPoints.h>

#include "view.h"
#include "vtkPiecewiseFunctionLookupTable.h"
#include "application.h"
#include "scenemanager.h"
#include "imageobject.h"
#include "vtkProbeFilter.h"
#include "vtkClipPolyData.h"
#include "vtkPassThrough.h"
#include "vtkCutter.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "lookuptablemanager.h"


#include "abstractpolydataobject.h"
#include "polydataobjectsettingsdialog.h"

ObjectSerializationMacro( AbstractPolyDataObject );


AbstractPolyDataObject::AbstractPolyDataObject()
{
    m_clippingSwitch = vtkSmartPointer<vtkPassThrough>::New();
    m_colorSwitch = vtkSmartPointer<vtkPassThrough>::New();

    m_referenceToPolyTransform = vtkSmartPointer<vtkTransform>::New();

    // Clip octant from polydata
    m_clippingOn = false;
    m_interacting = false;
    m_clippingPlanes = vtkSmartPointer<vtkPlanes>::New();
    m_clippingPlanes->SetTransform( m_referenceToPolyTransform );
    InitializeClippingPlanes();
    m_clipper = vtkSmartPointer<vtkClipPolyData>::New();
    m_clipper->SetClipFunction( m_clippingPlanes );

    // Cross section in 2d views
    for( int i = 0; i < 3; ++i )
    {
        m_cuttingPlane[i] = vtkSmartPointer<vtkPlane>::New();
        m_cuttingPlane[i]->SetTransform( m_referenceToPolyTransform );
        m_cutter[i] = vtkSmartPointer<vtkCutter>::New();
        m_cutter[i]->SetInputConnection( m_colorSwitch->GetOutputPort() );
        m_cutter[i]->SetCutFunction( m_cuttingPlane[i]);
    }
    m_cuttingPlane[0]->SetNormal( 1.0, 0.0, 0.0 );
    m_cuttingPlane[1]->SetNormal( 0.0, 1.0, 0.0 );
    m_cuttingPlane[2]->SetNormal( 0.0, 0.0, 1.0 );

    this->PolyData = 0;
    this->LutIndex = 0;
    this->renderingMode = VTK_SURFACE;
    this->ScalarsVisible = 0;
    this->VertexColorMode = 0;
    this->ScalarSourceObjectId = SceneManager::InvalidId;
    this->Property = vtkSmartPointer<vtkProperty>::New();
    this->CrossSectionVisible = false;

    m_2dProperty = vtkSmartPointer<vtkProperty>::New();
    m_2dProperty->SetAmbient( 1.0 );
    m_2dProperty->LightingOff();

    // Probe filter ( used to sample scalars from other dataset )
    this->ScalarSource = 0;
    this->LutBackup = vtkSmartPointer<vtkScalarsToColors>::New();
    this->ProbeFilter = vtkSmartPointer<vtkProbeFilter>::New();
    this->ProbeFilter->SetInputConnection( m_clippingSwitch->GetOutputPort() );
}

AbstractPolyDataObject::~AbstractPolyDataObject()
{
    if( this->PolyData )
        this->PolyData->UnRegister( this );
    if( this->ScalarSource )
        this->ScalarSource->UnRegister( this );
}

vtkPolyData * AbstractPolyDataObject::GetPolyData()
{
    return this->PolyData;
}

void AbstractPolyDataObject::SetPolyData(vtkPolyData *poly )
{
    if( poly == this->PolyData )
        return;

    if( this->PolyData )
        this->PolyData->UnRegister( this );
    this->PolyData = poly;
    if( this->PolyData )
    {
        this->PolyData->Register( this );
    }

    this->UpdatePipeline();

    emit ObjectModified();
}

void AbstractPolyDataObject::Serialize( Serializer * ser )
{
    double opacity = this->GetOpacity();
    double *objectColor = this->GetColor();
    int clippingPlaneOrientation[3] = { 1, 1, 1 };
    if(!ser->IsReader())
    {
        if( this->ScalarSource )
            this->ScalarSourceObjectId = this->ScalarSource->GetObjectID();
        else
            this->ScalarSourceObjectId = SceneManager::InvalidId;
        clippingPlaneOrientation[0] = GetClippingPlanesOrientation( 0 ) ? 1 : 0;
        clippingPlaneOrientation[1] = GetClippingPlanesOrientation( 1 ) ? 1 : 0;
        clippingPlaneOrientation[2] = GetClippingPlanesOrientation( 2 ) ? 1 : 0;
    }
    SceneObject::Serialize(ser);
    ::Serialize( ser, "RenderingMode", this->renderingMode );
    ::Serialize( ser, "LutIndex", this->LutIndex );
    ::Serialize( ser, "ScalarsVisible", this->ScalarsVisible );
    ::Serialize( ser, "VertexColorMode", this->VertexColorMode );
    ::Serialize( ser, "ScalarSourceObjectId", this->ScalarSourceObjectId );
    ::Serialize( ser, "Opacity", opacity );
    ::Serialize( ser, "ObjectColor", objectColor, 3 );
    ::Serialize( ser, "CrossSectionVisible", this->CrossSectionVisible );
    ::Serialize( ser, "ClippingEnabled", m_clippingOn );
    ::Serialize( ser, "ClippingPlanesOrientation", clippingPlaneOrientation, 3 );
    if( ser->IsReader() )
    {
        SetClippingPlanesOrientation( 0, clippingPlaneOrientation[0] == 1 ? true : false );
        SetClippingPlanesOrientation( 1, clippingPlaneOrientation[1] == 1 ? true : false );
        SetClippingPlanesOrientation( 2, clippingPlaneOrientation[2] == 1 ? true : false );
        this->SetOpacity( opacity );
        this->SetColor( objectColor );
        this->SetCrossSectionVisible( this->CrossSectionVisible );
    }
}

void AbstractPolyDataObject::SavePolyData( QString &fileName )
{
    vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
    writer->SetFileName( fileName.toUtf8().data() );
    writer->SetInputData(this->PolyData);
    writer->Update();
    writer->Write();
}

void AbstractPolyDataObject::Setup( View * view )
{
    SceneObject::Setup( view );

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetScalarVisibility( this->ScalarsVisible );
    mapper->UseLookupTableScalarRangeOn();  // make sure mapper doesn't try to modify our color table

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( mapper );
    actor->SetUserTransform( this->GetWorldTransform() );
    this->polydataObjectInstances[ view ] = actor;

    if( view->GetType() == THREED_VIEW_TYPE )
    {   
        actor->SetProperty( this->Property );
        mapper->SetInputConnection( m_colorSwitch->GetOutputPort() );
        actor->SetVisibility( this->ObjectHidden ? 0 : 1 );
        view->GetRenderer( this->RenderLayer )->AddActor( actor );
    }
    else
    {
        actor->SetProperty( m_2dProperty );
        int plane = (int)(view->GetType());
        mapper->SetInputConnection( m_cutter[plane]->GetOutputPort() );
        actor->SetVisibility( ( IsHidden() && GetCrossSectionVisible() ) ? 1 : 0 );
        view->GetOverlayRenderer()->AddActor( actor );
    }
}

void AbstractPolyDataObject::Release( View * view )
{
    SceneObject::Release( view );

    PolyDataObjectViewAssociation::iterator itAssociations = this->polydataObjectInstances.find( view );
    if( itAssociations != this->polydataObjectInstances.end() )
    {
        vtkSmartPointer<vtkActor> actor = (*itAssociations).second;
        if( view->GetType() == THREED_VIEW_TYPE )
            view->GetRenderer( this->RenderLayer )->RemoveViewProp( actor );
        else
            view->GetOverlayRenderer()->RemoveViewProp( actor );
        this->polydataObjectInstances.erase( itAssociations );
    }
}

void AbstractPolyDataObject::SetColor( double r, double g, double b )
{
    this->Property->SetColor( r, g, b );
    m_2dProperty->SetColor( r, g, b );
    emit ObjectModified();
}

double * AbstractPolyDataObject::GetColor()
{
    return this->Property->GetColor();
}

void AbstractPolyDataObject::SetLineWidth( double w )
{
    this->Property->SetLineWidth( w );
    m_2dProperty->SetLineWidth( w );
    emit ObjectModified();
}

void AbstractPolyDataObject::SetRenderingMode( int renderingMode )
{
    this->renderingMode = renderingMode;
    this->Property->SetRepresentation( this->renderingMode );
    m_2dProperty->SetRepresentation( this->renderingMode );
    emit ObjectModified();
}

void AbstractPolyDataObject::SetScalarsVisible( int use )
{
    this->ScalarsVisible = use;
    this->UpdatePipeline();
    emit ObjectModified();
}

void AbstractPolyDataObject::SetLutIndex( int index )
{
    this->LutIndex = index;
    if( this->CurrentLut )
    {
        this->CurrentLut = 0;
    }
    this->UpdatePipeline();
    emit ObjectModified();
}

void AbstractPolyDataObject::SetVertexColorMode( int mode )
{
    this->VertexColorMode = mode;
    this->UpdatePipeline();
    emit ObjectModified();
}

void AbstractPolyDataObject::SetOpacity( double opacity )
{
    Q_ASSERT( opacity >= 0.0 && opacity <= 1.0 );
    this->Property->SetOpacity( opacity );
    emit ObjectModified();
}

void AbstractPolyDataObject::UpdateSettingsWidget()
{
    emit ObjectViewChanged();
}

void AbstractPolyDataObject::SetCrossSectionVisible( bool showCrossSection )
{
    this->CrossSectionVisible = showCrossSection;
    if( IsHidden() )
        return;

    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        View * v = (*it).first;
        vtkSmartPointer<vtkActor> actor = (*it).second;
        if( v->GetType() != THREED_VIEW_TYPE )
            actor->SetVisibility( showCrossSection ? 1 : 0 );
    }
    emit ObjectModified();
}

void AbstractPolyDataObject::SetClippingEnabled( bool e )
{
    m_clippingOn = e;
    this->UpdatePipeline();
    emit ObjectModified();
}

void AbstractPolyDataObject::SetClippingPlanesOrientation( int plane, bool positive )
{
    Q_ASSERT( plane >= 0 && plane < 3 );
    vtkDoubleArray * normals = vtkDoubleArray::SafeDownCast( m_clippingPlanes->GetNormals() );
    double tuple[3] = { 0.0, 0.0, 0.0 };
    tuple[ plane ] = positive ? -1.0 : 1.0;
    normals->SetTuple( plane, tuple );
    m_clippingPlanes->Modified();
    emit ObjectModified();
}

bool AbstractPolyDataObject::GetClippingPlanesOrientation( int plane )
{
    Q_ASSERT( plane >= 0 && plane < 3 );
    vtkDoubleArray * normals = vtkDoubleArray::SafeDownCast( m_clippingPlanes->GetNormals() );
    double * tuple = normals->GetTuple3( plane );
    return tuple[plane] > 0.0 ? false : true;
}

void AbstractPolyDataObject::SetScalarSource( ImageObject * im )
{
    if( this->ScalarSource == im )
        return;
    if( this->ScalarSource )
    {
        this->ScalarSource->UnRegister( this );
        disconnect( this->ScalarSource, SIGNAL(RemovingFromScene()), this, SLOT(OnScalarSourceDeleted()) );
        disconnect( this->ScalarSource, SIGNAL(ObjectModified()), this, SLOT(OnScalarSourceModified()) );
    }
    this->ScalarSource = im;
    if( this->ScalarSource )
    {
        this->ScalarSource->Register( this );
        this->ProbeFilter->SetSourceData( this->ScalarSource->GetImage() );
        this->LutBackup->DeepCopy( this->ScalarSource->GetLut() );
        connect( this->ScalarSource, SIGNAL(RemovingFromScene()), this, SLOT(OnScalarSourceDeleted()) );
        connect( this->ScalarSource, SIGNAL(ObjectModified()), this, SLOT(OnScalarSourceModified()) );
    }
    else
    {
        this->ProbeFilter->SetSourceData( 0 );
    }
    this->UpdatePipeline();
    emit ObjectModified();
}

void AbstractPolyDataObject::OnStartCursorInteraction()
{
    m_interacting = true;
}

void AbstractPolyDataObject::OnEndCursorInteraction()
{
    m_interacting = false;
    this->UpdateClippingPlanes();
}

void AbstractPolyDataObject::OnCursorPositionChanged()
{
    this->UpdateCuttingPlane();
    if( !m_interacting )
        this->UpdateClippingPlanes();
}

void AbstractPolyDataObject::OnReferenceChanged()
{
    this->UpdateClippingPlanes();
}

void AbstractPolyDataObject::OnScalarSourceDeleted()
{
    this->SetScalarSource( 0 );
}

void AbstractPolyDataObject::OnScalarSourceModified()
{
    vtkScalarsToColors * newLut = this->ScalarSource->GetLut();
    if( this->LutBackup != newLut )
    {
        this->LutBackup->DeepCopy( newLut );
        this->UpdatePipeline();
    }
    emit ObjectModified();
}

void AbstractPolyDataObject::Hide()
{
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        vtkSmartPointer<vtkActor> actor = (*it).second;
        actor->VisibilityOff();
    }
    emit ObjectModified();
}

void AbstractPolyDataObject::Show()
{
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        View * v = (*it).first;
        vtkSmartPointer<vtkActor> actor = (*it).second;
        if( v->GetType() == THREED_VIEW_TYPE || GetCrossSectionVisible() )
            actor->VisibilityOn();
    }
    emit ObjectModified();
}

void AbstractPolyDataObject::ObjectAddedToScene()
{
    connect( GetManager(), SIGNAL(ReferenceObjectChanged()), this, SLOT(OnReferenceChanged()) );
    connect( GetManager(), SIGNAL(ReferenceTransformChanged()), this, SLOT(OnReferenceChanged()) );
    connect( GetManager(), SIGNAL(CursorPositionChanged()), this, SLOT(OnCursorPositionChanged()) );
    connect( GetManager(), SIGNAL(StartCursorInteraction()), this, SLOT(OnStartCursorInteraction()) );
    connect( GetManager(), SIGNAL(EndCursorInteraction()), this, SLOT(OnEndCursorInteraction()) );
    m_referenceToPolyTransform->Identity();
    m_referenceToPolyTransform->Concatenate( GetWorldTransform() );
    m_referenceToPolyTransform->Concatenate( GetManager()->GetInverseReferenceTransform() );
    UpdateCuttingPlane();
    UpdateClippingPlanes();
}

void AbstractPolyDataObject::ObjectAboutToBeRemovedFromScene()
{
    m_referenceToPolyTransform->Identity();
    disconnect( GetManager(), SIGNAL(ReferenceObjectChanged()), this, SLOT(OnReferenceChanged()) );
    disconnect( GetManager(), SIGNAL(ReferenceTransformChanged()), this, SLOT(OnReferenceChanged()) );
    disconnect( GetManager(), SIGNAL(CursorPositionChanged()), this, SLOT(OnCursorPositionChanged()) );
    disconnect( GetManager(), SIGNAL(StartCursorInteraction()), this, SLOT(OnStartCursorInteraction()) );
    disconnect( GetManager(), SIGNAL(EndCursorInteraction()), this, SLOT(OnEndCursorInteraction()) );
}

void AbstractPolyDataObject::InternalPostSceneRead()
{
    Q_ASSERT( this->GetManager() );

    // reconnect to the image object from which scalars are computed
    if( this->ScalarSourceObjectId != SceneManager::InvalidId )
    {
        SceneObject * scalarObj = this->GetManager()->GetObjectByID( this->ScalarSourceObjectId );
        ImageObject * scalarImageObj = ImageObject::SafeDownCast( scalarObj );
        this->SetScalarSource( scalarImageObj );
    }
    emit ObjectViewChanged();
}

void AbstractPolyDataObject::UpdateClippingPlanes()
{
    double pos[3] = { 0.0, 0.0, 0.0 };
    GetManager()->GetCursorPosition( pos );
    vtkPoints * origins = m_clippingPlanes->GetPoints();
    origins->SetPoint( 0, pos[0], 0.0, 0.0 );
    origins->SetPoint( 1, 0.0, pos[1], 0.0 );
    origins->SetPoint( 2, 0.0, 0.0, pos[2] );
    m_clippingPlanes->Modified();
    emit ObjectModified();
}

void AbstractPolyDataObject::UpdateCuttingPlane()
{
    double cursorPos[3] = { 0.0, 0.0, 0.0 };
    GetManager()->GetCursorPosition( cursorPos );
    m_cuttingPlane[0]->SetOrigin( cursorPos[0], 0.0, 0.0 );
    m_cuttingPlane[1]->SetOrigin( 0.0, cursorPos[1], 0.0 );
    m_cuttingPlane[2]->SetOrigin( 0.0, 0.0, cursorPos[2] );
    emit ObjectModified();
}


vtkScalarsToColors * AbstractPolyDataObject::GetCurrentLut()
{
    // No LUT
    if( this->LutIndex == -1 )  // Don't try to control the lookup table, use default
        return 0;

    // LUT already exists
    if( this->CurrentLut )
        return this->CurrentLut;

    // Create a new LUT if needed
    Q_ASSERT( this->GetManager() );
    Q_ASSERT( this->LutIndex < Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables() );
    Q_ASSERT( this->PolyData );
    QString tableName = Application::GetLookupTableManager()->GetTemplateLookupTableName( this->LutIndex );
    vtkSmartPointer<vtkPiecewiseFunctionLookupTable> lut = vtkSmartPointer<vtkPiecewiseFunctionLookupTable>::New();
    double range[2];
    this->PolyData->GetScalarRange( range );
    Application::GetLookupTableManager()->CreateLookupTable( tableName, range, lut );
    this->CurrentLut = lut;
    return this->CurrentLut;
}

void AbstractPolyDataObject::InitializeClippingPlanes()
{
    vtkSmartPointer<vtkDoubleArray> clipPlaneNormals = vtkSmartPointer<vtkDoubleArray>::New();
    clipPlaneNormals->SetNumberOfComponents( 3 );
    clipPlaneNormals->SetNumberOfTuples(3);
    clipPlaneNormals->SetTuple3( 0, -1.0, 0.0, 0.0 );
    clipPlaneNormals->SetTuple3( 1, 0.0, -1.0, 0.0 );
    clipPlaneNormals->SetTuple3( 2, 0.0, 0.0, -1.0 );
    m_clippingPlanes->SetNormals( clipPlaneNormals );

    vtkSmartPointer<vtkPoints> p = vtkSmartPointer<vtkPoints>::New();
    p->SetNumberOfPoints( 3 );
    p->SetPoint( 0, 0.0, 0.0, 0.0 );
    p->SetPoint( 1, 0.0, 0.0, 0.0 );
    p->SetPoint( 2, 0.0, 0.0, 0.0 );
    m_clippingPlanes->SetPoints( p );
}
