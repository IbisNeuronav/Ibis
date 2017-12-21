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
#include <QGridLayout>
#include <QLabel>
#include <QFont>
#include "QVTKWidget.h"
#include <QApplication>
#include <QSettings>
#include "vtkqtrenderwindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyle.h"
#include "scenemanager.h"
#include "view.h"
#include "vtkRenderWindow.h"
#include "vtkCornerAnnotation.h"

ObjectSerializationMacro( QuadViewWindow );

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
    m_toolboxFrame = new QFrame;
    m_generalLayout->addWidget( m_toolboxFrame );

    m_buttonBox = new QHBoxLayout( m_toolboxFrame );
    m_buttonBox->setContentsMargins( 6, 0, 6, 0 );
    m_buttonBox->setSpacing( 5 );
    m_buttonBox->setObjectName( "ButtonBox" );
    CreateToolButton( m_toolboxFrame, "ZoomInButton", ":/Icons/zoom-in.png", "Zoom In", SLOT(ZoomInButtonClicked()) );
    CreateToolButton( m_toolboxFrame, "ZoomOutButton", ":/Icons/zoom-out.png", "Zoom Out", SLOT(ZoomOutButtonClicked()) );
    CreateToolButton( m_toolboxFrame, "ZoomFitButton", ":/Icons/zoom-fit.png", "Zoom To Fit Window", SLOT( ResetCameraButtonClicked() ) );
    m_expandViewButton = CreateToolButton( m_toolboxFrame, "ExpandViewButton", ":/Icons/expand.png", "Expand Current View", SLOT( ExpandViewButtonClicked() ) );
    CreateToolButton( m_toolboxFrame, "ResetPlanesButton", ":/Icons/reset-planes-2.png", "Reset Image Planes to original position", SLOT( ResetPlanesButtonClicked() ) );
    CreateToolButton( m_toolboxFrame, "ViewFront", ":/Icons/front.png", "View Front Side", SLOT( ViewFrontButtonClicked() ));
    CreateToolButton( m_toolboxFrame, "ViewBack", ":/Icons/back.png", "View Back Side", SLOT( ViewBackButtonClicked() ));
    CreateToolButton( m_toolboxFrame, "ViewRight", ":/Icons/right.png", "View Right Side", SLOT( ViewRightButtonClicked() ));
    CreateToolButton( m_toolboxFrame, "ViewLeft", ":/Icons/left.png", "View Left Side", SLOT( ViewLeftButtonClicked() ));
    CreateToolButton( m_toolboxFrame, "ViewBottom", ":/Icons/bottom.png", "View Bottom", SLOT( ViewBottomButtonClicked() ));
    CreateToolButton( m_toolboxFrame, "ViewTop", ":/Icons/top.png", "View Top", SLOT( ViewTopButtonClicked() ));

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

    m_viewWindowsLayout = new QGridLayout( );
    m_viewWindowsLayout->setObjectName( "4Views" );
    m_generalLayout->addLayout( m_viewWindowsLayout );
    m_viewWindowsLayout->setContentsMargins( 1, 1, 1, 1 );
    m_viewWindowsLayout->setSpacing( 1 );

    MakeOneView( 0, "UpperLeftView" );
    MakeOneView( 1, "UpperRightView" );
    MakeOneView( 2, "LowerLeftView" );
    MakeOneView( 3, "LowerRightView" );

    // An empty frame for plugins to add custom widgets at the bottom
    m_bottomWidgetFrame = new QFrame( this );
    m_bottomWidgetLayout = new QVBoxLayout( m_bottomWidgetFrame );
    m_bottomWidgetLayout->setContentsMargins( 0, 0, 0, 0 );
    m_bottomWidgetLayout->setSpacing( 0 );
    m_generalLayout->addWidget( m_bottomWidgetFrame );

    resize( QSize(1000, 800).expandedTo(minimumSizeHint()) );

     m_viewExpanded = false;
     m_currentViewWindow = 1; // upper right, 3D window;
}

QAbstractButton * QuadViewWindow::CreateToolButton( QWidget * parent, QString name, QString iconPath, QString toolTip, const char * callbackSlot )
{
    QPushButton * button = new QPushButton( parent );
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

void QuadViewWindow::MakeOneView( int index, const char * name )
{
    m_vtkWindowFrames[index] = new QFrame( );
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

    int row, col;
    switch( index )
    {
    case 1:
        row = 0;
        col = 1;
        break;
    case 2:
        row = 1;
        col = 0;
        break;
    case 3:
        row = 1;
        col = 1;
        break;
    case 0:
    default:
        row = 0;
        col = 0;
        break;
    }

    m_viewWindowsLayout->addWidget( m_vtkWindowFrames[index], row, col);
}

QuadViewWindow::~QuadViewWindow()
{
}

#include "QVTKInteractor.h"
void QuadViewWindow::SetSceneManager( SceneManager * man )
{
    Q_ASSERT( man );
    View * view;
    view = man->CreateView( TRANSVERSE_VIEW_TYPE, ViewNames[0], TRANSVERSE_VIEW_ID );
    view->SetQtRenderWindow( m_vtkWindows[0] );
    view->SetInteractor( m_vtkWindows[0]->GetInteractor() );
    connect( view, SIGNAL( ViewModified() ), this, SLOT( Win0NeedsRender() ) );
    man->SetMainTransverseViewID( TRANSVERSE_VIEW_ID );

    view = man->CreateView( THREED_VIEW_TYPE, ViewNames[1], THREED_VIEW_ID );
    view->SetQtRenderWindow( m_vtkWindows[1] );
    view->SetInteractor( m_vtkWindows[1]->GetInteractor() );
    connect( view, SIGNAL( ViewModified() ), this, SLOT( Win1NeedsRender() ) );
    man->SetMain3DViewID( THREED_VIEW_ID );

    view = man->CreateView( CORONAL_VIEW_TYPE, ViewNames[2], CORONAL_VIEW_ID );
    view->SetQtRenderWindow( m_vtkWindows[2] );
    view->SetInteractor( m_vtkWindows[2]->GetInteractor() );
    connect( view, SIGNAL( ViewModified() ), this, SLOT( Win2NeedsRender() ) );
    man->SetMainCoronalViewID( CORONAL_VIEW_ID );

    view = man->CreateView( SAGITTAL_VIEW_TYPE, ViewNames[3], SAGITTAL_VIEW_ID );
    view->SetQtRenderWindow( m_vtkWindows[3] );
    view->SetInteractor( m_vtkWindows[3]->GetInteractor() );
    connect( view, SIGNAL( ViewModified() ), this, SLOT( Win3NeedsRender() ) );
    man->SetMainSagittalViewID( SAGITTAL_VIEW_ID );

    m_sceneManager = man;

    connect( man, SIGNAL(CursorPositionChanged()), this, SLOT(OnCursorMoved()) );
    OnCursorMoved();

    connect( man, SIGNAL(ShowGenericLabel(bool)), this, SLOT(OnShowGenericLabel(bool)) );
    connect( man, SIGNAL(ShowGenericLabelText()), this, SLOT(OnShowGenericLabelText()) );

    m_sceneManager->PreDisplaySetup();
//        this->PlaceCornerText(); temporarily blocked
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
        for( int i = 0; i < 4; i++ )
        {
            if( i != m_currentViewWindow )
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
            SetCurrentViewWindow( which );
    }
    return false;
}

void QuadViewWindow::SetExpandedView( bool on )
{
    if( m_viewExpanded != on )
    {
        ExpandViewButtonClicked();
    }
}

void QuadViewWindow::SetCurrentViewWindow(int index )
{
    m_currentViewWindow = index;
    for( int i = 0; i < 4; ++i )
    {
        if( i == m_currentViewWindow )
        {
            m_vtkWindowFrames[i]->setStyleSheet( "background-color:red" );
        }
        else
        {
            m_vtkWindowFrames[i]->setStyleSheet( "" );
        }
    }
}

void QuadViewWindow::SetShowToolbar( bool show )
{
    m_toolboxFrame->setHidden( !show );
}

void QuadViewWindow::Serialize( Serializer * ser )
{
    ser->BeginSection("QuadViewWindow");
    int curWin = m_currentViewWindow;
    bool expView = m_viewExpanded;
    ::Serialize( ser, "CurrentViewWindow", curWin );
    ::Serialize( ser, "ViewExpanded", expView );
    if( ser->IsReader() )
    {
        this->SetCurrentViewWindow( curWin );
        this->SetExpandedView(  expView );
    }
    ser->EndSection();
}

void QuadViewWindow::PlaceCornerText()
{
    View * v = m_sceneManager->GetMainCoronalView( );
    vtkRenderer *renderer = v->GetOverlayRenderer();
    vtkCornerAnnotation *ca = vtkCornerAnnotation::New();
    ca->SetText(0, "R");
    ca->SetText(1, "L");
    ca->SetText(2, "R");
    ca->SetText(3, "L");
    ca->SetMaximumFontSize(20);
    renderer->AddViewProp( ca );
    ca->Delete();

    v = m_sceneManager->GetMainTransverseView( );
    renderer = v->GetOverlayRenderer();
    ca = vtkCornerAnnotation::New();
    ca->SetText(0, "R");
    ca->SetText(1, "L");
    ca->SetText(2, "R");
    ca->SetText(3, "L");
    ca->SetMaximumFontSize(20);
    renderer->AddViewProp( ca );
    ca->Delete();

    v = m_sceneManager->GetMainSagittalView( );
    renderer = v->GetOverlayRenderer();
    ca = vtkCornerAnnotation::New();
    ca->SetText(0, "A");
    ca->SetText(1, "P");
    ca->SetText(2, "A");
    ca->SetText(3, "P");
    ca->SetMaximumFontSize(20);
    renderer->AddViewProp( ca );
    ca->Delete();

    v = m_sceneManager->GetMain3DView( );
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

void QuadViewWindow::LoadSettings( QSettings & s )
{
    int curWin = s.value( "CurrentViewWindow", 1 ).toInt();
    bool expView = s.value( "ExpandedView", false ).toBool();
    this->SetCurrentViewWindow( curWin );
    this->SetExpandedView(  expView );
}

void QuadViewWindow::SaveSettings( QSettings & s )
{
    s.setValue( "CurrentViewWindow", m_currentViewWindow );
    s.setValue( "ExpandedView", m_viewExpanded );
}
