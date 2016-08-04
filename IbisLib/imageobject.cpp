/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <sstream>
#include "imageobject.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkImageData.h"
#include "view.h"
#include "vtkTransform.h"
#include "vtkImageAccumulate.h"

#include "vtkSmartVolumeMapper.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunctionLookupTable.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageImport.h"
#include "vtkEventQtSlotConnect.h"

#include "application.h"
#include "imageobjectsettingsdialog.h"
#include "imageobjectvolumesettingswidget.h"
#include "scenemanager.h"
#include "triplecutplaneobject.h"
#include "lookuptablemanager.h"
#include <QMessageBox>

ObjectSerializationMacro( ImageObject );

#include <itkImageFileWriter.h>

const int ImageObject::NumberOfBinsInHistogram = 256;

template< class TInputImage >
IbisItkVTKImageExport< TInputImage >::IbisItkVTKImageExport()
{
    for( int i = 0; i < 3; ++i )
        vtkOrigin[ i ] = 0.0;
}

template< class TInputImage >
double * IbisItkVTKImageExport< TInputImage >::OriginCallback()
{
    // run base class
    double * orig = itk::VTKImageExport< TInputImage >::OriginCallback();

    // Get inverse of the dir cosine matrix
    InputImagePointer input = this->GetInput();
    itk::Matrix< double, 3, 3 > dir_cos = input->GetDirection();
    vnl_matrix_fixed< double, 3, 3 > inv_dir_cos = dir_cos.GetTranspose();

    // Transform the origin back to the way vtk sees it
    vnl_vector_fixed< double, 3 > origin;
    vnl_vector_fixed< double, 3 > o_origin;
    for( int j = 0; j < 3; j++ )
        o_origin[ j ] = orig[ j ];
    origin = inv_dir_cos * o_origin;

    for( int i = 0; i < 3; ++i )
        vtkOrigin[ i ] = origin[ i ];

    return vtkOrigin;
}

ImageObject::PerViewElements::PerViewElements()
{
    this->outlineActor = 0;
    this->volume = 0;
}   

ImageObject::PerViewElements::~PerViewElements()
{
}   

ImageObject::ImageObject()
{
    this->ItkImage = 0;
    this->ItkRGBImage = 0;
    this->ItkLabelImage = 0;
    this->Image = 0;
    this->Lut = 0;
    this->OutlineFilter = vtkOutlineFilter::New();
    this->viewOutline = 0;
    this->outlineWasVisible = 0;
    this->viewError = 0;
	this->lutIndex = -1;
    this->lutRange[0] = 0.0;
    this->lutRange[1] = 0.0;
    this->intensityFactor = 1.0;
    this->HistogramComputer = vtkImageAccumulate::New();

    // setup default volume properties for vtk volume rendering
    m_vtkVolumeRenderingEnabled = false;
    m_volumeProperty = vtkVolumeProperty::New();
    m_volumeProperty->SetInterpolationTypeToLinear();
    vtkPiecewiseFunction * scalarOpacity = vtkPiecewiseFunction::New();
    scalarOpacity->AddPoint( 0.0, 0.0 );
    scalarOpacity->AddPoint( 255.0, 1.0 );
    m_volumeProperty->SetScalarOpacity( scalarOpacity );
    scalarOpacity->Delete();
    vtkPiecewiseFunction * gradientOpacity = vtkPiecewiseFunction::New();
    gradientOpacity->AddPoint( 0.0, 1.0 );
    gradientOpacity->AddPoint( 255.0, 1.0 );
    m_volumeProperty->SetGradientOpacity( gradientOpacity );
    gradientOpacity->Delete();
    vtkColorTransferFunction * transferFunction = vtkColorTransferFunction::New();
    transferFunction->AddRGBPoint( 0.0 * 255, 0.0, 0.0, 0.0 );
    transferFunction->AddRGBPoint( 1.0 * 255, 1.0, 1.0, 1.0 );
    m_volumeProperty->SetColor( transferFunction );
    transferFunction->Delete();

    // Watch volume properties to be able to render when properties change
    m_volumePropertyWatcher = vtkEventQtSlotConnect::New();
    m_volumePropertyWatcher->Connect( m_volumeProperty, vtkCommand::ModifiedEvent, this, SLOT(MarkModified()) );
    m_volumePropertyWatcher->Connect( m_volumeProperty->GetScalarOpacity(), vtkCommand::ModifiedEvent, this, SLOT(MarkModified()) );
    m_volumePropertyWatcher->Connect( m_volumeProperty->GetGradientOpacity(), vtkCommand::ModifiedEvent, this, SLOT(MarkModified()) );
    m_volumePropertyWatcher->Connect( m_volumeProperty->GetRGBTransferFunction(), vtkCommand::ModifiedEvent, this, SLOT(MarkModified()) );

    m_colorWindow = 1.0;
    m_colorLevel = 0.5;
    m_vtkVolumeRenderMode = vtkSmartVolumeMapper::GPURenderMode;
    m_autoSampleDistance = true;
    m_sampleDistance = 1.0;
}

ImageObject::~ImageObject()
{
    if( this->Image )
    {
        this->Image->UnRegister( this );
    }
    if( this->Lut )
    {
        this->Lut->Delete();
    }

    this->OutlineFilter->Delete();
    this->HistogramComputer->Delete();

    m_volumeProperty->Delete();
    m_volumePropertyWatcher->Delete();
}

#include "serializerhelper.h"

void ImageObject::Serialize( Serializer * ser )
{
    SceneObject::Serialize(ser);
    bool labelImage = false;
    if( !ser->IsReader() )
    {
        labelImage = this->IsLabelImage();
    }
    ::Serialize( ser, "LabelImage", labelImage );
    ::Serialize( ser, "ViewOutline", this->viewOutline );
    ::Serialize( ser, "LutIndex", this->lutIndex );
    ::Serialize( ser, "LutRange", this->lutRange, 2 );
    ::Serialize( ser, "IntensityFactor", this->intensityFactor );

    // Vtk volume rendering
    bool enableShading = false;
    double ambiant = .3;
    double diffuse = .7;
    double specular = .2;
    double specularPower = 10.0;
    bool enableGradientOpacity = true;

    if( !ser->IsReader() )
    {
        enableShading = m_volumeProperty->GetShade() == 1 ? true : false;
        ambiant = m_volumeProperty->GetAmbient();
        diffuse = m_volumeProperty->GetDiffuse();
        specular = m_volumeProperty->GetSpecular();
        specularPower = m_volumeProperty->GetSpecularPower();
        enableGradientOpacity = m_volumeProperty->GetDisableGradientOpacity() == 1 ? false : true;
    }
    ::Serialize( ser, "VolumeRenderingEnabled", m_vtkVolumeRenderingEnabled );
    ::Serialize( ser, "VolumeRenderMode", m_vtkVolumeRenderMode );
    ::Serialize( ser, "ColorWindow", m_colorWindow );
    ::Serialize( ser, "ColorLevel", m_colorLevel );
    ::Serialize( ser, "EnableShading", enableShading );
    ::Serialize( ser, "Ambiant", ambiant );
    ::Serialize( ser, "Diffuse", diffuse );
    ::Serialize( ser, "Specular", specular );
    ::Serialize( ser, "SpecularPower", specularPower );
    ::Serialize( ser, "EnableGradientOpacity", enableGradientOpacity );
    ::Serialize( ser, "AutoSampleDistance", m_autoSampleDistance );
    ::Serialize( ser, "SampleDistance", m_sampleDistance );
    if( ser->IsReader() )
    {
        m_volumeProperty->SetShade( enableShading ? 1 : 0 );
        m_volumeProperty->SetAmbient( ambiant );
        m_volumeProperty->SetDiffuse( diffuse );
        m_volumeProperty->SetSpecular( specular );
        m_volumeProperty->SetSpecularPower( specularPower );
        m_volumeProperty->SetDisableGradientOpacity( enableGradientOpacity ? 0 : 1 );
    }
    ::Serialize( ser, "ScalarOpacity", m_volumeProperty->GetScalarOpacity() );
    ::Serialize( ser, "GradientOpacity", m_volumeProperty->GetGradientOpacity() );
    ::Serialize( ser, "ColorTransferFunction", m_volumeProperty->GetRGBTransferFunction() );
}

void ImageObject::Export()
{
    QString objectName(this->Name);
    objectName.append(".mnc");
    this->SetDataFileName(objectName);
    QString fullName(this->GetManager()->GetSceneDirectory());
    fullName.append("/");
    fullName.append(objectName);
    QString saveName = Application::GetInstance().GetSaveFileName( tr("Save Object"), fullName, tr("*.mnc") );
    if(saveName.isEmpty())
        return;
    if (QFile::exists(saveName))
    {
        int ret = QMessageBox::warning(0, tr("Save ImageObject: "), saveName,
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No | QMessageBox::Escape);
        if (ret == QMessageBox::No)
            return;
    }

    this->WriteFile(saveName);
}

void ImageObject::ObjectAddedToScene()
{
    this->SetupInCutPlanes();
}

bool ImageObject::IsLabelImage()
{
    if( this->ItkLabelImage )
        return true;
    return false;
}

void ImageObject::SetItkImage( IbisItk3DImageType::Pointer image )
{
    if( !this->ItkToVtkExporter )
        BuildItkToVtkExport();
    this->ItkImage = image;
    if( this->ItkImage )
    {
        this->ItkToVtkExporter->SetInput( this->ItkImage );
        this->ItkToVtkImporter->Update();
        this->SetImage( this->ItkToVtkImporter->GetOutput() );

        // Use itk image's dir cosines as the local transform for this image
        itk::Matrix< double, 3, 3 > dirCosines = this->ItkImage->GetDirection();
        vtkMatrix4x4 * rotMat = vtkMatrix4x4::New();
        for( unsigned i = 0; i < 3; ++i )
            for( unsigned j = 0; j < 3; ++j )
                rotMat->SetElement( i, j, dirCosines( i, j ) );
        vtkTransform * rotTrans = vtkTransform::New();
        rotTrans->SetMatrix( rotMat );
        this->SetLocalTransform( rotTrans );
        rotMat->Delete();
        rotTrans->Delete();
    }
}

void ImageObject::SetItkImage( IbisRGBImageType::Pointer image )
{
    if( !this->ItkRGBImageToVtkExporter )
        BuildItkRGBImageToVtkExport();
    this->ItkRGBImage = image;
    if( this->ItkRGBImage )
    {
        this->ItkRGBImageToVtkExporter->SetInput( this->ItkRGBImage );
        this->ItkToVtkImporter->Update();
        this->SetImage( this->ItkToVtkImporter->GetOutput() );

        // Use itk image's dir cosines as the local transform for this image
        itk::Matrix< double, 3, 3 > dirCosines = this->ItkRGBImage->GetDirection();
        vtkMatrix4x4 * rotMat = vtkMatrix4x4::New();
        for( unsigned i = 0; i < 3; ++i )
            for( unsigned j = 0; j < 3; ++j )
                rotMat->SetElement( i, j, dirCosines( i, j ) );
        vtkTransform * rotTrans = vtkTransform::New();
        rotTrans->SetMatrix( rotMat );
        this->SetLocalTransform( rotTrans );
        rotMat->Delete();
        rotTrans->Delete();
    }
}


void ImageObject::SetItkLabelImage( IbisItk3DLabelType::Pointer image )
{
    if( !this->ItkToVtkLabelExporter )
        BuildItkToVtkLabelExport();
    this->ItkLabelImage = image;
    if( this->ItkLabelImage )
    {
        this->ItkToVtkLabelExporter->SetInput( this->ItkLabelImage );
        this->ItkToVtkImporter->Update();
        this->SetImage( this->ItkToVtkImporter->GetOutput() );

        // Use itk image's dir cosines as the local transform for this image
        itk::Matrix< double, 3, 3 > dirCosines = this->ItkLabelImage->GetDirection();
        vtkMatrix4x4 * rotMat = vtkMatrix4x4::New();
        for( unsigned i = 0; i < 3; ++i )
            for( unsigned j = 0; j < 3; ++j )
                rotMat->SetElement( i, j, dirCosines( i, j ) );
        vtkTransform * rotTrans = vtkTransform::New();
        rotTrans->SetMatrix( rotMat );
        this->SetLocalTransform( rotTrans );
        rotMat->Delete();
        rotTrans->Delete();
    }
}

void ImageObject::SetImage(vtkImageData * image)
{
    if( this->Image == image )
    {
        return;
    }

    if( this->Image )
    {
        this->Image->UnRegister( this );
    }
    this->Image = image;
    if( this->Image )
    {
        this->Image->Register( this );
    }

    double range[2];
    this->Image->GetScalarRange( range );
    this->HistogramComputer->SetInputData( this->Image );
    SetupHistogramComputer();
    this->OutlineFilter->SetInputData( this->Image );
}

// simtodo : this shouldn't be needed, but calling Modified only on itk image
// doesn't seem to be enough to update the voxels in the vtk image.
void ImageObject::ForceUpdatePixels()
{
    if( this->ItkImage )
    {
        this->ItkImage->Modified();
        this->ItkToVtkExporter->Modified();
        this->ItkToVtkImporter->Modified();
    }
}

//================================================================================
// to get histogram information for an image with 1 scalar component:
//        vtkImageData * hist = imageObject->GetHistogramComputer()->GetOutput();
//        int * ptrData = (int*)hist->GetScalarPointer( 0, 0, 0 );
//        int nbBins = hist->GetExtent[1];
//        for( int i = 0; i < nbBins; ++i )
//            int binValuei = ptrData[i];
//================================================================================
void ImageObject::SetupHistogramComputer()
{
    if( !this->Image )
        return;

    Q_ASSERT_X( this->NumberOfBinsInHistogram > 0, "ImageObject::SetupHistogramComputer()", "Number of bins has to be > 0." );

    double range[2];
    this->Image->GetScalarRange( range );
    double binSize = ( range[1] - range[0] ) / this->NumberOfBinsInHistogram;

    this->HistogramComputer->SetComponentOrigin( range[0], 0, 0 );
    this->HistogramComputer->SetComponentExtent( 0, this->NumberOfBinsInHistogram - 1, 0, 0, 0, 0 );
    this->HistogramComputer->SetComponentSpacing( binSize, 1, 1 );
    this->HistogramComputer->Update();
}


void ImageObject::SetupInCutPlanes()
{
    Q_ASSERT( GetManager() );
    if( this->Image && this->Image->GetNumberOfScalarComponents() == 1 )
    {
        if( this->lutIndex < 0 )
            this->lutIndex = 0;
        if( this->lutRange[0] == 0.0 && this->lutRange[1] == 0.0 )
            this->Image->GetScalarRange( this->lutRange );
        this->ChooseColorTable( this->lutIndex );
	}
}

void ImageObject::Setup( View * view )
{
    SceneObject::Setup( view );

    TripleCutPlaneObject * mainPlane = this->GetManager()->GetMainImagePlanes();
    Q_ASSERT( mainPlane );
    mainPlane->AddImage( this->GetObjectID() );
    connect( this, SIGNAL(Modified()), mainPlane, SLOT(MarkModified()) );  // This is to make sure transforms update the reslice - simtodo : find better solution.
    mainPlane->Setup( view );

    switch( view->GetType() )
    {
        case SAGITTAL_VIEW_TYPE:
			this->Setup2DRepresentation( 0, view );
            break;
        case CORONAL_VIEW_TYPE:
			this->Setup2DRepresentation( 1, view );
            break;
        case TRANSVERSE_VIEW_TYPE:
			this->Setup2DRepresentation( 2, view );
            break;
        case THREED_VIEW_TYPE:
			this->Setup3DRepresentation( view );
            break;
    }

    this->SetViewOutline(this->viewOutline);
    this->GetManager()->ResetAllCameras();
}

void ImageObject::PreDisplaySetup()
{
    Q_ASSERT( this->GetManager() );
    this->GetManager()->GetMainImagePlanes()->PreDisplaySetup();
}

void ImageObject::Release( View * view )
{
    SceneObject::Release( view );

    Q_ASSERT( this->GetManager() );
    this->GetManager()->GetMainImagePlanes()->RemoveImage( this->GetObjectID() );

    switch( view->GetType() )
    {
        case SAGITTAL_VIEW_TYPE:
            this->Release2DRepresentation( 0, view );
            break;
        case CORONAL_VIEW_TYPE:
            this->Release2DRepresentation( 1, view );
            break;
        case TRANSVERSE_VIEW_TYPE:
            this->Release2DRepresentation( 2, view );
            break;
        case THREED_VIEW_TYPE:
            this->Release3DRepresentation( view );
            break;
    }
}

void ImageObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets )
{
    ImageObjectSettingsDialog * res = new ImageObjectSettingsDialog( parent );
    res->setAttribute(Qt::WA_DeleteOnClose);
    res->SetImageObject( this );
    res->setObjectName("Properties");
    widgets->append(res);

    ImageObjectVolumeSettingsWidget * volWidget = new ImageObjectVolumeSettingsWidget( parent );
    volWidget->setAttribute(Qt::WA_DeleteOnClose);
    volWidget->SetImageObject( this );
    volWidget->setObjectName("Volume");
    widgets->append( volWidget );
}

void ImageObject::SetVtkVolumeRenderingEnabled( bool on )
{
    if( m_vtkVolumeRenderingEnabled == on )
        return;

    m_vtkVolumeRenderingEnabled = on;

    ImageObjectViewAssociation::iterator it = this->imageObjectInstances.begin();
    while( it != imageObjectInstances.end() )
    {
        View * v = (*it).first;
        if( v->GetType() == THREED_VIEW_TYPE )
        {
            PerViewElements * pv = (*it).second;
            pv->volume->SetVisibility( m_vtkVolumeRenderingEnabled ? 1 : 0 );
        }
        ++it;
    }

    emit Modified();
}

void ImageObject::SetVtkVolumeRenderMode( int mode )
{
    m_vtkVolumeRenderMode = mode;

    ImageObjectViewAssociation::iterator it = this->imageObjectInstances.begin();
    while( it != imageObjectInstances.end() )
    {
        View * v = (*it).first;
        if( v->GetType() == THREED_VIEW_TYPE )
        {
            PerViewElements * pv = (*it).second;
            vtkSmartVolumeMapper * mapper = vtkSmartVolumeMapper::SafeDownCast( pv->volume->GetMapper() );
            if( mapper )
                mapper->SetRequestedRenderMode( m_vtkVolumeRenderMode );
        }
        ++it;
    }
    emit Modified();
}

void ImageObject::SetVolumeRenderingWindow( double window )
{
    m_colorWindow = window;
    UpdateVolumeRenderingParamsInMapper();
}

void ImageObject::SetVolumeRenderingLevel( double level )
{
    m_colorLevel = level;
    UpdateVolumeRenderingParamsInMapper();
}

void ImageObject::SetAutoSampleDistance( bool on )
{
    m_autoSampleDistance = on;
}

void ImageObject::SetSampleDistance( double dist )
{
    m_sampleDistance = dist;
}

void ImageObject::UpdateVolumeRenderingParamsInMapper()
{
    ImageObjectViewAssociation::iterator it = this->imageObjectInstances.begin();
    while( it != imageObjectInstances.end() )
    {
        View * v = (*it).first;
        if( v->GetType() == THREED_VIEW_TYPE )
        {
            PerViewElements * pv = (*it).second;
            vtkSmartVolumeMapper * mapper = vtkSmartVolumeMapper::SafeDownCast( pv->volume->GetMapper() );
            if( mapper )
            {
                mapper->SetFinalColorLevel( m_colorLevel );
                mapper->SetFinalColorWindow( m_colorWindow );
            }
        }
        ++it;
    }
    emit Modified();
}

void ImageObject::SetViewOutline( int isOn )
{
    this->viewOutline = isOn;

    if( isOn )
    {
        ImageObjectViewAssociation::iterator it = this->imageObjectInstances.begin();
        for( ; it != this->imageObjectInstances.end(); ++it )
        {
            if( (*it).second->outlineActor )
            {
                if ((*it).first->GetType() == THREED_VIEW_TYPE)
                    (*it).second->outlineActor->VisibilityOn();
                this->outlineWasVisible = 1;
            }
        }
    }
    else
    {
        ImageObjectViewAssociation::iterator it = this->imageObjectInstances.begin();
        for( ; it != this->imageObjectInstances.end(); ++it )
        {
            if( (*it).second->outlineActor )
            {
                (*it).second->outlineActor->VisibilityOff();
                this->outlineWasVisible = 0;
            }
        }
    }

    emit Modified();
}

int ImageObject::GetViewOutline()
{
    return this->viewOutline;
}

void ImageObject::ResetViewError( )
{
    this->viewError = 0;
}

int ImageObject::GetViewError()
{
    return this->viewError;
}

#include "vtkImageShiftScale.h"
void ImageObject::Setup3DRepresentation( View * view )
{
    // add an outline to the view to make sure camera includes the whole volume
    vtkPolyDataMapper * outMapper = vtkPolyDataMapper::New();
    this->OutlineFilter->Update();
    outMapper->SetInputConnection( this->OutlineFilter->GetOutputPort() );
    vtkActor * outActor = vtkActor::New();
    outActor->SetMapper( outMapper );
	outActor->SetUserTransform( this->WorldTransform );
    if( this->viewOutline )
        outActor->VisibilityOn();
    else
        outActor->VisibilityOff();
    outMapper->Delete();
    view->GetRenderer()->AddActor( outActor );

    // vtk volume renderer
    vtkSmartVolumeMapper * volumeMapper = vtkSmartVolumeMapper::New();
    volumeMapper->SetRequestedRenderMode( m_vtkVolumeRenderMode );
    volumeMapper->SetFinalColorLevel( m_colorLevel );
    volumeMapper->SetFinalColorWindow( m_colorWindow );
    vtkImageShiftScale * volumeShiftScale = vtkImageShiftScale::New();
    volumeShiftScale->SetOutputScalarTypeToUnsignedChar();
    volumeShiftScale->SetInputData( this->GetImage() );
    double imageScalarRange[2];
    this->GetImage()->GetScalarRange( imageScalarRange );
    volumeShiftScale->SetShift( -imageScalarRange[0] );
    double scale = 255.0 / ( imageScalarRange[1] - imageScalarRange[0] );
    volumeShiftScale->SetScale( scale );
    volumeMapper->SetInputConnection( volumeShiftScale->GetOutputPort() );
    volumeShiftScale->Delete();
    vtkVolume * volume = vtkVolume::New();
    volume->SetMapper( volumeMapper );
    volumeMapper->Delete();
    volume->SetProperty( m_volumeProperty );
    volume->SetUserTransform( this->WorldTransform );
    volume->SetVisibility( m_vtkVolumeRenderingEnabled ? 1 : 0 );
    view->GetRenderer()->AddVolume( volume );

    // Add the actors to the map of instances we keep
    PerViewElements * elem = new PerViewElements;
    elem->outlineActor = outActor;
    elem->volume = volume;
    this->imageObjectInstances[ view ] = elem;
}

void ImageObject::Release3DRepresentation( View * view )
{
    // release the outline
    ImageObjectViewAssociation::iterator itAssociations = this->imageObjectInstances.find( view );
    if( itAssociations != this->imageObjectInstances.end() )
    {
        PerViewElements * perView = (*itAssociations).second;

        // remove outline
		view->GetRenderer()->RemoveViewProp( perView->outlineActor );
		perView->outlineActor->Delete();

        // remove volume
        view->GetRenderer()->RemoveViewProp( perView->volume );
        perView->volume->Delete();

        delete perView;
        this->imageObjectInstances.erase( itAssociations );
    }
}

void ImageObject::Setup2DRepresentation( int viewType, View * view )
{
    // add an outline to the view to make sure camera includes the whole volume
    vtkPolyDataMapper * outMapper = vtkPolyDataMapper::New();
    this->OutlineFilter->Update();
    outMapper->SetInputConnection(this->OutlineFilter->GetOutputPort() );
    vtkActor * outActor = vtkActor::New();
    outActor->SetMapper( outMapper );
    outMapper->Delete();

	view->GetRenderer()->AddActor( outActor );

    // make the outline invisible in 2D, don't remove it yet, I may need it later
    outActor->VisibilityOff();

    // Add the actor to the map of instances we keep
    PerViewElements * elem = new PerViewElements;
    elem->outlineActor = outActor;
    this->imageObjectInstances[ view ] = elem;
}

void ImageObject::Release2DRepresentation( int viewType, View * view )
{
    // release the outline
    ImageObjectViewAssociation::iterator itAssociations = this->imageObjectInstances.find( view );
    if( itAssociations != this->imageObjectInstances.end() )
    {
        PerViewElements * perView = (*itAssociations).second;

        // delete the vtkAssembly we have
		view->GetRenderer()->RemoveViewProp( perView->outlineActor );
		perView->outlineActor->Delete();

        delete perView;
        this->imageObjectInstances.erase( itAssociations );
    }
}

void ImageObject::SetLut(vtkScalarsToColors *lut)
{
    if (this->Lut != lut)
    {
        if (this->Lut != NULL)
        {
            this->Lut->UnRegister(0);
        }
        this->Lut = lut;
        if (this->Lut != NULL)
        {
            this->Lut->Register(0);
        }
    }
    emit LutChanged();
    emit Modified();
}

int ImageObject::ChooseColorTable(int index)
{
    Q_ASSERT( this->GetManager() );

    // Make sure the index is valid
    if( index < 0 || index > Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables() - 1 )
    {
        if( Application::GetLookupTableManager()->GetNumberOfTemplateLookupTables() > 0 )
            index = 0;
        else
            return 0;
    }

    // Generate the lookup table
    this->lutIndex = index;
    if( IsLabelImage() )
    {
        vtkLookupTable * lut = vtkLookupTable::New();
        Application::GetLookupTableManager()->CreateLabelLookupTable( lut);
        this->SetLut( lut );
        lut->Delete();
    }
    else
    {
        QString tableName = Application::GetLookupTableManager()->GetTemplateLookupTableName( this->lutIndex );

        vtkPiecewiseFunctionLookupTable * lut = vtkPiecewiseFunctionLookupTable::New();
        lut->SetIntensityFactor( this->intensityFactor );
        Application::GetLookupTableManager()->CreateLookupTable( tableName, this->lutRange, lut );
        this->SetLut( lut );
        lut->Delete();
    }

	// Tell the main cut planes to change the lookup table

    this->GetManager()->GetMainImagePlanes()->UpdateLut( this->GetObjectID() );

    emit Modified();
    return 1;
}

double * ImageObject::GetLutRange()
{
    return this->lutRange;
}

void ImageObject::SetLutRange( double r[2] )
{
    this->lutRange[0] = r[0];
    this->lutRange[1] = r[1];
    this->Lut->SetRange( r );
    emit Modified();
}

void ImageObject::GetImageScalarRange(double *range)
{
    this->Image->GetScalarRange( range );
}

int ImageObject::GetNumberOfScalarComponents()
{
    if( this->Image )
        return this->Image->GetNumberOfScalarComponents();
    else
        return 0;
}

void ImageObject::GetBounds( double bounds[6] )
{
    this->Image->GetBounds( bounds );
}

void ImageObject::GetCenter( double center[3] )
{
    this->Image->GetCenter( center );
}

double * ImageObject::GetSpacing()
{
    return this->Image->GetSpacing();
}

void ImageObject::SetIntensityFactor( double factor )
{
    if( this->intensityFactor != factor )
    {
        this->intensityFactor = factor;
        vtkPiecewiseFunctionLookupTable * lut = vtkPiecewiseFunctionLookupTable::SafeDownCast(this->GetLut());
        if( lut )
            lut->SetIntensityFactor( factor );
        emit Modified();
    }
}

double ImageObject::GetIntensityFactor()
{
    return this->intensityFactor;
}

void ImageObject::Hide()
{
	if (this->viewOutline)
	{
		this->SetViewOutline(0);
		this->outlineWasVisible = 1;
	}
	else
		this->outlineWasVisible = 0;
    if( this->GetManager() )
        this->GetManager()->GetMainImagePlanes()->SetImageHidden( this->GetObjectID(), true );
    emit Modified();
}

void ImageObject::Show()
{
	if( !this->viewOutline && this->outlineWasVisible )
		this->SetViewOutline(1);
    if( this->GetManager() )
        this->GetManager()->GetMainImagePlanes()->SetImageHidden( this->GetObjectID(), false );
    emit Modified();
}

#include "mincinfowidget.h"
void ImageObject::ShowMincInfo()
{
    MincInfoWidget * w = new MincInfoWidget( );
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->SetImageObject( this );
    w->move( 0, 0 );
    w->show();
}

void ImageObject::BuildItkToVtkExport()
{
    this->ItkToVtkExporter = ItkExporterType::New();
    BuildVtkImport( this->ItkToVtkExporter );
}

void ImageObject::BuildItkRGBImageToVtkExport()
{
    this->ItkRGBImageToVtkExporter = ItkRGBImageExporterType::New();
    BuildVtkImport( this->ItkRGBImageToVtkExporter );
}


void ImageObject::BuildItkToVtkLabelExport()
{
    this->ItkToVtkLabelExporter = ItkLabelExporterType::New();
    BuildVtkImport( this->ItkToVtkLabelExporter );
}

void ImageObject::BuildVtkImport( itk::VTKImageExportBase * exporter )
{
    this->ItkToVtkImporter = vtkImageImport::New();
    this->ItkToVtkImporter->SetUpdateInformationCallback( exporter->GetUpdateInformationCallback() );
    this->ItkToVtkImporter->SetPipelineModifiedCallback( exporter->GetPipelineModifiedCallback() );
    this->ItkToVtkImporter->SetWholeExtentCallback( exporter->GetWholeExtentCallback() );
    this->ItkToVtkImporter->SetSpacingCallback( exporter->GetSpacingCallback() );
    this->ItkToVtkImporter->SetOriginCallback( exporter->GetOriginCallback() );
    this->ItkToVtkImporter->SetScalarTypeCallback( exporter->GetScalarTypeCallback() );
    this->ItkToVtkImporter->SetNumberOfComponentsCallback( exporter->GetNumberOfComponentsCallback() );
    this->ItkToVtkImporter->SetPropagateUpdateExtentCallback( exporter->GetPropagateUpdateExtentCallback() );
    this->ItkToVtkImporter->SetUpdateDataCallback( exporter->GetUpdateDataCallback() );
    this->ItkToVtkImporter->SetDataExtentCallback( exporter->GetDataExtentCallback() );
    this->ItkToVtkImporter->SetBufferPointerCallback( exporter->GetBufferPointerCallback() );
    this->ItkToVtkImporter->SetCallbackUserData( exporter->GetCallbackUserData() );
}

//generic file writer
void ImageObject::WriteFile(QString &name)
{
    if( this->ItkImage )
    {
        itk::ImageFileWriter< IbisItk3DImageType >::Pointer mincWriter = itk::ImageFileWriter<IbisItk3DImageType>::New();
        mincWriter->SetFileName(name.toUtf8().data());

        mincWriter->SetInput(this->ItkImage);

        try
        {
            mincWriter->Update();
        }
        catch(itk::ExceptionObject & exp)
        {
            std::cerr << "Exception caught!" << std::endl;
            std::cerr << exp << std::endl;
        }
    }
    else if( this->ItkRGBImage )
    {
        itk::ImageFileWriter< IbisRGBImageType >::Pointer mincWriter = itk::ImageFileWriter<IbisRGBImageType>::New();
        mincWriter->SetFileName(name.toUtf8().data());

        mincWriter->SetInput(this->ItkRGBImage);

        try
        {
            mincWriter->Update();
        }
        catch(itk::ExceptionObject & exp)
        {
            std::cerr << "Exception caught!" << std::endl;
            std::cerr << exp << std::endl;
        }
    }
}
