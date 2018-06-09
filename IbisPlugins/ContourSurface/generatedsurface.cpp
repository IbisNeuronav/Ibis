/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <iostream>

#include "vtkMarchingContourFilter.h"
#include "vtkImageData.h"
#include "vtkProperty.h"
#include "vtkPolyData.h" 
#include "vtkImageGaussianSmooth.h"
#include "vtkImageAccumulate.h"
#include "vtkScalarsToColors.h"
#include "vtkTriangleFilter.h"
#include "vtkDecimatePro.h"
#include "vtkSmartPointer.h"

#include "generatedsurface.h"
#include "surfacesettingswidget.h"
#include "ibisapi.h"
#include "imageobject.h"
#include "polydataobjectsettingsdialog.h"
#include "contoursurfaceplugininterface.h"


ObjectSerializationMacro( GeneratedSurface );

GeneratedSurface::GeneratedSurface()
{
    m_imageObjectID = IbisAPI::InvalidId;
    m_contourValue = 0.0;
    m_radius = DEFAULT_RADIUS;
    m_standardDeviation = DEFAULT_STANDARD_DEVIATION;
    m_gaussianSmoothing = false;
    m_reductionPercent = 0;
    AllowChangeParent = false;
}

GeneratedSurface::~GeneratedSurface()
{
}

void GeneratedSurface::Serialize( Serializer * ser )
{
    PolyDataObject::Serialize(ser);
    ::Serialize( ser, "ImageObjectId", m_imageObjectID );
    ::Serialize( ser, "ContourValue", m_contourValue );
    ::Serialize( ser, "PolygonReductionPercent", m_reductionPercent );
    ::Serialize( ser, "GaussianSmoothing", m_gaussianSmoothing );
    ::Serialize( ser, "Radius", m_radius );
    ::Serialize( ser, "StandardDeviation", m_standardDeviation );
    if( ser->IsReader() )
    {
        ImageObject *img = ImageObject::SafeDownCast(m_pluginInterface->GetIbisAPI()->GetObjectByID( m_imageObjectID ) );
        if ( img )
        {
            this->GenerateSurface();
            m_pluginInterface->GetIbisAPI()->ChangeParent( this, img, img->GetNumberOfChildren() );
        }
    }
}

vtkImageAccumulate * GeneratedSurface::GetImageHistogram()
{
    ImageObject *img = ImageObject::SafeDownCast(m_pluginInterface->GetIbisAPI()->GetObjectByID( m_imageObjectID ) );
    if ( img )
        return img->GetHistogramComputer();
    return 0;
}

vtkScalarsToColors * GeneratedSurface::GetImageLut()
{
    ImageObject *img = ImageObject::SafeDownCast(m_pluginInterface->GetIbisAPI()->GetObjectByID( m_imageObjectID ) );
    if ( img )
        return img->GetLut();
    else
        return 0;
}

void GeneratedSurface::SetImageLutRange(double range[2])
{
    ImageObject *img = ImageObject::SafeDownCast(m_pluginInterface->GetIbisAPI()->GetObjectByID( m_imageObjectID ) );
    if ( img )
    {
        img->GetLut()->SetRange(range);
        img->MarkModified();
    }
}

void GeneratedSurface::GetImageScalarRange(double range[2])
{
    ImageObject *img = ImageObject::SafeDownCast(m_pluginInterface->GetIbisAPI()->GetObjectByID( m_imageObjectID ) );
    if ( img )
        img->GetImageScalarRange(range);
    else
    {
        range[0] = 0.0;
        range[1] = 1.0;
    }
}

bool GeneratedSurface::GenerateSurface()
{
    ImageObject *img = ImageObject::SafeDownCast(m_pluginInterface->GetIbisAPI()->GetObjectByID( m_imageObjectID ) );
    if ( img )
    {
        vtkSmartPointer<vtkMarchingContourFilter> contourExtractor = vtkSmartPointer<vtkMarchingContourFilter>::New();
        if (m_gaussianSmoothing)
        {
            vtkSmartPointer<vtkImageGaussianSmooth> GaussianSmooth = vtkSmartPointer<vtkImageGaussianSmooth>::New();
            GaussianSmooth->SetInputData(img->GetImage());
            GaussianSmooth->SetStandardDeviation(m_standardDeviation);
            GaussianSmooth->SetRadiusFactor(m_radius);
            GaussianSmooth->Update();
            contourExtractor->SetInputData( GaussianSmooth->GetOutput() );
        }
        else
        {
            contourExtractor->SetInputData(img->GetImage());
        }
        contourExtractor->SetValue( 0, m_contourValue );
        contourExtractor->Update();

        vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
        triangleFilter->SetInputConnection(contourExtractor->GetOutputPort() );
        triangleFilter->ReleaseDataFlagOn();
        triangleFilter->Update();

        if( m_reductionPercent > 0 )
        {
            vtkSmartPointer<vtkDecimatePro> decimate = vtkSmartPointer<vtkDecimatePro>::New();
            decimate->SetInputConnection(triangleFilter->GetOutputPort());
            decimate->SetTargetReduction(m_reductionPercent/100.0);
            decimate->Update();
            this->SetPolyData(decimate->GetOutput());
        }
        else
            this->SetPolyData(triangleFilter->GetOutput());
        return true;
    }
    return false;
}

void GeneratedSurface::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets)
{
    PolyDataObject::CreateSettingsWidgets(parent, widgets);
    SurfaceSettingsWidget *w = this->CreateSurfaceSettingsWidget(parent);
    w->setObjectName("Surface");
    widgets->append(w);
    connect( this, SIGNAL( ObjectViewChanged() ), w, SLOT(UpdateSettings()) );
}

SurfaceSettingsWidget * GeneratedSurface::CreateSurfaceSettingsWidget(QWidget * parent)
{
    SurfaceSettingsWidget * settingsWidget = new SurfaceSettingsWidget( parent );
    settingsWidget->SetGeneratedSurface( this );
    return settingsWidget;
}

void GeneratedSurface::UpdateSettingsWidget()
{
    emit ObjectViewChanged();
}

void GeneratedSurface::SetPluginInterface( ContourSurfacePluginInterface * interf )
{
    m_pluginInterface = interf;
}

bool GeneratedSurface::IsValid()
{
    return ImageObject::SafeDownCast(m_pluginInterface->GetIbisAPI()->GetObjectByID( m_imageObjectID ) ) != 0;
}
