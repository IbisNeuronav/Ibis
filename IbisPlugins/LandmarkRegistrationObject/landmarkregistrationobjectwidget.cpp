/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "landmarkregistrationobjectwidget.h"

#include <vtkSmartPointer.h>

#include <QPalette>

#include "landmarkregistrationobject.h"
#include "landmarktransform.h"
#include "pointsobject.h"
#include "ui_landmarkregistrationobjectwidget.h"

LandmarkRegistrationObjectWidget::LandmarkRegistrationObjectWidget( QWidget * parent, const char * name,
                                                                    Qt::WindowFlags f )
    : QWidget( parent ), ui( new Ui::LandmarkRegistrationObjectWidget )
{
    ui->setupUi( this );

    m_registrationObject = nullptr;

    ui->rmsDisplayLabel->setText( tr( "<font size=\"+3\">0.0</font>" ) );

    m_model = new QStandardItemModel( 0, 5 );
    m_model->setHeaderData( 0, Qt::Horizontal, QObject::tr( "Label" ) );
    m_model->setHeaderData( 1, Qt::Horizontal, QObject::tr( "Local Position" ) );
    m_model->setHeaderData( 2, Qt::Horizontal, QObject::tr( "World Position" ) );
    m_model->setHeaderData( 3, Qt::Horizontal, QObject::tr( "FRE" ) );
    m_model->setHeaderData( 4, Qt::Horizontal, QObject::tr( "Cumulative RMS" ) );
    m_model->setRowCount( 0 );

    ui->tagTreeView->setModel( m_model );
    ui->tagTreeView->setAllColumnsShowFocus( true );
    ui->tagTreeView->setItemsExpandable( false );
    ui->tagTreeView->setRootIsDecorated( false );
    ui->tagTreeView->setAlternatingRowColors( true );
    ui->tagTreeView->setSortingEnabled( false );
}

LandmarkRegistrationObjectWidget::~LandmarkRegistrationObjectWidget()
{
    if( m_registrationObject ) m_registrationObject->UnRegister( nullptr );
    delete m_model;
    delete ui;
}

void LandmarkRegistrationObjectWidget::SetLandmarkRegistrationObject( LandmarkRegistrationObject * obj )
{
    if( m_registrationObject == obj ) return;
    m_registrationObject = obj;
    m_registrationObject->Register( nullptr );
    this->UpdateUI();
}

void LandmarkRegistrationObjectWidget::on_tagTreeView_clicked( QModelIndex idx )
{
    Q_ASSERT( m_registrationObject );
    int currentIndex = idx.row();
    if( m_registrationObject->GetPointEnabledStatus( currentIndex ) == 1 )
        m_registrationObject->SelectPoint( currentIndex );
    ui->tagTreeView->setCurrentIndex( idx );
    this->UpdateUI();
}

void LandmarkRegistrationObjectWidget::on_tagSizeSpinBox_valueChanged( int tagSize )
{
    Q_ASSERT( m_registrationObject );
    m_registrationObject->SetTagSize( tagSize );
}

void LandmarkRegistrationObjectWidget::PointDataChanged( QStandardItem * item )
{
    if( item->column() == 0 )
    {
        m_registrationObject->SetPointLabel( item->row(), item->text().toUtf8().data() );
    }
}

void LandmarkRegistrationObjectWidget::UpdateUI()
{
    Q_ASSERT( m_registrationObject );
    vtkSmartPointer<LandmarkTransform> landmarkTransform = m_registrationObject->GetLandmarkTransform();
    double rms                                           = landmarkTransform->GetFinalRMS();

    disconnect( m_model, SIGNAL( itemChanged( QStandardItem * ) ), this, SLOT( PointDataChanged( QStandardItem * ) ) );
    int numberOfRows;
    while( ( numberOfRows = m_model->rowCount() ) > 0 ) m_model->takeRow( numberOfRows - 1 );
    QStringList names = m_registrationObject->GetPointNames();
    ui->rmsDisplayLabel->setText( QString::number( rms ) );
    QColor disabled( "gray" );
    QBrush brush( disabled );
    QString sourceCoords;
    QString targetCoords;
    vtkSmartPointer<PointsObject> source = m_registrationObject->GetSourcePoints();
    vtkSmartPointer<PointsObject> target = m_registrationObject->GetTargetPoints();
    double coords[3];
    int activePointsCount = 0;
    for( int idx = 0; idx < m_registrationObject->GetNumberOfPoints(); idx++ )
    {
        m_model->insertRow( idx );
        m_model->setData( m_model->index( idx, 0 ), names.at( idx ) );
        source->GetPointCoordinates( idx, coords );
        sourceCoords = QString( "%1,%2,%3" ).arg( coords[0] ).arg( coords[1] ).arg( coords[2] );
        m_model->setData( m_model->index( idx, 1 ), sourceCoords );
        target->GetPointCoordinates( idx, coords );
        targetCoords = QString( "%1,%2,%3" ).arg( coords[0] ).arg( coords[1] ).arg( coords[2] );
        m_model->setData( m_model->index( idx, 2 ), targetCoords );
        double fre = 0, rms = 0;
        bool ok  = landmarkTransform->GetFRE( activePointsCount, fre );
        bool ok1 = landmarkTransform->GetRMS( activePointsCount, rms );
        if( m_registrationObject->GetPointEnabledStatus( idx ) == 1 && ok && ok1 )
        {
            m_model->setData( m_model->index( idx, 3 ), QString::number( ( fre ) ) );
            m_model->setData( m_model->index( idx, 4 ), QString::number( ( rms ) ) );
            activePointsCount++;
        }
        else
        {
            m_model->setData( m_model->index( idx, 3 ), "n/a" );
            m_model->setData( m_model->index( idx, 4 ), "n/a" );
        }
        m_model->item( idx, 0 )->setEditable( true );
        m_model->item( idx, 1 )->setEditable( false );
        m_model->item( idx, 2 )->setEditable( false );
        m_model->item( idx, 3 )->setEditable( false );
        m_model->item( idx, 4 )->setEditable( false );
        for( int j = 0; j < 2; j++ ) ui->tagTreeView->resizeColumnToContents( j );
        if( m_registrationObject->GetPointEnabledStatus( idx ) != 1 )
        {
            for( int j = 0; j < m_model->columnCount(); j++ )
            {
                QStandardItem * item = m_model->item( idx, j );
                if( item ) item->setForeground( brush );
            }
        }
    }
    ui->tagSizeSpinBox->blockSignals( true );
    ui->tagSizeSpinBox->setValue( (int)( m_registrationObject->GetSourcePoints()->Get3DRadius() ) );
    ui->tagSizeSpinBox->blockSignals( false );
    connect( m_model, SIGNAL( itemChanged( QStandardItem * ) ), this, SLOT( PointDataChanged( QStandardItem * ) ) );
    if( m_registrationObject->GetNumberOfPoints() > 0 )
    {
        int selectedPointIndex = m_registrationObject->GetSourcePoints()->GetSelectedPointIndex();
        if( selectedPointIndex == PointsObject::InvalidPointIndex ||
            selectedPointIndex >= m_registrationObject->GetNumberOfPoints() )
            selectedPointIndex = 0;
        ui->tagTreeView->setCurrentIndex( m_model->index( selectedPointIndex, 0 ) );
    }
}
