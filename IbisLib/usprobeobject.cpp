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
#include "imageobject.h"
#include "serializer.h"
#include "serializerhelper.h"
#include <QDir>
#include <QWidget>
#include "usprobeobjectsettingswidget.h"
#include "usmask.h"
#include "usmasksettingswidget.h"
#include "lookuptablemanager.h"
#include "vtkImageConstantPad.h"
#include "vtkPassThrough.h"
#include "vtkPiecewiseFunctionLookupTable.h"

ObjectSerializationMacro( UsProbeObject );
ObjectSerializationMacro( UsProbeObject::CalibrationMatrixInfo );

//---------------------------------------------------------------------------------------
// CalibrationMatrixInfo class implementation
//---------------------------------------------------------------------------------------

UsProbeObject::CalibrationMatrixInfo::CalibrationMatrixInfo()
{
    name = QString( "None" );
    matrix = vtkMatrix4x4::New();
}

UsProbeObject::CalibrationMatrixInfo::CalibrationMatrixInfo( const CalibrationMatrixInfo & other )
{
    name = other.name;
    matrix = vtkMatrix4x4::New();
    matrix->DeepCopy( other.matrix );
}

UsProbeObject::CalibrationMatrixInfo::~CalibrationMatrixInfo()
{
    matrix->Delete();
}

bool UsProbeObject::CalibrationMatrixInfo::Serialize( Serializer * serializer )
{
    bool res = true;
    res &= ::Serialize( serializer, "Name", name );
    res &= ::Serialize( serializer, "Matrix", matrix );
    return res;
}

//---------------------------------------------------------------------------------------
// UsProbeObject class implementation
//---------------------------------------------------------------------------------------

UsProbeObject::UsProbeObject()
{
    this->SetCanChangeParent( false );

    m_currentCalibrationMatrixIndex = -1;

    m_videoInput = vtkSmartPointer<vtkPassThrough>::New();
    m_actorInput = vtkSmartPointer<vtkPassThrough>::New();

    m_mask = USMask::New(); // mask in use
    m_defaultMask = USMask::New(); // default mask saved in ibis config file
    m_maskOn = false;

    m_lutIndex = 1;         // default to hot metal
    m_mapToColors = vtkSmartPointer<vtkImageMapToColors>::New();
    m_mapToColors->SetOutputFormatToRGBA();
    m_mapToColors->SetInputConnection( m_videoInput->GetOutputPort() );
    SetCurrentLUTIndex( m_lutIndex );

    m_imageStencilSource = vtkSmartPointer<vtkImageToImageStencil>::New();
    m_imageStencilSource->ThresholdByUpper( 128.0 );
    m_imageStencilSource->SetInputData( m_mask->GetMask() );
    m_imageStencilSource->UpdateWholeExtent();

    m_constantPad = vtkSmartPointer<vtkImageConstantPad>::New();
    m_constantPad->SetConstant(255);
    m_constantPad->SetOutputNumberOfScalarComponents(4);
    m_constantPad->SetInputConnection( m_videoInput->GetOutputPort() );

    m_sliceStencil = vtkSmartPointer<vtkImageStencil>::New();
    m_sliceStencil->SetStencilData( m_imageStencilSource->GetOutput() );
    m_sliceStencil->SetInputConnection( m_mapToColors->GetOutputPort() );
    m_sliceStencil->SetBackgroundColor( 1.0, 1.0, 1.0, 0.0 );

    m_imageTransform = vtkSmartPointer<vtkTransform>::New();
    m_imageTransform->SetInput( GetWorldTransform() );

    m_acquisitionType = ACQ_B_MODE;

    UpdatePipeline();
}

UsProbeObject::~UsProbeObject()
{
    m_mask->Delete();
    m_defaultMask->Delete();
}

void UsProbeObject::SerializeTracked( Serializer * ser )
{
    TrackedSceneObject::SerializeTracked( ser );
    ::Serialize( ser, "DefaultMask", m_defaultMask );
    ::Serialize( ser, "CurrentCalibrationMatrix", m_currentCalibrationMatrixIndex );
    ::Serialize( ser, "AllCalibrationMatrices", m_calibrationMatrices );
    if( ser->IsReader() )
    {
        m_defaultMask->SetAsDefault(); // this sets default params
        m_defaultMask->ResetToDefault(); // this will set mask params to deafult values and build the mask
        *m_mask = *m_defaultMask;
        m_mask->SetAsDefault();

        if( m_currentCalibrationMatrixIndex != -1 )
            SetCurrentCalibrationMatrixIndex( m_currentCalibrationMatrixIndex );
    }
}

void UsProbeObject::AddClient()
{
    GetHardwareModule()->AddTrackedVideoClient( this );
}

void UsProbeObject::RemoveClient()
{
    GetHardwareModule()->RemoveTrackedVideoClient( this );
}

void UsProbeObject::ObjectAddedToScene()
{
    // Check for updates
    connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(OnUpdate()) );
    connect( m_mask, SIGNAL(MaskChanged()), this, SLOT(UpdateMask()) );
}

void UsProbeObject::ObjectRemovedFromScene()
{
    disconnect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(OnUpdate()) );
}

void UsProbeObject::Setup( View * view )
{
    SceneObject::Setup( view );

    if( view->GetType() == THREED_VIEW_TYPE )
    {
        PerViewElements pv;
        pv.imageActor = vtkImageActor::New();
        pv.imageActor->SetUserTransform( m_imageTransform.GetPointer() );
        pv.imageActor->SetVisibility( this->IsHidden() ? 0 : 1 );
        pv.imageActor->GetMapper()->SetInputConnection( m_actorInput->GetOutputPort() );

        if( !this->IsHidden() )
            AddClient();

        view->GetRenderer()->AddActor( pv.imageActor );

        m_perViews[ view ] = pv;
    }
}

void UsProbeObject::Release( View * view )
{
    SceneObject::Release( view );

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
                RemoveClient();
        }
    }
}

void UsProbeObject::Hide()
{
    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        PerViewElements perView = (*it).second;
        View * view = (*it).first;
        perView.imageActor->VisibilityOff();
        RemoveClient();
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
        AddClient();
        ++it;
    }
    emit Modified();
}

int UsProbeObject::GetVideoImageWidth()
{
    return GetVideoOutput()->GetDimensions()[0];
}

int UsProbeObject::GetVideoImageHeight()
{
    return GetVideoOutput()->GetDimensions()[1];
}

int UsProbeObject::GetVideoImageNumberOfComponents()
{
    return GetVideoOutput()->GetNumberOfScalarComponents();
}

vtkImageData * UsProbeObject::GetVideoOutput()
{
    return vtkImageData::SafeDownCast( m_videoInput->GetOutput() );
}

vtkAlgorithmOutput * UsProbeObject::GetVideoOutputPort()
{
    return m_videoInput->GetOutputPort();
}

int UsProbeObject::GetNumberOfCalibrationMatrices()
{
    return m_calibrationMatrices.size();
}

void UsProbeObject::SetCurrentCalibrationMatrixIndex( int index )
{
    Q_ASSERT( index >= 0 && index < m_calibrationMatrices.size() );
    m_currentCalibrationMatrixIndex = index;
    SetCalibrationMatrix( m_calibrationMatrices[index].matrix );
}

QString UsProbeObject::GetCalibrationMatrixName( int index )
{
    Q_ASSERT( index >= 0 && index < m_calibrationMatrices.size() );
    return m_calibrationMatrices[ index ].name;
}

void UsProbeObject::SetCurrentCalibrationMatrixName( QString name )
{
    Q_ASSERT( m_currentCalibrationMatrixIndex >= 0 && m_currentCalibrationMatrixIndex < m_calibrationMatrices.size() );
    m_calibrationMatrices[ m_currentCalibrationMatrixIndex ].name = name;
}

QString UsProbeObject::GetCurrentCalibrationMatrixName()
{
    if( m_currentCalibrationMatrixIndex >= 0 )
        return m_calibrationMatrices[ m_currentCalibrationMatrixIndex ].name;
    return QString("NONE");
}

void UsProbeObject::SetCurrentCalibrationMatrix( vtkMatrix4x4 * mat )
{
    Q_ASSERT( m_currentCalibrationMatrixIndex >= 0 && m_currentCalibrationMatrixIndex < m_calibrationMatrices.size() );
    m_calibrationMatrices[ m_currentCalibrationMatrixIndex ].matrix->DeepCopy( mat );
    SetCalibrationMatrix( mat );
}

vtkMatrix4x4 * UsProbeObject::GetCurrentCalibrationMatrix()
{
    Q_ASSERT( m_currentCalibrationMatrixIndex >= 0 && m_currentCalibrationMatrixIndex < m_calibrationMatrices.size() );
    return m_calibrationMatrices[ m_currentCalibrationMatrixIndex ].matrix;
}

void UsProbeObject:: AddCalibrationMatrix( QString name )
{
    CalibrationMatrixInfo newEntry;
    int index = m_calibrationMatrices.size();
    m_calibrationMatrices.push_back( newEntry );
    this->SetCurrentCalibrationMatrixIndex( index );
    this->SetCurrentCalibrationMatrixName( name );
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

void UsProbeObject::SetVideoInputConnection( vtkAlgorithmOutput * port )
{
    m_videoInput->SetInputConnection( port );
}

void UsProbeObject::SetVideoInputData( vtkImageData * image )
{
    m_videoInput->SetInputData( image );
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
    vtkSmartPointer<vtkPiecewiseFunctionLookupTable> lut = vtkSmartPointer<vtkPiecewiseFunctionLookupTable>::New();
    lut->SetIntensityFactor( 1.0 );
    Application::GetLookupTableManager()->CreateLookupTable( slicesLutName, range, lut.GetPointer() );
    m_mapToColors->SetLookupTable( lut.GetPointer() );
    emit Modified();
}

void UsProbeObject::OnUpdate()
{
    //std::cout << "image size: ( " << GetVideoImageWidth() << ", " << GetVideoImageHeight() << " ) - num comp: " << GetVideoImageNumberOfComponents() << std::endl;

    //vtkImageData * videoImage = GetVideoOutput();
    //double * orig = videoImage->GetOrigin();
    //std::cout << "origin: ( " << orig[0] << ", " << orig[1] << ", " << orig[2] << " )" << std::endl;
    //double * spacing = videoImage->GetSpacing();
    //std::cout << "spacing: ( " << spacing[0] << ", " << spacing[1] << ", " << spacing[2] << " )" << std::endl;
    emit Modified();
}

void UsProbeObject::UpdateMask()
{
    m_imageStencilSource->Update();
    m_mapToColors->Update();
    emit Modified();
}

void UsProbeObject::UpdatePipeline()
{
    bool bMode = m_acquisitionType == ACQ_B_MODE;
    if( !bMode && !m_maskOn )
        m_actorInput->SetInputConnection( m_videoInput->GetOutputPort() );
    else if( bMode && !m_maskOn )
        m_actorInput->SetInputConnection( m_mapToColors->GetOutputPort() );
    else if( bMode && m_maskOn )
    {
        m_sliceStencil->SetInputConnection( m_mapToColors->GetOutputPort() );
        m_actorInput->SetInputConnection( m_sliceStencil->GetOutputPort() );
    }
    else // !bMode && m_maskOn
    {
        m_sliceStencil->SetInputConnection( m_constantPad->GetOutputPort() );
        m_actorInput->SetInputConnection( m_sliceStencil->GetOutputPort() );
    }
}
