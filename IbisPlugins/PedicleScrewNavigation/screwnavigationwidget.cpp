/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Author: Houssem-Eddine Gueziri
#include "screwnavigationwidget.h"

#include <Windows.h>

#include <cstdio>

#include "screwtablewidget.h"
#include "ui_screwnavigationwidget.h"

#define INSTRUMENT_LENGTH 100
#define SCREW_TRAJECTORY_LENGTH 300

ScrewNavigationWidget::ScrewNavigationWidget( std::vector<Screw *> plannedScrews, QWidget * parent )
    : QWidget( parent ),
      ui( new Ui::ScrewNavigationWidget ),
      m_pluginInterface( nullptr ),
      m_axialScrewSource( nullptr ),
      m_screwDiameter( 5.5 ),
      m_screwLength( 50 ),
      m_screwTipSize( 3 ),
      m_isNavigating( false ),
      m_isAxialViewFlipped( false ),
      m_isSagittalViewFlipped( false )
{
    ui->setupUi( this );

    /* Setup axial view
     * Use 2 renderers:
     *  m_axialRenderer for image reslice (layer 0)
     *  m_axialInstrumentRenderer for instrument, screw trajectory, ruler and planned screws (layer 1)
     */

    // Set interactor for image renderer
    vtkRenderWindowInteractor * axialInteractor          = ui->axialImageWindow->interactor();
    vtkSmartPointer<vtkInteractorStyleImage2> axialStyle = vtkSmartPointer<vtkInteractorStyleImage2>::New();
    axialInteractor->SetInteractorStyle( axialStyle );

    // Create 2 layers
    ui->axialImageWindow->renderWindow()->SetNumberOfLayers( 2 );

    // Add image renderer
    m_axialRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_axialRenderer->SetLayer( 0 );
    ui->axialImageWindow->renderWindow()->AddRenderer( m_axialRenderer );

    // Add instruments renderer
    m_axialInstrumentRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_axialInstrumentRenderer->SetLayer( 1 );
    m_axialInstrumentRenderer->InteractiveOff();
    ui->axialImageWindow->renderWindow()->AddRenderer( m_axialInstrumentRenderer );

    // Assign same camera to image and instruments
    m_axialInstrumentRenderer->SetActiveCamera( m_axialRenderer->GetActiveCamera() );

    // Add image actor that contains resliced image
    m_axialActor = vtkSmartPointer<vtkImageActor>::New();
    m_axialActor->InterpolateOff();
    m_axialActor->VisibilityOff();
    m_axialRenderer->AddViewProp( m_axialActor );

    // Create reslice
    m_axialReslice = vtkSmartPointer<vtkImageResliceToColors>::New();
    m_axialReslice->SetInterpolationModeToLinear();
    m_axialReslice->SetOutputDimensionality( 2 );

    m_axialActor->GetMapper()->SetInputConnection( m_axialReslice->GetOutputPort() );
    m_axialActor->GetProperty()->SetLayerNumber( 0 );

    /* Setup sagittal view
     * Use 2 renderers:
     *  m_sagittalRenderer for image reslice (layer 0)
     *  m_sagittalInstrumentRenderer for instrument, screw trajectory, ruler and planned screws (layer 1)
     */

    // Set interactor for image renderer
    vtkRenderWindowInteractor * sagittalInteractor          = ui->sagittalImageWindow->interactor();
    vtkSmartPointer<vtkInteractorStyleImage2> sagittalStyle = vtkSmartPointer<vtkInteractorStyleImage2>::New();
    sagittalInteractor->SetInteractorStyle( sagittalStyle );

    // Create 2 layers
    ui->sagittalImageWindow->renderWindow()->SetNumberOfLayers( 2 );

    // Add image renderer
    m_sagittalRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_sagittalRenderer->SetLayer( 0 );
    ui->sagittalImageWindow->renderWindow()->AddRenderer( m_sagittalRenderer );

    // Add instruments renderer
    m_sagittalInstrumentRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_sagittalInstrumentRenderer->SetLayer( 1 );
    m_sagittalInstrumentRenderer->InteractiveOff();
    ui->sagittalImageWindow->renderWindow()->AddRenderer( m_sagittalInstrumentRenderer );

    // Assign same camera to image and instruments
    m_sagittalInstrumentRenderer->SetActiveCamera( m_sagittalRenderer->GetActiveCamera() );

    // Add image actor that contains resliced image
    m_sagittalActor = vtkSmartPointer<vtkImageActor>::New();
    m_sagittalActor->InterpolateOff();
    m_sagittalActor->VisibilityOff();
    m_sagittalRenderer->AddViewProp( m_sagittalActor );

    // Create reslice
    m_sagittalReslice = vtkSmartPointer<vtkImageResliceToColors>::New();
    m_sagittalReslice->SetInterpolationModeToLinear();
    m_sagittalReslice->SetOutputDimensionality( 2 );

    m_sagittalActor->GetMapper()->SetInputConnection( m_sagittalReslice->GetOutputPort() );
    m_sagittalActor->GetProperty()->SetLayerNumber( 0 );

    m_screwPlanImageDataId = IbisAPI::InvalidId;

    // Initialize plane position
    for( int i = 0; i < 3; ++i )
    {
        m_currentAxialPosition[i]       = 0;
        m_currentAxialOrientation[i]    = 0;
        m_currentSagittalPosition[i]    = 0;
        m_currentSagittalOrientation[i] = 0;
    }

    m_showScrew   = ui->displayScrewCheckBox->isChecked();
    m_showRuler   = ui->displayRulerCheckBox->isChecked();
    m_rulerLength = ui->rulerSpinBox->value();

    // Load planned screws, if any
    this->SetPlannedScrews( plannedScrews );

    // Update displays
    this->UpdatePointerDirection();
    this->UpdateInstrumentDrawing( m_axialInstrumentRenderer );
    this->UpdateInstrumentDrawing( m_sagittalInstrumentRenderer );
    this->InitializeAnnotationDrawing();
}

ScrewNavigationWidget::~ScrewNavigationWidget()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        if( ibisApi )
        {
            disconnect( ibisApi, SIGNAL( IbisClockTick() ), this, SLOT( OnPointerPositionUpdated() ) );
            disconnect( ibisApi, SIGNAL( NavigationPointerChanged() ), this, SLOT( NavigationPointerChangedSlot() ) );
            disconnect( ibisApi, SIGNAL( ObjectAdded( int ) ), this, SLOT( OnObjectAddedSlot( int ) ) );
            disconnect( ibisApi, SIGNAL( ObjectRemoved( int ) ), this, SLOT( OnObjectRemovedSlot( int ) ) );
        }
    }
    m_axialActor->VisibilityOff();
    m_sagittalActor->VisibilityOff();
    m_axialRenderer->RemoveAllViewProps();
    m_axialInstrumentRenderer->RemoveAllViewProps();
    m_sagittalRenderer->RemoveAllViewProps();
    m_sagittalInstrumentRenderer->RemoveAllViewProps();
    m_pluginInterface = 0;
    delete ui;
}

void ScrewNavigationWidget::SetPluginInterface( PedicleScrewNavigationPluginInterface * inter )
{
    m_pluginInterface = inter;
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();

        this->UpdatePointerDirection();
        connect( ibisApi, SIGNAL( NavigationPointerChanged() ), this, SLOT( NavigationPointerChangedSlot() ) );
        connect( ibisApi, SIGNAL( ObjectAdded( int ) ), this, SLOT( OnObjectAddedSlot( int ) ) );
        connect( ibisApi, SIGNAL( ObjectRemoved( int ) ), this, SLOT( OnObjectRemovedSlot( int ) ) );
        this->InitializeUi();
        this->UpdateInputs();
    }
}

void ScrewNavigationWidget::InitializeUi()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();

        // image combobox setup
        QList<ImageObject *> imageList;
        ibisApi->GetAllImageObjects( imageList );
        ui->masterImageComboBox->clear();
        for( int i = 0; i < imageList.size(); ++i )
        {
            ui->masterImageComboBox->addItem( imageList.at( i )->GetName(),
                                              QVariant( imageList.at( i )->GetObjectID() ) );
        }

        if( ui->masterImageComboBox->count() == 0 )
        {
            ui->masterImageComboBox->addItem( tr( "None" ), QVariant( IbisAPI::InvalidId ) );
        }
    }
    else
    {
        ui->masterImageComboBox->clear();
        ui->masterImageComboBox->addItem( tr( "None" ), QVariant( IbisAPI::InvalidId ) );
    }

    this->UpdateScrewComboBox();
    this->OnScrewSizeComboBoxModified( ui->screwSizeComboBox->currentIndex() );

    connect( ui->masterImageComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( UpdateInputs() ) );
    connect( ui->screwSizeComboBox, SIGNAL( currentIndexChanged( int ) ), this,
             SLOT( OnScrewSizeComboBoxModified( int ) ) );
    connect( ui->screwListWidget, SIGNAL( itemChanged( QListWidgetItem * ) ), this,
             SLOT( OnScrewListItemChanged( QListWidgetItem * ) ) );
}

vtkRenderer * ScrewNavigationWidget::GetAxialRenderer() { return m_axialRenderer; }

vtkRenderer * ScrewNavigationWidget::GetSagittalRenderer() { return m_sagittalRenderer; }

void ScrewNavigationWidget::UpdatePointerDirection()
{
    if( m_pluginInterface && m_isNavigating )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();

        std::vector<double> ptip, pbase;

        // Read config file of the pointer in .ibis/PedicleScrewNavigationData/<ToolName>
        QString foldername( QDir( ibisApi->GetConfigDirectory() ).filePath( "PedicleScrewNavigationData" ) );
        QString filename( QDir( foldername ).filePath( ibisApi->GetNavigationPointerObject()->GetName() ) );
        QFile file( filename );
        if( QFileInfo::exists( filename ) )
        {
            if( file.open( QIODevice::ReadOnly ) )
            {
                bool readingTip  = false;
                bool readingBase = false;
                while( !file.atEnd() )
                {
                    QString line = file.readLine().trimmed();
                    if( line.contains( "TipPoint" ) )
                    {
                        readingTip  = true;
                        readingBase = false;
                    }
                    else if( line.contains( "BasePoint" ) )
                    {
                        readingTip  = false;
                        readingBase = true;
                    }
                    else
                    {
                        if( readingTip )
                        {
                            ptip.push_back( line.toDouble() );
                        }
                        else if( readingBase )
                        {
                            pbase.push_back( line.toDouble() );
                        }
                    }
                }

                // If config file found
                if( ( ptip.size() == 3 ) && ( pbase.size() == 3 ) )
                {
                    for( int i = 0; i < 3; ++i )
                    {
                        m_pointerDirection[i] = ptip[i] - pbase[i];
                    }
                }
                else
                {
                    // If file format doesn't match
                    std::cerr << "Something wrong happend while reading file "
                              << file.errorString().toUtf8().constData() << std::endl;
                    m_pointerDirection[0] = 0.0;
                    m_pointerDirection[1] = 0.0;
                    m_pointerDirection[2] = 1.0;
                }

                file.close();
            }
            else
            {
                // If cannot open config file
                std::cerr << "Cannot open file " << file.errorString().toUtf8().constData() << std::endl;
                m_pointerDirection[0] = 0.0;
                m_pointerDirection[1] = 0.0;
                m_pointerDirection[2] = 1.0;
            }
        }
        else
        {
            // If config file does not exist
            std::cerr << "File " << file.errorString().toUtf8().constData() << " doesn't exist." << std::endl;
            m_pointerDirection[0] = 0.0;
            m_pointerDirection[1] = 0.0;
            m_pointerDirection[2] = 1.0;
        }
    }
    else
    {
        // Default value for pointer direction
        m_pointerDirection[0] = 0.0;
        m_pointerDirection[1] = 0.0;
        m_pointerDirection[2] = 1.0;
    }

    vtkMath::Normalize( m_pointerDirection );
}

void ScrewNavigationWidget::UpdateScrewComboBox()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();

        ui->screwSizeComboBox->clear();
        QList<ScrewTableWidget::ScrewProperties> screwList =
            ScrewTableWidget::GetTable( ibisApi->GetConfigDirectory() );
        for( int i = 0; i < screwList.size(); ++i )
        {
            ui->screwSizeComboBox->addItem(
                Screw::GetName( screwList.at( i ).first, screwList.at( i ).second ).c_str(),
                Screw::GetScrewID( screwList.at( i ).first, screwList.at( i ).second ).c_str() );
        }

        if( ui->screwSizeComboBox->count() == 0 )
        {
            ui->screwSizeComboBox->addItem( Screw::GetName( m_screwLength, m_screwDiameter ).c_str(),
                                            Screw::GetScrewID( m_screwLength, m_screwDiameter ).c_str() );
        }
    }
    else
    {
        ui->screwSizeComboBox->addItem( Screw::GetName( m_screwLength, m_screwDiameter ).c_str(),
                                        Screw::GetScrewID( m_screwLength, m_screwDiameter ).c_str() );
    }
}

void ScrewNavigationWidget::UpdateInputs()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        int imageObjectId = ui->masterImageComboBox->itemData( ui->masterImageComboBox->currentIndex() ).toInt();
        ImageObject * currentObject = ImageObject::SafeDownCast( ibisApi->GetObjectByID( imageObjectId ) );
        if( ( currentObject ) && ( imageObjectId != IbisAPI::InvalidId ) )
        {
            if( ( currentObject->IsA( "ImageObject" ) ) && ( currentObject->GetObjectID() != IbisAPI::InvalidId ) )
            {
                if( !m_isAxialViewFlipped )
                {
                    vtkSmartPointer<vtkImageFlip> flipXFilter = vtkSmartPointer<vtkImageFlip>::New();
                    flipXFilter->SetFilteredAxis( 0 );  // flip x axis
                    flipXFilter->SetInputData( currentObject->GetImage() );
                    flipXFilter->FlipAboutOriginOn();
                    flipXFilter->Update();
                    m_axialReslice->SetInputData( flipXFilter->GetOutput() );
                }
                else
                {
                    m_axialReslice->SetInputData( currentObject->GetImage() );
                }

                m_axialReslice->SetLookupTable( currentObject->GetLut() );
                m_axialReslice->Update();

                if( !m_isSagittalViewFlipped )
                {
                    vtkSmartPointer<vtkImageFlip> flipXFilter = vtkSmartPointer<vtkImageFlip>::New();
                    flipXFilter->SetFilteredAxis( 2 );  // flip x axis
                    flipXFilter->SetInputData( currentObject->GetImage() );
                    flipXFilter->FlipAboutOriginOn();
                    flipXFilter->Update();
                    m_sagittalReslice->SetInputData( flipXFilter->GetOutput() );
                }
                else
                {
                    m_sagittalReslice->SetInputData( currentObject->GetImage() );
                }

                m_sagittalReslice->SetLookupTable( currentObject->GetLut() );
                m_sagittalReslice->Update();
            }
        }
    }
}

void ScrewNavigationWidget::Navigate()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi          = m_pluginInterface->GetIbisAPI();
        PointerObject * navPointer = ibisApi->GetNavigationPointerObject();
        int imageObjectId = ui->masterImageComboBox->itemData( ui->masterImageComboBox->currentIndex() ).toInt();
        ImageObject * currentObject = ImageObject::SafeDownCast( ibisApi->GetObjectByID( imageObjectId ) );

        if( navPointer && currentObject )
        {
            if( ( currentObject->IsA( "ImageObject" ) ) && ( currentObject->GetObjectID() != IbisAPI::InvalidId ) )
            {
                m_isNavigating = true;

                this->UpdatePointerDirection();

                this->SetDefaultView( m_axialRenderer );
                this->SetDefaultView( m_sagittalRenderer );
                this->SetDefaultView( m_axialInstrumentRenderer );
                this->SetDefaultView( m_sagittalInstrumentRenderer );

                connect( ibisApi, SIGNAL( IbisClockTick() ), this, SLOT( OnPointerPositionUpdated() ) );
                m_axialActor->VisibilityOn();
                m_sagittalActor->VisibilityOn();
            }
        }
    }
}

void ScrewNavigationWidget::StopNavigation()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        if( ibisApi )
        {
            m_isNavigating = false;
            disconnect( ibisApi, SIGNAL( IbisClockTick() ), this, SLOT( OnPointerPositionUpdated() ) );
        }
    }
}

void ScrewNavigationWidget::GetPlannedScrews( std::vector<Screw *> & screws )
{
    screws.clear();
    for( int i = 0; i < m_PlannedScrewList.size(); ++i )
    {
        Screw * sc = new Screw( m_PlannedScrewList[i] );
        screws.push_back( sc );
    }
}

void ScrewNavigationWidget::SetPlannedScrews( std::vector<Screw *> screws )
{
    m_PlannedScrewList.clear();
    for( int i = 0; i < screws.size(); ++i )
    {
        m_PlannedScrewList.push_back( screws[i] );

        QString screwName( screws[i]->GetName().c_str() );
        QListWidgetItem * item = new QListWidgetItem( screwName );
        item->setFlags( item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable );
        item->setCheckState( Qt::Checked );
        ui->screwListWidget->addItem( item );

        if( m_pluginInterface )
        {
            m_axialInstrumentRenderer->AddViewProp( screws[i]->GetAxialActor() );
            m_sagittalInstrumentRenderer->AddViewProp( screws[i]->GetSagittalActor() );
        }
    }
}

void ScrewNavigationWidget::OnPointerPositionUpdated()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi          = m_pluginInterface->GetIbisAPI();
        PointerObject * navPointer = ibisApi->GetNavigationPointerObject();
        int imageObjectId = ui->masterImageComboBox->itemData( ui->masterImageComboBox->currentIndex() ).toInt();
        ImageObject * currentObject = ImageObject::SafeDownCast( ibisApi->GetObjectByID( imageObjectId ) );
        if( navPointer && currentObject )
        {
            if( ( currentObject->IsA( "ImageObject" ) ) && ( currentObject->GetObjectID() != IbisAPI::InvalidId ) )
            {
                vtkTransform * pointerTransform = navPointer->GetWorldTransform();

                // update axial scene
                double axPos[3];
                double axOrientation[3];

                this->GetAxialPositionAndOrientation( currentObject, pointerTransform, axPos, axOrientation );
                this->MoveAxialPlane( m_axialReslice, axPos, axOrientation );

                // update sagittal scene
                double sagPos[3];
                double sagOrientation[3];

                this->GetSagittalPositionAndOrientation( currentObject, pointerTransform, sagPos, sagOrientation );
                this->MoveSagittalPlane( sagPos, sagOrientation );

                if( m_PlannedScrewList.size() > 0 )
                {
                    this->UpdatePlannedScrews();
                }
            }
        }
    }
}

void ScrewNavigationWidget::GetAxialPositionAndOrientation( ImageObject * currentObject,
                                                            vtkTransform * pointerTransform, double ( &pos )[3],
                                                            double ( &orientation )[3] )
{
    double center[3];
    vtkTransform * inverseImageTransform = vtkTransform::New();
    inverseImageTransform->SetMatrix( currentObject->GetWorldTransform()->GetMatrix() );
    inverseImageTransform->Inverse();
    double * tip  = pointerTransform->GetPosition();
    double * ttip = inverseImageTransform->TransformPoint( tip );
    center[0]     = ttip[0];
    center[1]     = ttip[1];
    center[2]     = ttip[2];

    double pointerTip[4] = { m_pointerDirection[0], m_pointerDirection[1], m_pointerDirection[2], 1.0 };
    double worldPointerTip[4];
    pointerTransform->MultiplyPoint( pointerTip, worldPointerTip );
    double imagePointerTip[4];
    inverseImageTransform->MultiplyPoint( worldPointerTip, imagePointerTip );

    double pointerBase[4] = { 0.0, 0.0, 0.0, 1.0 };
    double worldPointerBase[4];
    pointerTransform->MultiplyPoint( pointerBase, worldPointerBase );
    double imagePointerBase[4];
    inverseImageTransform->MultiplyPoint( worldPointerBase, imagePointerBase );

    double vectorOrientation[3] = { imagePointerTip[0] - imagePointerBase[0], imagePointerTip[1] - imagePointerBase[1],
                                    imagePointerTip[2] - imagePointerBase[2] };

    vtkMath::Normalize( vectorOrientation );
    vectorOrientation[0] = std::acos( vectorOrientation[0] ) * 180.0 / vtkMath::Pi();
    vectorOrientation[1] = std::acos( vectorOrientation[1] ) * 180.0 / vtkMath::Pi();
    vectorOrientation[2] = std::acos( vectorOrientation[2] ) * 180.0 / vtkMath::Pi();

    for( int i = 0; i < 3; ++i )
    {
        pos[i]         = center[i];
        orientation[i] = vectorOrientation[i];
    }
}

void ScrewNavigationWidget::GetSagittalPositionAndOrientation( ImageObject * currentObject,
                                                               vtkTransform * pointerTransform, double ( &pos )[3],
                                                               double ( &orientation )[3] )
{
    double center[3];
    vtkTransform * inverseImageTransform = vtkTransform::New();
    inverseImageTransform->SetMatrix( currentObject->GetWorldTransform()->GetMatrix() );
    inverseImageTransform->Inverse();
    double * tip  = pointerTransform->GetPosition();
    double * ttip = inverseImageTransform->TransformPoint( tip );
    center[0]     = -ttip[2];  // center[0] = -ttip[2];
    center[1]     = ttip[1];
    center[2]     = ttip[0];

    double pointerTip[4] = { m_pointerDirection[0], m_pointerDirection[1], m_pointerDirection[2], 1.0 };
    double worldPointerTip[4];
    pointerTransform->MultiplyPoint( pointerTip, worldPointerTip );
    double imagePointerTip[4];
    inverseImageTransform->MultiplyPoint( worldPointerTip, imagePointerTip );

    double pointerBase[4] = { 0.0, 0.0, 0.0, 1.0 };
    double worldPointerBase[4];
    pointerTransform->MultiplyPoint( pointerBase, worldPointerBase );
    double imagePointerBase[4];
    inverseImageTransform->MultiplyPoint( worldPointerBase, imagePointerBase );

    double vectorOrientation[3] = { imagePointerTip[0] - imagePointerBase[0], imagePointerTip[1] - imagePointerBase[1],
                                    imagePointerTip[2] - imagePointerBase[2] };

    vtkMath::Normalize( vectorOrientation );
    vectorOrientation[0] = std::acos( vectorOrientation[0] ) * 180.0 / vtkMath::Pi();
    vectorOrientation[1] = std::acos( vectorOrientation[1] ) * 180.0 / vtkMath::Pi();
    vectorOrientation[2] = std::acos( vectorOrientation[2] ) * 180.0 / vtkMath::Pi();

    for( int i = 0; i < 3; ++i )
    {
        pos[i]         = center[i];
        orientation[i] = vectorOrientation[i] + 90;
    }
}

void ScrewNavigationWidget::MoveAxialPlane( vtkSmartPointer<vtkImageResliceToColors> reslice, double pos[3],
                                            double orientation[3] )
{
    vtkSmartPointer<vtkTransform> currentTransform = vtkTransform::New();
    currentTransform->Identity();

    if( m_isAxialViewFlipped )
    {
        currentTransform->Translate( pos );
        currentTransform->RotateX( orientation[2] - 90 );
        currentTransform->RotateZ( 90 - orientation[0] );
    }
    else
    {
        currentTransform->Translate( -pos[0], pos[1], pos[2] );
        currentTransform->RotateX( orientation[2] - 90 );
        currentTransform->RotateZ( -90 + orientation[0] );
    }

    // Set the slice orientation
    vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
    resliceAxes->DeepCopy( currentTransform->GetMatrix() );
    // Set the point through which to slice
    reslice->SetResliceAxes( resliceAxes );

    ui->axialImageWindow->renderWindow()->Render();
}

void ScrewNavigationWidget::MoveSagittalPlane( double pos[3], double orientation[3] )
{
    vtkSmartPointer<vtkTransform> currentTransform = vtkTransform::New();
    currentTransform->Identity();
    currentTransform->RotateY( 90 );  // set sagittal orientation

    if( m_isSagittalViewFlipped )
    {
        currentTransform->Translate( pos[0], pos[1], pos[2] );
        currentTransform->RotateX( orientation[0] );
        currentTransform->RotateZ( -orientation[2] );
    }
    else
    {
        currentTransform->Translate( -pos[0], pos[1], pos[2] );
        currentTransform->RotateX( orientation[0] );
        currentTransform->RotateZ( orientation[2] );
    }

    // Set the slice orientation
    vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
    resliceAxes->DeepCopy( currentTransform->GetMatrix() );
    // Set the point through which to slice
    m_sagittalReslice->SetResliceAxes( resliceAxes );

    ui->sagittalImageWindow->renderWindow()->Render();
}

void ScrewNavigationWidget::UpdatePlannedScrews()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi          = m_pluginInterface->GetIbisAPI();
        PointerObject * navPointer = ibisApi->GetNavigationPointerObject();
        int imageObjectId = ui->masterImageComboBox->itemData( ui->masterImageComboBox->currentIndex() ).toInt();
        ImageObject * currentObject = ImageObject::SafeDownCast( ibisApi->GetObjectByID( imageObjectId ) );
        if( !navPointer || !currentObject ) return;
        vtkTransform * pointerTransform = vtkTransform::New();
        pointerTransform->SetMatrix( navPointer->GetWorldTransform()->GetMatrix() );

        vtkTransform * inverseImageTransform = vtkTransform::New();
        inverseImageTransform->SetMatrix( currentObject->GetWorldTransform()->GetMatrix() );
        inverseImageTransform->Inverse();

        for( int i = 0; i < m_PlannedScrewList.size(); ++i )
        {
            double screwDiameter = m_PlannedScrewList[i]->GetScrewDiameter();
            if( ui->screwListWidget->item( i )->checkState() == Qt::Checked )
            {
                double pointerPos[3], pointerOrientation[3];
                m_PlannedScrewList[i]->GetPointerPosition( pointerPos );
                m_PlannedScrewList[i]->GetPointerOrientation( pointerOrientation );

                if( m_PlannedScrewList[i]->IsCoordinateWorldTransform() )
                {
                    inverseImageTransform->TransformPoint( pointerPos, pointerPos );
                    inverseImageTransform->TransformPoint( pointerOrientation, pointerOrientation );
                }

                vtkMath::Normalize( pointerOrientation );
                pointerOrientation[0] = std::acos( pointerOrientation[0] ) * 180.0 / vtkMath::Pi();
                pointerOrientation[1] = std::acos( pointerOrientation[1] ) * 180.0 / vtkMath::Pi();
                pointerOrientation[2] = std::acos( pointerOrientation[2] ) * 180.0 / vtkMath::Pi();

                this->GetAxialPositionAndOrientation( currentObject, pointerTransform, m_currentAxialPosition,
                                                      m_currentAxialOrientation );

                double diffOrientation[3], diffPos[3];
                vtkMath::Subtract( m_currentAxialOrientation, pointerOrientation, diffOrientation );  // TODO: remove
                vtkMath::Subtract( m_currentAxialPosition, pointerPos, diffPos );

                double delta = std::abs( diffPos[2] );

                if( ( delta <= screwDiameter ) )
                {
                    vtkSmartPointer<vtkTransform> axialTransform = vtkSmartPointer<vtkTransform>::New();
                    axialTransform->PostMultiply();
                    double screwrot = 90 - pointerOrientation[0];
                    double planerot = -90 + m_currentAxialOrientation[0];
                    if( !m_isAxialViewFlipped )
                    {
                        screwrot   = -screwrot;
                        diffPos[0] = -diffPos[0];
                        planerot   = -planerot;
                    }

                    axialTransform->RotateZ( screwrot );
                    axialTransform->Translate( -diffPos[0], -diffPos[1], 0.0 );
                    axialTransform->RotateZ( planerot );
                    axialTransform->Update();
                    m_PlannedScrewList[i]->GetAxialActor()->SetUserTransform( axialTransform );
                    if( ui->displayPlanningCheckBox->isChecked() )
                        m_PlannedScrewList[i]->GetAxialActor()->VisibilityOn();
                }
                else
                {
                    m_PlannedScrewList[i]->GetAxialActor()->VisibilityOff();
                }

                // move sagittal planning trajectory
                double sagPointerPos[3]         = { -pointerPos[2], pointerPos[1], pointerPos[0] };
                double sagPointerOrientation[3] = { pointerOrientation[0] + 90, pointerOrientation[1] + 90,
                                                    pointerOrientation[2] + 90 };

                this->GetSagittalPositionAndOrientation( currentObject, pointerTransform, m_currentSagittalPosition,
                                                         m_currentSagittalOrientation );

                vtkMath::Subtract( m_currentSagittalOrientation, sagPointerOrientation,
                                   diffOrientation );  // TODO: remove
                vtkMath::Subtract( m_currentSagittalPosition, sagPointerPos, diffPos );

                delta = std::abs( diffPos[2] );

                if( ( delta <= screwDiameter ) )
                {
                    vtkSmartPointer<vtkTransform> sagittalTransform = vtkSmartPointer<vtkTransform>::New();
                    sagittalTransform->PostMultiply();
                    double screwrot = sagPointerOrientation[2];
                    double planerot = -m_currentSagittalOrientation[2];
                    if( m_isSagittalViewFlipped )
                    {
                        screwrot   = -screwrot;
                        diffPos[0] = -diffPos[0];
                        planerot   = -planerot;
                    }
                    sagittalTransform->RotateZ( screwrot );
                    sagittalTransform->Translate( diffPos[0], diffPos[1], 0.0 );
                    sagittalTransform->RotateZ( planerot );

                    sagittalTransform->Update();
                    m_PlannedScrewList[i]->GetSagittalActor()->SetUserTransform( sagittalTransform );
                    if( ui->displayPlanningCheckBox->isChecked() )
                        m_PlannedScrewList[i]->GetSagittalActor()->VisibilityOn();
                }
                else
                {
                    m_PlannedScrewList[i]->GetSagittalActor()->VisibilityOff();
                }
            }
        }
    }
}

void ScrewNavigationWidget::AddPlannedScrew( Screw * screw )
{
    double position[3], orientation[3];
    double screwLength, screwDiameter, screwTipSize;
    bool useWorld;

    screw->GetPointerPosition( position );
    screw->GetPointerOrientation( orientation );
    screwLength   = screw->GetScrewLength();
    screwDiameter = screw->GetScrewDiameter();
    screwTipSize  = screw->GetScrewTipSize();
    useWorld      = screw->IsCoordinateWorldTransform();

    this->AddPlannedScrew( position, orientation, screwLength, screwDiameter, screwTipSize, useWorld );
}

void ScrewNavigationWidget::AddPlannedScrew( double position[3], double orientation[3], double screwLength,
                                             double screwDiameter, double screwTipSize, bool useWorld )
{
    // create a polydata 3D screw
    // the screw consists of a cylinder representing the body + a cone representing the tip
    vtkSmartPointer<vtkPolyData> trajectory             = vtkPolyData::New();
    vtkSmartPointer<vtkCylinderSource> trajectorySource = vtkSmartPointer<vtkCylinderSource>::New();
    trajectorySource->SetRadius( 0.5 );
    double trajectoryLength = SCREW_TRAJECTORY_LENGTH;
    trajectorySource->SetHeight( trajectoryLength );
    trajectorySource->SetCenter( 0, trajectoryLength / 2.0, 0 );
    trajectorySource->SetResolution( 30 );
    trajectorySource->SetCapping( true );

    vtkSmartPointer<vtkPolyData> cylinder             = vtkPolyData::New();
    vtkSmartPointer<vtkCylinderSource> cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
    cylinderSource->SetRadius( screwDiameter / 2.0 );
    cylinderSource->SetHeight( screwLength - screwTipSize );
    cylinderSource->SetCenter( 0, -( screwLength - screwTipSize ) / 2.0, 0 );
    cylinderSource->SetResolution( 30 );
    cylinderSource->SetCapping( true );

    {
        double normalizedY[3];
        double normalizedZ[3];
        // The Z axis is an arbitrary vector cross X
        double arbitrary[3] = { 10, -10, 5.0 };
        vtkMath::Cross( orientation, arbitrary, normalizedZ );
        vtkMath::Normalize( normalizedZ );

        // The Y axis is Z cross X
        vtkMath::Cross( normalizedZ, orientation, normalizedY );
        vtkSmartPointer<vtkMatrix4x4> cylinderDirCosMatrix = vtkSmartPointer<vtkMatrix4x4>::New();

        // Create the direction cosine matrix
        cylinderDirCosMatrix->Identity();
        for( unsigned int i = 0; i < 3; i++ )
        {
            cylinderDirCosMatrix->SetElement( i, 0, -orientation[i] );
            cylinderDirCosMatrix->SetElement( i, 1, normalizedY[i] );
            cylinderDirCosMatrix->SetElement( i, 2, normalizedZ[i] );
        }

        // Apply the transforms
        vtkSmartPointer<vtkTransform> cylinderTransform = vtkSmartPointer<vtkTransform>::New();
        cylinderTransform->Concatenate( cylinderDirCosMatrix );  // apply direction cosines
        cylinderTransform->RotateZ( -90.0 );                     // align cylinder to x axis

        // Transform the polydata
        vtkSmartPointer<vtkTransformPolyDataFilter> cylinderTransformPolyData =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        cylinderTransformPolyData->SetTransform( cylinderTransform );
        cylinderTransformPolyData->SetInputConnection( cylinderSource->GetOutputPort() );
        cylinderTransformPolyData->Update();

        cylinder->ShallowCopy( cylinderTransformPolyData->GetOutput() );

        cylinderTransformPolyData->SetInputConnection( trajectorySource->GetOutputPort() );
        cylinderTransformPolyData->Update();

        trajectory->ShallowCopy( cylinderTransformPolyData->GetOutput() );
    }

    vtkSmartPointer<vtkPolyData> cone         = vtkPolyData::New();
    vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
    coneSource->SetRadius( screwDiameter / 2.0 );
    coneSource->SetHeight( screwTipSize );
    coneSource->SetDirection( orientation );
    double yoffset       = ( screwLength - screwTipSize ) + screwTipSize / 2.0;
    double coneOffset[3] = { yoffset * orientation[0], yoffset * orientation[1], yoffset * orientation[2] };
    coneSource->SetCenter( coneOffset );
    coneSource->SetCapping( true );

    coneSource->SetResolution( 30 );
    coneSource->Update();

    cone->ShallowCopy( coneSource->GetOutput() );

    vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
    appendFilter->AddInputData( cylinder );
    appendFilter->AddInputData( cone );
    appendFilter->AddInputData( trajectory );

    vtkSmartPointer<vtkCleanPolyData> cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
    cleanFilter->SetInputConnection( appendFilter->GetOutputPort() );

    vtkSmartPointer<vtkTransform> screwTransform = vtkTransform::New();
    // screwTransform->Translate(api->GetNavigationPointerObject()->GetTipPosition());
    screwTransform->Translate( position );

    vtkSmartPointer<vtkTransformPolyDataFilter> screwTransformPolyData =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    screwTransformPolyData->SetTransform( screwTransform );
    screwTransformPolyData->SetInputConnection( cleanFilter->GetOutputPort() );
    screwTransformPolyData->Update();

    double color[3];
    QString screwNameType;
    if( useWorld )
    {
        // set color to red for (navigated) world screws
        color[0]      = 1.0;
        color[1]      = 0.0;
        color[2]      = 0.0;
        screwNameType = " (W)";
    }
    else
    {
        // set color to yellow for (imported) local screws
        color[0]      = 1.0;
        color[1]      = 1.0;
        color[2]      = 0.0;
        screwNameType = " (L)";
    }

    QString screwName = tr( "Screw " ) + QString::number( ui->screwListWidget->count() + 1 ) + tr( ": " ) +
                        tr( Screw::GetName( screwLength, screwDiameter ).c_str() ) + screwNameType;

    if( m_pluginInterface )
    {
        IbisAPI * api          = m_pluginInterface->GetIbisAPI();
        PolyDataObject * pdObj = PolyDataObject::New();
        pdObj->SetName( screwName );
        pdObj->SetPolyData( screwTransformPolyData->GetOutput() );
        pdObj->SetColor( color );
        int imageObjectId = ui->masterImageComboBox->itemData( ui->masterImageComboBox->currentIndex() ).toInt();
        ImageObject * currentObject = ImageObject::SafeDownCast( api->GetObjectByID( imageObjectId ) );
        SceneObject * parent        = nullptr;
        if( ( currentObject ) && ( !useWorld ) )
        {
            parent = currentObject->GetParent();
        }
        api->AddObject( pdObj, parent );
    }

    // Create a polydata representing screw cross-section
    Screw * sp = new Screw();
    sp->SetPointerPosition( position );
    sp->SetPointerOrientation( orientation );
    sp->SetUseWorldTransformCoordinate( useWorld );

    sp->SetScrewProperties( screwLength, screwDiameter, screwTipSize );

    vtkSmartPointer<vtkPolyData> screwPolyData = vtkSmartPointer<vtkPolyData>::New();
    Screw::GetScrewPolyData( screwLength, screwDiameter, screwTipSize, screwPolyData );

    vtkSmartPointer<vtkPolyDataMapper> sagittalPlannedScrewMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    sagittalPlannedScrewMapper->SetInputData( screwPolyData );

    vtkSmartPointer<vtkActor> sagittalPlannedScrewActor = vtkSmartPointer<vtkActor>::New();
    sagittalPlannedScrewActor->SetMapper( sagittalPlannedScrewMapper );
    sagittalPlannedScrewActor->GetProperty()->SetLineWidth( 3 );
    sagittalPlannedScrewActor->GetProperty()->SetColor( color );
    sagittalPlannedScrewActor->GetProperty()->SetOpacity( 0.6 );
    sagittalPlannedScrewActor->GetProperty()->SetLineStipplePattern( 0xf0f0 );
    sagittalPlannedScrewActor->GetProperty()->SetLineStippleRepeatFactor( 1 );
    sagittalPlannedScrewActor->GetProperty()->SetPointSize( 1 );
    sagittalPlannedScrewActor->VisibilityOn();

    m_sagittalInstrumentRenderer->AddViewProp( sagittalPlannedScrewActor );
    sp->SetSagittalActor( sagittalPlannedScrewActor );

    vtkSmartPointer<vtkPolyDataMapper> axialPlannedScrewMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    axialPlannedScrewMapper->SetInputData( screwPolyData );

    vtkSmartPointer<vtkActor> axialPlannedScrewActor = vtkSmartPointer<vtkActor>::New();
    axialPlannedScrewActor->SetMapper( axialPlannedScrewMapper );
    axialPlannedScrewActor->GetProperty()->SetLineWidth( 3 );
    axialPlannedScrewActor->GetProperty()->SetColor( color );
    axialPlannedScrewActor->GetProperty()->SetOpacity( 0.6 );
    axialPlannedScrewActor->GetProperty()->SetLineStipplePattern( 0xf0f0 );
    axialPlannedScrewActor->GetProperty()->SetLineStippleRepeatFactor( 1 );
    axialPlannedScrewActor->GetProperty()->SetPointSize( 1 );
    axialPlannedScrewActor->VisibilityOn();

    m_axialInstrumentRenderer->AddViewProp( axialPlannedScrewActor );
    sp->SetAxialActor( axialPlannedScrewActor );

    m_PlannedScrewList.push_back( sp );

    // add screw to ui
    QListWidgetItem * item = new QListWidgetItem( screwName );
    item->setFlags( item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable );
    item->setCheckState( Qt::Checked );
    ui->screwListWidget->addItem( item );

    ui->displayPlanningCheckBox->setCheckState( Qt::Checked );
}

bool ScrewNavigationWidget::GetPointerDirection( double ( &direction )[3] )
{
    if( m_pluginInterface )
    {
        IbisAPI * api        = m_pluginInterface->GetIbisAPI();
        double pointerTip[4] = { m_pointerDirection[0], m_pointerDirection[1], m_pointerDirection[2], 1.0 };
        double worldPointerTip[4];
        api->GetNavigationPointerObject()->GetWorldTransform()->MultiplyPoint( pointerTip, worldPointerTip );
        double pointerBase[4] = { 0.0, 0.0, 0.0, 1.0 };
        double worldPointerBase[4];
        api->GetNavigationPointerObject()->GetWorldTransform()->MultiplyPoint( pointerBase, worldPointerBase );
        direction[0] = worldPointerTip[0] - worldPointerBase[0];
        direction[1] = worldPointerTip[1] - worldPointerBase[1];
        direction[2] = worldPointerTip[2] - worldPointerBase[2];
        vtkMath::Normalize( direction );

        // QString message = tr("m_pointerDirection : [") + QString::number(m_pointerDirection[0]) +
        //    tr(", ") + QString::number(m_pointerDirection[1]) +
        //    tr(", ") + QString::number(m_pointerDirection[2]) + tr("]\n");

        // message += tr("worldPointerTip : [") + QString::number(worldPointerTip[0]) +
        //    tr(", ") + QString::number(worldPointerTip[1]) +
        //    tr(", ") + QString::number(worldPointerTip[2]) + tr("]\n");

        // message += tr("worldPointerBase : [") + QString::number(worldPointerBase[0]) +
        //    tr(", ") + QString::number(worldPointerBase[1]) +
        //    tr(", ") + QString::number(worldPointerBase[2]) + tr("]\n");

        // m_pluginInterface->GetIbisAPI()->Warning(tr("Pointer orientation"), message);

        return true;
    }

    return false;
}

void ScrewNavigationWidget::SetDefaultView( vtkSmartPointer<vtkRenderer> renderer )
{
    vtkCamera * cam = renderer->GetActiveCamera();
    renderer->ResetCamera();
    cam->ParallelProjectionOn();
    cam->SetParallelScale( m_screwLength * 2.0 );
    double * prevPos   = cam->GetPosition();
    double * prevFocal = cam->GetFocalPoint();
    cam->SetPosition( 0.0, -m_screwLength * 0.5, prevPos[2] );
    cam->SetFocalPoint( 0.0, -m_screwLength * 0.5, prevFocal[2] );
}

void ScrewNavigationWidget::UpdateInstrumentDrawing( vtkSmartPointer<vtkRenderer> renderer )
{
    renderer->RemoveAllViewProps();

    // Create instrument line
    vtkSmartPointer<vtkLineSource> instrumentSource = vtkSmartPointer<vtkLineSource>::New();
    instrumentSource->SetPoint1( 0.0, 0.0, 0.0 );
    instrumentSource->SetPoint2( 0.0, INSTRUMENT_LENGTH, 0.0 );

    vtkSmartPointer<vtkPolyDataMapper> instrumentLineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    instrumentLineMapper->SetInputConnection( instrumentSource->GetOutputPort( 0 ) );

    vtkSmartPointer<vtkActor> instrumentActor = vtkSmartPointer<vtkActor>::New();
    instrumentActor->SetMapper( instrumentLineMapper );
    instrumentActor->GetProperty()->SetLineWidth( 3.0 );
    instrumentActor->GetProperty()->SetColor( 1, 0, 0 );
    instrumentActor->VisibilityOn();
    renderer->AddViewProp( instrumentActor );

    // Create screw
    vtkSmartPointer<vtkActor> screwActor = vtkSmartPointer<vtkActor>::New();
    screwActor->GetProperty()->SetLineWidth( 3.0 );
    screwActor->GetProperty()->SetColor( 1, 0, 0 );
    screwActor->SetVisibility( m_showScrew );

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    Screw::GetScrewPolyData( m_screwLength, m_screwDiameter, m_screwTipSize, polyData );

    vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    lineMapper->SetInputData( polyData );
    screwActor->SetMapper( lineMapper );

    renderer->AddViewProp( screwActor );

    // Create ruler
    vtkSmartPointer<vtkActor> rulerActor = vtkSmartPointer<vtkActor>::New();
    rulerActor->GetProperty()->SetLineWidth( 0.5 );
    rulerActor->GetProperty()->SetColor( 1, 1, 0 );
    rulerActor->GetProperty()->SetOpacity( 0.5 );
    rulerActor->SetVisibility( m_showRuler );

    renderer->AddViewProp( rulerActor );

    this->UpdateRulerDrawing( rulerActor );

    for( int i = 0; i < m_PlannedScrewList.size(); ++i )
    {
        if( m_pluginInterface )
        {
            m_axialInstrumentRenderer->AddViewProp( m_PlannedScrewList[i]->GetAxialActor() );
            m_sagittalInstrumentRenderer->AddViewProp( m_PlannedScrewList[i]->GetSagittalActor() );
        }
    }
}

void ScrewNavigationWidget::InitializeAnnotationDrawing()
{
    // Axial view
    ui->axialLeftLabel->setText( "L" );
    ui->axialLeftLabel->move( 10, ui->axialImageWindow->height() / 2.0 );
    ui->axialLeftLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );

    ui->axialRightLabel->setText( "R" );
    ui->axialRightLabel->move( ui->axialImageWindow->width() - 10 - ui->axialRightLabel->width(),
                               ui->axialImageWindow->height() / 2.0 );
    ui->axialRightLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );

    ui->axialTopLabel->setText( "P" );
    ui->axialTopLabel->move( ui->axialImageWindow->width() / 2.0 - ui->axialTopLabel->width() - 10, 10 );
    ui->axialTopLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );

    ui->axialBottomLabel->setText( "A" );
    ui->axialBottomLabel->move( ui->axialImageWindow->width() / 2.0 - ui->axialTopLabel->width() - 10,
                                ui->axialImageWindow->height() - 10 );
    ui->axialBottomLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );

    // Sagittal view
    ui->sagittalLeftLabel->setText( "S" );
    ui->sagittalLeftLabel->move( 10, ui->sagittalImageWindow->height() / 2.0 );
    ui->sagittalLeftLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );

    ui->sagittalRightLabel->setText( "I" );
    ui->sagittalRightLabel->move( ui->sagittalImageWindow->width() - 10 - ui->sagittalRightLabel->width(),
                                  ui->sagittalImageWindow->height() / 2.0 );
    ui->sagittalRightLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );

    ui->sagittalTopLabel->setText( "P" );
    ui->sagittalTopLabel->move( ui->sagittalImageWindow->width() / 2.0 - ui->sagittalTopLabel->width() - 10, 10 );
    ui->sagittalTopLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );

    ui->sagittalBottomLabel->setText( "A" );
    ui->sagittalBottomLabel->move( ui->sagittalImageWindow->width() / 2.0 - ui->sagittalTopLabel->width() - 10,
                                   ui->sagittalImageWindow->height() - 10 );
    ui->sagittalBottomLabel->setStyleSheet( "QLabel { background-color: black; color : white; }" );
}

void ScrewNavigationWidget::UpdateRulerDrawing( vtkSmartPointer<vtkActor> rulerActor )
{
    vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();

    // Create ruler main line
    vtkSmartPointer<vtkLineSource> mainLineSource = vtkSmartPointer<vtkLineSource>::New();
    mainLineSource->SetPoint1( 0.0, 0.0, 0.0 );
    mainLineSource->SetPoint2( 0.0, -m_rulerLength, 0.0 );
    mainLineSource->Update();

    appendFilter->AddInputData( mainLineSource->GetOutput() );

    // Create ticks
    for( int i = 0; i <= m_rulerLength; ++i )
    {
        double lineSize = 0.5;
        if( ( i % 5 ) == 0 ) lineSize += 0.5;
        if( ( i % 10 ) == 0 ) lineSize += 0.5;
        vtkSmartPointer<vtkLineSource> tickSource = vtkSmartPointer<vtkLineSource>::New();
        tickSource->SetPoint1( -lineSize, -i, 0.0 );
        tickSource->SetPoint2( lineSize, -i, 0.0 );
        tickSource->Update();
        appendFilter->AddInputData( tickSource->GetOutput() );
    }

    vtkSmartPointer<vtkCleanPolyData> cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
    cleanFilter->SetInputConnection( appendFilter->GetOutputPort() );
    cleanFilter->Update();

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();

    polyData->SetPoints( cleanFilter->GetOutput()->GetPoints() );
    polyData->SetLines( cleanFilter->GetOutput()->GetLines() );

    vtkSmartPointer<vtkPolyDataMapper> rulerMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    rulerMapper->SetInputData( polyData );
    rulerActor->SetMapper( rulerMapper );
}

void ScrewNavigationWidget::OnScrewSizeComboBoxModified( int index )
{
    QString length, diameter;
    QVariant value   = ui->screwSizeComboBox->itemData( index );
    QStringList list = value.toString().split( "x" );
    if( list.size() == 2 )
    {
        length          = list.at( 0 );
        diameter        = list.at( 1 );
        m_screwLength   = length.toDouble();
        m_screwDiameter = diameter.toDouble();
    }
    else
    {
        // ComboBox item is None
        m_screwLength   = 50.0;
        m_screwDiameter = 5.5;
    }

    this->UpdateInstrumentDrawing( m_axialInstrumentRenderer );
    this->UpdateInstrumentDrawing( m_sagittalInstrumentRenderer );
}

void ScrewNavigationWidget::on_rulerSpinBox_valueChanged( int value )
{
    m_rulerLength = value;
    this->UpdateInstrumentDrawing( m_axialInstrumentRenderer );
    this->UpdateInstrumentDrawing( m_sagittalInstrumentRenderer );
}

void ScrewNavigationWidget::OnScrewListItemChanged( QListWidgetItem * item )
{
    if( item->checkState() == Qt::Checked )
    {
        m_PlannedScrewList[ui->screwListWidget->row( item )]->GetAxialActor()->VisibilityOn();
        m_PlannedScrewList[ui->screwListWidget->row( item )]->GetSagittalActor()->VisibilityOn();
    }
    else
    {
        m_PlannedScrewList[ui->screwListWidget->row( item )]->GetAxialActor()->VisibilityOff();
        m_PlannedScrewList[ui->screwListWidget->row( item )]->GetSagittalActor()->VisibilityOff();
    }
}

void ScrewNavigationWidget::OnObjectAddedSlot( int imageObjectId )
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        if( ibisApi )
        {
            SceneObject * sceneObject = ibisApi->GetObjectByID( imageObjectId );
            if( sceneObject->IsA( "ImageObject" ) )
            {
                if( ui->masterImageComboBox->count() == 0 )
                {
                    ui->masterImageComboBox->addItem( sceneObject->GetName(), QVariant( imageObjectId ) );
                }
                else if( ui->masterImageComboBox->count() == 1 )
                {
                    int currentItemId =
                        ui->masterImageComboBox->itemData( ui->masterImageComboBox->currentIndex() ).toInt();
                    if( currentItemId == IbisAPI::InvalidId )
                    {
                        ui->masterImageComboBox->clear();
                    }
                    ui->masterImageComboBox->addItem( sceneObject->GetName(), QVariant( imageObjectId ) );
                }
                else
                {
                    ui->masterImageComboBox->addItem( sceneObject->GetName(), QVariant( imageObjectId ) );
                }
            }
        }
    }
}

void ScrewNavigationWidget::OnObjectRemovedSlot( int imageObjectId )
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        if( ibisApi )
        {
            QList<int> itemsToBeRemoved;
            for( int i = 0; i < ui->masterImageComboBox->count(); ++i )
            {
                int currentItemId = ui->masterImageComboBox->itemData( i ).toInt();
                if( currentItemId == imageObjectId )
                {
                    itemsToBeRemoved.append( i );
                }
            }
            for( int i = 0; i < itemsToBeRemoved.size(); ++i )
            {
                ui->masterImageComboBox->removeItem( itemsToBeRemoved.at( i ) );
            }
            if( ui->masterImageComboBox->count() == 0 )
                ui->masterImageComboBox->addItem( tr( "None" ), QVariant( IbisAPI::InvalidId ) );
        }
    }
}

void ScrewNavigationWidget::NavigationPointerChangedSlot() { this->UpdatePointerDirection(); }

void ScrewNavigationWidget::on_resetDefaultViewButton_clicked()
{
    this->SetDefaultView( m_axialRenderer );
    this->SetDefaultView( m_sagittalRenderer );
    this->SetDefaultView( m_axialInstrumentRenderer );
    this->SetDefaultView( m_sagittalInstrumentRenderer );
}

void ScrewNavigationWidget::on_displayScrewCheckBox_toggled( bool checked )
{
    m_showScrew = checked;
    this->UpdateInstrumentDrawing( m_axialInstrumentRenderer );
    this->UpdateInstrumentDrawing( m_sagittalInstrumentRenderer );
}

void ScrewNavigationWidget::on_displayRulerCheckBox_toggled( bool checked )
{
    ui->rulerSpinBox->setEnabled( checked );
    m_showRuler = checked;
    this->UpdateInstrumentDrawing( m_axialInstrumentRenderer );
    this->UpdateInstrumentDrawing( m_sagittalInstrumentRenderer );
}

void ScrewNavigationWidget::on_displayPlanningCheckBox_toggled( bool checked )
{
    ui->screwListWidget->setEnabled( checked );
    for( int i = 0; i < m_PlannedScrewList.size(); ++i )
    {
        bool activate = false;
        if( ui->screwListWidget->item( i )->checkState() == Qt::Checked ) activate = true;
        m_PlannedScrewList[i]->GetAxialActor()->SetVisibility( (int)( checked & activate ) );
        m_PlannedScrewList[i]->GetSagittalActor()->SetVisibility( (int)( checked & activate ) );
    }
}

void ScrewNavigationWidget::on_openScrewTableButton_clicked()
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisAPI                   = m_pluginInterface->GetIbisAPI();
        ScrewTableWidget * screwTableWidget = new ScrewTableWidget( this, ibisAPI->GetConfigDirectory() );
        screwTableWidget->setWindowTitle( "Screw Table" );
        screwTableWidget->setAttribute( Qt::WA_DeleteOnClose );
        ibisAPI->ShowFloatingDock( screwTableWidget );
        screwTableWidget->activateWindow();
        screwTableWidget->raise();
        connect( screwTableWidget, SIGNAL( WidgetAboutToClose() ), this, SLOT( UpdateScrewComboBox() ) );
    }
}

void ScrewNavigationWidget::on_saveScrewPositionButton_clicked()
{
    double pointerOrientation[3] = { 0.0, 0.0, 0.0 };
    if( !this->GetPointerDirection( pointerOrientation ) )
    {
        m_pluginInterface->GetIbisAPI()->Warning(
            tr( "Pointer orientation" ),
            tr( "Warning! pointer orientation set to default, which may not match tool orientation." ) );
        pointerOrientation[1] = -1;
    }

    // QString message = tr("[") + QString::number(pointerOrientation[0]) + tr(", ") +
    // QString::number(pointerOrientation[1]) + tr(", ") +
    //    QString::number(pointerOrientation[2]) + tr("]");
    // m_pluginInterface->GetIbisAPI()->Warning(tr("Pointer orientation"), message);
    double pointerPosition[3] = { 0.0, 0.0, 0.0 };
    if( m_pluginInterface )
    {
        IbisAPI * api      = m_pluginInterface->GetIbisAPI();
        double * pos       = api->GetNavigationPointerObject()->GetTipPosition();
        pointerPosition[0] = pos[0];
        pointerPosition[1] = pos[1];
        pointerPosition[2] = pos[2];
    }

    this->AddPlannedScrew( pointerPosition, pointerOrientation, m_screwLength, m_screwDiameter, m_screwTipSize, true );
    ui->exportPlanButton->setEnabled( true );
}

void ScrewNavigationWidget::on_flipAxialViewCheckBox_toggled( bool checked )
{
    m_isAxialViewFlipped = checked;
    if( checked )
    {
        ui->axialLeftLabel->setText( "R" );
        ui->axialRightLabel->setText( "L" );
    }
    else
    {
        ui->axialLeftLabel->setText( "L" );
        ui->axialRightLabel->setText( "R" );
    }
    this->UpdateInputs();
}

void ScrewNavigationWidget::on_flipSagittalViewCheckBox_toggled( bool checked )
{
    m_isSagittalViewFlipped = checked;
    if( checked )
    {
        ui->sagittalLeftLabel->setText( "I" );
        ui->sagittalRightLabel->setText( "S" );
    }
    else
    {
        ui->sagittalLeftLabel->setText( "S" );
        ui->sagittalRightLabel->setText( "I" );
    }
    this->UpdateInputs();
}

void ScrewNavigationWidget::on_importPlanButton_clicked()
{
    QString dir = "";
    if( m_pluginInterface )
    {
        IbisAPI * api = m_pluginInterface->GetIbisAPI();
        dir           = api->GetWorkingDirectory();
    }

    QString filename =
        QFileDialog::getOpenFileName( this->parentWidget(), tr( "Import screw plan" ), dir, tr( "Screw Files (*.xml)" ),
                                      nullptr, QFileDialog::DontUseNativeDialog );
    if( !filename.isEmpty() )
    {
        // Make sure the file exists
        QFileInfo info( filename );
        if( !info.exists() || !info.isReadable() ) return;

        std::vector<Screw *> screwList;

        // Read config file
        SerializerReader reader;
        reader.SetFilename( filename.toUtf8().data() );
        reader.Start();
        ::Serialize( &reader, "PlannedScrews", screwList );
        reader.Finish();

        for( int i = 0; i < screwList.size(); ++i )
        {
            this->AddPlannedScrew( screwList[i] );
        }

        if( screwList.size() > 0 ) ui->exportPlanButton->setEnabled( true );
    }
}

void ScrewNavigationWidget::on_exportPlanButton_clicked()
{
    QString dir = "";
    if( m_pluginInterface )
    {
        IbisAPI * api = m_pluginInterface->GetIbisAPI();
        dir           = api->GetWorkingDirectory();
    }

    QString filename = QFileDialog::getSaveFileName( this, tr( "Export screw plan" ), dir, tr( "*.xml" ), nullptr,
                                                     QFileDialog::DontUseNativeDialog );
    if( !filename.isEmpty() )
    {
        if( !filename.toLower().endsWith( ".xml" ) ) filename += ".xml";

        SerializerWriter writer0;
        writer0.SetFilename( filename.toUtf8().data() );
        writer0.Start();
        ::Serialize( &writer0, "PlannedScrews", m_PlannedScrewList );
        writer0.EndSection();
        writer0.Finish();
    }
}

void ScrewNavigationWidget::on_closeButton_clicked()
{
    if( m_pluginInterface )
    {
        emit CloseNavigationWidget();
    }
}
