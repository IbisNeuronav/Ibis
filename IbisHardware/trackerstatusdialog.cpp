/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "trackerstatusdialog.h"
#include "tracker.h"
#include "pointerobject.h"
#include "scenemanager.h"
#include "vtkQtMatrixDialog.h"
#include "vtkTrackerTool.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qmessagebox.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qcombobox.h>

ToolUI::ToolUI( QWidget * parent )
    : QWidget( parent)
    , SnapshotMatrixWidget( 0 )
{
    this->ToolLayout = new QHBoxLayout( this );
    
    this->ToolNameLabel = new QLabel( this );
    this->ToolNameLabel->setMinimumSize( QSize( 150, 0 ) );
    this->ToolLayout->addWidget( this->ToolNameLabel );

    this->ToolStateLabel = new QLabel( this );
    this->ToolStateLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    this->ToolStateLabel->setMinimumSize( QSize( 100, 0 ) );
    this->ToolStateLabel->setFrameShape( QLabel::Box );
    this->ToolStateLabel->setFrameShadow( QLabel::Plain );
    this->ToolStateLabel->setAlignment( Qt::AlignCenter  );
    this->ToolStateLabel->setIndent( -1 );
    this->ToolLayout->addWidget( this->ToolStateLabel );

    this->ToolLayout->addSpacing( 15 );

    this->SnapshotButton = new QPushButton(this);
    this->SnapshotButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    this->SnapshotButton->setMinimumSize( 20, 20 );
    this->SnapshotButton->setMaximumSize( 20, 20 );
    this->SnapshotButton->setText("S");
    this->ToolLayout->addWidget( this->SnapshotButton );
    connect( this->SnapshotButton, SIGNAL(clicked()), this, SLOT(SnapshotButtonClicked()) );
    
    this->PreviousState = Undefined;
    this->ToolIndex = -1;
    this->Track = 0;
}

ToolUI::~ToolUI()
{
    if( this->SnapshotMatrixWidget )
    {
        disconnect( this->SnapshotMatrixWidget, SIGNAL(destroyed()), this, SLOT(SnapshotMatrixWidgetClosed()) );
        this->SnapshotMatrixWidget->close();
        delete this->SnapshotMatrixWidget;
    }
    if( this->Track )
    {
        this->Track->UnRegister( 0 );
    }
}
    
void ToolUI::SetTool( int index, Tracker * tracker )
{
    if( this->Track == tracker && index == this->ToolIndex )
    {
        return;
    }
    
    if( this->Track && this->Track != tracker )
    {
        this->Track->UnRegister( 0 );
    }
    
    this->Track = tracker;
    this->ToolIndex = index;
    
    if( this->Track )
    {
        this->Track->Register( 0 );
        if( this->ToolIndex != -1 )
        {
            this->ToolNameLabel->setText( this->Track->GetToolName( this->ToolIndex ) );
        }
    }
    
    this->Update();
}

void ToolUI::Update() 
{
    if( this->Track && this->ToolIndex != -1 )
    {
        int n = this->Track->GetNumberOfTools();
        if (this->ToolIndex >= n)
            return;
        TrackerToolState newState = Track->GetCurrentToolState( ToolIndex );
        if( newState != this->PreviousState )
        {
            switch( newState )
            {
            case Ok:
                this->ToolStateLabel->setText( "OK" );
                this->ToolStateLabel->setStyleSheet("background-color: lightGreen");
                break;
            case Missing:
                this->ToolStateLabel->setText( "Missing" );
                this->ToolStateLabel->setStyleSheet("background-color: red");
                break;
            case OutOfVolume:
                this->ToolStateLabel->setText( "Out of volume" );
                this->ToolStateLabel->setStyleSheet("background-color: yellow");
                break;
            case OutOfView:
                this->ToolStateLabel->setText( "Out of view" );
                this->ToolStateLabel->setStyleSheet("background-color: red");
                break;
            case Undefined:
                this->ToolStateLabel->setText( "Tracker not initialized" );
                this->ToolStateLabel->setStyleSheet("background-color: red");
                break;
            }
            this->PreviousState = newState;
        }
    }
    else
    {
        this->ToolStateLabel->setText( "Missing" );
        this->ToolStateLabel->setStyleSheet("background-color: red");
    }
}

void ToolUI::SnapshotButtonClicked()
{
    if( !this->SnapshotMatrixWidget )
    {
        this->SnapshotMatrixWidget = new vtkQtMatrixDialog( false );
        connect( this->SnapshotMatrixWidget, SIGNAL(destroyed()), this, SLOT(SnapshotMatrixWidgetClosed()) );

    }

    vtkTrackerTool * tool = this->Track->GetTool( this->ToolIndex );
    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    mat->DeepCopy( tool->GetTransform()->GetMatrix() );

    this->SnapshotMatrixWidget->SetMatrix( mat );
    mat->Delete();
    this->SnapshotMatrixWidget->show();
}

void ToolUI::SnapshotMatrixWidgetClosed()
{
    delete this->SnapshotMatrixWidget;
    this->SnapshotMatrixWidget = 0;
}

TrackerStatusDialog::TrackerStatusDialog( QWidget * parent )
    : QWidget( parent )
    , m_tracker( 0 )
{
    setWindowTitle("Tracker status");

	this->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    
    // General layout
    TrackerStatusDialogLayout = new QVBoxLayout( this );
    
    // Tracker update rate
    m_updateRateLabel = new QLabel( this );
    m_updateRateLabel->setText( tr("Tracker Update Rate: ") );
    m_updateRateEdit = new QLineEdit( this );
    m_updateRateEdit->setReadOnly( true );
    QHBoxLayout * layout = new QHBoxLayout();
    layout->addWidget( m_updateRateLabel );
    layout->addWidget( m_updateRateEdit );
    TrackerStatusDialogLayout->addLayout( layout );
    // navigation
    QVBoxLayout * layout1 = new QVBoxLayout();
    m_navigationCheckBox = new QCheckBox(this);
    m_navigationCheckBox->setText(tr("Navigation"));
    m_navigationCheckBox->setChecked(false);
    layout1->addWidget(m_navigationCheckBox);
    m_pointersLabel = new QLabel( this );
    m_pointersLabel->setText(tr("Navigation Pointer:"));
    m_pointersLabel->setMaximumWidth(140);
    m_pointerToolCombo = new QComboBox(this);
    m_pointerToolCombo->setMinimumWidth(200);
    QHBoxLayout * layout2 = new QHBoxLayout();
    layout2->addWidget(m_pointersLabel);
    layout2->addWidget(m_pointerToolCombo);
    layout1->addLayout(layout2);
    TrackerStatusDialogLayout->addLayout( layout1 );
    connect(m_pointerToolCombo, SIGNAL(activated(int)), this, SLOT(PointerToolComboChangedEvent(int)));
    connect(m_navigationCheckBox, SIGNAL(toggled(bool)), this, SLOT(NavigationStateChangedEvent(bool)));

    QFont font;
    font.setBold( true );
    font.setPointSize( 12 );

    m_warningLabel = new QLabel( this );
    m_warningLabel->setFont( font );
    m_warningLabel->setText( tr("Wrong or missing\nreference tool.") );
    QPalette p = m_warningLabel->palette();
    p.setColor( m_warningLabel->foregroundRole(), Qt::red );
    m_warningLabel->setPalette( p );

    m_warningLabel->hide();
    TrackerStatusDialogLayout->addWidget( m_warningLabel );
}


TrackerStatusDialog::~TrackerStatusDialog()
{
    if( m_tracker )
    {
        m_tracker->UnRegister( 0 );
    }
}


void TrackerStatusDialog::SetTracker( Tracker * track )
{
    if( m_tracker == track )
    {
        return;
    }
    
    if( m_tracker )
    {
        m_tracker->disconnect( this );
        m_tracker->UnRegister( 0 );
    }
    
    m_tracker = track;
    
    if( m_tracker )
    {
        m_tracker->Register( 0 );
        
        // create the event callbacks
        connect( m_tracker, SIGNAL( Updated() ), this, SLOT( TrackerUpdateEvent() ) );
        connect( m_tracker, SIGNAL( ToolActivated(int) ), this, SLOT( TrackerToolActivatedEvent() ) );
        connect( m_tracker, SIGNAL( ToolDeactivated(int) ), this, SLOT( TrackerToolActivatedEvent() ) );
        connect( m_tracker, SIGNAL( TrackerInitialized() ), this, SLOT( TrackerToolActivatedEvent() ) );
        connect( m_tracker, SIGNAL( TrackerStopped() ), this, SLOT( TrackerStoppedEvent() ) );
        connect( m_tracker, SIGNAL( ReferenceToolChanged(bool) ), this, SLOT( TrackerToolActivatedEvent() ) );
        connect( m_tracker, SIGNAL( NavigationPointerChangedSignal() ), this, SLOT( TrackerToolActivatedEvent() ) );
        connect( m_tracker, SIGNAL( ToolNameChanged( int, QString )), this, SLOT( TrackerToolActivatedEvent() ) );
        
        // update the dialog
        UpdateToolList();
    }
}


void TrackerStatusDialog::TrackerUpdateEvent()
{
    this->UpdateToolStatus();
}


void TrackerStatusDialog::TrackerToolActivatedEvent()
{
    this->UpdateToolList();
}

void TrackerStatusDialog::TrackerStoppedEvent()
{
    this->ClearAllTools();
    m_warningLabel->setText( "Tracker stopped." );
    m_warningLabel->show();
}

void TrackerStatusDialog::PointerToolComboChangedEvent(int index)
{
    if( m_tracker )
    {
        int navigationPointerIndex = m_tracker->GetNavigationPointerIndex();
        int numberOfTools = m_tracker->GetNumberOfTools();
        if (navigationPointerIndex != index)
        {
            if (index >= 0 && index < numberOfTools &&
                m_tracker->GetToolUse(index) == Pointer && m_tracker->IsToolActive(index))
            {
                m_tracker->SetNavigationPointerIndex(index);
            }
            else
            {
                QMessageBox::warning( 0, "Warning!", "Selected tool may not be used as navigation pointer: " + m_tracker->GetToolName(index), 1, 0 );
                if (navigationPointerIndex >= 0)
                    m_pointerToolCombo->setCurrentIndex(navigationPointerIndex);
                else
                    m_pointerToolCombo->setCurrentIndex(numberOfTools);
            }
        }
    }
}

void TrackerStatusDialog::NavigationStateChangedEvent(bool navigate)
{
    if( m_tracker )
    {
        SceneManager *manager = GetSceneManager();
        if( manager )
            manager->EnablePointerNavigation( navigate );
        else
            m_navigationCheckBox->setChecked(false);
    }
}

void TrackerStatusDialog::UpdateToolList()
{
    if( m_tracker )
    {
        // clear all prev tools
        this->ClearAllTools();
        m_pointerToolCombo->clear();
        
        // Show only the ones that are active
        int numberOfTools = m_tracker->GetNumberOfTools();
        int reference = m_tracker->GetReferenceToolIndex();
        if (!m_tracker->IsInitialized())
        {
            m_warningLabel->setText( "Tracking is off." );
            m_warningLabel->show();
            m_updateRateEdit->hide();
            m_updateRateLabel->hide();
            m_navigationCheckBox->hide();
            m_pointerToolCombo->hide();
            m_pointersLabel->hide();
            return;
        }
        if (reference < 0 || !m_tracker->IsToolActive(reference) || m_tracker->GetToolUse(reference) != Reference)
        {
            m_warningLabel->setText( "Wrong or missing\nreference tool." );
            m_warningLabel->show();
        }
        else
        {
            m_updateRateEdit->show();
            m_updateRateLabel->show();
            m_navigationCheckBox->show();
            m_pointerToolCombo->show();
            m_pointersLabel->show();
            m_warningLabel->hide();
        }
        for( int i = 0; i < numberOfTools; i++ )
        {
            if( m_tracker->IsToolActive( i ) )
            {
                ToolUI * tool = new ToolUI( this );
                tool->setAttribute( Qt::WA_DeleteOnClose, true );
                tool->SetTool( i, m_tracker );
                this->TrackerStatusDialogLayout->addWidget( tool );
                tool->show();
                this->ToolsWidget.push_back( tool );
            }
            m_pointerToolCombo->addItem(m_tracker->GetToolName(i));
        }
        int navigationPointer = m_tracker->GetNavigationPointerIndex();
        m_pointerToolCombo->addItem("None");
        if (navigationPointer > -1)
            m_pointerToolCombo->setCurrentIndex(navigationPointer);
        else
            m_pointerToolCombo->setCurrentIndex(numberOfTools);
        SceneManager *manager = GetSceneManager();
        if (manager && navigationPointer > -1)
        {
            PointerObject *pointer = (PointerObject *)manager->GetObjectByID(m_tracker->GetToolObjectId(navigationPointer));
            if (pointer)
            {
                m_navigationCheckBox->setChecked( manager->GetNavigationState() );
                manager->SetNavigationPointerID( m_tracker->GetToolObjectId(navigationPointer) );
            }
        }
        else
            m_navigationCheckBox->setChecked(false);
    }
}


void TrackerStatusDialog::UpdateToolStatus()
{
    // Update state labels
    std::vector< ToolUI *>::iterator it = this->ToolsWidget.begin();
    for( ; it != this->ToolsWidget.end(); ++it )
    {
        (*it)->Update();
    }
    
    // Update update rate display
    double updateRate = m_tracker->GetUpdateRate();
    m_updateRateEdit->setText( QString::number( updateRate ) + " Hz" );
    // update navigation
    if( m_tracker )
    {
        SceneManager *manager = GetSceneManager();
        if (manager)
        {
            m_navigationCheckBox->setChecked(manager->GetNavigationState());
        }
        else
            m_navigationCheckBox->setChecked(false);
    }
}
   

void TrackerStatusDialog::ClearAllTools()
{
    std::vector< ToolUI *>::iterator it = this->ToolsWidget.begin();
    for( ; it != this->ToolsWidget.end(); ++it )
    {
        (*it)->close();
    }
    this->ToolsWidget.clear();
}

#include "application.h"

SceneManager * TrackerStatusDialog::GetSceneManager()
{
    return Application::GetSceneManager();
}
