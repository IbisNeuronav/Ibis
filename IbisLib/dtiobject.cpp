/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "dtiobject.h"
#include "vtkPolyData.h"
#include "vtkTubeFilter.h"
#include "polydataobjectsettingsdialog.h"
#include "dtiobjectsettingswidget.h"

ObjectSerializationMacro( DTIObject );

DTIObject::DTIObject()
{
    m_tubeFilter = 0;
    m_tubeRadius = 0.1;
    m_tubeResolution = 12;
    m_originalData = 0;
    m_generateTubes = false;
}

DTIObject::~DTIObject()
{
    if (m_tubeFilter)
        m_tubeFilter->Delete();
}

void DTIObject::Serialize( Serializer * ser )
{
    PolyDataObject::Serialize(ser);
    ::Serialize( ser, "m_generateTubes", m_generateTubes );
    ::Serialize( ser, "m_tubeRadius", m_tubeRadius );
    ::Serialize( ser, "m_tubeResolution", m_tubeResolution );
}

void DTIObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets )
{
    PolyDataObject::CreateSettingsWidgets(parent, widgets);
    DTIObjectSettingsWidget *w = this->CreateDTISettingsWidget(parent);
    w->setObjectName("Tracks");
    widgets->append(w);
}

DTIObjectSettingsWidget * DTIObject::CreateDTISettingsWidget(QWidget * parent)
{
    DTIObjectSettingsWidget * settingsWidget = new DTIObjectSettingsWidget( parent );
    settingsWidget->SetDTIObject( this );
    return settingsWidget;
}

bool DTIObject::Setup( View * view )
{
    bool success = PolyDataObject::Setup( view );
    if( !success )
        return false;
    this->PrepareDTITracks();
    this->SetGenerateTubes( m_generateTubes );
    this->SetTubeRadius(m_tubeRadius);
    this->SetTubeResolution(m_tubeResolution);
}

void DTIObject::PrepareDTITracks()
{
    if (this->PolyData)
    {
        m_originalData = vtkPolyData::New();
        m_originalData->DeepCopy(this->PolyData);
        m_tubeFilter = vtkTubeFilter::New();
        m_tubeFilter->SetInputData(m_originalData);

        m_tubeFilter->SetNumberOfSides(m_tubeResolution);
        m_tubeFilter->SetRadius(m_tubeRadius);
        m_tubeFilter->CappingOn();
        //this->SetPolyData(m_tubeFilter->GetOutput());
    }
}

void DTIObject::SetGenerateTubes( bool gen )
{
    m_generateTubes = gen;
    if( m_generateTubes )
    {
        Q_ASSERT( m_tubeFilter );
        this->SetPolyData( m_tubeFilter->GetOutput() );
    }
    else
    {
        Q_ASSERT( m_originalData );
        this->SetPolyData( m_originalData );
    }
    emit Modified();
}

void DTIObject::SetTubeResolution( int res )
{
    if (m_tubeFilter->GetNumberOfSides() != res)
    {
        m_tubeResolution = res;
        m_tubeFilter->SetNumberOfSides(m_tubeResolution);
        m_tubeFilter->Update();
        emit Modified();
    }
}

void DTIObject::SetTubeRadius( double radius)
{
    if (m_tubeFilter->GetRadius() != radius)
    {
        m_tubeRadius = radius;
        m_tubeFilter->SetRadius(m_tubeRadius);
        m_tubeFilter->Update();
        emit Modified();
    }
}
