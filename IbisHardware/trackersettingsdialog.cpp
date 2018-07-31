/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "trackersettingsdialog.h"
#include <QVariant>
#include <QInputDialog>
#include <QFileDialog>
#include "application.h"
#include "tracker.h"

TrackerSettingsDialog::TrackerSettingsDialog(QWidget * parent, const char * name )
    : QWidget(parent)
{
    setupUi(this);
    m_tracker = 0;
}


TrackerSettingsDialog::~TrackerSettingsDialog()
{
}

void TrackerSettingsDialog::SetTracker( Tracker * tracker )
{   
    Q_ASSERT( m_tracker == 0 );
    Q_ASSERT( tracker );

    m_tracker = tracker;
    connect( m_tracker, SIGNAL( ToolListChanged() ), this, SLOT( UpdateToolList()) );
    this->UpdateToolList();
    this->UpdateGlobalSettings();
}

void TrackerSettingsDialog::InitializeTrackerButtonClicked()
{
    if( !m_tracker->IsInitialized() )
        m_tracker->Initialize();
    else
        m_tracker->StopTracking();

    this->UpdateGlobalSettings();
    this->UpdateToolDescription();
    this->UpdateToolList();
}

void TrackerSettingsDialog::ToolsListSelectionChanged()
{
    int toolIndex = m_toolsListBox->currentItem()->data( Qt::UserRole ).toInt();
    m_tracker->SetCurrentToolIndex( toolIndex );
    this->UpdateToolDescription( );
    this->UpdateUI();
}

void TrackerSettingsDialog::AddToolButtonClicked()
{
    bool ok;
    QString text = QInputDialog::getText( this, "New tool", "Enter tool name:", QLineEdit::Normal, QString::null, &ok );
    if ( ok && !text.isEmpty() ) 
    {
        m_tracker->AddNewTool( Passive, text );
    } 
    this->UpdateToolList( );
}

void TrackerSettingsDialog::RemoveToolButtonClicked()
{
    m_tracker->RemoveTool( m_tracker->GetCurrentToolIndex() );
    this->UpdateToolList();
}

void TrackerSettingsDialog::RenameToolButtonClicked()
{
    int currentTool = m_tracker->GetCurrentToolIndex();
    if( currentTool != -1 )
    {
        bool ok;
        QString toolName( m_tracker->GetToolName( currentTool ) );
        QString text = QInputDialog::getText( this, "Edit tool name", "Enter tool name:", QLineEdit::Normal, toolName, &ok );
        if ( ok && !text.isEmpty() && text != toolName ) 
        {
            m_tracker->SetToolName( currentTool, text );
            this->UpdateToolList( );
        } 
    }
}

void TrackerSettingsDialog::ReferenceToolComboChanged( int index )
{
    if( index < m_tracker->GetNumberOfTools() )
    {
        m_tracker->SetReferenceToolIndex( index );
    }
    else
    {
        m_tracker->SetReferenceToolIndex( -1 );
    }
}

void TrackerSettingsDialog::ActiveCheckBoxToggled( bool isOn )
{
    int currentTool = m_tracker->GetCurrentToolIndex();
    if( currentTool != -1 )
    {
        if( isOn )
            m_tracker->ActivateTool( currentTool, 1 );
        else
            m_tracker->ActivateTool( currentTool, 0 );
    }
    this->UpdateUI();
}

void TrackerSettingsDialog::BrowseRomFileButtonClicked()
{
    QString romFile = Application::GetInstance().GetFileNameOpen( "Rom file name", ".", "Rom files (*.rom)" );
    m_tracker->SetRomFileName( m_tracker->GetCurrentToolIndex(), romFile );
    UpdateToolDescription();
}

void TrackerSettingsDialog::UpdateToolDescription( )
{
    m_activationCheckBox->blockSignals( true );
    m_toolUseComboBox->blockSignals( true );
    m_currentToolDescriptionTextBox->clear();

    int currentTool = m_tracker->GetCurrentToolIndex();
    if( currentTool != -1 )
    {     
        bool toolTypeActive = m_tracker->GetToolType( currentTool ) == Active;
        QString romFileName = m_tracker->GetRomFileName( currentTool );
        bool canActivate = !( romFileName.isEmpty() || toolTypeActive );
        bool isActive = m_tracker->IsToolActive( currentTool );

        m_romFilenameEdit->setEnabled( !toolTypeActive );
        m_browseButton->setEnabled( !toolTypeActive );
        m_romFilenameEdit->setText( romFileName );


        m_activationCheckBox->setEnabled( canActivate );
        m_activationCheckBox->setChecked( isActive );

        
        m_currentToolDescriptionTextBox->setPlainText( m_tracker->GetToolDescription( currentTool ) );
        
        m_toolUseComboBox->setCurrentIndex( (int)(m_tracker->GetToolUse( currentTool ) ) );
        m_toolUseComboBox->setEnabled( true );
    }
    else
    {
        m_romFilenameEdit->clear();
        m_activationCheckBox->setChecked( false );
        this->m_toolUseComboBox->setCurrentIndex( (int)Generic);
        this->m_toolUseComboBox->setEnabled( false );
    }

    m_activationCheckBox->blockSignals( false );
    m_toolUseComboBox->blockSignals( false );
}

void TrackerSettingsDialog::UpdateToolList( )
{
    m_referenceToolCombo->blockSignals( true );
    m_toolsListBox->blockSignals( true );

    m_referenceToolCombo->clear();
    m_toolsListBox->clear();
    int numberOfTools = m_tracker->GetNumberOfTools();
    for( int i = 0; i < numberOfTools; i++ )
    {
        QListWidgetItem * item = new QListWidgetItem( m_tracker->GetToolName( i ), m_toolsListBox );
        item->setData( Qt::UserRole, QVariant(i) );
        m_referenceToolCombo->addItem( m_tracker->GetToolName( i ) );
    }
    m_referenceToolCombo->addItem( "None");
    int currentTool = m_tracker->GetCurrentToolIndex();
    m_toolsListBox->setCurrentRow ( currentTool );
    m_renameToolButton->setEnabled( currentTool != -1 );
    m_removeToolButton->setEnabled( currentTool != -1 );
    this->UpdateToolDescription();
    
    int referenceTool = m_tracker->GetReferenceToolIndex();
    if( referenceTool > -1 )
    {
        if (m_tracker->IsToolActive(referenceTool ))
           m_referenceToolCombo->setCurrentIndex( referenceTool );
        else
           m_referenceToolCombo->setCurrentIndex( m_tracker->GetNumberOfTools() ) ;
    }
    else
    {
        m_referenceToolCombo->setCurrentIndex( m_tracker->GetNumberOfTools() ) ;
    }

    m_referenceToolCombo->blockSignals( false );
    m_toolsListBox->blockSignals( false );
    this->UpdateUI();
}


void TrackerSettingsDialog::UpdateGlobalSettings()
{
    if( m_tracker->IsInitialized() )
    {
        m_initializeTrackerButton->setText( "Initialize" );
        m_initializeTrackerButton->setEnabled( false );
    }
    else
    {
        m_initializeTrackerButton->setText( "Initialize" );
        m_initializeTrackerButton->setEnabled( true );
    }
}

void TrackerSettingsDialog::ToolUseComboChanged( int index )
{
    if( index >= 0 && index < (int)InvalidUse )
    {
        ToolUse use = (ToolUse)index;
        m_tracker->SetToolUse( m_tracker->GetCurrentToolIndex(), use );
        if (m_activationCheckBox->isChecked())
            m_activationCheckBox->toggle();
        this->UpdateToolDescription();
    }
}

void TrackerSettingsDialog::UpdateUI()
{
    int currentTool = m_tracker->GetCurrentToolIndex();
    bool enableButton = false;
    if( currentTool >= 0 ) // currentTool == -1 means no tools were configured
        enableButton = m_tracker->IsToolActive( currentTool ) == 0;
    m_removeToolButton->setEnabled( enableButton );
    m_renameToolButton->setEnabled( enableButton );
    m_romFilenameEdit->setEnabled( enableButton );
    m_browseButton->setEnabled( enableButton );
    m_toolUseComboBox->setEnabled( enableButton );
}
