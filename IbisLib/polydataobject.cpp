/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "polydataobject.h"

#include <vtkActor.h>
#include <vtkClipPolyData.h>
#include <vtkImageData.h>
#include <vtkPNGReader.h>
#include <vtkPassThrough.h>
#include <vtkPiecewiseFunctionLookupTable.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProbeFilter.h>
#include <vtkTexture.h>
#include <vtkTextureMapToCylinder.h>
#include <vtkTextureMapToPlane.h>
#include <vtkTextureMapToSphere.h>

#include <QMessageBox>

#include "application.h"
#include "imageobject.h"
#include "lookuptablemanager.h"
#include "polydataobjectsettingsdialog.h"
#include "scenemanager.h"
#include "view.h"

ObjectSerializationMacro( PolyDataObject );

vtkSmartPointer<vtkImageData> PolyDataObject::checkerBoardTexture;

PolyDataObject::PolyDataObject()
{
    this->showTexture     = false;
    this->Texture         = 0;
    this->VertexColorMode = 0;

    // Probe filter ( used to sample scalars from other dataset )
    this->ScalarSource         = 0;
    this->LutIndex             = 0;
    this->ScalarSourceObjectId = SceneManager::InvalidId;
    this->LutBackup            = vtkSmartPointer<vtkScalarsToColors>::New();
    this->ProbeFilter          = vtkSmartPointer<vtkProbeFilter>::New();
    this->ProbeFilter->SetInputConnection( m_clippingSwitch->GetOutputPort() );

    // Texture map filter
    vtkSmartPointer<vtkTextureMapToSphere> map = vtkSmartPointer<vtkTextureMapToSphere>::New();
    map->AutomaticSphereGenerationOn();
    map->PreventSeamOn();
    map->SetInputConnection( m_clippingSwitch->GetOutputPort() );
    this->TextureMap = map;
}

PolyDataObject::~PolyDataObject()
{
    if( this->ScalarSource ) this->ScalarSource->UnRegister( this );
    if( this->Texture ) this->Texture->UnRegister( this );
}

void PolyDataObject::Serialize( Serializer * ser )
{
    AbstractPolyDataObject::Serialize( ser );
    QString newTextureFilePath;
    QString textureFileNameOnly;
    if( !ser->IsReader() )
    {
        if( this->ScalarSource )
            this->ScalarSourceObjectId = this->ScalarSource->GetObjectID();
        else
            this->ScalarSourceObjectId = SceneManager::InvalidId;
        QString oldPath    = this->textureFileName;
        newTextureFilePath = QString( this->GetManager()->GetSceneDirectory() );
        newTextureFilePath.append( "/" );
        if( !oldPath.isEmpty() )
        {
            QFileInfo fi( this->textureFileName );
            textureFileNameOnly = fi.fileName();
            newTextureFilePath.append( textureFileNameOnly );
            // Copy the file to the scene directory
            if( !QFile::exists( newTextureFilePath ) ) QFile::copy( oldPath, newTextureFilePath );
        }
    }
    ::Serialize( ser, "LutIndex", this->LutIndex );
    ::Serialize( ser, "VertexColorMode", this->VertexColorMode );
    ::Serialize( ser, "ScalarSourceObjectId", this->ScalarSourceObjectId );
    ::Serialize( ser, "ShowTexture", this->showTexture );
    if( ser->IsReader() )
    {
        ::Serialize( ser, "TextureFileName", this->textureFileName );
    }
    else
        ::Serialize( ser, "TextureFileName", textureFileNameOnly );
}

void PolyDataObject::Export()
{
    Q_ASSERT( this->GetManager() );
    QString surfaceName( this->Name );
    surfaceName.append( ".vtk" );
    this->SetDataFileName( surfaceName );
    QString fullName( this->GetManager()->GetSceneDirectory() );
    fullName.append( "/" );
    fullName.append( surfaceName );
    QString saveName = Application::GetInstance().GetFileNameSave( tr( "Save Object" ), fullName, tr( "*.vtk" ) );
    if( saveName.isEmpty() ) return;
    if( QFile::exists( saveName ) )
    {
        int ret =
            QMessageBox::warning( 0, tr( "Save PolyDataObject" ), saveName, QMessageBox::Yes |
                                  QMessageBox::No, QMessageBox::No );
        if( ret == QMessageBox::No ) return;
    }
    this->SavePolyData( saveName );
}

void PolyDataObject::CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets )
{
    PolyDataObjectSettingsDialog * res = new PolyDataObjectSettingsDialog( parent );
    res->setAttribute( Qt::WA_DeleteOnClose );
    res->SetPolyDataObject( this );
    res->setObjectName( "Properties" );
    connect( this, SIGNAL( ObjectViewChanged() ), res, SLOT( UpdateSettings() ) );
    widgets->append( res );
}

void PolyDataObject::UpdatePipeline()
{
    if( !this->PolyData ) return;

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
        vtkSmartPointer<vtkActor> actor = ( *it ).second;
        vtkMapper * mapper              = actor->GetMapper();
        mapper->SetScalarVisibility( this->ScalarsVisible );
        if( this->VertexColorMode == 1 && this->ScalarSource )
            mapper->SetLookupTable( this->ScalarSource->GetLut() );
        else if( this->CurrentLut )
            mapper->SetLookupTable( this->CurrentLut );
        ++it;
    }
}

void PolyDataObject::SetVertexColorMode( int mode )
{
    this->VertexColorMode = mode;
    this->UpdatePipeline();
    emit ObjectModified();
}

void PolyDataObject::SetScalarSource( ImageObject * im )
{
    if( this->ScalarSource == im ) return;
    if( this->ScalarSource )
    {
        this->ScalarSource->UnRegister( this );
        disconnect( this->ScalarSource, SIGNAL( RemovingFromScene() ), this, SLOT( OnScalarSourceDeleted() ) );
        disconnect( this->ScalarSource, SIGNAL( ObjectModified() ), this, SLOT( OnScalarSourceModified() ) );
    }
    this->ScalarSource = im;
    if( this->ScalarSource )
    {
        this->ScalarSource->Register( this );
        this->ProbeFilter->SetSourceData( this->ScalarSource->GetImage() );
        this->LutBackup->DeepCopy( this->ScalarSource->GetLut() );
        connect( this->ScalarSource, SIGNAL( RemovingFromScene() ), this, SLOT( OnScalarSourceDeleted() ) );
        connect( this->ScalarSource, SIGNAL( ObjectModified() ), this, SLOT( OnScalarSourceModified() ) );
    }
    else
    {
        this->ProbeFilter->SetSourceData( 0 );
    }
    this->UpdatePipeline();
    emit ObjectModified();
}

void PolyDataObject::OnScalarSourceDeleted() { this->SetScalarSource( 0 ); }

void PolyDataObject::OnScalarSourceModified()
{
    vtkScalarsToColors * newLut = this->ScalarSource->GetLut();
    if( this->LutBackup != newLut )
    {
        this->LutBackup->DeepCopy( newLut );
        this->UpdatePipeline();
    }
    emit ObjectModified();
}

void PolyDataObject::SetLutIndex( int index )
{
    this->LutIndex = index;
    if( this->CurrentLut )
    {
        this->CurrentLut = 0;
    }
    this->UpdatePipeline();
    emit ObjectModified();
}

vtkScalarsToColors * PolyDataObject::GetCurrentLut()
{
    // No LUT
    if( this->LutIndex == -1 )  // Don't try to control the lookup table, use default
        return 0;

    // LUT already exists
    if( this->CurrentLut ) return this->CurrentLut;

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

void PolyDataObject::InternalPostSceneRead()
{
    Q_ASSERT( this->GetManager() );

    // reconnect to the image object from which scalars are computed, process texture
    if( this->ScalarSourceObjectId != SceneManager::InvalidId )
    {
        SceneObject * scalarObj      = this->GetManager()->GetObjectByID( this->ScalarSourceObjectId );
        ImageObject * scalarImageObj = ImageObject::SafeDownCast( scalarObj );
        this->SetScalarSource( scalarImageObj );
    }
    QString textureFilePath;
    if( !this->textureFileName.isEmpty() )
    {
        textureFilePath = this->GetManager()->GetSceneDirectory() + "/" + this->textureFileName;
        this->SetTextureFileName( textureFilePath );
        this->SetRenderingMode( this->renderingMode );
        this->SetShowTexture( this->showTexture );
    }
    emit ObjectViewChanged();
}

void PolyDataObject::SetTexture( vtkImageData * texImage )
{
    // standard set code
    if( this->Texture ) this->Texture->UnRegister( this );
    this->Texture = texImage;
    if( this->Texture ) this->Texture->Register( this );

    // update texture in actor
    PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
    for( ; it != this->polydataObjectInstances.end(); ++it )
    {
        View * view = ( *it ).first;
        if( view->GetType() == THREED_VIEW_TYPE )
        {
            vtkSmartPointer<vtkActor> actor = ( *it ).second;
            vtkTexture * tex                = actor->GetTexture();
            if( tex )
            {
                if( this->Texture )
                    tex->SetInputData( this->Texture );
                else
                    tex->SetInputData( this->checkerBoardTexture );
            }
        }
    }

    emit ObjectModified();
}

void PolyDataObject::SetShowTexture( bool show )
{
    this->showTexture = show;

    if( show )
    {
        // Create the default texture if needed
        if( !this->Texture && !this->checkerBoardTexture )
        {
            int size                  = 200;
            int squareSize            = 5;
            this->checkerBoardTexture = vtkSmartPointer<vtkImageData>::New();
            this->checkerBoardTexture->SetDimensions( size, size, 1 );
            this->checkerBoardTexture->AllocateScalars( VTK_UNSIGNED_CHAR, 1 );
            unsigned char * row = (unsigned char *)this->checkerBoardTexture->GetScalarPointer();
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
        if( !texture ) texture = this->checkerBoardTexture;

        // Add a vtkTexture to each actor to map the checker board onto object
        View * view = 0;
        vtkSmartPointer<vtkActor> actor;
        PolyDataObjectViewAssociation::iterator it = this->polydataObjectInstances.begin();
        for( ; it != this->polydataObjectInstances.end(); ++it )
        {
            view = ( *it ).first;
            if( view->GetType() == THREED_VIEW_TYPE )
            {
                actor            = ( *it ).second;
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
            view = ( *it ).first;
            if( view->GetType() == THREED_VIEW_TYPE )
            {
                actor = ( *it ).second;
                actor->SetTexture( 0 );
            }
        }
    }
    this->UpdatePipeline();
    emit ObjectModified();
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

void PolyDataObject::ObjectRemovedFromScene()
{
    Q_ASSERT( this->GetManager() );
    // refresh 3D view
    this->GetManager()->GetMain3DView()->NotifyNeedRender();
}
