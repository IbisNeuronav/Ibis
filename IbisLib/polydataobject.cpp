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
#include "vtkProperty.h"
#include "vtkPolyDataWriter.h"
#include "vtkImageData.h"
#include "vtkPNGReader.h"

#include "vtkTextureMapToPlane.h"
#include "vtkTextureMapToSphere.h"
#include "vtkTextureMapToCylinder.h"

#include "polydataobjectsettingsdialog.h"
#include "application.h"
#include "scenemanager.h"
#include "triplecutplaneobject.h"
#include "imageobject.h"
#include "surfacecuttingplane.h"
#include "polydataclipper.h"
#include "vtkProbeFilter.h"
#include "octants.h"
#include "lookuptablemanager.h"

vtkCxxSetObjectMacro(PolyDataObject, Property, vtkProperty);
ObjectSerializationMacro( PolyDataObject );

vtkImageData * PolyDataObject::checkerBoardTexture = 0;

PolyDataObject::PolyDataObject()
{
    this->PolyData = 0;
    this->LutIndex = 0;
    this->CurrentLut = 0;
    this->renderingMode = VTK_SURFACE;
    this->ScalarsVisible = 0;
    this->VertexColorMode = 0;
    this->ScalarSourceObjectId = SceneObject::InvalidObjectId;
    this->opacity = 1.0;
    this->Property = vtkProperty::New();
    for (int i = 0; i< 3; i++)
       objectColor[i] = 1.0;
    this->SurfaceCutter = 0;
    this->CrossSectionVisible = false;
    this->Clipper = 0;
    this->SurfaceRemovedOctantNumber = -1;
    this->showTexture = false;
    this->Texture = 0;

    // Texture map filter
    vtkTextureMapToSphere * map = vtkTextureMapToSphere::New();
    map->AutomaticSphereGenerationOn();
    map->PreventSeamOn();
    this->TextureMap = map;

    // Probe filter ( used to sample scalars from other dataset )
    this->ScalarSource = 0;
    this->LutBackup = 0;
    this->ProbeFilter = vtkProbeFilter::New();
    this->ProbeFilter->SetInputConnection( map->GetOutputPort() );
}

PolyDataObject::~PolyDataObject()
{
    if( this->PolyData )
    {
        this->PolyData->UnRegister( this );
    }
    this->Property->Delete();
    if (this->SurfaceCutter)
        this->SurfaceCutter->Delete();
    if (this->Clipper)
    {
        disconnect(this->GetManager()->GetMainImagePlanes(), SIGNAL(EndPlaneMove(int)), this, SLOT(UpdateOctant(int)));
        this->Clipper->RemoveClipActor();
        this->Clipper->Delete();
    }
    if( this->Texture )
        this->Texture->UnRegister( this );
    this->TextureMap->Delete();
    this->ProbeFilter->Delete();
}

void PolyDataObject::SetPolyData( vtkPolyData * poly )
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
    this->TextureMap->SetInputData( this->PolyData );

    UpdateMapperPipeline();

    this->Modified();
}

void PolyDataObject::Serialize( Serializer * ser )
{
    if(!ser->IsReader())
    {
        if( this->ScalarSource )
            this->ScalarSourceObjectId = this->ScalarSource->GetObjectID();
        else
            this->ScalarSourceObjectId = SceneObject::InvalidObjectId;
    }
    SceneObject::Serialize(ser);
    ::Serialize( ser, "RenderingMode", this->renderingMode );
    ::Serialize( ser, "LutIndex", this->LutIndex );
    ::Serialize( ser, "ScalarsVisible", this->ScalarsVisible );
    ::Serialize( ser, "VertexColorMode", this->VertexColorMode );
    ::Serialize( ser, "ScalarSourceObjectId", this->ScalarSourceObjectId );
    ::Serialize( ser, "Opacity", this->opacity );
    ::Serialize( ser, "ObjectColor", this->objectColor, 3 );
    ::Serialize( ser, "CrossSectionVisible", this->CrossSectionVisible );
    ::Serialize( ser, "SurfaceRemovedOctantNumber", this->SurfaceRemovedOctantNumber );
    ::Serialize( ser, "ShowTexture", this->showTexture );
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
    this->SetFullFileName(saveName);
    vtkPolyDataWriter *writer1 = vtkPolyDataWriter::New();
    writer1->SetFileName( saveName.toUtf8().data() );
    writer1->SetInputData(this->PolyData);
    writer1->Update();
    writer1->Write();
    writer1->Delete();
}

bool PolyDataObject::Setup( View * view )
{
	bool success = SceneObject::Setup( view );
	if( !success )
		return false;

    this->SetRenderingMode(this->renderingMode);
    this->SetColor(this->objectColor);
    this->SetScalarsVisible(this->ScalarsVisible);
    this->SetOpacity(this->opacity);
    this->SetShowTexture(this->showTexture);
    if (this->SurfaceRemovedOctantNumber > -1)
        this->RemoveSurfaceFromOctant(this->SurfaceRemovedOctantNumber, true);
    this->UpdateSettingsWidget();
    if( view->GetType() == THREED_VIEW_TYPE )
    {   
        vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
        mapper->SetScalarVisibility( this->ScalarsVisible );
        mapper->UseLookupTableScalarRangeOn();  // make sure mapper doesn't try to modify our color table

		vtkActor * actor = vtkActor::New();
        actor->SetMapper( mapper );
		actor->SetProperty( this->Property );
		actor->SetUserTransform( this->WorldTransform );
        actor->SetVisibility( this->ObjectHidden ? 0 : 1 );

        view->GetRenderer( this->RenderLayer )->AddActor( actor );
        this->polydataObjectInstances[ view ] = actor;

		mapper->Delete();

        UpdateMapperPipeline();
        
        connect( this, SIGNAL( Modified() ), view, SLOT( NotifyNeedRender() ) );
        this->GetProperty()->GetColor(this->objectColor);
    }
    else
    {
        this->ShowCrossSection( this->CrossSectionVisible );
    }

	return true;
}

void PolyDataObject::SetColor(double color[3])
{
    for (int i = 0; i < 3; i++)
        this->objectColor[i] = color[i];
    this->Property->SetColor(this->objectColor);
    if (this->SurfaceCutter)
        this->SurfaceCutter->SetPropertyColor(this->objectColor);
    emit Modified();
}

void PolyDataObject::SetLineWidth( double w )
{
    this->Property->SetLineWidth( w );
    emit Modified();
}

bool PolyDataObject::Release( View * view )
{
	bool success = SceneObject::Release( view );
	if( !success )
		return false;

    if( view->GetType() == THREED_VIEW_TYPE )
    {
        this->disconnect( view );
        
        PolyDataObjectViewAssociation::iterator itAssociations = this->polydataObjectInstances.find( view );
    
        if( itAssociations != this->polydataObjectInstances.end() )
        {
            vtkActor * actor = (*itAssociations).second;
            view->GetRenderer( this->RenderLayer )->RemoveViewProp( actor );
            actor->Delete();
            this->polydataObjectInstances.erase( itAssociations );
			disconnect( view, SLOT(NotifyNeedRender()) );
        }
        if (this->Clipper)
        {
            disconnect(this->GetManager()->GetMainImagePlanes(), SIGNAL(EndPlaneMove(int)), this, SLOT(UpdateOctant(int)));
            this->Clipper->RemoveClipActor();
            this->Clipper->Delete();
            this->Clipper = 0;
        }
        if (this->SurfaceCutter)
        {
            this->SurfaceCutter->Delete();
            this->SurfaceCutter = 0;
        }
    }
        return true;
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
    if( renderingMode != this->renderingMode )
    {
        this->renderingMode = renderingMode;
        this->Property->SetRepresentation( this->renderingMode );
        emit Modified();
    }
}

void PolyDataObject::SetScalarsVisible( int use )
{
    this->ScalarsVisible = use;
        
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        (*it).second->GetMapper()->SetScalarVisibility( use );
    }

    emit Modified();
}

void PolyDataObject::SetLutIndex( int index )
{
    this->LutIndex = index;
    if( this->CurrentLut )
    {
        this->CurrentLut->Delete();
        this->CurrentLut = 0;
    }
    UpdateMapperPipeline();
    emit Modified();
}

void PolyDataObject::SetVertexColorMode( int mode )
{
    this->VertexColorMode = mode;
    UpdateMapperPipeline();
    emit Modified();
}

void PolyDataObject::SetOpacity( double opacity )
{
    if( opacity > 1 )
        this->opacity = 1;
    else if( opacity < 0 )
        this->opacity = 0;
    else
        this->opacity = opacity;

    this->Property->SetOpacity( this->opacity );

    emit Modified();
}

void PolyDataObject::Hide()
{
    View * view = 0;
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        view = (*it).first;
        if (view->GetType() == THREED_VIEW_TYPE)
        {
            vtkActor * actor = (*it).second;
            actor->VisibilityOff();
            if (this->CrossSectionVisible)
            {
                this->ShowCrossSection(false);
            }
            if (this->SurfaceRemovedOctantNumber > -1)
                this->Clipper->HideClipActor();
            emit Modified();
        }
    }
}

void PolyDataObject::Show()
{
    View * view = 0;
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        view = (*it).first;
        if (view->GetType() == THREED_VIEW_TYPE)
        {
            vtkActor * actor = (*it).second;
            if (this->CrossSectionVisible)
                this->ShowCrossSection(true);
            if (this->SurfaceRemovedOctantNumber > -1)
            {
                this->Clipper->ShowClipActor();
                actor->VisibilityOff();
            }
            else
                actor->VisibilityOn();
            emit Modified();
        }
    }
}

void PolyDataObject::UpdateSettingsWidget()
{
    emit ObjectViewChanged();
}

void PolyDataObject::SetCrossSectionVisible( bool showCrossSection )
{
    this->CrossSectionVisible = showCrossSection;
    this->ShowCrossSection( showCrossSection );
}

void PolyDataObject::ShowCrossSection(bool showCrossSection)
{
    if( !this->GetManager() ) // too early to set cross section, obect is not added to the scene yet
        return;
    if (showCrossSection  && !this->ObjectHidden)
    {
        if (!this->SurfaceCutter)
        {
            this->SurfaceCutter = SurfaceCuttingPlane::New();
            this->SurfaceCutter->SetManager(this->GetManager());
            this->SurfaceCutter->SetInput(this->PolyData);
            this->SurfaceCutter->SetWorldTransform(this->WorldTransform);
        }
        this->SurfaceCutter->SetPropertyColor(this->Property->GetColor());
        this->SurfaceCutter->UpdateCuttingPlane(0);
        this->SurfaceCutter->UpdateCuttingPlane(1);
        this->SurfaceCutter->UpdateCuttingPlane(2);

    }
    else
    {
        if (this->SurfaceCutter)
        {
            this->SurfaceCutter->Delete();
            this->SurfaceCutter = 0;
        }
    }
    emit Modified();
}

void PolyDataObject::RemoveSurfaceFromOctant(int octantNo, bool remove)
{
    Q_ASSERT(this->GetManager());
    View * view = 0;
    vtkActor * actor = 0;
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        view = (*it).first;
        if (view->GetType() == THREED_VIEW_TYPE)
        {
            actor = (*it).second;
            break;
        }
    }
    if (remove)
    {
        // we have to remove the original PolyDataObject actor from view
        actor->VisibilityOff();
        Octants *octant = this->GetManager()->GetOctantsOfInterest();
        octant->SetOctantNumber(octantNo);
        if (!this->Clipper)
        {
            this->Clipper = PolyDataClipper::New();
            this->Clipper->SetManager(this->GetManager());
            this->Clipper->SetWorldTransform(this->WorldTransform);
            this->Clipper->SetInput(this->PolyData);
            this->Clipper->SetActorProperty(this->Property);
            this->Clipper->SetScalarsVisible(this->ScalarsVisible);
            this->Clipper->SetClipFunction(octant->GetOctantOfInterest());
            connect(this->GetManager()->GetMainImagePlanes(), SIGNAL(EndPlaneMove(int)), this, SLOT(UpdateOctant(int)));
        }
        this->SurfaceRemovedOctantNumber = octantNo;
        for (int i = 0; i < 3; i++)
            this->UpdateOctant(i);
    }
    else
    {
        actor->VisibilityOn();
        disconnect(this->GetManager()->GetMainImagePlanes(), SIGNAL(EndPlaneMove(int)), this, SLOT(UpdateOctant(int)));
        this->Clipper->RemoveClipActor();
        this->Clipper->Delete();
        this->Clipper = 0;
        this->SurfaceRemovedOctantNumber = -1;
    }
    emit Modified();
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
            vtkActor * actor = (*it).second;
            vtkTexture * tex = actor->GetTexture();
            if( tex )
            {
                if( this->Texture )
                    tex->SetInputData( this->Texture );
                else
                    tex->SetInputData( this->checkerBoardTexture );
            }
        }
    }

    emit Modified();
}

void PolyDataObject::SetShowTexture( bool show )
{
    if( showTexture == show )
        return;

    this->showTexture = show;

    if( show )
    {
        // Create the default texture if needed
        if( !this->Texture && !this->checkerBoardTexture )
        {
            int size = 200;
            int squareSize = 5;
            this->checkerBoardTexture = vtkImageData::New();
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
            texture = this->checkerBoardTexture;

        // Add a vtkTexture to each actor to map the checker board onto object
        View * view = 0;
        vtkActor * actor = 0;
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
        vtkActor * actor = 0;
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
    emit Modified();
}

void PolyDataObject::SetTextureFileName( QString filename )
{
    this->textureFileName = filename;
    if( QFile::exists( filename ) )
    {
        vtkPNGReader * reader = vtkPNGReader::New();
        reader->SetFileName( filename.toUtf8().data() );
        reader->Update();
        this->SetTexture( reader->GetOutput() );
        reader->Delete();
    }
    else
        this->SetTexture( 0 );
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
        this->LutBackup = this->ScalarSource->GetLut();
        connect( this->ScalarSource, SIGNAL(RemovingFromScene()), this, SLOT(OnScalarSourceDeleted()) );
        connect( this->ScalarSource, SIGNAL(Modified()), this, SLOT(OnScalarSourceModified()) );
    }
    else
    {
        this->ProbeFilter->SetSourceData( 0 );
    }
    UpdateMapperPipeline();
    emit Modified();
}

void PolyDataObject::UpdateOctant(int planeNo)
{
    Q_ASSERT( this->GetManager() );

    double pos[3];
    this->GetManager()->GetMainImagePlanes()->GetPlanesPosition( pos );
    this->GetManager()->GetOctantsOfInterest()->UpdateOctantOfInterest( pos[planeNo], planeNo );
}

void PolyDataObject::OnScalarSourceDeleted()
{
    this->SetScalarSource( 0 );
}

void PolyDataObject::OnScalarSourceModified()
{
    vtkScalarsToColors * newLut = this->ScalarSource->GetLut();
    if( this->LutBackup != newLut )
    {
        this->LutBackup = newLut;
        UpdateMapperPipeline();
    }
    emit Modified();
}

void PolyDataObject::UpdateMapperPipeline()
{
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    while( it != this->polydataObjectInstances.end() )
    {
        vtkActor * actor = (*it).second;
        vtkMapper * mapper = actor->GetMapper();
        if( this->VertexColorMode == 1 && this->ScalarSource )
        {
            mapper->SetInputConnection( this->ProbeFilter->GetOutputPort() );
            mapper->SetLookupTable( this->ScalarSource->GetLut() );
        }
        else if( this->showTexture )
        {
            mapper->SetInputConnection( this->TextureMap->GetOutputPort() );
        }
        else
        {
            mapper->SetInputDataObject( this->PolyData );
            if( this->CurrentLut )
                mapper->SetLookupTable( this->CurrentLut );
        }
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
        return this->CurrentLut;

    // Create a new LUT if needed
    Q_ASSERT( this->GetManager() );
    Q_ASSERT( this->LutIndex < Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables() );
    Q_ASSERT( this->PolyData );
    QString tableName = Application::GetLookupTableManager()->GetTemplateLookupTableName( this->LutIndex );
    vtkPiecewiseFunctionLookupTable * lut = vtkPiecewiseFunctionLookupTable::New();
    double range[2];
    this->PolyData->GetScalarRange( range );
    Application::GetLookupTableManager()->CreateLookupTable( tableName, range, lut );
    this->CurrentLut = lut;
    return this->CurrentLut;
}

void PolyDataObject::InternalPostSceneRead()
{
    Q_ASSERT( this->GetManager() );

    // reconnect to the image object from which scalars are computed
    if( this->ScalarSourceObjectId != SceneObject::InvalidObjectId )
    {
        SceneObject * scalarObj = this->GetManager()->GetObjectByID( this->ScalarSourceObjectId );
        ImageObject * scalarImageObj = ImageObject::SafeDownCast( scalarObj );
        this->SetScalarSource( scalarImageObj );
    }
    emit ObjectViewChanged();
}
