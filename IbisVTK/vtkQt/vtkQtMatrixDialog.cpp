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
#include <qmessagebox.h>
#include "vtkObject.h"
#include "vtkMatrix4x4.h"
#include "vtkHomogeneousTransform.h"
#include "vtkTransform.h"
#include "vtkXFMReader.h"
#include "vtkXFMWriter.h"
#include "vtkEventQtSlotConnect.h"

vtkQtMatrixDialog::vtkQtMatrixDialog( bool readOnly, QWidget* parent )
    : QDialog( parent, Qt::WindowStaysOnTopHint )
    , m_readOnly( readOnly )
    , m_transform( 0 )
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
    Layout3->addWidget( m_identityButton );
    
    m_invertButton = new QPushButton( this );
	m_invertButton->setObjectName("m_invertButton");
    m_invertButton->setText( tr( "Invert" ) );
    m_invertButton->setMinimumSize( QSize( 70, 30 ) );
    m_invertButton->setMaximumSize( QSize( 70, 30 ) );
    m_invertButton->setEnabled(!readOnly);
    Layout3->addWidget( m_invertButton );
    
    m_loadButton = new QPushButton( this );
	m_loadButton->setObjectName("m_loadButton");
    m_loadButton->setText( tr( "Load" ) );
    m_loadButton->setMinimumSize( QSize( 70, 30 ) );
    m_loadButton->setMaximumSize( QSize( 70, 30 ) );
    m_loadButton->setEnabled(!readOnly);
    Layout3->addWidget( m_loadButton );
    
    m_saveButton = new QPushButton( this );
	m_saveButton->setObjectName("m_loadButton");
    m_saveButton->setText( tr( "Save" ) );
    m_saveButton->setMinimumSize( QSize( 70, 30 ) );
    m_saveButton->setMaximumSize( QSize( 70, 30 ) );
    Layout3->addWidget( m_saveButton );
    
    m_revertButton = new QPushButton( this );
	m_revertButton->setObjectName("m_revertButton");
    m_revertButton->setText( tr( "Restore" ) );
    m_revertButton->setMinimumSize( QSize( 70, 30 ) );
    m_revertButton->setMaximumSize( QSize( 70, 30 ) );
    m_revertButton->setEnabled(!readOnly);
    Layout3->addWidget( m_revertButton );
    
    QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout3->addItem( spacer_2 );

    m_applyButton = new QPushButton( this );
    m_applyButton->setObjectName("m_applyButton");
    m_applyButton->setText( tr( "Apply" ) );
    m_applyButton->setMinimumSize( QSize( 70, 30 ) );
    m_applyButton->setMaximumSize( QSize( 70, 30 ) );
    m_applyButton->setEnabled(!readOnly);
    Layout3->addWidget( m_applyButton );
    
    MatrixDialogLayout->addLayout( Layout3 );
    
    // signals and slots connections
    connect( m_invertButton, SIGNAL( clicked() ), this, SLOT( InvertButtonClicked() ) );
    connect( m_identityButton, SIGNAL( clicked() ), this, SLOT( IdentityButtonClicked() ) );
    connect( m_revertButton, SIGNAL( clicked() ), this, SLOT( RevertButtonClicked() ) );
    connect( m_applyButton, SIGNAL( clicked() ), this, SLOT( ApplyButtonClicked() ) );
    connect( m_loadButton, SIGNAL( clicked() ), this, SLOT( LoadButtonClicked() ) );
    connect( m_saveButton, SIGNAL( clicked() ), this, SLOT( SaveButtonClicked() ) );
    UpdateUI();
    m_directory = QDir::homePath();
}

vtkQtMatrixDialog::~vtkQtMatrixDialog()
{
    m_eventSlotConnect->Delete();

    if( m_matrix )
        m_matrix->UnRegister( 0 );
    if( m_transform )
        m_transform->UnRegister( 0 );

    m_copy_matrix->Delete();
}

void vtkQtMatrixDialog::SetMatrixElements( )
{
    vtkMatrix4x4 * mat = 0;
    if( m_transform )
        mat = m_transform->GetMatrix();
    else
        mat = m_matrix;

    if( mat )
    {
        for( int i = 0; i < 4; i++ )
        {
            for( int j = 0; j < 4; j++ )
            {
                mat->SetElement( i, j, m_matEdit[i][j]->text().toDouble() );
            }
        }
    }

    if( m_transform )
        m_transform->Modified();
}

void vtkQtMatrixDialog::InvertButtonClicked()
{
    if( m_matrix )
        m_matrix->Invert();
    if( m_transform )
        m_transform->Inverse();
}


void vtkQtMatrixDialog::IdentityButtonClicked()
{
    if( m_matrix )
        m_matrix->Identity();
    if( m_transform )
    {
        vtkTransform * t = vtkTransform::SafeDownCast( m_transform );
        Q_ASSERT_X( t, "vtkQtMatrixDialog::IdentityButtonClicked()", "Can't edit a transform that is not a vtkTransform." );
        t->Identity();
    }
}


void vtkQtMatrixDialog::RevertButtonClicked()
{
    if( m_matrix )
        m_matrix->DeepCopy( m_copy_matrix );
    if( m_transform )
    {
        vtkTransform * t = vtkTransform::SafeDownCast( m_transform );
        Q_ASSERT_X( t, "vtkQtMatrixDialog::RevertButtonClicked()", "Can't edit a transform that is not a vtkTransform." );
        t->SetMatrix( m_copy_matrix );
    }
}


void vtkQtMatrixDialog::MatrixChanged()
{
    UpdateUI();
}


void vtkQtMatrixDialog::SetMatrix( vtkMatrix4x4 * mat )
{
    Q_ASSERT_X( !m_transform, "vtkQtMatrixDialog::SetMatrix", "Can't set both matrix and transform." );

    if( m_matrix )
    {
        m_eventSlotConnect->Disconnect( m_matrix );
        m_matrix->UnRegister( NULL );
    }

    m_matrix = mat;
    if( m_matrix )
    {
        m_copy_matrix->DeepCopy( m_matrix );
        m_matrix->Register( NULL );
        m_eventSlotConnect->Connect( m_matrix, vtkCommand::ModifiedEvent, this, SLOT(MatrixChanged()), 0, 0.0, Qt::QueuedConnection );
    }
    UpdateUI();   
}

void vtkQtMatrixDialog::SetTransform( vtkHomogeneousTransform * t )
{
    Q_ASSERT_X( !m_matrix, "vtkQtMatrixDialog::SetTransform", "Can't set both matrix and transform." );
    Q_ASSERT_X( m_readOnly || vtkTransform::SafeDownCast( t ), "vtkQtMatrixDialog::SetTransform()", "Transform has to be a vtkTransform or dialog read-only" );

    if( m_transform )
    {
        m_eventSlotConnect->Disconnect( m_transform );
        m_transform->UnRegister( NULL );
    }

    m_transform = t;
    if( m_transform )
    {
        m_copy_matrix->DeepCopy( m_transform->GetMatrix() );
        m_transform->Register( NULL );
        m_eventSlotConnect->Connect( m_transform, vtkCommand::ModifiedEvent, this, SLOT(MatrixChanged()), 0, 0.0, Qt::QueuedConnection );
    }
    UpdateUI();
}

vtkMatrix4x4 * vtkQtMatrixDialog::GetMatrix( )
{
    if( m_transform )
        return m_transform->GetMatrix();
    return m_matrix;
}

void vtkQtMatrixDialog::UpdateUI()
{
    vtkMatrix4x4 * mat = 0;
    if( m_transform )
        mat = m_transform->GetMatrix();
    else
        mat = m_matrix;

    if( mat )
    {
        for( int i = 0; i < 4; i++ )
        {
            for( int j = 0; j < 4; j++ )
            {
                m_matEdit[i][j]->setText( QString::number( mat->Element[i][j], 'g', 15 ) );
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

void vtkQtMatrixDialog::ApplyButtonClicked()
{
    SetMatrixElements( );
}

void vtkQtMatrixDialog::LoadButtonClicked()
{
    // Get filename from user
    if( !QFile::exists( m_directory ) )
    {
        m_directory = QDir::homePath();
    }

    QString filename = QFileDialog::getOpenFileName( this, tr("Open XFM file"), m_directory, tr("*.xfm") );
    if( !filename.isEmpty() )
    {
        QFile OpenFile( filename );
        vtkMatrix4x4 * mat = vtkMatrix4x4::New();
        bool valid = false;

        int type = GetFileType(&OpenFile);
        if (type == TEXT_FILE)
        {
            valid = LoadFromTextFile( &OpenFile, mat );
        }
        else if (type == XFM_FILE)
        {
            valid = LoadFromXFMFile( &OpenFile, mat); // I'm not checking the return because it would be redundant. GetType()  would fail first.
        }
        else
        {
            QMessageBox::warning( this, "Error: ", "Unknown file format.", 1, 0 );
        }

        if( valid )
        {
            if( m_matrix )
                m_matrix->DeepCopy( mat );
            if( m_transform )
            {
                vtkTransform * t = vtkTransform::SafeDownCast( m_transform );
                Q_ASSERT_X( t, "vtkQtMatrixDialog::LoadButtonClicked()", "Can't edit a transform that is not a vtkTransform." );
                t->SetMatrix( mat );
            }
        }

        QFileInfo info( OpenFile.fileName() );
        m_directory = info.absolutePath();

        mat->Delete();
    }
 }

void vtkQtMatrixDialog::SaveButtonClicked()
{
    vtkMatrix4x4 * mat = 0;
    if( m_transform )
        mat = m_transform->GetMatrix();
    else
        mat = m_matrix;

    if( !mat )
    {
        std::cerr << "Trying to save matrix that is null";
        return;
    }

    if( !QFile::exists( m_directory ) )
    {
        m_directory = QDir::homePath();
    }

    QString filename = QFileDialog::getSaveFileName( this, tr("Save XFM file"), m_directory, tr("*.xfm") );
    if( !filename.isEmpty() )
    {
        vtkXFMWriter * writer = vtkXFMWriter::New();
        writer->SetFileName( filename.toUtf8().data() );
        writer->SetMatrix( mat );
        writer->Write();
        writer->Delete();
        QFileInfo info( filename );
        m_directory = info.absolutePath();
    }
}

int vtkQtMatrixDialog::GetFileType(QFile *f)
{
    char line[256];
    if (!f->open(QIODevice::ReadOnly))
    {
        QMessageBox::warning( this, "Read error: can't open file" , f->fileName(), 1, 0 );
        return UNKNOWN_FILE_TYPE;
    }
    line[0] = 0;
    f->readLine( line, 256 );
    f->close();
    if ( isdigit(line[0]) || (line[0] == '-') )
        return TEXT_FILE;
    else
    {
        if( strncmp( line, "MNI Transform File", 18 ) == 0 )
            return XFM_FILE;
    }
    return UNKNOWN_FILE_TYPE;
}

bool vtkQtMatrixDialog::LoadFromXFMFile( QFile *f, vtkMatrix4x4 * mat )
{
    vtkXFMReader *reader = vtkXFMReader::New();
    if( reader->CanReadFile( f->fileName().toUtf8() ) )
    {    
        reader->SetFileName(f->fileName().toUtf8() );
        reader->SetMatrix(mat);
        reader->Update();
        reader->Delete();    
        return true;
    }
    reader->Delete();    
    return false;
}

bool vtkQtMatrixDialog::LoadFromTextFile(QFile *f, vtkMatrix4x4 * mat )
{
    char line[256], *tok;
    int ok = 1;
    if (!f->open(QIODevice::ReadOnly))
    {
        QMessageBox::warning( this, "Read error: can't open file" , f->fileName(), 1, 0 );
        return false;
    }
    line[0] = 0;
    f->readLine( line, 256 );
    f->close();
    

    tok = strtok( line, " \t" );
    for( int i = 0; i < 4 && ok; i++ )
    {
        for( int j = 0; j < 4 && ok; j++ )
        {
            if ( tok )
            {
                mat->SetElement( i, j, atof(tok) );
                tok = strtok( NULL, " \t" );
            }
            else
                ok = 0;
        }
    }

    return true;
}

void vtkQtMatrixDialog::SetDirectory(const QString &dir)
{
    m_directory = dir;
    if( !QFile::exists( m_directory ) )
    {
        m_directory = QDir::homePath();
    }
}
