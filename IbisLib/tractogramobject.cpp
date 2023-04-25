/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <cmath>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkTransform.h>
#include <vtkScalarsToColors.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkProbeFilter.h>
#include <vtkClipPolyData.h>
#include <vtkCutter.h>
#include <vtkPassThrough.h>
#include <vtkTubeFilter.h>

#include "view.h"
#include "ibistypes.h"
#include "imageobject.h"
#include "polydataobject.h"
#include "tractogramobject.h"
#include "tractogramobjectsettingsdialog.h"

ObjectSerializationMacro( TractogramObject );

TractogramObject::TractogramObject()
{
    this->m_tubeSwitch = vtkSmartPointer<vtkPassThrough>::New();
    this->m_tubeSwitch->SetInputConnection( m_clippingSwitch->GetOutputPort() );

    this->tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    this->tubeFilter->SetRadius(.2);
    this->tubeFilter->SetNumberOfSides(11);
    this->tubeFilter->SetCapping(1);
    this->tubeFilter->Update();
    this->tubeFilter->SetInputConnection( m_clippingSwitch->GetOutputPort() );

    this->tube_enabled = false;
    this->renderingMode = VTK_WIREFRAME;

    this->SetScalarsVisible(true);
    this->SetVertexColorMode(2);
}

TractogramObject::~TractogramObject()
{
}

void TractogramObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets)
{
    TractogramObjectSettingsDialog * res = new TractogramObjectSettingsDialog( parent );
    res->setAttribute(Qt::WA_DeleteOnClose);
    res->SetTractogramObject( this );
    res->setObjectName("Properties");
    connect( this, SIGNAL( ObjectViewChanged() ), res, SLOT(UpdateSettings()) );
    widgets->append(res);
}

void TractogramObject::SetVertexColorMode( int mode )
{
    this->VertexColorMode = mode;
    this->UpdatePipeline();
    emit ObjectModified();
}

void TractogramObject::UpdatePipeline()
{
    if( !this->PolyData )
        return;

    m_clipper->SetInputData( this->PolyData );
    if( this->IsClippingEnabled() )
        m_clippingSwitch->SetInputConnection( m_clipper->GetOutputPort() );
    else
        m_clippingSwitch->SetInputData( this->PolyData );

    this->ProbeFilter->SetInputConnection( m_clippingSwitch->GetOutputPort() );

    m_colorSwitch->SetInputConnection( m_clippingSwitch->GetOutputPort() );
    if( this->ScalarsVisible )
    {
        if( this->VertexColorMode == 1 && this->ScalarSource )
            m_colorSwitch->SetInputConnection( this->ProbeFilter->GetOutputPort() );
        else if( this->VertexColorMode == 2 )
            this->GenerateLocalColoring();
        else if( this->VertexColorMode == 3 )
            this->GenerateEndPtsColoring();
    }

    if( this->tube_enabled )
    {
        // this->splineFilter->SetInputConnection( this->m_colorSwitch->GetOutputPort() );
        // this->tubeFilter->SetInputConnection( this->splineFilter->GetOutputPort() );
        this->tubeFilter->SetInputConnection( this->m_colorSwitch->GetOutputPort() );
        this->m_tubeSwitch->SetInputConnection( this->tubeFilter->GetOutputPort() );
    }
    else
    {
        this->m_tubeSwitch->SetInputConnection( this->m_colorSwitch->GetOutputPort() );
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
        if ( this->VertexColorMode == 2)
            mapper->SetScalarModeToUsePointData();
        else if ( this->VertexColorMode == 3)
            mapper->SetScalarModeToUseCellData();
        else
            mapper->SetScalarModeToDefault();
        
        ++it;
    }
}

void TractogramObject::Setup( View * view )
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
        mapper->SetInputConnection( this->m_tubeSwitch->GetOutputPort() );
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

void TractogramObject::SetRenderingMode( int renderingMode )
{
    if (renderingMode == 2)
        this->tube_enabled = true;
    else
        this->tube_enabled = false;
    
    this->UpdatePipeline();
    this->renderingMode = renderingMode;
    this->Property->SetRepresentation( this->renderingMode );
    m_2dProperty->SetRepresentation( this->renderingMode );
    emit ObjectModified();
}

void TractogramObject::GenerateLocalColoring()
{
    // Setup the colors array
    vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(this->PolyData->GetPoints()->GetNumberOfPoints());
    colors->SetName("Colors");

    unsigned long first_id = 0, last_id;
    unsigned long nb_vts;
    // Loop over each streamlines
    for (unsigned long s = 0; s < this->PolyData->GetNumberOfLines(); s++)
    {
        nb_vts = this->PolyData->GetLines()->GetCellSize(s);
        if (nb_vts > 0)
        {
            last_id = first_id + nb_vts;
            this->AddLocalColor(colors, first_id, first_id+1, first_id);

            if (nb_vts > 1)
            {
                // color all vts of the line
                for (unsigned long i = first_id + 1; i < last_id - 1; i++)
                {
                    this->AddLocalColor(colors, i-1, i+1, i);
                }
                // color the last segment of the line
                this->AddLocalColor(colors, last_id-2, last_id-1, last_id-1);
            }
            first_id = last_id;
        }
    }
    this->PolyData->GetPointData()->SetScalars(colors);
}

void TractogramObject::GenerateEndPtsColoring()
{
    // Setup the colors array
    vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(this->PolyData->GetNumberOfLines());
    colors->SetName("Colors");

    unsigned long first_id = 0, last_id;
    // Loop over each streamlines
    for (unsigned long s = 0; s < this->PolyData->GetNumberOfLines(); s++)
    {
        // color from the first and last point
        last_id = first_id + this->PolyData->GetLines()->GetCellSize(s);
        this->AddLocalColor(colors, first_id, last_id-1, s);
        first_id = last_id;
    }
    this->PolyData->GetCellData()->SetScalars(colors);
}

void TractogramObject::AddLocalColor(vtkSmartPointer<vtkUnsignedCharArray> colors, unsigned long vts1_id, unsigned long vts2_id, unsigned long current_id)
{
        vtkDataArray* vts = this->PolyData->GetPoints()->GetData();
        float x = std::abs(vts->GetComponent(vts1_id, 0) - vts->GetComponent(vts2_id, 0));
        float y = std::abs(vts->GetComponent(vts1_id, 1) - vts->GetComponent(vts2_id, 1));
        float z = std::abs(vts->GetComponent(vts1_id, 2) - vts->GetComponent(vts2_id, 2));
        float norm = 255.0/std::sqrt(x*x + y*y + z*z);

        colors->SetTuple3(current_id, x*norm, y*norm, z*norm);
}

