/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "usprobeobject.h"
#include "vtkImageData.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkRenderer.h"
#include "view.h"
#include "vtkTransform.h"
#include "vtkImageProperty.h"
#include "vtkImageMapToColors.h"
#include "vtkImageToImageStencil.h"
#include "vtkImageStencil.h"
#include "scenemanager.h"
#include "application.h"
#include "hardwaremodule.h"
#include "filereader.h"
#include "imageobject.h"
#include <QDir>
#include <QWidget>
#include "usprobeobjectsettingswidget.h"
#include "usmask.h"
#include "usmasksettingswidget.h"
#include "lookuptablemanager.h"
#include "vtkImageConstantPad.h"

UsProbeObject::UsProbeObject()
{
    this->SetCanChangeParent( false );

    m_mask = USMask::New(); // mask in use
    m_defaultMask = USMask::New(); // default mask saved in ibis config file
    m_maskOn = false;
    m_sliceProperties = vtkImageProperty::New();
    m_lutIndex = 1;         // default to hot metal
    m_mapToColors = vtkImageMapToColors::New();
    m_mapToColors->SetOutputFormatToRGBA();

    m_imageStencilSource = vtkImageToImageStencil::New();
    m_imageStencilSource->ThresholdByUpper( 128.0 );
    m_imageStencilSource->SetInputData( m_mask->GetMask() );
    m_imageStencilSource->UpdateWholeExtent();

    m_sliceStencil = vtkImageStencil::New();
    m_sliceStencil->SetStencilData( m_imageStencilSource->GetOutput() );
    m_sliceStencil->SetInputConnection( m_mapToColors->GetOutputPort() );
    m_sliceStencil->SetBackgroundColor( 1.0, 1.0, 1.0, 0.0 );

    m_toolTransform = vtkTransform::New();

    m_acquisitionType = ACQ_B_MODE;
}

UsProbeObject::~UsProbeObject()
{
    m_sliceProperties->Delete();
    m_mapToColors->Delete();
    m_imageStencilSource->Delete();
    m_sliceStencil->Delete();
    m_toolTransform->Delete();
    m_mask->Delete();
    m_defaultMask->Delete();
}

void UsProbeObject::AddClient()
{
    Application::GetHardwareModule()->AddTrackedVideoClient();
}

void UsProbeObject::RemoveClient()
{
    Application::GetHardwareModule()->RemoveTrackedVideoClient();
}

void UsProbeObject::ObjectAddedToScene()
{
    SetCurrentLUTIndex( m_lutIndex );
    UpdatePipeline();
    m_toolTransform->SetInput( Application::GetHardwareModule()->GetTrackedVideoTransform() );

    // Check for updates
    connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(OnUpdate()) );
    connect( m_mask, SIGNAL(MaskChanged()), this, SLOT(UpdateMask()) );
}

void UsProbeObject::ObjectRemovedFromScene()
{
    disconnect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(OnUpdate()) );
}

bool UsProbeObject::Setup( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        PerViewElements pv;
        pv.imageActor = vtkImageActor::New();
        pv.imageActor->SetUserTransform( m_toolTransform );
        pv.imageActor->SetVisibility( this->IsHidden() ? 0 : 1 );
        if( !this->IsHidden() )
        {
            connect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
            Application::GetHardwareModule()->AddTrackedVideoClient();
        }

        UpdatePipelineOneView( pv );

        view->GetRenderer()->AddActor( pv.imageActor );

        m_perViews[ view ] = pv;
    }
	return true;
}

bool UsProbeObject::Release( View * view )
{
    if( view->GetType() == THREED_VIEW_TYPE )
    {
        PerViewContainer::iterator it = m_perViews.find( view );
        if( it != m_perViews.end() )
        {
            PerViewElements perView = (*it).second;
            view->GetRenderer()->RemoveViewProp( perView.imageActor );
            perView.imageActor->Delete();
            m_perViews.erase( it );
            if( !this->IsHidden() )
            {
                disconnect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
                Application::GetHardwareModule()->RemoveTrackedVideoClient();
            }
        }
    }
    return true;
}

void UsProbeObject::Hide()
{
    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        PerViewElements perView = (*it).second;
        View * view = (*it).first;
        perView.imageActor->VisibilityOff();
        Application::GetHardwareModule()->RemoveTrackedVideoClient();
        disconnect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
        view->NotifyNeedRender();
        ++it;
    }
    emit Modified();
}

void UsProbeObject::Show()
{
    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        PerViewElements perView = (*it).second;
        View * view = (*it).first;
        perView.imageActor->VisibilityOn();
        Application::GetHardwareModule()->AddTrackedVideoClient();
        connect( this, SIGNAL(Modified()), view, SLOT(NotifyNeedRender()) );
        ++it;
    }
    emit Modified();
}

TrackerToolState UsProbeObject::GetState()
{
    return Application::GetHardwareModule()->GetVideoTrackerState();
}

int UsProbeObject::GetVideoImageWidth()
{
    GetVideoOutput()->GetDimensions()[0];
}

int UsProbeObject::GetVideoImageHeight()
{
    GetVideoOutput()->GetDimensions()[1];
}

int UsProbeObject::GetVideoImageNumberOfComponents()
{
    GetVideoOutput()->GetNumberOfScalarComponents();
}

vtkImageData * UsProbeObject::GetVideoOutput()
{
    return Application::GetHardwareModule()->GetTrackedVideoOutput();
}

vtkTransform * UsProbeObject::GetTransform()
{
    return Application::GetHardwareModule()->GetTrackedVideoTransform();
}

vtkTransform * UsProbeObject::GetUncalibratedTransform()
{
    return Application::GetHardwareModule()->GetTrackedVideoUncalibratedTransform();
}

int UsProbeObject::GetNumberOfCalibrationMatrices()
{
    Application::GetHardwareModule()->GetNumberOfVideoCalibrationMatrices();
}

QString UsProbeObject::GetCalibrationMatrixName( int index )
{
    return Application::GetHardwareModule()->GetVideoCalibrationMatrixName( index );
}

void UsProbeObject::SetCurrentCalibrationMatrixName( QString name )
{
    Application::GetHardwareModule()->SetCurrentVideoCalibrationMatrixName( name );
}

QString UsProbeObject::GetCurrentCalibrationMatrixName()
{
    return Application::GetHardwareModule()->GetCurrentVideoCalibrationMatrixName();
}

void UsProbeObject::SetCurrentCalibrationMatrix( vtkMatrix4x4 * mat )
{
    Application::GetHardwareModule()->SetVideoCalibrationMatrix( mat );
}

vtkMatrix4x4 * UsProbeObject::GetCurrentCalibrationMatrix()
{
    return Application::GetHardwareModule()->GetVideoCalibrationMatrix();
}

void UsProbeObject::SetAcquisitionType( ACQ_TYPE type )
{
    m_acquisitionType = type;
    UpdatePipeline();
}

void UsProbeObject::SetUseMask( bool useMask )
{
    m_maskOn = useMask;
    UpdatePipeline();
}
    
void UsProbeObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets )
{
    UsProbeObjectSettingsWidget * res = new UsProbeObjectSettingsWidget( parent );
    res->setAttribute( Qt::WA_DeleteOnClose, true );
    res->SetUsProbeObject( this );
    res->setObjectName( "Properties" );
    widgets->append( res );
    USMaskSettingsWidget *res1 = new USMaskSettingsWidget( parent );
    res1->setObjectName( "Mask" );
    res1->SetMask( m_mask );
    widgets->append( res1 );
}

int UsProbeObject::GetNumberOfAvailableLUT()
{
    return Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables();
}

QString UsProbeObject::GetLUTName( int index )
{
    return Application::GetLookupTableManager()->GetTemplateLookupTableName( index );
}

void UsProbeObject::SetCurrentLUTIndex( int index )
{
    m_lutIndex = index;
    double range[2] = { 0.0, 255.0 };
    QString slicesLutName = Application::GetLookupTableManager()->GetTemplateLookupTableName( m_lutIndex );
    vtkPiecewiseFunctionLookupTable * lut = vtkPiecewiseFunctionLookupTable::New();
    lut->SetIntensityFactor( 1.0 );
    Application::GetLookupTableManager()->CreateLookupTable( slicesLutName, range, lut );
    m_mapToColors->SetLookupTable( lut );
    lut->Delete();
    emit Modified();
}

void UsProbeObject::OnUpdate()
{
    emit Modified();
}

void UsProbeObject::UpdateMask()
{
    m_imageStencilSource->Update();
    m_mapToColors->Update();
    emit Modified();
}

void UsProbeObject::InitialSetMask( USMask *mask )
{
    m_defaultMask->operator =( *mask );
    m_mask->operator =( *mask );
    m_mask->SetAsDefault();
}

void UsProbeObject::UpdatePipeline()
{
    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        PerViewElements pv = (*it).second;
        this->UpdatePipelineOneView( pv );
        ++it;
    }
}

void UsProbeObject::UpdatePipelineOneView( UsProbeObject::PerViewElements & pv )
{
    bool bMode = m_acquisitionType == ACQ_B_MODE;
    if( !bMode && !m_maskOn )
    {
        pv.imageActor->GetMapper()->SetInputData( Application::GetHardwareModule()->GetTrackedVideoOutput() );
    }
    else if( bMode && !m_maskOn )
    {
        m_mapToColors->SetInputData( Application::GetHardwareModule()->GetTrackedVideoOutput() );
        pv.imageActor->GetMapper()->SetInputConnection( m_mapToColors->GetOutputPort() );
    }
    else if( bMode && m_maskOn )
    {
        m_mapToColors->SetInputData( Application::GetHardwareModule()->GetTrackedVideoOutput() );
        m_sliceStencil->SetInputConnection( m_mapToColors->GetOutputPort() );
        pv.imageActor->GetMapper()->SetInputConnection( m_sliceStencil->GetOutputPort() );
    }
    else // !bMode && m_maskOn
    {
        // added convertion from RGB to RGBa
        vtkImageConstantPad *icp = NULL;
        icp = vtkImageConstantPad::New();
        icp->SetInputData(Application::GetHardwareModule()->GetTrackedVideoOutput());
        icp->SetConstant(255);
        icp->SetOutputNumberOfScalarComponents(4);
        //m_sliceStencil->SetInputData( Application::GetHardwareModule()->GetTrackedVideoOutput() );
        m_sliceStencil->SetInputConnection(icp->GetOutputPort());
        m_sliceStencil->SetBackgroundColor( 1.0, 1.0, 1.0, 0.0 );
        pv.imageActor->GetMapper()->SetInputConnection( m_sliceStencil->GetOutputPort() );
    }
}
