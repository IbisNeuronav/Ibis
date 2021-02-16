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
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkTexture.h>
#include <vtkImageData.h>
#include <vtkPNGReader.h>
#include "vtkPiecewiseFunctionLookupTable.h"

#include <vtkTextureMapToPlane.h>
#include <vtkTextureMapToSphere.h>
#include <vtkTextureMapToCylinder.h>

#include "view.h"
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

#include "polydataobjectsettingsdialog.h"
#include "polydataobject.h"

ObjectSerializationMacro( PolyDataObject );

vtkSmartPointer<vtkImageData> PolyDataObject::checkerBoardTexture;

PolyDataObject::PolyDataObject()
{
    this->showTexture = false;
    this->Texture = 0;

    // Texture map filter
    vtkSmartPointer<vtkTextureMapToSphere> map = vtkSmartPointer<vtkTextureMapToSphere>::New();
    map->AutomaticSphereGenerationOn();
    map->PreventSeamOn();
    map->SetInputConnection( m_clippingSwitch->GetOutputPort() );
    this->TextureMap = map;
}

PolyDataObject::~PolyDataObject()
{
    if( this->Texture )
        this->Texture->UnRegister( this );
}

void PolyDataObject::Serialize( Serializer * ser )
{
    AbstractPolyDataObject::Serialize( ser );
    if( ser->IsReader() )
    {
        this->SetRenderingMode( this->renderingMode );
        this->SetShowTexture( this->showTexture );
    }
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
            mapper->SetLookupTable( this->CurrentLut );
        ++it;
    }
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
            texture = this->checkerBoardTexture;

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

