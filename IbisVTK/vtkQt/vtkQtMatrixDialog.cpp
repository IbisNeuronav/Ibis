/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "vtkQtMatrixDialog.h"
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qstring.h>
#include <QApplication>
#include <QMimeData>
#include <QClipboard>
#include <QMessageBox>
#include "vtkObject.h"
#include "vtkMatrix4x4.h"
#include "vtkXFMReader.h"
#include "vtkXFMWriter.h"
#include "vtkEventQtSlotConnect.h"

vtkQtMatrixDialog::vtkQtMatrixDialog( bool readOnly, QWidget* parent )
    : QDialog( parent, Qt::WindowStaysOnTopHint )
    , m_readOnly( readOnly )
    , m_matrix( 0 )
{
    resize( 511, 210 );

    m_copy_matrix = vtkMatrix4x4::New();
    m_copy_matrix->Identity();

    m_eventSlotConnect = vtkEventQtSlotConnect::New();

    MatrixDialogLayout = new QVBoxLayout( this ); 
    MatrixDialogLayout->setSpacing( 6 );
    MatrixDialogLayout->setMargin( 11 );

    gridBox = new QGridLayout; 
    gridBox->setSpacing( 12 );
    gridBox->setMargin( 2 );

    // Create the text boxes for every matrix entry
    for( int i = 0; i < 4; i++ )
    {
        for( int j = 0; j < 4; j++ )
        {
            m_matEdit[i][j] = new QLineEdit( this );
            m_matEdit[i][j]->setReadOnly( readOnly );
            m_matEdit[i][j]->setAlignment( Qt::AlignRight );
            m_matEdit[i][j]->setMaxLength( 12 );
            connect( m_matEdit[i][j], SIGNAL(returnPressed()), this, SLOT(LineEditChanged()) );
            gridBox->addWidget( m_matEdit[i][j], i, j );
        }
    }

    MatrixDialogLayout->addLayout( gridBox );
    
    // create the box that contains the ok and cancel buttons
    QSpacerItem * spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    MatrixDialogLayout->addItem( spacer );

    Layout3 = new QHBoxLayout; 
    Layout3->setSpacing( 6 );
    Layout3->setMargin( 0 );
    
    m_identityButton = new QPushButton( this );
	m_identityButton->setObjectName("m_identityButton" );
    m_identityButton->setText( tr( "Identity" ) );
    m_identityButton->setMinimumSize( QSize( 70, 30 ) );
    m_identityButton->setMaximumSize( QSize( 70, 30 ) );
    m_identityButton->setEnabled(!readOnly);
    m_identityButton->setAutoDefault( false );
    Layout3->addWidget( m_identityButton );
    
    m_invertButton = new QPushButton( this );
	m_invertButton->setObjectName("m_invertButton");
    m_invertButton->setText( tr( "Invert" ) );
    m_invertButton->setMinimumSize( QSize( 70, 30 ) );
    m_invertButton->setMaximumSize( QSize( 70, 30 ) );
    m_invertButton->setEnabled(!readOnly);
    m_invertButton->setAutoDefault( false );
    Layout3->addWidget( m_invertButton );
    
    m_loadButton = new QPushButton( this );
	m_loadButton->setObjectName("m_loadButton");
    m_loadButton->setText( tr( "Load" ) );
    m_loadButton->setMinimumSize( QSize( 70, 30 ) );
    m_loadButton->setMaximumSize( QSize( 70, 30 ) );
    m_loadButton->setEnabled(!readOnly);
    m_loadButton->setAutoDefault( false );
    Layout3->addWidget( m_loadButton );
    
    m_saveButton = new QPushButton( this );
    m_saveButton->setObjectName("m_saveButton");
    m_saveButton->setText( tr( "Save" ) );
    m_saveButton->setMinimumSize( QSize( 70, 30 ) );
    m_saveButton->setMaximumSize( QSize( 70, 30 ) );
    m_saveButton->setAutoDefault( false );
    Layout3->addWidget( m_saveButton );
    
    m_revertButton = new QPushButton( this );
	m_revertButton->setObjectName("m_revertButton");
    m_revertButton->setText( tr( "Restore" ) );
    m_revertButton->setMinimumSize( QSize( 70, 30 ) );
    m_revertButton->setMaximumSize( QSize( 70, 30 ) );
    m_revertButton->setEnabled(!readOnly);
    m_revertButton->setAutoDefault( false );
    Layout3->addWidget( m_revertButton );
    
    QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout3->addItem( spacer_2 );
    
    MatrixDialogLayout->addLayout( Layout3 );
    
    Layout4 = new QHBoxLayout;
    Layout4->setSpacing( 6 );
    Layout4->setMargin( 0 );

    m_copyButton = new QPushButton( this );
    m_copyButton->setObjectName("m_copyButton");
    m_copyButton->setText( tr( "Copy" ) );
    m_copyButton->setMinimumSize( QSize( 70, 30 ) );
    m_copyButton->setMaximumSize( QSize( 70, 30 ) );
    m_copyButton->setAutoDefault( false );
    Layout4->addWidget( m_copyButton );

    m_pasteButton = new QPushButton( this );
    m_pasteButton->setObjectName("m_pasteButton");
    m_pasteButton->setText( tr( "Paste" ) );
    m_pasteButton->setMinimumSize( QSize( 70, 30 ) );
    m_pasteButton->setMaximumSize( QSize( 70, 30 ) );
    m_pasteButton->setEnabled(!readOnly);
    m_pasteButton->setAutoDefault( false );
    Layout4->addWidget( m_pasteButton );

    QSpacerItem* spacer_3 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout4->addItem( spacer_3 );

    MatrixDialogLayout->addLayout( Layout4 );

    // signals and slots connections
    connect( m_invertButton, SIGNAL( clicked() ), this, SLOT( InvertButtonClicked() ) );
    connect( m_identityButton, SIGNAL( clicked() ), this, SLOT( IdentityButtonClicked() ) );
    connect( m_revertButton, SIGNAL( clicked() ), this, SLOT( RevertButtonClicked() ) );
    connect( m_loadButton, SIGNAL( clicked() ), this, SLOT( LoadButtonClicked() ) );
    connect( m_saveButton, SIGNAL( clicked() ), this, SLOT( SaveButtonClicked() ) );
    connect( m_copyButton, SIGNAL( clicked() ), this, SLOT( CopyButtonClicked() ) );
    connect( m_pasteButton, SIGNAL( clicked() ), this, SLOT( PasteButtonClicked() ) );
    UpdateUI();
    m_directory = QDir::homePath();
}

vtkQtMatrixDialog::~vtkQtMatrixDialog()
{
    m_eventSlotConnect->Delete();

    if( m_matrix )
        m_matrix->UnRegister( 0 );

    m_copy_matrix->Delete();
}

void vtkQtMatrixDialog::SetMatrixElements( )
{
    for( int i = 0; i < 4; i++ )
    {
        for( int j = 0; j < 4; j++ )
        {
            m_matrix->SetElement( i, j, m_matEdit[i][j]->text().toDouble() );
        }
    }
    m_matrix->Modified();
    emit Modified();
}

void vtkQtMatrixDialog::InvertButtonClicked()
{
    if( m_matrix )
    {
        m_copy_matrix->DeepCopy( m_matrix );
        m_matrix->Invert();
        emit Modified();
    }
}


void vtkQtMatrixDialog::IdentityButtonClicked()
{
    if( m_matrix )
    {
        m_copy_matrix->DeepCopy( m_matrix );
        m_matrix->Identity();
        emit Modified();
    }
}


void vtkQtMatrixDialog::RevertButtonClicked()
{
    if( m_matrix )
    {
        m_matrix->DeepCopy( m_copy_matrix );
        m_copy_matrix->Identity();
        emit Modified();
    }
    UpdateUI();
}


void vtkQtMatrixDialog::MatrixChanged()
{
    UpdateUI();
}


void vtkQtMatrixDialog::SetMatrix( vtkMatrix4x4 * mat )
{
    if( m_matrix )
    {
        m_eventSlotConnect->Disconnect( m_matrix );
        m_matrix->UnRegister( NULL );
    }

    m_matrix = mat;
    if( m_matrix )
    {
        m_copy_matrix->Identity();
        m_matrix->Register( NULL );
        m_eventSlotConnect->Connect( m_matrix, vtkCommand::ModifiedEvent, this, SLOT(MatrixChanged()), 0, 0.0, Qt::QueuedConnection );
    }
    UpdateUI();
}

void vtkQtMatrixDialog::UpdateUI()
{
    if( m_matrix )
    {
        for( int i = 0; i < 4; i++ )
        {
            for( int j = 0; j < 4; j++ )
            {
                m_matEdit[i][j]->setText( QString::number( m_matrix->Element[i][j], 'g', 15 ) );
            }
        }
    }
    else
    {
        for( int i = 0; i < 4; i++ )
        {
            for( int j = 0; j < 4; j++ )
            {
                m_matEdit[i][j]->setText( "----" );
            }
        }
    }
}

void vtkQtMatrixDialog::LoadButtonClicked()
{
    // Get filename from user

    QString filename = QFileDialog::getOpenFileName( this, tr("Open XFM file"), m_directory, tr("*.xfm") );
    if( !filename.isEmpty() )
    {
        QFile OpenFile( filename );
        vtkMatrix4x4 * mat = vtkMatrix4x4::New();
        vtkXFMReader *reader = vtkXFMReader::New();
        if( reader->CanReadFile( filename.toUtf8() ) )
        {
            reader->SetFileName(filename.toUtf8() );
            reader->SetMatrix(mat);
            reader->Update();
            reader->Delete();
            if( m_matrix )
                m_matrix->DeepCopy( mat );
            mat->Delete();
            UpdateUI();
            emit Modified();
        }
        else
        {
            reader->Delete();
            mat->Delete();
            return;
        }

        QFileInfo info( OpenFile.fileName() );
        m_directory = info.absolutePath();
    }
 }

void vtkQtMatrixDialog::SaveButtonClicked()
{
    Q_ASSERT( m_matrix );

    QString filename = QFileDialog::getSaveFileName( this, tr("Save XFM file"), m_directory, tr("*.xfm") );
    if( !filename.isEmpty() )
    {
        vtkXFMWriter * writer = vtkXFMWriter::New();
        writer->SetFileName( filename.toUtf8().data() );
        writer->SetMatrix( m_matrix );
        writer->Write();
        writer->Delete();
        QFileInfo info( filename );
        m_directory = info.absolutePath();
    }
}

void vtkQtMatrixDialog::CopyButtonClicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString all_matrix_elements;
    for( int i = 0; i < 4; i++ )
    {
        for( int j = 0; j < 4; j++ )
        {
            all_matrix_elements.append( m_matEdit[i][j]->text() );
            all_matrix_elements.append(' ');
        }
    }
    all_matrix_elements.remove( all_matrix_elements.size()-1, 1);
    clipboard->setText( all_matrix_elements );
}

void vtkQtMatrixDialog::PasteButtonClicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    m_copy_matrix->DeepCopy( m_matrix );
    if (mimeData->hasText())
    {
        QString text(mimeData->text());
        QStringList list = mimeData->text().split( ' ' );
        if( list.size() == 16 )
        {
            int l = 0;
            for( int i = 0; i < 4; i++ )
            {
                for( int j = 0; j < 4; j++ )
                {
                    m_matEdit[i][j]->setText( list.at( l++ ) );
                }
            }
            this->SetMatrixElements();
        }
        else
        {
            QMessageBox::warning( 0, "Warning", "Clipboard does not contain matrix elements." );
        }
    }
    else
    {
        QMessageBox::warning( 0, "Warning", "Nothing saved in clipboard." );
    }
}

void vtkQtMatrixDialog::SetDirectory(const QString &dir)
{
    m_directory = dir;
    if( !QFile::exists( m_directory ) )
    {
        m_directory = QDir::homePath();
    }
}

void vtkQtMatrixDialog::LineEditChanged()
{
    m_copy_matrix->DeepCopy( m_matrix );
    this->SetMatrixElements();
}
