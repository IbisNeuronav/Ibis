#include "singleviewwindow.h"

#include <qvariant.h>
#include <qlayout.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QCloseEvent>
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "scenemanager.h"
#include "view.h"
#include "vtkQtRenderWindow.h"


SingleViewWindow::SingleViewWindow( QWidget* parent, const char* name, Qt::WFlags fl ) : QWidget( parent, name, fl )
{
    this->m_sceneManager = 0;
    
    if ( !name )
    {
        setName( "SingleViewWindow" );
    }

    m_generalLayout = new Q3VBoxLayout( this, 11, 6, "GeneralLayout");

    // Add a button box at the top of the window. These buttons control view layout and manipulation tools    
    m_buttonBox = new Q3HBoxLayout( 0, 0, 6, "ButtonBox" );
    m_generalLayout->addItem( m_buttonBox );
    
    // Reset camera button
    m_resetCamerasButton = new QPushButton( this, "ResetCameraButton" );
    m_resetCamerasButton->setMinimumSize( QSize( 120, 0 ) );
    m_resetCamerasButton->setText( "Reset cameras" );
    m_buttonBox->addWidget( m_resetCamerasButton );
    connect( m_resetCamerasButton, SIGNAL(clicked()), this, SLOT( ResetCameraButtonClicked() ) );
    
    // Spacer at the end of the button box so that the buttons don't scale
    m_buttonBoxSpacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_buttonBox->addItem( m_buttonBoxSpacer );

    // Add the 3 splitters that separate the 4 vtk windows
    m_vtkWindow = new vtkQtRenderWindow( this, "Vtk View" );
    m_generalLayout->addWidget( m_vtkWindow );

    resize( QSize(1000, 800).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // Create interactor for upper right view
    vtkRenderWindowInteractor * interactor = m_vtkWindow->MakeRenderWindowInteractor();
    interactor->SetRenderWindow( m_vtkWindow );
    interactor->Initialize();
}


SingleViewWindow::~SingleViewWindow()
{
    // vtk windows need implicit deleting to decrement reference count.
    m_vtkWindow->Delete();
    if( this->m_sceneManager )
    {
        this->m_sceneManager->UnRegister( 0 );
    }
}


void SingleViewWindow::SetSceneManager( SceneManager * man )
{
    if( man == this->m_sceneManager )
    {
        return;
    }
    
    if( this->m_sceneManager )
    {
        this->m_sceneManager->UnRegister( 0 );
    }
    
    this->m_sceneManager = man;
        
    if( man )
    {
        View * view = man->CreateView( THREED_VIEW_TYPE );
        view->SetInteractor( m_vtkWindow->GetInteractor() );
        connect( view, SIGNAL( Modified() ), this, SLOT( WinNeedsRender() ) );
        
        this->m_sceneManager->Register( 0 );
        
        this->m_sceneManager->PreDisplaySetup();
    }
}


void SingleViewWindow::WinNeedsRender() { m_vtkWindow->Render(); }


void SingleViewWindow::ResetCameraButtonClicked()
{
    this->m_sceneManager->ResetAllCameras();
    m_vtkWindow->Render();
}

void SingleViewWindow::closeEvent(  QCloseEvent * e )
{
    m_sceneManager->ReleaseAllViews();
    QWidget::closeEvent( e );
}