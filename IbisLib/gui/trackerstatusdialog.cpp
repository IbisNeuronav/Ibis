#include "trackerstatusdialog.h"
#include "application.h"
#include "pointerobject.h"
#include "scenemanager.h"
#include <vtkQtMatrixDialog.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>

#include <QVariant>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QComboBox>
#include "guiutilities.h"

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

    this->m_toolObjectId = SceneManager::InvalidId;
}

ToolUI::~ToolUI()
{
    if( this->SnapshotMatrixWidget )
    {
        disconnect( this->SnapshotMatrixWidget, SIGNAL(destroyed()), this, SLOT(SnapshotMatrixWidgetClosed()) );
        this->SnapshotMatrixWidget->close();
        delete this->SnapshotMatrixWidget;
    }
}

void ToolUI::SetSceneManager( SceneManager * man, int toolObjectId )
{
    m_manager = man;
    m_toolObjectId = toolObjectId;
    UpdateUI();
}

void ToolUI::UpdateUI()
{
    Q_ASSERT( m_manager && m_toolObjectId != SceneManager::InvalidId );
    TrackedSceneObject * toolObject = TrackedSceneObject::SafeDownCast( m_manager->GetObjectByID( m_toolObjectId ) );
    Q_ASSERT( toolObject );

    this->ToolNameLabel->setText( toolObject->GetName() );

    TrackerToolState newState = toolObject->GetState();
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
	  case HighError:
		    this->ToolStateLabel->setText( "High error" );
		    this->ToolStateLabel->setStyleSheet("background-color: red");
		    break;
	  case Disabled:
		    this->ToolStateLabel->setText( "Disabled" );
		    this->ToolStateLabel->setStyleSheet("background-color: grey");
		    break;
    case Undefined:
        this->ToolStateLabel->setText( "Undefined" );
        this->ToolStateLabel->setStyleSheet("background-color: grey");
        break;
    }
}

void ToolUI::SnapshotButtonClicked()
{
    TrackedSceneObject * toolObject = TrackedSceneObject::SafeDownCast( m_manager->GetObjectByID( m_toolObjectId ) );
    Q_ASSERT( toolObject );

    if( !this->SnapshotMatrixWidget )
    {
        this->SnapshotMatrixWidget = new vtkQtMatrixDialog( true );
        connect( this->SnapshotMatrixWidget, SIGNAL(destroyed()), this, SLOT(SnapshotMatrixWidgetClosed()) );
    }


    vtkMatrix4x4 * mat = vtkMatrix4x4::New();
    mat->DeepCopy( toolObject->GetWorldTransform()->GetMatrix() );
    this->SnapshotMatrixWidget->SetMatrix( mat );
    mat->Delete();
    this->SnapshotMatrixWidget->show();
}

void ToolUI::SnapshotMatrixWidgetClosed()
{
    delete this->SnapshotMatrixWidget;
    this->SnapshotMatrixWidget = 0;
}

TrackerStatusDialog::TrackerStatusDialog( QWidget * parent ) : QWidget( parent )
{
    m_sceneManager = 0;
    setWindowTitle("Tracker status");

	this->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    // General layout
    m_trackerStatusDialogLayout = new QVBoxLayout( this );

    // navigation layout
    QVBoxLayout * layout1 = new QVBoxLayout();

    // navigation checkbox
    m_navigationCheckBox = new QCheckBox(this);
    m_navigationCheckBox->setText(tr("Navigation"));
    m_navigationCheckBox->setChecked(false);
    layout1->addWidget(m_navigationCheckBox);

    // navigation pointer checkbox
    m_pointersLabel = new QLabel( this );
    m_pointersLabel->setText(tr("Navigation Pointer:"));
    m_pointersLabel->setMaximumWidth(140);
    m_pointerToolCombo = new QComboBox(this);
    m_pointerToolCombo->setMinimumWidth(100);
    QHBoxLayout * layout2 = new QHBoxLayout();
    layout2->addWidget(m_pointersLabel);
    layout2->addWidget(m_pointerToolCombo);
    layout1->addLayout(layout2);
    m_trackerStatusDialogLayout->addLayout( layout1 );

    connect(m_pointerToolCombo, SIGNAL(activated(int)), this, SLOT(OnNavigationComboBoxActivated(int)));
    connect(m_navigationCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnNavigationCheckboxToggled(bool)));
}

TrackerStatusDialog::~TrackerStatusDialog()
{
    ClearAllTools();
}

void TrackerStatusDialog::SetSceneManager( SceneManager * man )
{
    m_sceneManager = man;
    connect( m_sceneManager, SIGNAL(ObjectAdded(int)), this, SLOT(UpdateUI()) );
    connect( m_sceneManager, SIGNAL(ObjectRemoved(int)), this, SLOT(UpdateUI()) );
    connect( m_sceneManager, SIGNAL(ObjectNameChanged(QString,QString)), this, SLOT(UpdateUI()) );
    connect( m_sceneManager, SIGNAL(NavigationPointerChanged()), this, SLOT(UpdateUI()) );
    connect( &Application::GetInstance(), SIGNAL(IbisClockTick()), this, SLOT(OnIbisClockTick()) );
    UpdateUI();
}

void TrackerStatusDialog::OnIbisClockTick()
{
    for( int i = 0; i < m_toolsWidget.size(); ++i )
        m_toolsWidget[i]->UpdateUI();
}

void TrackerStatusDialog::UpdateUI()
{
    Q_ASSERT( m_sceneManager );

    // nav checkbox
    m_navigationCheckBox->blockSignals( true );
    m_navigationCheckBox->setChecked( m_sceneManager->GetNavigationState() );
    m_navigationCheckBox->blockSignals( false );

    // nav pointer
    QList<PointerObject*> allPointers;
    m_sceneManager->GetAllPointerObjects( allPointers );
    GuiUtilities::UpdateSceneObjectComboBox( m_pointerToolCombo, allPointers, m_sceneManager->GetNavigationPointerObjectId() );

    // tool list
    this->ClearAllTools();
    QList<TrackedSceneObject*> allTools;
    m_sceneManager->GetAllTrackedObjects( allTools );
    for( int i = 0; i < allTools.size(); i++ )
    {
        ToolUI * tool = new ToolUI( this );
        tool->SetSceneManager( m_sceneManager, allTools[i]->GetObjectID() );
        this->m_trackerStatusDialogLayout->addWidget( tool );
        tool->show();
        this->m_toolsWidget.push_back( tool );
    }
}

void TrackerStatusDialog::OnNavigationComboBoxActivated( int index )
{
    Q_ASSERT( m_sceneManager );
    int newPointerId = GuiUtilities::ObjectIdFromObjectComboBox( m_pointerToolCombo, index );
    m_sceneManager->SetNavigationPointerID( newPointerId );
}

void TrackerStatusDialog::OnNavigationCheckboxToggled( bool navigate )
{
    Q_ASSERT( m_sceneManager );
    m_sceneManager->EnablePointerNavigation( navigate );
}

void TrackerStatusDialog::ClearAllTools()
{
    for( int i = 0; i < m_toolsWidget.size(); ++i )
    {
        delete m_toolsWidget[i];
    }
    this->m_toolsWidget.clear();
}
