/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "usacquisitionobject.h"
#include "trackedvideobuffer.h"
#include "application.h"
#include "filereader.h"
#include "ignsconfig.h"
#include "scenemanager.h"
#include "usacquisitionsettingswidget.h"
#include "usmasksettingswidget.h"
#include "imageobject.h"
#include "ignsmsg.h"
#include "usmask.h"
#include <QDir>
#include <QMessageBox>
#include <qapplication.h>
#include <qprogressdialog.h>
#include <QDateTime>
#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkImageLuminance.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkTransform.h"
#include "vtkmatrix4x4utilities.h"
#include "vtkImageStencil.h"
#include "vtkImageToImageStencil.h"
//#include "vtkImageStack.h"
//#include "vtkImageSliceCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageConstantPad.h" // added Mar 2, 2016, Xiao
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkLookupTable.h"
#include "vtkImageMapToColors.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "view.h"
#include "imageobject.h"
#include "lookuptablemanager.h"

ObjectSerializationMacro( USAcquisitionObject );

USAcquisitionObject::USAcquisitionObject()
{
    m_videoBuffer = new TrackedVideoBuffer;

    m_isRecording = false;
    m_baseDirectory = QDir::homePath() + "/" + IGNS_CONFIGURATION_SUBDIRECTORY + "/" + IGNS_ACQUISITION_BASE_DIR;

    m_usDepth = "9cm";
    m_acquisitionType = UsProbeObject::ACQ_B_MODE;

    // current slice
    m_calibrationTransform = vtkTransform::New();
    m_sliceTransform = vtkTransform::New();
    m_sliceTransform->Concatenate( this->WorldTransform );
    m_currentImageTransform = vtkTransform::New();
    m_sliceTransform->Concatenate( m_currentImageTransform );
    m_sliceTransform->Concatenate( m_calibrationTransform );
    m_sliceProperties = vtkImageProperty::New();
    m_sliceLutIndex = 1;         // default to hot metal
    m_mapToColors = vtkImageMapToColors::New();
    m_mapToColors->SetOutputFormatToRGBA();

    m_mask = USMask::New(); // default mask

    m_imageStencilSource = vtkImageToImageStencil::New();
    m_imageStencilSource->SetInputData( m_mask->GetMask() );
    m_imageStencilSource->ThresholdByUpper( 128.0 );
    m_imageStencilSource->UpdateWholeExtent();

    m_sliceStencil = vtkImageStencil::New();
    m_sliceStencil->SetStencilData( m_imageStencilSource->GetOutput() );
    m_sliceStencil->SetInputConnection( m_mapToColors->GetOutputPort() );
    m_sliceStencil->SetBackgroundColor( 1.0, 1.0, 1.0, 0.0 );

    m_isMaskOn = true;


    m_constantPad = vtkImageConstantPad::New(); // added Mar 2,2016, by Xiao
    m_constantPad->SetConstant(255);
    m_constantPad->SetOutputNumberOfScalarComponents(4);
    m_isDopplerOn = false; // default state is B-mode

    m_sliceStencilDoppler = vtkImageStencil::New();
    m_sliceStencilDoppler->SetStencilData( m_imageStencilSource->GetOutput() );
    m_sliceStencilDoppler->SetInputConnection( m_constantPad->GetOutputPort() );
    m_sliceStencilDoppler->SetBackgroundColor( 1.0, 1.0, 1.0, 0.0 );



    // static slices
    m_staticSlicesEnabled = false;
    m_numberOfStaticSlices = 2;   // default = first and last
    m_staticSlicesProperties = vtkImageProperty::New();
    m_staticSlicesLutIndex = 0;  // default to greyscale
    m_staticSlicesDataNeedUpdate = true;
}

USAcquisitionObject::~USAcquisitionObject()
{
    disconnect(this);

    m_sliceTransform->Delete();
    m_sliceProperties->Delete();
    m_staticSlicesProperties->Delete();

    m_sliceStencil->Delete();
    m_sliceStencilDoppler->Delete();//added
    m_mapToColors->Delete();
    m_mask->Delete();
    m_constantPad->Delete(); // added Mar 2, 2016, Xiao
    ClearStaticSlicesData();

    delete m_videoBuffer;
}

void USAcquisitionObject::ObjectAddedToScene()
{
    SetSliceLutIndex( m_sliceLutIndex );
    SetStaticSlicesLutIndex( m_staticSlicesLutIndex );
    connect( m_mask, SIGNAL(MaskChanged()), this, SLOT(UpdateMask()) );
}

void USAcquisitionObject::SetUsProbe( UsProbeObject * probe )
{
    m_acquisitionType = probe->GetAcquisitionType();
    m_usDepth = probe->GetCurrentCalibrationMatrixName();
    SetCalibrationMatrix( probe->GetCurrentCalibrationMatrix() );
    m_mask->operator =( *(probe->GetMask()) );
}

bool USAcquisitionObject::Setup( View * view )
{
    this->SceneObject::Setup( view );

    if( view->GetType() == THREED_VIEW_TYPE )
    {
        PerViewElements elem;
        elem.imageSlice = vtkImageActor::New();
        elem.imageSlice->SetUserTransform( m_sliceTransform );
        elem.imageSlice->SetVisibility( !this->IsHidden() && this->GetNumberOfSlices()> 0 ? 1 : 0 );
        elem.imageSlice->SetProperty( m_sliceProperties );
        if( m_isMaskOn )
            elem.imageSlice->GetMapper()->SetInputConnection( m_sliceStencil->GetOutputPort() );
        else
            elem.imageSlice->GetMapper()->SetInputConnection( m_mapToColors->GetOutputPort() );


        view->GetRenderer()->AddActor( elem.imageSlice );

        SetupAllStaticSlices( view, elem );

        m_perViews[ view ] = elem;
    }

    return true;
}

bool USAcquisitionObject::Release( View * view )
{
    this->SceneObject::Release(view);

    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        View * view = (*it).first;
        vtkRenderer * ren = view->GetRenderer();
        PerViewElements & perView = (*it).second;
        ren->RemoveActor( perView.imageSlice );
        perView.imageSlice->Delete();
        ReleaseAllStaticSlices( view, perView );
        ++it;
    }

    m_perViews.clear();

    return true;
}

void USAcquisitionObject::Hide()
{
    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        PerViewElements & perView = (*it).second;
        perView.imageSlice->VisibilityOff();
        HideStaticSlices( perView );
        ++it;
    }

    emit Modified();
}

void USAcquisitionObject::Show()
{
    if( this->GetNumberOfSlices() > 0 )
    {
        PerViewContainer::iterator it = m_perViews.begin();
        while( it != m_perViews.end() )
        {
            PerViewElements & perView = (*it).second;
            perView.imageSlice->VisibilityOn();
            if( m_staticSlicesEnabled )
                ShowStaticSlices( perView );
            ++it;
        }
        emit Modified();
    }
}

/*void USAcquisitionObject::SetUseMask( bool use )
{
    if( m_isMaskOn == use )
        return;
    m_isMaskOn = use;

    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        PerViewElements & perView = (*it).second;
        if( m_isMaskOn )
            perView.imageSlice->GetMapper()->SetInputConnection( m_sliceStencil->GetOutputPort() );
        else
            perView.imageSlice->GetMapper()->SetInputConnection( m_mapToColors->GetOutputPort() );

        for( unsigned i = 0; i < perView.staticSlices.size(); ++i )
        {
            vtkImageActor * staticActor = perView.staticSlices[i];
            if( m_isMaskOn )
                staticActor->GetMapper()->SetInputConnection( m_staticSlicesData[i].imageStencil->GetOutputPort() );
            else
                staticActor->GetMapper()->SetInputConnection( m_staticSlicesData[i].mapToColors->GetOutputPort() );
        }

        ++it;
    }

    emit Modified();
}*/

// here are the major changes - Mar, , 2016, by Xiao
void USAcquisitionObject::SetUseMask( bool useMask )
{
    m_isMaskOn = useMask;
    UpdatePipeline();
}


void USAcquisitionObject::SetUseDoppler( bool useDoppler )
{
    m_isDopplerOn = useDoppler;
    UpdatePipeline();
}


void USAcquisitionObject::UpdatePipeline()
{
  PerViewContainer::iterator it = m_perViews.begin();
  while( it != m_perViews.end() )
  {
      PerViewElements & perView = (*it).second;
      if( m_isMaskOn && !m_isDopplerOn)
          perView.imageSlice->GetMapper()->SetInputConnection( m_sliceStencil->GetOutputPort() );
      else if (!m_isMaskOn && !m_isDopplerOn)
          perView.imageSlice->GetMapper()->SetInputConnection( m_mapToColors->GetOutputPort() );
      else if (m_isMaskOn && m_isDopplerOn){


          //m_sliceStencil->SetInputConnection( m_constantPad->GetOutputPort() );
          perView.imageSlice->GetMapper()->SetInputConnection( m_sliceStencilDoppler->GetOutputPort() );
      }
      else // !m_isMaskOn && m_isDopplerOn
          perView.imageSlice->GetMapper()->SetInputConnection( m_constantPad->GetOutputPort());


      for( unsigned i = 0; i < perView.staticSlices.size(); ++i ) // don't touch the static for now
      {
          vtkImageActor * staticActor = perView.staticSlices[i];
          if( m_isMaskOn )
              staticActor->GetMapper()->SetInputConnection( m_staticSlicesData[i].imageStencil->GetOutputPort() );//
          else
              staticActor->GetMapper()->SetInputConnection( m_staticSlicesData[i].mapToColors->GetOutputPort() );
      }

      ++it;
  }

  emit Modified();

}
// here is where things end.

void USAcquisitionObject::HideStaticSlices( PerViewElements & perView )
{
    std::vector<vtkImageActor*>::iterator it = perView.staticSlices.begin();
    while( it != perView.staticSlices.end() )
    {
        (*it)->VisibilityOff();
        ++it;
    }
    emit Modified();
}

void USAcquisitionObject::ShowStaticSlices( PerViewElements & perView )
{
    std::vector<vtkImageActor*>::iterator it = perView.staticSlices.begin();
    while( it != perView.staticSlices.end() )
    {
        (*it)->VisibilityOn();
        ++it;
    }
    emit Modified();
}

#include "hardwaremodule.h"

void USAcquisitionObject::Record()
{
    Q_ASSERT( !m_isRecording );
    m_isRecording = true;

    // Add the frame that was last captured by the system
    HardwareModule * hw = Application::GetHardwareModule();
    if( hw->GetVideoTrackerState() == Ok )
        m_videoBuffer->AddFrame( hw->GetTrackedVideoOutput(), hw->GetTrackedVideoUncalibratedTransform()->GetMatrix() );

    // Start watching the clock for updates
    connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(Updated()));

    // Disable static slices
    this->SetEnableStaticSlices(false);

    emit Modified();
}

void USAcquisitionObject::Updated()
{
    if( m_isRecording )
    {
        HardwareModule * hw = Application::GetHardwareModule();
        if( hw->GetVideoTrackerState() == Ok )
        {
            m_videoBuffer->AddFrame( hw->GetTrackedVideoOutput(), hw->GetTrackedVideoUncalibratedTransform()->GetMatrix() );
            emit Modified();
        }
    }
}

void USAcquisitionObject::UpdateMask()
{
    int currentFrameIndex = m_videoBuffer->GetCurrentFrame();
    if( currentFrameIndex < 0 ) // no frames yet
        return;
    this->SetCurrentFrame( 0 );
    m_imageStencilSource->Update();
    m_mapToColors->Update();
    m_constantPad->Update(); // added Mar 3, 2016, Xiao
    this->SetNumberOfStaticSlices( m_numberOfStaticSlices );
    this->SetCurrentFrame( currentFrameIndex );
}

void USAcquisitionObject::Stop()
{
    if( m_isRecording )
    {
        m_isRecording = false;
        disconnect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(Updated()));
    }
}

void USAcquisitionObject::SetCurrentFrame( int frameIndex )
{
    m_videoBuffer->SetCurrentFrame( frameIndex );
    m_mapToColors->SetInputData( m_videoBuffer->GetCurrentImage() );
    m_constantPad->SetInputData( m_videoBuffer->GetCurrentImage() ); // added Mar , 2016, Xiao
    m_currentImageTransform->SetMatrix( m_videoBuffer->GetCurrentMatrix() );
    m_sliceTransform->Update();
    emit Modified();
}

void USAcquisitionObject::Clear()
{
    m_videoBuffer->Clear();
    emit Modified();
}

QString USAcquisitionObject::GetAcquisitionTypeAsString()
{
    QString ret;
    switch( m_acquisitionType )
    {
    case UsProbeObject::ACQ_B_MODE:
        ret = "B-Mode";
        break;
    case UsProbeObject::ACQ_DOPPLER:
        ret = "Doppler";
        break;
    case UsProbeObject::ACQ_POWER_DOPPLER:
        ret = "Power Doppler";
        break;
    default:
        ret = "Unknown";
        break;
    }
    return ret;
}

QString USAcquisitionObject::GetAcquisitionColor()
{
    QString ret;
    int nbComp = m_videoBuffer->GetFrameNumberOfComponents();
    if( nbComp == 1 )
        ret = ACQ_COLOR_GRAYSCALE;
    else if( nbComp == 3 )
        ret = ACQ_COLOR_RGB;
    else
        ret = "Unknown";
    return ret;
}

void USAcquisitionObject::SetCalibrationMatrix( vtkMatrix4x4 * mat )
{
    vtkMatrix4x4 * matCopy = vtkMatrix4x4::New();
    matCopy->DeepCopy( mat );
    m_calibrationTransform->SetMatrix( matCopy );
    matCopy->Delete();
    emit Modified();
}

vtkTransform * USAcquisitionObject::GetCalibrationTransform()
{
    return m_calibrationTransform;
}

vtkImageData * USAcquisitionObject::GetVideoOutput()
{
    return m_videoBuffer->GetVideoOutput();
}

vtkTransform * USAcquisitionObject::GetTransform()
{
    return m_sliceTransform;
}

void USAcquisitionObject::Export()
{
    QString directoryName(this->GetBaseDirectory());
    QString saveDir = Application::GetInstance().GetExistingDirectory(tr("Export Acquisition"), directoryName);
    if (!saveDir.isEmpty())
    {
        int numberOfFramesExported = 0;
        this->ExportTrackedVideoBuffer( numberOfFramesExported, saveDir );
        this->SetBaseDirectory(directoryName);
    }
}

void USAcquisitionObject::SetupAllStaticSlicesInAllViews()
{
    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        SetupAllStaticSlices( it->first, it->second );
        ++it;
    }
}

void USAcquisitionObject::SetupAllStaticSlices( View * view, PerViewElements & perView )
{
    if( m_staticSlicesDataNeedUpdate )
        ComputeAllStaticSlicesData();

    for( unsigned i = 0; i < m_staticSlicesData.size(); ++i )
    {
        PerStaticSlice & pss = m_staticSlicesData[i];

        vtkImageActor * imageActor = vtkImageActor::New();
        if( m_isMaskOn )
            imageActor->GetMapper()->SetInputConnection( pss.imageStencil->GetOutputPort() );
        else
            imageActor->GetMapper()->SetInputConnection( pss.mapToColors->GetOutputPort() );
        imageActor->SetProperty( m_staticSlicesProperties );
        imageActor->SetUserTransform( pss.transform );
        if( !this->IsHidden() && m_staticSlicesEnabled )
            imageActor->VisibilityOn();
        else
            imageActor->VisibilityOff();
        view->GetRenderer()->AddActor( imageActor );

        perView.staticSlices.push_back( imageActor );
    }
}

void USAcquisitionObject::ReleaseAllStaticSlicesInAllViews()
{
    PerViewContainer::iterator it = m_perViews.begin();
    while( it != m_perViews.end() )
    {
        ReleaseAllStaticSlices( it->first, it->second );
        ++it;
    }
}

void USAcquisitionObject::ReleaseAllStaticSlices( View * view, PerViewElements & perView )
{
    std::vector<vtkImageActor*>::iterator it = perView.staticSlices.begin();
    while( it != perView.staticSlices.end() )
    {
        vtkImageActor * actor = (*it);
        view->GetRenderer()->RemoveActor( actor );
        actor->Delete();
        ++it;
    }
    perView.staticSlices.clear();
}

void USAcquisitionObject::ComputeAllStaticSlicesData()
{
    // clear old data
    ClearStaticSlicesData();

    // compute slices data at regular interval
    int nbSlices = this->GetNumberOfSlices();
    if (nbSlices > 1)
    {
        double interval = double(nbSlices) / m_numberOfStaticSlices;
        for( int i = 0; i < m_numberOfStaticSlices - 1; ++i )
        {
            int index = (int)floor( interval * i );
            ComputeOneStaticSliceData( index );
        }

        // Last slice
        ComputeOneStaticSliceData( nbSlices - 1 );

        SetStaticSlicesLutIndex( this->m_staticSlicesLutIndex );
        m_staticSlicesDataNeedUpdate = false;
    }
}

void USAcquisitionObject::ComputeOneStaticSliceData( int sliceIndex )
{
    PerStaticSlice pss;

    // Get the slice image and matrices
    vtkImageData * slice = m_videoBuffer->GetImage( sliceIndex );
    vtkMatrix4x4 * sliceUncalibratedMatrix = m_videoBuffer->GetMatrix( sliceIndex );

    // Compute the (masked) image
    pss.mapToColors = vtkImageMapToColors::New();
    pss.mapToColors->SetOutputFormatToRGBA();
    pss.mapToColors->SetInputData( slice );


    pss.imageStencil = vtkImageStencil::New();
    pss.imageStencil->SetStencilData( m_imageStencilSource->GetOutput() );
    pss.imageStencil->SetInputConnection( pss.mapToColors->GetOutputPort() );
    pss.imageStencil->SetBackgroundColor( 1.0, 1.0, 1.0, 0.0 );

    // compute the transform of the slice
    vtkTransform * sliceUncalibratedTransform = vtkTransform::New();
    sliceUncalibratedTransform->SetMatrix( sliceUncalibratedMatrix );
    pss.transform = vtkTransform::New();
    pss.transform->Concatenate( this->WorldTransform );
    pss.transform->Concatenate( sliceUncalibratedTransform );
    pss.transform->Concatenate( m_calibrationTransform );
    pss.transform->Update();
    m_staticSlicesData.push_back( pss );

    // cleanup
    sliceUncalibratedTransform->Delete();
}

void USAcquisitionObject::ClearStaticSlicesData()
{
    for( unsigned i = 0; i < m_staticSlicesData.size(); ++i )
    {
        PerStaticSlice & pss = m_staticSlicesData[i];
        pss.mapToColors->Delete();
        pss.imageStencil->Delete();
        pss.transform->Delete();
    }
    m_staticSlicesData.clear();
}


void USAcquisitionObject::Save( int & numberOfFramesSaved )
{
    if (this->ExportTrackedVideoBuffer( numberOfFramesSaved ) )
    {
        QString relPath("./");
        relPath.append(m_baseDirectory.section('/',-1));
        this->SetBaseDirectory(relPath);
    }
}

bool USAcquisitionObject::LoadFramesFromMINCFile( QStringList & allMINCFiles )
{
    OpenFileParams params;
    bool frameOK = true;
    m_itkRGBImages.clear();
    FileReader *fileReader = new FileReader;
    // First check if it is MINC 1 format
    FILE *fp = fopen(allMINCFiles.at(0).toUtf8().data(), "rb");
    if (fp)
    {
        char first4[4];
        size_t count = fread(first4, 4, 1, fp);
        fclose(fp);

        if (count == 1 &&
        first4[0] == 'C' &&
        first4[1] == 'D' &&
        first4[2] == 'F' &&
        first4[3] == '\001')
        {
            QFileInfo fi( allMINCFiles.at(0) );
            QString tmp("Acquired files in directory: ");
            tmp.append( fi.absolutePath() + " are of MINC1 type. You have to convert them to MINC2 format.\n" +
                        "Please convert using command: \nmincconvert -2 <input> <output>\n" +
                        "or using ConvertMINC1toMINC2 plugin");
            QMessageBox::warning( 0, "Error", tmp );
            return false;
        }
    }
    for( int i = 0; i < allMINCFiles.count() && frameOK; ++i )
    {
        QFileInfo fi( allMINCFiles.at(i) );
        if( !(fi.isReadable()) )
        {
            QString message( "No read permission on file: " );
            message.append( allMINCFiles.at(i) );
            QMessageBox::critical( 0, "Error", message, 1, 0 );
            frameOK = false;
            break;
        }
        params.AddInputFile(allMINCFiles.at(i));
        params.defaultParent = this->Parent;
        params.filesParams[0].isVideoFrame = true;
        fileReader->SetParams( &params );
        fileReader->start();
        fileReader->wait();
        QList<SceneObject*> loadedObject;
        fileReader->GetReadObjects( loadedObject );
        if( !loadedObject.isEmpty() )
        {
            ImageObject *img = ImageObject::SafeDownCast( loadedObject.at(0) );
            if( img )
            {
                // create full transform and reset image step and origin in order to avoid
                // double translation and scaling and display slices correctly in double view
                double start[3], step[3];
                img->GetImage()->GetOrigin(start);
                img->GetImage()->GetSpacing(step);
                vtkMatrix4x4 *sliceMatrix = img->GetLocalTransform()->GetMatrix();
                vtkTransform *t = vtkTransform::New();
                t->SetMatrix(img->GetLocalTransform()->GetMatrix());
                t->Translate(start);
                t->Scale(step);
                img->SetLocalTransform(t);
                t->Delete();
                img->GetImage()->SetOrigin(0,0,0);
                img->GetImage()->SetSpacing(1,1,1);

                img->GetImage()->GetOrigin(start);
                img->GetImage()->GetSpacing(step);
                sliceMatrix = img->GetLocalTransform()->GetMatrix();
                m_videoBuffer->AddFrame( img->GetImage(), img->GetLocalTransform()->GetMatrix() );

                if( img->GetItkRGBImage() )
                    m_itkRGBImages.push_back(img->GetItkRGBImage());
                IbisRGBImageType::Pointer itkImage = img->GetItkRGBImage();
                float origin[3];
                for( int i = 0; i < 3; i++ )
                    origin[i] = itkImage->GetOrigin()[i];
                itk::Matrix< double, 3, 3 > dir_cos = itkImage->GetDirection();
                float spacing[3];
                for ( int i = 0; i < 3; i++ )
                {
                    spacing[i] = itkImage->GetSpacing()[i];
                }
            }
            params.filesParams.pop_back();
        }
        else // if first file was not read in, the others won't neither
            frameOK = false;
    }
    const QStringList & warnings = fileReader->GetWarnings();
    if( warnings.size() )
    {
        QString message("");
        for( int i = 0; i < warnings.size(); ++i )
        {
            message += warnings[i];
            message += QString("\n");
        }
        QMessageBox::warning( 0, "Error", message );
    }
    delete fileReader;
    return frameOK;
}

bool USAcquisitionObject::LoadFramesFromMINCFile( Serializer * ser )
{    
    QString baseFileName;
    QString baseDirName;
    QString subDirName;
    if ( ser->FileVersionIsLowerThan( QString::number(5.0) ) )
        baseFileName = this->GetName();
    else
        baseFileName = QString::number(this->GetObjectID());
    baseDirName = this->GetBaseDirectory();
    subDirName = baseDirName + "/" + baseFileName;
    if (!QFile::exists(subDirName))
    {
        QString  accessError = "Directory not found - " + subDirName;
        QMessageBox::warning(0, "Error: ", accessError, 1, 0);
        return false;
    }
    QDir dir(subDirName);
    QStringList allMINCFiles = dir.entryList(QStringList("*.mnc"), QDir::Files, QDir::Name);
    if (allMINCFiles.isEmpty())
    {
        QString  accessError = "No acquisition found in  " + subDirName;
        QMessageBox::warning(0, "Error: ", accessError, 1, 0);
        return false;
    }

    QStringList allMincPaths;
    for( int i = 0; i < allMINCFiles.size(); ++i )
    {
        QString filename(subDirName);
        filename.append("/");
        filename.append(allMINCFiles.at(i));
        allMincPaths.push_back( filename );
    }

    return LoadFramesFromMINCFile( allMincPaths );
}

void USAcquisitionObject::Serialize( Serializer * ser )
{
    SceneObject::Serialize(ser);

    double currentSliceOpacity = 1.0;
    double staticSlicesOpacity = 1.0;
    int currentSlice = 0;
    int acquisitionType = (int)m_acquisitionType;
    if (!ser->IsReader())
    {
        int numberOfFrames = m_videoBuffer->GetNumberOfFrames();
        currentSlice = this->GetCurrentSlice();
        currentSliceOpacity = m_sliceProperties->GetOpacity();
        staticSlicesOpacity = m_staticSlicesProperties->GetOpacity();
        QString relPath("./");
        relPath.append(m_baseDirectory.section('/',-1));
        this->SetBaseDirectory(relPath);
        this->Save( numberOfFrames );
    }

    ::Serialize( ser, "BaseDirectory", m_baseDirectory );
    if (ser->IsReader())
    {   
        if (m_baseDirectory.at(0) == '.')
            m_baseDirectory.replace(0, 1, ser->GetCurrentDirectory() );
        if (!QDir(m_baseDirectory).exists())
        {
            QString  accessError = "Cannot find acquisition directory: " + m_baseDirectory;
            QMessageBox::warning( 0, "Error: ", accessError, 1, 0 );
            return;
        }

        // in old formats, acquisitions were not deletable and were managed by the system. It
        // is no longer the case, so we have to enforce what is read in scene object.
        this->SetObjectManagedBySystem(false);
        this->SetObjectDeletable(true);
        this->SetCanChangeParent(true);
    }
    ::Serialize( ser, "AcquisitionType", acquisitionType );
    ::Serialize( ser, "UsDepth", m_usDepth );
    ::Serialize( ser, "CalibrationMatrix", GetCalibrationTransform()->GetMatrix() );
    ::Serialize( ser, "CurrentSlice", currentSlice );
    ::Serialize( ser, "SliceLutIndex", m_sliceLutIndex );
    ::Serialize( ser, "SliceOpacity", currentSliceOpacity );
    ::Serialize( ser, "StaticSlicesEnabled", m_staticSlicesEnabled );
    ::Serialize( ser, "NumberOfStaticSlices", m_numberOfStaticSlices );
    ::Serialize( ser, "StaticSlicesOpacity", staticSlicesOpacity );
    ::Serialize( ser, "StaticSlicesLutIndex", m_staticSlicesLutIndex );
    ::Serialize( ser, "IsMaskOn", m_isMaskOn );
    ::Serialize( ser, "Mask", m_mask );

    if (ser->IsReader())
    {
        m_acquisitionType = (UsProbeObject::ACQ_TYPE)acquisitionType;
        SetSliceLutIndex( m_sliceLutIndex );
        SetStaticSlicesLutIndex( m_staticSlicesLutIndex );
        m_sliceProperties->SetOpacity( currentSliceOpacity );
        m_staticSlicesProperties->SetOpacity( staticSlicesOpacity );

        if (this->LoadFramesFromMINCFile( ser ))
            SetCurrentFrame( currentSlice );
        this->UpdateMask();
    }
}


void USAcquisitionObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets)
{
    UsAcquisitionSettingsWidget * w = new UsAcquisitionSettingsWidget( parent );
    w->setObjectName( "Properties" );
    w->SetUSAcquisitionObject( this );
    widgets->append( w );
    USMaskSettingsWidget *w1 = new USMaskSettingsWidget( parent );
    w1->setObjectName( "Mask" );
    w1->SetMask( m_mask );
    w1->DisableSetASDefault();
    widgets->append( w1 );
}

// GetItkImage in release 2.3.1 is used only in GPU Volume Reconstruction and is not meant for general application
// It uses calibrated slice matrix
// We should write a special function converting vtk to itk image
bool USAcquisitionObject::GetItkImage(IbisItk3DImageType::Pointer itkOutputImage, int frameNo,
     vtkMatrix4x4 * sliceMatrix)
{
    Q_ASSERT_X(itkOutputImage, "USAcquisitionObject::GetItkImage()", "itkOutputImage must be allocated before this call");
    Q_ASSERT_X(sliceMatrix, "USAcquisitionObject::GetItkImage()", "sliceMatrix must be allocated before this callL");

    int currentFrame = m_videoBuffer->GetCurrentFrame();
    this->SetCurrentFrame(frameNo);
    sliceMatrix->DeepCopy( m_sliceTransform->GetMatrix() );
    this->SetCurrentFrame(currentFrame);
    vtkImageData * initialImage = m_videoBuffer->GetImage( frameNo );

    double org[3], st[3];
    initialImage->GetOrigin(org);
    initialImage->GetSpacing(st);
    int numberOfScalarComponents = initialImage->GetNumberOfScalarComponents();
    vtkImageData *grayImage = initialImage;
    if (numberOfScalarComponents > 1)
    {
        vtkImageLuminance *luminanceFilter = vtkImageLuminance::New();
        luminanceFilter->SetInputData(initialImage);
        luminanceFilter->Update();
        grayImage = luminanceFilter->GetOutput();
    }
    vtkImageData * image;
    vtkImageShiftScale *shifter = vtkImageShiftScale::New();
    if (initialImage->GetScalarType() != VTK_FLOAT)
    {
        shifter->SetOutputScalarType(VTK_FLOAT);
        shifter->SetClampOverflow(1);
        shifter->SetInputData(grayImage);
        shifter->SetShift(0);
        shifter->SetScale(1.0);
        shifter->Update();
        image = shifter->GetOutput();
    }
    else
        image = initialImage;

    image->GetOrigin(org);
    image->GetSpacing(st);
    int * dimensions = initialImage->GetDimensions();
    IbisItk3DImageType::SizeType  size;
    IbisItk3DImageType::IndexType start;
    IbisItk3DImageType::RegionType region;
    const long unsigned int numberOfPixels =  dimensions[0] * dimensions[1] * dimensions[2];
    double imageOrigin[3];
    image->GetOrigin(imageOrigin);
    for (int i = 0; i < 3; i++)
    {
        size[i] = dimensions[i];
    }

    start.Fill(0);
    region.SetIndex( start );
    region.SetSize( size );
    itkOutputImage->SetRegions(region);

    itk::Matrix< double, 3,3 > dirCosine;
    itk::Vector< double, 3 > origin;
    itk::Vector< double, 3 > itkOrigin;
    // set direction cosines
    vtkMatrix4x4 * tmpMat = vtkMatrix4x4::New();
    vtkMatrix4x4::Transpose( sliceMatrix, tmpMat );
    double step[3], mincStartPoint[3], dirCos[3][3];
    for( int i = 0; i < 3; i++ )
    {
        step[i] = vtkMath::Dot( (*tmpMat)[i], (*tmpMat)[i] );
        step[i] = sqrt( step[i] );
        for( int j = 0; j < 3; j++ )
        {
            dirCos[i][j] = (*tmpMat)[i][j] / step[i];
            dirCosine[j][i] = dirCos[i][j];
        }
    }

    double rotation[3][3];
    vtkMath::Transpose3x3( dirCos, rotation );
    vtkMath::LinearSolve3x3( rotation, (*tmpMat)[3], mincStartPoint );

    for( int i = 0; i < 3; i++ )
        origin[i] =  mincStartPoint[i];
    itkOrigin = dirCosine * origin;
    itkOutputImage->SetSpacing(step);
    itkOutputImage->SetOrigin(itkOrigin);
    itkOutputImage->SetDirection(dirCosine);
    itkOutputImage->Allocate();
    float *itkImageBuffer = itkOutputImage->GetBufferPointer();
    memcpy(itkImageBuffer, image->GetScalarPointer(), numberOfPixels*sizeof(float));
    tmpMat->Delete();
    shifter->Delete();
    return true;
}

// GetItkRGBImage is used only to convert captured video frames to itk images that will be then exported using itk image writer
// we export only uncalibrated matrices
void USAcquisitionObject::GetItkRGBImage(IbisRGBImageType::Pointer itkOutputImage, int frameNo )
{
    Q_ASSERT_X(itkOutputImage, "USAcquisitionObject::GetItkImage()", "itkOutputImage must be allocated before this call");

    vtkImageData * image = m_videoBuffer->GetImage( frameNo );
    vtkMatrix4x4 * uncalibratedMatrix = vtkMatrix4x4::New();
    uncalibratedMatrix->DeepCopy( m_videoBuffer->GetMatrix( frameNo ) );

    int numberOfScalarComponents = image->GetNumberOfScalarComponents();

    int * dimensions = image->GetDimensions();
    IbisItk3DImageType::SizeType  size;
    IbisItk3DImageType::IndexType start;
    IbisItk3DImageType::RegionType region;
    const long unsigned int numberOfPixels =  dimensions[0] * dimensions[1] * dimensions[2];
    for (int i = 0; i < 3; i++)
    {
        size[i] = dimensions[i];
    }

    start.Fill(0);
    region.SetIndex( start );
    region.SetSize( size );
    itkOutputImage->SetRegions(region);

    itk::Matrix< double, 3,3 > dirCosine;
    itk::Vector< double, 3 > origin;
    itk::Vector< double, 3 > itkOrigin;
    // set direction cosines
    vtkMatrix4x4 * tmpMat = vtkMatrix4x4::New();
    vtkMatrix4x4::Transpose( uncalibratedMatrix, tmpMat );
    double step[3], mincStartPoint[3], dirCos[3][3];
    for( int i = 0; i < 3; i++ )
    {
        step[i] = vtkMath::Dot( (*tmpMat)[i], (*tmpMat)[i] );
        step[i] = sqrt( step[i] );
        for( int j = 0; j < 3; j++ )
        {
            dirCos[i][j] = (*tmpMat)[i][j] / step[i];
            dirCosine[j][i] = dirCos[i][j];
        }
    }

    double rotation[3][3];
    vtkMath::Transpose3x3( dirCos, rotation );
    vtkMath::LinearSolve3x3( rotation, (*tmpMat)[3], mincStartPoint );

    for( int i = 0; i < 3; i++ )
        origin[i] =  mincStartPoint[i];
    itkOrigin = dirCosine * origin;
    itkOutputImage->SetSpacing(step);
    itkOutputImage->SetOrigin(itkOrigin);
    itkOutputImage->SetDirection(dirCosine);

    itkOutputImage->Allocate();
    RGBPixelType *itkImageBuffer = itkOutputImage->GetBufferPointer();
    memcpy(itkImageBuffer, image->GetScalarPointer(), numberOfPixels*sizeof(RGBPixelType));
    uncalibratedMatrix->Delete();
}

int USAcquisitionObject::GetSliceWidth()
{
    return m_videoBuffer->GetFrameWidth();
}

int USAcquisitionObject::GetSliceHeight()
{
    return m_videoBuffer->GetFrameHeight();
}

int USAcquisitionObject::GetNumberOfSlices()
{
    return m_videoBuffer->GetNumberOfFrames();
}

vtkImageData * USAcquisitionObject::GetMask()
{
    return m_mask->GetMask();
}

#include <itkImageFileWriter.h>

void USAcquisitionObject::ConvertVtkImagesToItkRGBImages()
{
    m_itkRGBImages.clear();
    int numberOfFrames = m_videoBuffer->GetNumberOfFrames();
    for( int i = 0; i < numberOfFrames; i++ )
    {
        IbisRGBImageType::Pointer itkOutputImage = IbisRGBImageType::New();
        this->GetItkRGBImage( itkOutputImage, i );
        m_itkRGBImages.push_back( itkOutputImage );
    }
}

bool USAcquisitionObject::ExportTrackedVideoBuffer( int & numberOfFramesExported, QString dir )
{
    Q_ASSERT( GetManager() );

    // have to provide for vtkImage only
    int numberOfFrames = m_videoBuffer->GetNumberOfFrames();
    // we have to take copy of current settings and change base directory
    QString partFileName;
    QString baseFileName;
    QString baseDirName(dir);
    if (baseDirName.isEmpty())
    {
        baseDirName = this->GetManager()->GetSceneDirectory();
        baseDirName.append('/');
        baseDirName.append(m_baseDirectory.section('/',-1));
    }
    QString subDirName;
    bool dirMade;
    baseFileName = QString::number(this->GetObjectID());
    subDirName = baseDirName + "/" + baseFileName;
    partFileName = subDirName + "/" + baseFileName;
    if (!QFile::exists(baseDirName))
    {
        QDir baseDir;
        dirMade = baseDir.mkpath(baseDirName);
        if (!dirMade)
        {
            QString  accessError = IGNS_MSG_CANT_MAKE_DIR + baseDirName;
            QMessageBox::warning(0, "Error: ", accessError, 1, 0);
            return false;
        }
    }
    if (QFile::exists(subDirName))
    {
        QDir tmp(subDirName);
        QStringList allFiles = tmp.entryList(QStringList("*.*"), QDir::Files, QDir::Name);
        if (!allFiles.isEmpty())
        {
            for (int i = 0; i < allFiles.size(); i++)
                tmp.remove(allFiles[i]);
        }
        if (!tmp.rmdir(subDirName))
        {
            QString  accessError = IGNS_MSG_ACQ_EXIST + subDirName;
            QMessageBox::warning(0, "Error: ", accessError, 1, 0);
            return false;
        }
    }
    QDir subDir;
    dirMade = subDir.mkdir(subDirName);
    if (!dirMade)
    {
        QString  accessError = IGNS_MSG_CANT_MAKE_DIR + subDirName;
        QMessageBox::warning( 0, "Error: ", accessError, 1, 0 );
        return false;
    }
    bool processOK = false;
    QProgressDialog * progress = new QProgressDialog("Exporting frames", "Cancel", 0, numberOfFrames);
    progress->setAttribute(Qt::WA_DeleteOnClose, true);
    if( numberOfFrames > 0)
    {
        int backupCurrentFrame = this->GetCurrentSlice();
        progress = new QProgressDialog("Exporting frames", "Cancel", 0, numberOfFrames);
        progress->show();
        processOK = true;

        numberOfFramesExported = 0;
        int sequenceNumber = 0; // make sure to number output files sequentially
        if( m_itkRGBImages.size() == 0 )
            this->ConvertVtkImagesToItkRGBImages();
        itk::ImageFileWriter< IbisRGBImageType >::Pointer mincWriter = itk::ImageFileWriter<IbisRGBImageType>::New();
        for( int i = 0; i < numberOfFrames && processOK; i++ )
        {
            QString Number( QString::number( ++sequenceNumber ));
            int numLength = Number.length();
            QString numberedFileName(partFileName);
            numberedFileName += '.';
            for(int j = 0; j < 5 - numLength; j++)
            {
                numberedFileName += "0";
            }

            numberedFileName += Number;
            numberedFileName += ".mnc";
            mincWriter->SetFileName(numberedFileName.toUtf8().data());

            mincWriter->SetInput( m_itkRGBImages[i] );

            try
            {
                mincWriter->Update();
            }
            catch(itk::ExceptionObject & exp)
            {
                std::cerr << "Exception caught!" << std::endl;
                std::cerr << exp << std::endl;
                processOK = false;
                break;
            }
            progress->setValue(i);
            qApp->processEvents();
            if ( progress->wasCanceled() )
            {
                QMessageBox::information(0, "Exporting frames", "Process cancelled", 1, 0);
                processOK = false;
            }
            numberOfFramesExported++;
        }
        progress->close();
        return processOK;
    }
    return false;
}

int USAcquisitionObject::GetCurrentSlice()
{
    return m_videoBuffer->GetCurrentFrame();
}

void USAcquisitionObject::SetSliceImageOpacity( double opacity )
{
    m_sliceProperties->SetOpacity( opacity );
    emit Modified();
}

double USAcquisitionObject::GetSliceImageOpacity()
{
    return m_sliceProperties->GetOpacity();
}

void USAcquisitionObject::SetSliceLutIndex( int index )
{
    m_sliceLutIndex = index;
    double range[2] = { 0.0, 255.0 };
    QString slicesLutName = Application::GetLookupTableManager()->GetTemplateLookupTableName( m_sliceLutIndex );
    vtkPiecewiseFunctionLookupTable * lut = vtkPiecewiseFunctionLookupTable::New();
    lut->SetIntensityFactor( 1.0 );
    Application::GetLookupTableManager()->CreateLookupTable( slicesLutName, range, lut );
    m_mapToColors->SetLookupTable( lut );
    lut->Delete();
    emit Modified();
}

void USAcquisitionObject::SetEnableStaticSlices( bool enable )
{
    if( m_staticSlicesEnabled == enable )
        return;

    m_staticSlicesEnabled = enable;
    if( m_staticSlicesEnabled )
    {
        if( !this->IsHidden() )
        {
            PerViewContainer::iterator it = m_perViews.begin();
            while( it != m_perViews.end() )
            {
                PerViewElements & perView = (*it).second;
                ShowStaticSlices( perView );
                ++it;
            }
        }
    }
    else
    {
        PerViewContainer::iterator it = m_perViews.begin();
        while( it != m_perViews.end() )
        {
            PerViewElements & perView = (*it).second;
            HideStaticSlices( perView );
            ++it;
        }
    }
    emit Modified();
}

void USAcquisitionObject::SetNumberOfStaticSlices( int nb )
{
    Q_ASSERT( nb >= 2 );
    m_numberOfStaticSlices = nb;
    ReleaseAllStaticSlicesInAllViews();
    m_staticSlicesDataNeedUpdate = true;
    SetupAllStaticSlicesInAllViews();
    emit Modified();
}

void USAcquisitionObject::SetStaticSlicesOpacity( double opacity )
{
    m_staticSlicesProperties->SetOpacity( opacity );
    emit Modified();
}

double USAcquisitionObject::GetStaticSlicesOpacity()
{
    return m_staticSlicesProperties->GetOpacity();
}

void USAcquisitionObject::SetStaticSlicesLutIndex( int index )
{
    m_staticSlicesLutIndex = index;
    double range[2] = { 0.0, 255.0 };
    QString staticSlicesLutName = Application::GetLookupTableManager()->GetTemplateLookupTableName( m_staticSlicesLutIndex );
    vtkPiecewiseFunctionLookupTable * staticLut = vtkPiecewiseFunctionLookupTable::New();
    staticLut->SetIntensityFactor( 1.0 );
    Application::GetLookupTableManager()->CreateLookupTable( staticSlicesLutName, range, staticLut );
    for( unsigned i = 0; i < m_staticSlicesData.size(); ++i )
    {
        m_staticSlicesData[i].mapToColors->SetLookupTable( staticLut );
    }
    staticLut->Delete();
    emit Modified();
}

