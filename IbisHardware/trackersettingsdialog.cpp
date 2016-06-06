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
#include <QMessageBox>
#include <QVariant>
#include <QInputDialog>
#include <QFileDialog>
#include "vtkQtMatrixDialog.h"
#include "application.h"
#include "tracker.h"
#include "pointercalibrationdialog.h"

TrackerSettingsDialog::TrackerSettingsDialog(QWidget * parent, const char * name )
    : QWidget(parent)
{
    setupUi(this);
    m_toolCalibrationDialog = NULL;
    m_worldCalibrationDialog = NULL;
    m_tipCalibrationDialog = NULL;
   
    m_tracker = 0;
}


TrackerSettingsDialog::~TrackerSettingsDialog()
{
    if( m_worldCalibrationDialog )
        m_worldCalibrationDialog->close();
    if( m_toolCalibrationDialog )
        m_toolCalibrationDialog->close();
    if( m_tipCalibrationDialog )
        m_tipCalibrationDialog->close();
}


void TrackerSettingsDialog::SetTracker( Tracker * tracker )
{
    if( m_tracker != tracker )
    {
        if( m_tracker )
        {
            m_tracker->disconnect( this );
        }
        
        m_tracker = tracker;
        
        if( m_tracker )
        {
            connect( m_tracker, SIGNAL( ToolActivated( int ) ), this, SLOT( ToolActivationEvent() ) );
            connect( m_tracker, SIGNAL( ToolDeactivated( int ) ), this, SLOT( ToolActivationEvent() ) );
            connect( m_tracker, SIGNAL( NavigationPointerChangedSignal( ) ), this, SLOT( ToolActivationEvent() ) );
            connect( m_tracker, SIGNAL( ToolNameChanged(int,QString) ), this, SLOT(UpdateToolList()) );
        }
        this->UpdateToolList();
        this->UpdateGlobalSettings();
    }
}


void TrackerSettingsDialog::WorldCalibrationMatrixButtonClicked()
{
    m_worldCalibrationDialog = new vtkQtMatrixDialog( false, 0 );
    m_worldCalibrationDialog->setWindowTitle( "World calibration matrix" );
    m_worldCalibrationDialog->setAttribute(Qt::WA_DeleteOnClose);
    m_worldCalibrationDialog->SetMatrix( m_tracker->GetWorldCalibrationMatrix() );
    connect( m_worldCalibrationDialog, SIGNAL( destroyed() ), this, SLOT( WorldCalibrationDialogClosed() ) );
    m_worldCalibrationDialog->show();
}


void TrackerSettingsDialog::WorldCalibrationDialogClosed()
{
    m_worldCalibrationDialog = NULL;
}


void TrackerSettingsDialog::InitializeTrackerButtonClicked()
{
    if( !m_tracker->IsInitialized() )
    {
        QMessageBox::warning( this, "Warning", "This operation may take long time.\nPlease wait patiently untill you see changes in the Tracker Status dialog.\nVerify if all your tools are active and correct reference tool is selected.", 1, 0 );
        m_tracker->Initialize();
        if( m_tracker->IsInitialized() )
        {
            this->UpdateGlobalSettings();
            this->UpdateToolDescription();
            this->UpdateToolList();
        }
    }
    else
    {
        QMessageBox::warning( this, "Warning", "This operation may take long time.\nPlease wait patiently untill you see changes in the Tracker Status dialog.", 1, 0 );
        m_tracker->StopTracking();
        this->UpdateGlobalSettings();
    }
}


void TrackerSettingsDialog::ToolsListSelectionChanged()
{
    int toolIndex = m_toolsListBox->currentItem()->data( Qt::UserRole ).toInt();
    m_tracker->SetCurrentToolIndex( toolIndex );
    this->UpdateToolDescription( );
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
    int ok, saveIndex = m_tracker->GetReferenceToolIndex();
    if( index < m_tracker->GetNumberOfTools() )
    {
        ok = m_tracker->SetReferenceToolIndex( index );
    }
    else
    {
        ok = m_tracker->SetReferenceToolIndex( -1 );
    }
    if (!ok)
    {
        ok = m_tracker->SetReferenceToolIndex( saveIndex );
        m_referenceToolCombo->setCurrentIndex( saveIndex );
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
}


void TrackerSettingsDialog::BrowseRomFileButtonClicked()
{
    QString romFile = Application::GetInstance().GetOpenFileName( "Rom file name", ".", "Rom files (*.rom)" );
    m_tracker->SetRomFileName( m_tracker->GetCurrentToolIndex(), romFile );
    UpdateToolDescription();
}


void TrackerSettingsDialog::TipCalibrationButtonClicked()
{
    int currentTool = m_tracker->GetCurrentToolIndex();
    if( currentTool != -1 && !m_tipCalibrationDialog )
    {
		m_tipCalibrationDialog = new PointerCalibrationDialog( NULL, "Tip calibration" );
        m_tipCalibrationDialog->setAttribute( Qt::WA_DeleteOnClose, true );
        connect( m_tipCalibrationDialog, SIGNAL( destroyed() ), SLOT( TipCalibrationDialogClosed() ) );
        m_tipCalibrationDialog->SetTracker( m_tracker );
        m_tipCalibrationDialog->show();
    }
}


void TrackerSettingsDialog::TipCalibrationDialogClosed()
{
    m_tipCalibrationDialog = NULL;
}


void TrackerSettingsDialog::ToolCalibrationMatrixButtonClicked()
{
    bool readOnly = false;
    if (m_tracker->GetToolUse( m_tracker->GetCurrentToolIndex()) == UsProbe)
        readOnly = true;
    m_toolCalibrationDialog = new vtkQtMatrixDialog( readOnly, 0 );
    m_toolCalibrationDialog->setWindowTitle( "Current tool calibration matrix" );
    m_toolCalibrationDialog->setAttribute(Qt::WA_DeleteOnClose);
    m_toolCalibrationDialog->SetMatrix( m_tracker->GetToolCalibrationMatrix( m_tracker->GetCurrentToolIndex() ) );
    connect( m_toolCalibrationDialog, SIGNAL( destroyed() ), this, SLOT( ToolCalibrationMatrixDialogClosed() ) );
    m_toolCalibrationDialog->show();
}


void TrackerSettingsDialog::ToolCalibrationMatrixDialogClosed()
{
    m_toolCalibrationDialog = NULL;
}


void TrackerSettingsDialog::UpdateToolDescription( )
{
    int currentTool = m_tracker->GetCurrentToolIndex();
    if( currentTool != -1 )
    {     
        if( m_tracker->GetToolType( currentTool ) == Active )
        {
            m_romFilenameEdit->setEnabled( false );
            m_browseButton->setEnabled( false );
            m_romFilenameEdit->clear();
        }
        else
        {
            m_romFilenameEdit->setEnabled( true );
            m_browseButton->setEnabled( true );
            m_romFilenameEdit->setText( m_tracker->GetRomFileName( currentTool ));
        }
        
        if (m_tracker->GetToolUse(currentTool) == NoUse 
                || m_romFilenameEdit->text().isEmpty() 
                || m_tracker->GetToolType( currentTool ) == Active)
            m_activationCheckBox->setEnabled(0);
        else
        {
            m_activationCheckBox->setEnabled(1);
            if( m_tracker->IsToolActive( currentTool ) )
            {
                if (!m_activationCheckBox->isChecked())
                    m_activationCheckBox->setChecked( true );
            }
            else
            {
                if (m_activationCheckBox->isChecked())
                    m_activationCheckBox->setChecked( false );
            }
        }
        
        m_currentToolDescriptionTextBox->setPlainText( m_tracker->GetToolDescription( currentTool ) );
        
        this->m_toolUseComboBox->setCurrentIndex( (int)(m_tracker->GetToolUse( currentTool ) ) );
        this->m_toolUseComboBox->setEnabled( true );
        
        if( m_toolCalibrationDialog )
            m_toolCalibrationDialog->SetMatrix( m_tracker->GetToolCalibrationMatrix( currentTool ) );
    }
    else
    {
        m_currentToolDescriptionTextBox->clear();
        m_romFilenameEdit->clear();
        m_activationCheckBox->setChecked( false );
        this->m_toolUseComboBox->setCurrentIndex( (int)NoUse );
        this->m_toolUseComboBox->setEnabled( false );
        if( m_toolCalibrationDialog )
            m_toolCalibrationDialog->SetMatrix( NULL );
        if( m_tipCalibrationDialog )
            m_tipCalibrationDialog->close();
    }
}


void TrackerSettingsDialog::UpdateToolList( )
{
    m_referenceToolCombo->clear();
    m_pointerToolCombo->clear();
    m_toolsListBox->clear();
    int numberOfTools = m_tracker->GetNumberOfTools();
    for( int i = 0; i < numberOfTools; i++ )
    {
        QListWidgetItem * item = new QListWidgetItem( m_tracker->GetToolName( i ), m_toolsListBox );
        item->setData( Qt::UserRole, QVariant(i) );
        m_referenceToolCombo->addItem( m_tracker->GetToolName( i ) );
        m_pointerToolCombo->addItem( m_tracker->GetToolName( i ) );
    }
    m_referenceToolCombo->addItem( "None");
    m_pointerToolCombo->addItem( "None");
    int currentTool = m_tracker->GetCurrentToolIndex();
    if( currentTool != -1 )
    {
        m_renameToolButton->setEnabled( true );
        m_toolsListBox->setCurrentRow ( currentTool );
    }
    else
    {
        // in this case, no selection => no call to ToolsListSelectionChanged
        // so we call UpdateToolDescription.
        m_renameToolButton->setEnabled( false );
        this->UpdateToolDescription();
    }
    
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
    int navigationPointer = m_tracker->GetNavigationPointerIndex();
    if (navigationPointer > -1)
    {
        if (m_tracker->IsToolActive(navigationPointer ))
           m_pointerToolCombo->setCurrentIndex( navigationPointer );
        else
           m_pointerToolCombo->setCurrentIndex( m_tracker->GetNumberOfTools() ) ;
    }
    else
    {
        m_pointerToolCombo->setCurrentIndex( m_tracker->GetNumberOfTools() ) ;
    }
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

void TrackerSettingsDialog::ToolActivationEvent()
{
    this->UpdateToolList();
}


void TrackerSettingsDialog::ToolUseComboChanged( int index )
{
    if( index >= 0 && index < (int)NoUse )
    {
        if ( index == (int)UsProbe  && m_tracker->GetUSProbeToolIndex() >= 0 )
        {
            QMessageBox::warning( this, "Error", "Only one US Probe supported in the current ibis version.", 1, 0 );
            m_toolUseComboBox->setCurrentIndex((int)NoUse);
            return;
        }
        ToolUse use = (ToolUse)index;
        if ((use == Pointer || use == UsProbe) && m_tracker->GetToolUse( m_tracker->GetCurrentToolIndex()) == NoUse)
        {
            QMessageBox::warning( this, "Warning", "Please calibrate the tool before using it.", 1, 0 );
        }
        m_tracker->SetToolUse( m_tracker->GetCurrentToolIndex(), use );
        if (m_activationCheckBox->isChecked())
            m_activationCheckBox->toggle();
        this->UpdateToolDescription();
    }
}

void TrackerSettingsDialog::NavigationPointerChanged(int index)
{
    int ok, saveIndex = m_tracker->GetNavigationPointerIndex();
    if( index < m_tracker->GetNumberOfTools() )
    {
        ok = m_tracker->SetNavigationPointerIndex( index );
    }
    else
    {
        ok = m_tracker->SetNavigationPointerIndex( -1 );
    }
    if (!ok)
    {
        ok = m_tracker->SetNavigationPointerIndex( saveIndex );
        m_pointerToolCombo->setCurrentIndex( saveIndex );
    }
}
