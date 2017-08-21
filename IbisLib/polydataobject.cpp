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
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "view.h"
#include "vtkTransform.h"
#include "vtkAssembly.h"
#include "vtkPolyDataWriter.h"
#include "vtkImageData.h"
#include "vtkPNGReader.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"
#include "vtkPiecewiseFunctionLookupTable.h"

#include "vtkTextureMapToPlane.h"
#include "vtkTextureMapToSphere.h"
#include "vtkTextureMapToCylinder.h"

#include "polydataobjectsettingsdialog.h"
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

ObjectSerializationMacro( PolyDataObject );

vtkSmartPointer<vtkImageData> PolyDataObject::checkerBoardTexture;

PolyDataObject::PolyDataObject()
{
    m_clippingSwitch = vtkSmartPointer<vtkPassThrough>::New();
    m_colorSwitch = vtkSmartPointer<vtkPassThrough>::New();

    m_referenceToPolyTransform = vtkSmartPointer<vtkTransform>::New();

    // Clip octant from polydata
    m_clippingOn = false;
    m_interacting = false;
    m_clippingPlanes = vtkSmartPointer<vtkPlanes>::New();
    m_clippingPlanes->SetTransform( m_referenceToPolyTransform.GetPointer() );
    InitializeClippingPlanes();
    m_clipper = vtkSmartPointer<vtkClipPolyData>::New();
    m_clipper->SetClipFunction( m_clippingPlanes.GetPointer() );

    // Cross section in 2d views
    for( int i = 0; i < 3; ++i )
    {
        m_cuttingPlane[i] = vtkSmartPointer<vtkPlane>::New();
        m_cuttingPlane[i]->SetTransform( m_referenceToPolyTransform.GetPointer() );
        m_cutter[i] = vtkSmartPointer<vtkCutter>::New();
        m_cutter[i]->SetInputConnection( m_colorSwitch->GetOutputPort() );
        m_cutter[i]->SetCutFunction( m_cuttingPlane[i].GetPointer());
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
    this->showTexture = false;
    this->Texture = 0;

    m_2dProperty = vtkSmartPointer<vtkProperty>::New();
    m_2dProperty->SetAmbient( 1.0 );
    m_2dProperty->LightingOff();

    // Texture map filter
    vtkSmartPointer<vtkTextureMapToSphere> map = vtkSmartPointer<vtkTextureMapToSphere>::New();
    map->AutomaticSphereGenerationOn();
    map->PreventSeamOn();
    map->SetInputConnection( m_clippingSwitch->GetOutputPort() );
    this->TextureMap = map;

    // Probe filter ( used to sample scalars from other dataset )
    this->ScalarSource = 0;
    this->LutBackup = vtkSmartPointer<vtkScalarsToColors>::New();
    this->ProbeFilter = vtkSmartPointer<vtkProbeFilter>::New();
    this->ProbeFilter->SetInputConnection( m_clippingSwitch->GetOutputPort() );
}

PolyDataObject::~PolyDataObject()
{
    if( this->PolyData )
        this->PolyData->UnRegister( this );
    if( this->ScalarSource )
        this->ScalarSource->UnRegister( this );
    if( this->Texture )
        this->Texture->UnRegister( this );
}

vtkPolyData * PolyDataObject::GetPolyData()
{
    return PolyData;
}

void PolyDataObject::SetPolyData(vtkPolyData *poly )
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

    UpdatePipeline();

    this->Modified();
}

void PolyDataObject::Serialize( Serializer * ser )
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
    ::Serialize( ser, "ShowTexture", this->showTexture );
    ::Serialize( ser, "TextureFileName", this->textureFileName );
    if( ser->IsReader() )
    {
        SetClippingPlanesOrientation( 0, clippingPlaneOrientation[0] == 1 ? true : false );
        SetClippingPlanesOrientation( 1, clippingPlaneOrientation[1] == 1 ? true : false );
        SetClippingPlanesOrientation( 2, clippingPlaneOrientation[2] == 1 ? true : false );
        this->SetOpacity( opacity );
        this->SetColor( objectColor );
        this->SetCrossSectionVisible( this->CrossSectionVisible );
        this->SetRenderingMode( this->renderingMode );
        this->SetShowTexture( this->showTexture );
    }
}

void PolyDataObject::Export()
{
    Q_ASSERT( this->GetManager() );
    QString surfaceName(this->Name);
    surfaceName.append(".vtk");
    this->SetDataFileName(surfaceName);
    QString fullName(this->GetManager()->GetSceneDirectory());
    fullName.append("/");
    fullName.append(surfaceName);
    QString saveName = Application::GetInstance().GetSaveFileName( tr("Save Object"), fullName, tr("*.vtk") );
    if(saveName.isEmpty())
        return;
    if (QFile::exists(saveName))
    {
        int ret = QMessageBox::warning(0, tr("Save PolyDataObject"), saveName,
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No | QMessageBox::Escape);
        if (ret == QMessageBox::No)
            return;
    }
    this->SavePolyData( saveName );
}

void PolyDataObject::SavePolyData( QString &fileName )
{
    vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
    writer->SetFileName( fileName.toUtf8().data() );
    writer->SetInputData(this->PolyData);
    writer->Update();
    writer->Write();
}

void PolyDataObject::Setup( View * view )
{
    SceneObject::Setup( view );

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetScalarVisibility( this->ScalarsVisible );
    mapper->UseLookupTableScalarRangeOn();  // make sure mapper doesn't try to modify our color table

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( mapper.GetPointer() );
    actor->SetUserTransform( this->WorldTransform );
    this->polydataObjectInstances[ view ] = actor;

    if( view->GetType() == THREED_VIEW_TYPE )
    {   
        actor->SetProperty( this->Property.GetPointer() );
        mapper->SetInputConnection( m_colorSwitch->GetOutputPort() );
        actor->SetVisibility( this->ObjectHidden ? 0 : 1 );
        view->GetRenderer( this->RenderLayer )->AddActor( actor.GetPointer() );
    }
    else
    {
        actor->SetProperty( m_2dProperty.GetPointer() );
        int plane = (int)(view->GetType());
        mapper->SetInputConnection( m_cutter[plane]->GetOutputPort() );
        actor->SetVisibility( ( IsHidden() && GetCrossSectionVisible() ) ? 1 : 0 );
        view->GetOverlayRenderer()->AddActor( actor.GetPointer() );
    }
}

void PolyDataObject::Release( View * view )
{
    SceneObject::Release( view );

    PolyDataObjectViewAssociation::iterator itAssociations = this->polydataObjectInstances.find( view );
    if( itAssociations != this->polydataObjectInstances.end() )
    {
        vtkSmartPointer<vtkActor> actor = (*itAssociations).second;
        if( view->GetType() == THREED_VIEW_TYPE )
            view->GetRenderer( this->RenderLayer )->RemoveViewProp( actor.GetPointer() );
        else
            view->GetOverlayRenderer()->RemoveViewProp( actor.GetPointer() );
        this->polydataObjectInstances.erase( itAssociations );
    }
}

void PolyDataObject::SetColor( double r, double g, double b )
{
    this->Property->SetColor( r, g, b );
    m_2dProperty->SetColor( r, g, b );
    emit Modified();
}

double * PolyDataObject::GetColor()
{
    return this->Property->GetColor();
}

void PolyDataObject::SetLineWidth( double w )
{
    this->Property->SetLineWidth( w );
    m_2dProperty->SetLineWidth( w );
    emit Modified();
}

void PolyDataObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets)
{
    PolyDataObjectSettingsDialog * res = new PolyDataObjectSettingsDialog( parent );
    res->setAttribute(Qt::WA_DeleteOnClose);
    res->SetPolyDataObject( this );
    res->setObjectName("Properties");
    connect( this, SIGNAL( ObjectViewChanged() ), res, SLOT(UpdateSettings()) );
    widgets->append(res);
}

void PolyDataObject::SetRenderingMode( int renderingMode )
{
    this->renderingMode = renderingMode;
    this->Property->SetRepresentation( this->renderingMode );
    m_2dProperty->SetRepresentation( this->renderingMode );
    emit Modified();
}

void PolyDataObject::SetScalarsVisible( int use )
{
    this->ScalarsVisible = use;
    this->UpdatePipeline();
    emit Modified();
}

void PolyDataObject::SetLutIndex( int index )
{
    this->LutIndex = index;
    if( this->CurrentLut )
    {
        this->CurrentLut = 0;
    }
    UpdatePipeline();
    emit Modified();
}

void PolyDataObject::SetVertexColorMode( int mode )
{
    this->VertexColorMode = mode;
    UpdatePipeline();
    emit Modified();
}

void PolyDataObject::SetOpacity( double opacity )
{
    Q_ASSERT( opacity >= 0.0 && opacity <= 1.0 );
    this->Property->SetOpacity( opacity );
    emit Modified();
}

void PolyDataObject::UpdateSettingsWidget()
{
    emit ObjectViewChanged();
}

void PolyDataObject::SetCrossSectionVisible( bool showCrossSection )
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
    emit Modified();
}

void PolyDataObject::SetClippingEnabled( bool e )
{
    m_clippingOn = e;
    this->UpdatePipeline();
    emit Modified();
}

void PolyDataObject::SetClippingPlanesOrientation( int plane, bool positive )
{
    Q_ASSERT( plane >= 0 && plane < 3 );
    vtkDoubleArray * normals = vtkDoubleArray::SafeDownCast( m_clippingPlanes->GetNormals() );
    double tuple[3] = { 0.0, 0.0, 0.0 };
    tuple[ plane ] = positive ? -1.0 : 1.0;
    normals->SetTuple( plane, tuple );
    m_clippingPlanes->Modified();
    emit Modified();
}

bool PolyDataObject::GetClippingPlanesOrientation( int plane )
{
    Q_ASSERT( plane >= 0 && plane < 3 );
    vtkDoubleArray * normals = vtkDoubleArray::SafeDownCast( m_clippingPlanes->GetNormals() );
    double * tuple = normals->GetTuple3( plane );
    return tuple[plane] > 0.0 ? false : true;
}

void PolyDataObject::SetTexture( vtkImageData * texImage )
{
    // standard set code
    if( this->Texture )
        this->Texture->UnRegister( this );
    this->Texture = texImage;
    if( this->Texture )
        this->Texture->Register( this );

    // update texture in actor
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        View * view = (*it).first;
        if( view->GetType() == THREED_VIEW_TYPE )
        {
            vtkSmartPointer<vtkActor> actor = (*it).second;
            vtkTexture * tex = actor->GetTexture();
            if( tex )
            {
                if( this->Texture )
                    tex->SetInputData( this->Texture );
                else
                    tex->SetInputData( this->checkerBoardTexture.GetPointer() );
            }
        }
    }

    emit Modified();
}

void PolyDataObject::SetShowTexture( bool show )
{
    this->showTexture = show;

    if( show )
    {
        // Create the default texture if needed
        if( !this->Texture && !this->checkerBoardTexture )
        {
            int size = 200;
            int squareSize = 5;
            this->checkerBoardTexture = vtkSmartPointer<vtkImageData>::New();
            this->checkerBoardTexture->SetDimensions( size, size, 1 );
            this->checkerBoardTexture->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
            unsigned char * row = (unsigned char*) this->checkerBoardTexture->GetScalarPointer();
            for( int y = 0; y < size; ++y )
            {
                unsigned char * col = row;
                for( int x = 0; x < size; ++x )
                {
                    bool xOn = ( x / squareSize % 2 ) == 0;
                    bool yOn = ( y / squareSize % 2 ) == 0;
                    if( xOn == yOn )
                        *col = 255;
                    else
                        *col = 160;
                    ++col;
                }
                row += size;
            }
        }

        vtkImageData * texture = this->Texture;
        if( !texture )
            texture = this->checkerBoardTexture.GetPointer();

        // Add a vtkTexture to each actor to map the checker board onto object
        View * view = 0;
        vtkSmartPointer<vtkActor> actor;
        PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
        for( ; it != this->polydataObjectInstances.end(); ++it )
        {
            view = (*it).first;
            if( view->GetType() == THREED_VIEW_TYPE )
            {
                actor = (*it).second;
                vtkTexture * tex = vtkTexture::New();
                tex->SetBlendingMode( vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE );
                tex->InterpolateOff();
                tex->RepeatOn();
                tex->SetInputData( texture );
                actor->SetTexture( tex );
                tex->Delete();
            }
        }
    }
    else
    {
        // remove texture from each of the actors
        View * view = 0;
        vtkSmartPointer<vtkActor> actor;
        PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
        for( ; it != this->polydataObjectInstances.end(); ++it )
        {
            view = (*it).first;
            if( view->GetType() == THREED_VIEW_TYPE )
            {
                actor = (*it).second;
                actor->SetTexture( 0 );
            }
        }
    }
    this->UpdatePipeline();
    emit Modified();
}

void PolyDataObject::SetTextureFileName( QString filename )
{
    if( QFile::exists( filename ) )
    {
        vtkSmartPointer<vtkPNGReader> reader = vtkSmartPointer<vtkPNGReader>::New();
        reader->SetFileName( filename.toUtf8().data() );
        reader->Update();
        this->SetTexture( reader->GetOutput() );
        this->textureFileName = filename;
    }
    else
    {
        this->textureFileName = "";
        this->SetTexture( 0 );
    }
}


void PolyDataObject::SetScalarSource( ImageObject * im )
{
    if( this->ScalarSource == im )
        return;
    if( this->ScalarSource )
    {
        this->ScalarSource->UnRegister( this );
        disconnect( this->ScalarSource, SIGNAL(RemovingFromScene()), this, SLOT(OnScalarSourceDeleted()) );
        disconnect( this->ScalarSource, SIGNAL(Modified()), this, SLOT(OnScalarSourceModified()) );
    }
    this->ScalarSource = im;
    if( this->ScalarSource )
    {
        this->ScalarSource->Register( this );
        this->ProbeFilter->SetSourceData( this->ScalarSource->GetImage() );
        this->LutBackup->DeepCopy( this->ScalarSource->GetLut() );
        connect( this->ScalarSource, SIGNAL(RemovingFromScene()), this, SLOT(OnScalarSourceDeleted()) );
        connect( this->ScalarSource, SIGNAL(Modified()), this, SLOT(OnScalarSourceModified()) );
    }
    else
    {
        this->ProbeFilter->SetSourceData( 0 );
    }
    UpdatePipeline();
    emit Modified();
}

void PolyDataObject::OnStartCursorInteraction()
{
    m_interacting = true;
}

void PolyDataObject::OnEndCursorInteraction()
{
    m_interacting = false;
    this->UpdateClippingPlanes();
}

void PolyDataObject::OnCursorPositionChanged()
{
    this->UpdateCuttingPlane();
    if( !m_interacting )
        this->UpdateClippingPlanes();
}

void PolyDataObject::OnReferenceChanged()
{
    this->UpdateClippingPlanes();
}

void PolyDataObject::OnScalarSourceDeleted()
{
    this->SetScalarSource( 0 );
}

void PolyDataObject::OnScalarSourceModified()
{
    vtkScalarsToColors * newLut = this->ScalarSource->GetLut();
    if( this->LutBackup.GetPointer() != newLut )
    {
        this->LutBackup->DeepCopy( newLut );
        UpdatePipeline();
    }
    emit Modified();
}

void PolyDataObject::Hide()
{
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        vtkSmartPointer<vtkActor> actor = (*it).second;
        actor->VisibilityOff();
    }
    emit Modified();
}

void PolyDataObject::Show()
{
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        View * v = (*it).first;
        vtkSmartPointer<vtkActor> actor = (*it).second;
        if( v->GetType() == THREED_VIEW_TYPE || GetCrossSectionVisible() )
            actor->VisibilityOn();
    }
    emit Modified();
}

void PolyDataObject::ObjectAddedToScene()
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

void PolyDataObject::ObjectAboutToBeRemovedFromScene()
{
    m_referenceToPolyTransform->Identity();
    disconnect( GetManager(), SIGNAL(ReferenceObjectChanged()), this, SLOT(OnReferenceChanged()) );
    disconnect( GetManager(), SIGNAL(ReferenceTransformChanged()), this, SLOT(OnReferenceChanged()) );
    disconnect( GetManager(), SIGNAL(CursorPositionChanged()), this, SLOT(OnCursorPositionChanged()) );
    disconnect( GetManager(), SIGNAL(StartCursorInteraction()), this, SLOT(OnStartCursorInteraction()) );
    disconnect( GetManager(), SIGNAL(EndCursorInteraction()), this, SLOT(OnEndCursorInteraction()) );
}

void PolyDataObject::InternalPostSceneRead()
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

void PolyDataObject::UpdateClippingPlanes()
{
    double pos[3] = { 0.0, 0.0, 0.0 };
    GetManager()->GetCursorPosition( pos );
    vtkPoints * origins = m_clippingPlanes->GetPoints();
    origins->SetPoint( 0, pos[0], 0.0, 0.0 );
    origins->SetPoint( 1, 0.0, pos[1], 0.0 );
    origins->SetPoint( 2, 0.0, 0.0, pos[2] );
    m_clippingPlanes->Modified();
    emit Modified();
}

void PolyDataObject::UpdateCuttingPlane()
{
    double cursorPos[3] = { 0.0, 0.0, 0.0 };
    GetManager()->GetCursorPosition( cursorPos );
    m_cuttingPlane[0]->SetOrigin( cursorPos[0], 0.0, 0.0 );
    m_cuttingPlane[1]->SetOrigin( 0.0, cursorPos[1], 0.0 );
    m_cuttingPlane[2]->SetOrigin( 0.0, 0.0, cursorPos[2] );
    emit Modified();
}

void PolyDataObject::UpdatePipeline()
{
    if( !this->PolyData )
        return;

    m_clipper->SetInputData( this->PolyData );
    if( IsClippingEnabled() )
        m_clippingSwitch->SetInputConnection( m_clipper->GetOutputPort() );
    else
        m_clippingSwitch->SetInputData( this->PolyData );

    this->ProbeFilter->SetInputConnection( m_clippingSwitch->GetOutputPort() );
    this->TextureMap->SetInputConnection( m_clippingSwitch->GetOutputPort() );

    m_colorSwitch->SetInputConnection( m_clippingSwitch->GetOutputPort() );
    if( this->ScalarsVisible )
    {
        if( this->VertexColorMode == 1 && this->ScalarSource )
            m_colorSwitch->SetInputConnection( this->ProbeFilter->GetOutputPort() );
        else if( this->showTexture )
            m_colorSwitch->SetInputConnection( this->TextureMap->GetOutputPort() );
    }

    // Update mappers
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    while( it != this->polydataObjectInstances.end() )
    {
        vtkSmartPointer<vtkActor> actor = (*it).second;
        vtkMapper * mapper = actor->GetMapper();
        mapper->SetScalarVisibility( this->ScalarsVisible );
        if( this->VertexColorMode == 1 && this->ScalarSource )
            mapper->SetLookupTable( this->ScalarSource->GetLut() );
        else if ( this->CurrentLut )
            mapper->SetLookupTable( this->CurrentLut.GetPointer() );
        ++it;
    }
}

vtkScalarsToColors * PolyDataObject::GetCurrentLut()
{
    // No LUT
    if( this->LutIndex == -1 )  // Don't try to control the lookup table, use default
        return 0;

    // LUT already exists
    if( this->CurrentLut )
        return this->CurrentLut.GetPointer();

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
    return this->CurrentLut.GetPointer();
}

void PolyDataObject::InitializeClippingPlanes()
{
    vtkSmartPointer<vtkDoubleArray> clipPlaneNormals = vtkSmartPointer<vtkDoubleArray>::New();
    clipPlaneNormals->SetNumberOfComponents( 3 );
    clipPlaneNormals->SetNumberOfTuples(3);
    clipPlaneNormals->SetTuple3( 0, -1.0, 0.0, 0.0 );
    clipPlaneNormals->SetTuple3( 1, 0.0, -1.0, 0.0 );
    clipPlaneNormals->SetTuple3( 2, 0.0, 0.0, -1.0 );
    m_clippingPlanes->SetNormals( clipPlaneNormals.GetPointer() );

    vtkSmartPointer<vtkPoints> p = vtkSmartPointer<vtkPoints>::New();
    p->SetNumberOfPoints( 3 );
    p->SetPoint( 0, 0.0, 0.0, 0.0 );
    p->SetPoint( 1, 0.0, 0.0, 0.0 );
    p->SetPoint( 2, 0.0, 0.0, 0.0 );
    m_clippingPlanes->SetPoints( p );
}
