/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "quadviewwindow.h"

#include <qvariant.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include "QVTKWidget.h"
#include <QApplication>
#include "vtkqtrenderwindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyle.h"
#include "scenemanager.h"
#include "view.h"
#include "vtkRenderWindow.h"
#include "vtkCornerAnnotation.h"

const QString QuadViewWindow::ViewNames[4] = { "Transverse","ThreeD","Coronal","Sagittal" };

QuadViewWindow::QuadViewWindow( QWidget* parent, Qt::WindowFlags fl ) : QWidget( parent, fl )
{
    setObjectName( "QuadViewWindow" );
    m_detachedWidget = 0;

    m_generalLayout = new QVBoxLayout( this );
    m_generalLayout->setObjectName( "GeneralLayout" );
    m_generalLayout->setContentsMargins( 0, 0, 0, 0 );
    m_generalLayout->setSpacing( 0 );

    // Add a button box at the top of the window. These buttons control view layout and manipulation tools    
    m_buttonBox = new QHBoxLayout();
    m_buttonBox->setContentsMargins( 6, 0, 6, 0 );
    m_buttonBox->setSpacing( 5 );
    m_buttonBox->setObjectName( "ButtonBox" );
    m_generalLayout->addItem( m_buttonBox );
    CreateToolButton( "ZoomInButton", ":/Icons/zoom-in.png", "Zoom In", SLOT(ZoomInButtonClicked()) );
    CreateToolButton( "ZoomOutButton", ":/Icons/zoom-out.png", "Zoom Out", SLOT(ZoomOutButtonClicked()) );
    CreateToolButton( "ZoomFitButton", ":/Icons/zoom-fit.png", "Zoom To Fit Window", SLOT( ResetCameraButtonClicked() ) );
    m_expandViewButton = CreateToolButton( "ExpandViewButton", ":/Icons/expand.png", "Expand Current View", SLOT( ExpandViewButtonClicked() ) );
    CreateToolButton( "ResetPlanesButton", ":/Icons/reset-planes-2.png", "Reset Image Planes to original position", SLOT( ResetPlanesButtonClicked() ) );
    CreateToolButton( "ViewFront", ":/Icons/front.png", "View Front Side", SLOT( ViewFrontButtonClicked() ));
    CreateToolButton( "ViewBack", ":/Icons/back.png", "View Back Side", SLOT( ViewBackButtonClicked() ));
    CreateToolButton( "ViewRight", ":/Icons/right.png", "View Right Side", SLOT( ViewRightButtonClicked() ));
    CreateToolButton( "ViewLeft", ":/Icons/left.png", "View Left Side", SLOT( ViewLeftButtonClicked() ));
    CreateToolButton( "ViewBottom", ":/Icons/bottom.png", "View Bottom", SLOT( ViewBottomButtonClicked() ));
    CreateToolButton( "ViewTop", ":/Icons/top.png", "View Top", SLOT( ViewTopButtonClicked() ));

    // Create a label to display cursor pos.
    m_buttonBox->addSpacing( 30 );
    m_cursorPosLabel = new QLabel( this );
    m_buttonBox->addWidget( m_cursorPosLabel );

    // Spacer at the end of the button box so that the buttons don't scale
    m_buttonBoxSpacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_buttonBox->addItem( m_buttonBoxSpacer );

    m_genericLabel = new QLabel( this );
    QFont font;
    font.setPointSize( 20 );
    m_genericLabel->setFont( font );
    QPalette p = m_genericLabel->palette();
    p.setColor( m_genericLabel->foregroundRole(), Qt::blue );
    m_genericLabel->setPalette( p );
    m_buttonBox->addWidget( m_genericLabel );
    m_genericLabel->hide();


    // Add the 3 splitters that separate the 4 vtk windows
    m_verticalSplitter = new QSplitter( this );
    m_verticalSplitter->setObjectName( "VerticalSplitter" );
    m_verticalSplitter->setOrientation( Qt::Vertical );
    m_generalLayout->addWidget( m_verticalSplitter );

    m_upperHorizontalSplitter = new QSplitter( m_verticalSplitter );
    m_upperHorizontalSplitter->setObjectName( "UpperHorizontalSplitter" );
    m_upperHorizontalSplitter->setOrientation( Qt::Horizontal );
    
    m_lowerHorizontalSplitter = new QSplitter( m_verticalSplitter );
    m_lowerHorizontalSplitter->setObjectName( "LowerHorizontalSplitter" );
    m_lowerHorizontalSplitter->setOrientation( Qt::Horizontal );

    // Create the 4 basic vtk windows
    MakeOneView( 0, "UpperLeftView", m_upperHorizontalSplitter );
    MakeOneView( 1, "UpperRightView", m_upperHorizontalSplitter );
    MakeOneView( 2, "LowerLeftView", m_lowerHorizontalSplitter );
    MakeOneView( 3, "LowerRightView", m_lowerHorizontalSplitter );

    // An empty frame for plugins to add custom widgets at the bottom
    m_bottomWidgetFrame = new QFrame( this );
    m_bottomWidgetLayout = new QVBoxLayout( m_bottomWidgetFrame );
    m_bottomWidgetLayout->setContentsMargins( 0, 0, 0, 0 );
    m_bottomWidgetLayout->setSpacing( 0 );
    m_generalLayout->addWidget( m_bottomWidgetFrame );

    resize( QSize(1000, 800).expandedTo(minimumSizeHint()) );

     m_viewExpanded = false;
     m_currentView = THREED_VIEW_TYPE;
//    this->currentViewExpanded = 0;
}

QAbstractButton * QuadViewWindow::CreateToolButton( QString name, QString iconPath, QString toolTip, const char * callbackSlot )
{
    QPushButton * button = new QPushButton( this );
    button->setObjectName( name );
    button->setIcon( QIcon( iconPath ) );
    button->setIconSize( QSize(30,30) );
    button->setFlat( true );
    button->setToolTip( toolTip );
    button->setFixedSize( 32, 32 );
    m_buttonBox->addWidget( button );
    connect( button, SIGNAL(clicked()), this, callbackSlot );
    return button;
}

void QuadViewWindow::MakeOneView( int index, const char * name, QSplitter * splitter )
{
    m_vtkWindowFrames[index] = new QFrame( splitter );
    m_vtkWindowFrames[index]->setFrameShape( QFrame::NoFrame );

#ifdef USE_QVTKWIDGET_2
    QGLFormat fmt;
    //fmt.setDepth( true );
    //fmt.setDepthBufferSize( 32 );
    fmt.setSampleBuffers( false );
    //fmt.setSamples( 16 );
    m_vtkWindows[index] = new QVTKWidget2( fmt, m_vtkWindowFrames[index] );
#else
    m_vtkWindows[index] = new vtkQtRenderWindow( m_vtkWindowFrames[index] );
#endif
    m_vtkWindows[index]->setObjectName( name );
    m_vtkWindows[index]->installEventFilter( this );
    m_frameLayouts[index] = new QVBoxLayout( m_vtkWindowFrames[index] );
    m_frameLayouts[index]->addWidget( m_vtkWindows[index] );
    m_frameLayouts[index]->setContentsMargins( 1, 1, 1, 1 );
}


QuadViewWindow::~QuadViewWindow()
{
}

#include "QVTKInteractor.h"
void QuadViewWindow::SetSceneManager( SceneManager * man )
{
    if( man )
    {
        View * view = man->GetView( ViewNames[0] );
        if( !view )
        {
            view = man->CreateView( TRANSVERSE_VIEW_TYPE );
            view->SetName( ViewNames[0] );
        }
        view->SetQtRenderWindow( m_vtkWindows[0] );
        view->SetInteractor( m_vtkWindows[0]->GetInteractor() );
        connect( view, SIGNAL( Modified() ), this, SLOT( Win0NeedsRender() ) );
        
        view = man->GetView( ViewNames[1] );
        if( !view )
        {
            view = man->CreateView( THREED_VIEW_TYPE );
            view->SetName( ViewNames[1] );
        }
        view->SetQtRenderWindow( m_vtkWindows[1] );
        view->SetInteractor( m_vtkWindows[1]->GetInteractor() );
        connect( view, SIGNAL( Modified() ), this, SLOT( Win1NeedsRender() ) );
        
        view = man->GetView( ViewNames[2] );
        if( !view )
        {
            view = man->CreateView( CORONAL_VIEW_TYPE );
            view->SetName( ViewNames[2] );
        }
        view->SetQtRenderWindow( m_vtkWindows[2] );
        view->SetInteractor( m_vtkWindows[2]->GetInteractor() );
        connect( view, SIGNAL( Modified() ), this, SLOT( Win2NeedsRender() ) );
        
        view = man->GetView( ViewNames[3] );
        if( !view )
        {
            view = man->CreateView( SAGITTAL_VIEW_TYPE );
            view->SetName( ViewNames[3] );
        }
        view->SetQtRenderWindow( m_vtkWindows[3] );
        view->SetInteractor( m_vtkWindows[3]->GetInteractor() );
        connect( view, SIGNAL( Modified() ), this, SLOT( Win3NeedsRender() ) );
        
        m_sceneManager = man;
        int expandedView = m_sceneManager->GetExpandedView();
        if (expandedView > -1)
        {
            m_sceneManager->SetCurrentView(expandedView);
            ExpandViewButtonClicked();
        }
        else
            SetCurrentView( 1 );

        connect( man, SIGNAL(CursorPositionChanged()), this, SLOT(OnCursorMoved()) );
        OnCursorMoved();
        
        connect( man, SIGNAL(ShowGenericLabel(bool)), this, SLOT(OnShowGenericLabel(bool)) );
        connect( man, SIGNAL(ShowGenericLabelText()), this, SLOT(OnShowGenericLabelText()) );

        connect (m_sceneManager, SIGNAL(ExpandView()), this, SLOT(ExpandViewButtonClicked()));
        m_sceneManager->PreDisplaySetup();
//        this->PlaceCornerText(); temporarily blocked
    }
}

void QuadViewWindow::AddBottomWidget( QWidget * w )
{
    w->setParent( m_bottomWidgetFrame );
    m_bottomWidgetLayout->addWidget( w );
}

void QuadViewWindow::RemoveBottomWidget( QWidget * w )
{
    w->close();
}

#include <QDesktopWidget>

void QuadViewWindow::Detach3DView( QWidget * parent )
{
    Q_ASSERT( !m_detachedWidget );

    m_detachedWidget = new QWidget();
    QVBoxLayout * layout = new QVBoxLayout( m_detachedWidget );
    layout->setMargin( 0 );
    layout->addWidget( m_vtkWindows[1] );

    int nbScreens = QApplication::desktop()->screenCount();
    if( nbScreens > 1 )
    {
        QRect screenres = QApplication::desktop()->screenGeometry( 1 );
        m_detachedWidget->move( QPoint(screenres.x(), screenres.y()) );
        m_detachedWidget->showFullScreen();
    }
    else
        m_detachedWidget->show();
}

void QuadViewWindow::Attach3DView()
{
    m_frameLayouts[1]->addWidget( m_vtkWindows[1] );
    delete m_detachedWidget;
    m_detachedWidget = 0;
}

void QuadViewWindow::Win0NeedsRender() { this->WinNeedsRender( 0 ); }
void QuadViewWindow::Win1NeedsRender() { this->WinNeedsRender( 1 ); }
void QuadViewWindow::Win2NeedsRender() { this->WinNeedsRender( 2 ); }
void QuadViewWindow::Win3NeedsRender() { this->WinNeedsRender( 3 ); }

void QuadViewWindow::WinNeedsRender( int winIndex )
{
    m_vtkWindows[ winIndex ]->update();
}

void QuadViewWindow::ZoomInButtonClicked()
{
    m_sceneManager->ZoomAllCameras( 1.5 );
    this->RenderAll();
}

void QuadViewWindow::ZoomOutButtonClicked()
{
    m_sceneManager->ZoomAllCameras( .75 );
    this->RenderAll();
}

void QuadViewWindow::ResetCameraButtonClicked()
{
    m_sceneManager->ResetAllCameras();
    this->RenderAll();
}

void QuadViewWindow::RenderAll()
{
    for( int i = 0; i < 4; i++ )
    {
        m_vtkWindows[i]->update();
    }
}

void QuadViewWindow::ExpandViewButtonClicked()
{
    if( m_viewExpanded )
    {
        for( int i = 0; i < 4; i++ )
        {
            m_vtkWindowFrames[i]->show();
            m_vtkWindows[i]->update();
        }
        m_viewExpanded = false;
    }
    else
    {
        int currentWindow;
        switch( m_currentView )
        {
        case TRANSVERSE_VIEW_TYPE:
            currentWindow = TRANSVERSE_WIN;
            break;
        case THREED_VIEW_TYPE:
            currentWindow = THREED_WIN;
            break;
        case CORONAL_VIEW_TYPE:
            currentWindow = CORONAL_WIN;
            break;
        case SAGITTAL_VIEW_TYPE:
            currentWindow = SAGITTAL_WIN;
            break;
        }
        for( int i = 0; i < 4; i++ )
        {
            if( i != currentWindow )
            {
                m_vtkWindowFrames[i]->hide();
            }
        }
        m_viewExpanded = true;
    }
}

void QuadViewWindow::ResetPlanesButtonClicked()
{
    if (m_sceneManager)
        m_sceneManager->ResetCursorPosition();
}

void QuadViewWindow::ViewFrontButtonClicked()
{
    m_sceneManager->SetStandardView(SV_FRONT);
}

void QuadViewWindow::ViewBackButtonClicked()
{
    m_sceneManager->SetStandardView(SV_BACK);
}

void QuadViewWindow::ViewRightButtonClicked()
{
    m_sceneManager->SetStandardView(SV_RIGHT);
}

void QuadViewWindow::ViewLeftButtonClicked()
{
    m_sceneManager->SetStandardView(SV_LEFT);
}

void QuadViewWindow::ViewBottomButtonClicked()
{
    m_sceneManager->SetStandardView(SV_BOTTOM);
}

void QuadViewWindow::ViewTopButtonClicked()
{
    m_sceneManager->SetStandardView(SV_TOP);
}

void QuadViewWindow::OnCursorMoved()
{
    double cursorPos[3];
    m_sceneManager->GetCursorPosition( cursorPos );
    QString text = QString( "Cursor: ( %1, %2, %3 )" ).arg( cursorPos[0] ).arg( cursorPos[1] ).arg( cursorPos[2] );
    m_cursorPosLabel->setText( text );
}

void QuadViewWindow::OnShowGenericLabelText( )
{
    QString text = m_sceneManager->GetGenericLabelText();
    m_genericLabel->setText( text );
}

void QuadViewWindow::OnShowGenericLabel( bool show )
{
    if( show )
        m_genericLabel->show();
    else
        m_genericLabel->hide();
}


bool QuadViewWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusIn
        || event->type() == QEvent::MouseButtonPress)
    {
        int which = -1;
        for( int i = 0; i < 4; ++i )
        {
            if( m_vtkWindows[i] == obj )
            {
                which = i;
                break;
            }
        }
        if( which != -1 )
            SetCurrentView( which );
    }
    return false;
}

void QuadViewWindow::SetCurrentView( int index )
{
    switch(index)
    {
    case TRANSVERSE_WIN:
        m_currentView = TRANSVERSE_VIEW_TYPE;
        break;
    case THREED_WIN:
        m_currentView = THREED_VIEW_TYPE;
        break;
    case CORONAL_WIN:
        m_currentView = CORONAL_VIEW_TYPE;
        break;
    case SAGITTAL_WIN:
        m_currentView = SAGITTAL_VIEW_TYPE ;
        break;
    }
    for( int i = 0; i < 4; ++i )
    {
        if( i == index )
        {
            m_vtkWindowFrames[i]->setStyleSheet( "background-color:red" );
        }
        else
        {
            m_vtkWindowFrames[i]->setStyleSheet( "" );
        }
    }
}

void QuadViewWindow::PlaceCornerText()
{
    View * v = m_sceneManager->GetView( CORONAL_VIEW_TYPE );
    vtkRenderer *renderer = v->GetOverlayRenderer();
    vtkCornerAnnotation *ca = vtkCornerAnnotation::New();
    ca->SetText(0, "R");
    ca->SetText(1, "L");
    ca->SetText(2, "R");
    ca->SetText(3, "L");
    ca->SetMaximumFontSize(20);
    renderer->AddViewProp( ca );
    ca->Delete();

    v = m_sceneManager->GetView( TRANSVERSE_VIEW_TYPE );
    renderer = v->GetOverlayRenderer();
    ca = vtkCornerAnnotation::New();
    ca->SetText(0, "R");
    ca->SetText(1, "L");
    ca->SetText(2, "R");
    ca->SetText(3, "L");
    ca->SetMaximumFontSize(20);
    renderer->AddViewProp( ca );
    ca->Delete();

    v = m_sceneManager->GetView( SAGITTAL_VIEW_TYPE );
    renderer = v->GetOverlayRenderer();
    ca = vtkCornerAnnotation::New();
    ca->SetText(0, "A");
    ca->SetText(1, "P");
    ca->SetText(2, "A");
    ca->SetText(3, "P");
    ca->SetMaximumFontSize(20);
    renderer->AddViewProp( ca );
    ca->Delete();

    v = m_sceneManager->GetView(THREED_VIEW_TYPE);
    renderer = v->GetOverlayRenderer();
    ca = vtkCornerAnnotation::New();
    ca->SetText(0, "I");
    ca->SetText(1, "I");
    ca->SetText(2, "S");
    ca->SetText(3, "S");
    ca->SetMaximumFontSize(20);
    renderer->AddViewProp( ca );
    ca->Delete();
}

