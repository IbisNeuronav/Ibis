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

#include "generatedsurface.h"
#include "surfacesettingswidget.h"
#include "scenemanager.h"
#include "imageobject.h"
#include "polydataobjectsettingsdialog.h"
#include "contoursurfaceplugininterface.h"


ObjectSerializationMacro( GeneratedSurface );

GeneratedSurface::GeneratedSurface()
{
    m_imageObject = 0;

    m_contourValue = 0.0;
    m_radius = DEFAULT_RADIUS;
    m_standardDeviation = DEFAULT_STANDARD_DEVIATION;
    m_gaussianSmoothing = false;
    m_reductionPercent = 0;
}

GeneratedSurface::~GeneratedSurface()
{
    if (m_imageObject)
        m_imageObject->UnRegister(this);
}

void GeneratedSurface::Serialize( Serializer * ser )
{
    PolyDataObject::Serialize(ser);
    int imageId = SceneObject::InvalidObjectId;
    if (m_imageObject)
        imageId = m_imageObject->GetObjectID();
    ::Serialize( ser, "ImageObjectId", imageId );
    ::Serialize( ser, "ContourValue", m_contourValue );
    ::Serialize( ser, "PolygonReductionPercent", m_reductionPercent );
    ::Serialize( ser, "GaussianSmoothing", m_gaussianSmoothing );
    ::Serialize( ser, "Radius", m_radius );
    ::Serialize( ser, "StandardDeviation", m_standardDeviation );
    if( ser->IsReader() )
    {
        ImageObject *img = ImageObject::SafeDownCast(m_pluginInterface->GetSceneManager()->GetObjectByID( imageId ) );
        if ( img )
        {
            this->SetImageObject( img );
            this->GenerateSurface();
            m_pluginInterface->GetSceneManager()->ChangeParent( this, img, img->GetNumberOfChildren() );
        }
    }
}

void GeneratedSurface::SetImageObject(ImageObject *obj)
{
    if( m_imageObject == obj )
        return;
    if (m_imageObject)
        m_imageObject->UnRegister(this);
    m_imageObject = obj;
    if (m_imageObject)
    {
        m_imageObject->Register(this);
    }
}

vtkImageAccumulate * GeneratedSurface::GetImageHistogram()
{
    if (m_imageObject)
        return m_imageObject->GetHistogramComputer();
    return 0;
}

vtkScalarsToColors * GeneratedSurface::GetImageLut()
{
    if (m_imageObject)
        return m_imageObject->GetLut();
    else
        return 0;
}

void GeneratedSurface::SetImageLutRange(double range[2])
{
    if (m_imageObject)
    {
        m_imageObject->GetLut()->SetRange(range);
        m_imageObject->MarkModified();
    }
}

void GeneratedSurface::GetImageScalarRange(double range[2])
{
    if (m_imageObject)
        m_imageObject->GetImageScalarRange(range);
    else
    {
        range[0] = 0.0;
        range[1] = 1.0;
    }
}

vtkPolyData * GeneratedSurface::GenerateSurface()
{
    vtkMarchingContourFilter *contourExtractor = vtkMarchingContourFilter::New();
    if (m_gaussianSmoothing)
    {
        vtkImageGaussianSmooth *GaussianSmooth = vtkImageGaussianSmooth::New();
        GaussianSmooth->SetInputData(m_imageObject->GetImage());
        GaussianSmooth->SetStandardDeviation(m_standardDeviation);
        GaussianSmooth->SetRadiusFactor(m_radius);
        GaussianSmooth->Update();
        contourExtractor->SetInputData( GaussianSmooth->GetOutput() );
        GaussianSmooth->Delete();
    }
    else
    {
        contourExtractor->SetInputData(m_imageObject->GetImage());
    }
    contourExtractor->SetValue( 0, m_contourValue );
    contourExtractor->Update();

    vtkTriangleFilter *triangleFilter = vtkTriangleFilter::New();
    triangleFilter->SetInputConnection(contourExtractor->GetOutputPort() );
    triangleFilter->ReleaseDataFlagOn();
    triangleFilter->Update();

    if( m_reductionPercent > 0 )
    {
        vtkDecimatePro *decimate = vtkDecimatePro::New();
        decimate->SetInputConnection(triangleFilter->GetOutputPort());
        decimate->SetTargetReduction(m_reductionPercent/100.0);
        decimate->Update();
        this->SetPolyData(decimate->GetOutput());
        decimate->Delete();
    }
    else
        this->SetPolyData(triangleFilter->GetOutput());
    triangleFilter->Delete();
    contourExtractor->Delete();
    return this->PolyData;
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
