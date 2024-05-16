/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "landmarkregistrationobjectsettingswidget.h"

#include <vtkPoints.h>
#include <vtkSmartPointer.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QString>
#include <QStringList>

#include "application.h"
#include "landmarkregistrationobjectwidget.h"
#include "landmarktransform.h"
#include "pointerobject.h"
#include "scenemanager.h"
#include "ui_landmarkregistrationobjectsettingswidget.h"

LandmarkRegistrationObjectSettingsWidget::LandmarkRegistrationObjectSettingsWidget( QWidget * parent )
    : QWidget( parent ), ui( new Ui::LandmarkRegistrationObjectSettingsWidget )
{
    ui->setupUi( this );

    m_model = new QStandardItemModel( 0, 2 );
    m_model->setHeaderData( 0, Qt::Horizontal, QObject::tr( "Label" ) );
    m_model->setHeaderData( 1, Qt::Horizontal, QObject::tr( "FRE" ) );
    m_model->setRowCount( 0 );
    ui->pointsTreeView->setModel( m_model );
    ui->pointsTreeView->setAllColumnsShowFocus( true );
    ui->pointsTreeView->setItemsExpandable( false );
    ui->pointsTreeView->setRootIsDecorated( false );
    ui->pointsTreeView->setAlternatingRowColors( true );
    ui->pointsTreeView->setSortingEnabled( false );

    m_registrationObject   = nullptr;
    m_application          = nullptr;
    m_capture_button_color = Qt::lightGray;
}

LandmarkRegistrationObjectSettingsWidget::~LandmarkRegistrationObjectSettingsWidget()
{
    if( m_registrationObject ) m_registrationObject->UnRegister( nullptr );
    delete ui;
    delete m_model;
}

void LandmarkRegistrationObjectSettingsWidget::SetApplication( Application * app )
{
    Q_ASSERT( app );
    Q_ASSERT( !m_application );  // we make sure this is not called more than once

    m_application = app;

    connect( m_application, SIGNAL( IbisClockTick() ), this, SLOT( UpdateCaptureButton() ) );

    m_capture_button_color = ui->capturePushButton->palette().color( ui->capturePushButton->backgroundRole() );
    this->UpdateUI();
}

void LandmarkRegistrationObjectSettingsWidget::SetLandmarkRegistrationObject( LandmarkRegistrationObject * obj )
{
    if( m_registrationObject == obj ) return;
    m_registrationObject = obj;
    m_registrationObject->Register( nullptr );
    connect( m_registrationObject, SIGNAL( UpdateSettings() ), this, SLOT( UpdateUI() ) );
    this->UpdateUI();
}

void LandmarkRegistrationObjectSettingsWidget::on_registerPushButton_toggled( bool checked )
{
    Q_ASSERT( m_registrationObject );
    m_registrationObject->RegisterObject( checked );
    this->UpdateUI();
}

void LandmarkRegistrationObjectSettingsWidget::on_capturePushButton_clicked()
{
    Q_ASSERT( m_registrationObject );
    int index = ui->pointsTreeView->currentIndex().row();
    if( ! (index >= 0 && index < m_registrationObject->GetNumberOfPoints()) ){
        std::cerr << "No point selected, nothing was captured." << std::endl;
        return;
    }

    Q_ASSERT( m_registrationObject->GetManager() );
    PointerObject * pointer = m_registrationObject->GetManager()->GetNavigationPointerObject();
    if( pointer && pointer->IsOk() )
    {
        double * pos        = pointer->GetTipPosition();
        QDateTime timeStamp = QDateTime::currentDateTime();
        // move selection to next item
        if( index + 1 < m_registrationObject->GetNumberOfPoints() )
        {
            ui->pointsTreeView->setCurrentIndex( m_model->index( index + 1, 0 ) );
            m_registrationObject->SelectPoint( index + 1 );
        }
        m_registrationObject->SetTargetPointTimeStamp( index, timeStamp.toString( Qt::ISODate ) );
        m_registrationObject->SetTargetPointCoordinates( index, pos );
    }
}

void LandmarkRegistrationObjectSettingsWidget::on_importPushButton_clicked()
{
    if( this->ReadTagFile() ) this->UpdateUI();
}

void LandmarkRegistrationObjectSettingsWidget::on_detailsPushButton_clicked()
{
    LandmarkRegistrationObjectWidget * registrationWidget =
        new LandmarkRegistrationObjectWidget( 0, "Registration", Qt::WindowStaysOnTopHint );
    registrationWidget->setAttribute( Qt::WA_DeleteOnClose, true );
    registrationWidget->setWindowTitle( "Register Subject" );
    registrationWidget->SetLandmarkRegistrationObject( m_registrationObject );
    registrationWidget->show();
}

void LandmarkRegistrationObjectSettingsWidget::on_pointsTreeView_clicked( QModelIndex idx )
{
    Q_ASSERT( m_registrationObject );
    int currentIndex = idx.row();
    if( m_registrationObject->GetPointEnabledStatus( currentIndex ) == 1 )
        m_registrationObject->SelectPoint( currentIndex );
    ui->pointsTreeView->setCurrentIndex( idx );
}

void LandmarkRegistrationObjectSettingsWidget::on_targetComboBox_currentIndexChanged( int index )
{
    Q_ASSERT( m_registrationObject );
    int targetID = ui->targetComboBox->itemData( index ).toInt();
    m_registrationObject->SetTargetObjectID( targetID );
}

void LandmarkRegistrationObjectSettingsWidget::UpdateCaptureButton()
{
    Q_ASSERT( m_registrationObject );

    // Set button color according to navigation pointer state and availability of points to capture
    Q_ASSERT( m_registrationObject->GetManager() );
    PointerObject * pointer = m_registrationObject->GetManager()->GetNavigationPointerObject();
    if( pointer && pointer->IsOk() )
    {
        SetCaptureButtonBackgroundColor( m_capture_button_color );
        int numPoints = m_registrationObject->GetNumberOfPoints();
        if( numPoints > 0 )
            ui->capturePushButton->setEnabled( true );
        else
            ui->capturePushButton->setEnabled( false );
    }
    else
    {
        SetCaptureButtonBackgroundColor( Qt::red );
        ui->capturePushButton->setEnabled( false );
    }
}

void LandmarkRegistrationObjectSettingsWidget::SetCaptureButtonBackgroundColor( QColor color )
{
    QString styleColor =
        QString( "background-color: rgb(%1,%2,%3);" ).arg( color.red() ).arg( color.green() ).arg( color.blue() );
    ui->capturePushButton->setStyleSheet( styleColor );
}

void LandmarkRegistrationObjectSettingsWidget::contextMenuEvent( QContextMenuEvent * event )
{
    if( !ui->pointsTreeView->hasFocus() ) return;
    Q_ASSERT( m_registrationObject );
    QModelIndex currentIndex = ui->pointsTreeView->currentIndex();
    if( currentIndex.isValid() )
    {
        int index = currentIndex.row();
        if( index >= 0 && index < m_model->rowCount() )
        {
            QMenu contextMenu;
            contextMenu.addAction( tr( "Delete" ), this, SLOT( DeletePoint() ) );
            if( m_registrationObject->GetPointEnabledStatus( index ) == 1 )
                contextMenu.addAction( tr( "Disable" ), this, SLOT( EnableDisablePoint() ) );
            else
                contextMenu.addAction( tr( "Enable" ), this, SLOT( EnableDisablePoint() ) );
            contextMenu.exec( event->globalPos() );
        }
    }
}

void LandmarkRegistrationObjectSettingsWidget::EnableDisablePoint()
{
    Q_ASSERT( m_registrationObject );
    QModelIndex currentIndex = ui->pointsTreeView->currentIndex();
    if( currentIndex.isValid() )
    {
        int index = currentIndex.row();
        if( index >= 0 && index < m_model->rowCount() )
        {
            int stat = m_registrationObject->GetPointEnabledStatus( index );
            if( stat == 1 )
            {
                m_registrationObject->SetPointEnabledStatus( index, 0 );
            }
            else
            {
                m_registrationObject->SetPointEnabledStatus( index, 1 );
            }
        }
        this->UpdateUI();
    }
}

void LandmarkRegistrationObjectSettingsWidget::DeletePoint()
{
    Q_ASSERT( m_registrationObject );
    QModelIndex currentIndex = ui->pointsTreeView->currentIndex();
    if( currentIndex.isValid() )
    {
        int index = currentIndex.row();
        if( index >= 0 && index < m_model->rowCount() )
        {
            m_registrationObject->DeletePoint( index );
        }
        this->UpdateUI();
    }
}

void LandmarkRegistrationObjectSettingsWidget::UpdateUI()
{
    Q_ASSERT( m_registrationObject );

    ui->targetComboBox->blockSignals( true );
    ui->targetComboBox->clear();
    QList<SceneObject *> allObjects;
    SceneManager * manager = m_registrationObject->GetManager();
    manager->GetAllListableNonTrackedObjects( allObjects );
    int index = 0;
    for( int i = 0; i < allObjects.size(); i++ )
    {
        if( allObjects[i] != m_registrationObject && allObjects[i]->CanAppendChildren() && allObjects[i]->IsListable() )
        {
            ui->targetComboBox->addItem( allObjects[i]->GetName(), QVariant( allObjects[i]->GetObjectID() ) );
            if( allObjects[i] == manager->GetObjectByID( m_registrationObject->GetTargetObjectID() ) )
                ui->targetComboBox->setCurrentIndex( index );
            index++;
        }
    }
    ui->targetComboBox->blockSignals( false );
    vtkSmartPointer<LandmarkTransform> landmarkTransform = m_registrationObject->GetLandmarkTransform();
    double rms                                           = landmarkTransform->GetFinalRMS();

    // Update register button
    bool isRegistered = m_registrationObject->IsRegistered();
    ui->registerPushButton->blockSignals( true );
    if( isRegistered )
    {
        ui->registerPushButton->setChecked( true );
        ui->registerPushButton->setText( "Register\n\nON" );
    }
    else
    {
        ui->registerPushButton->setChecked( false );
        ui->registerPushButton->setText( "Register\n\nOFF" );
    }
    ui->registerPushButton->blockSignals( false );

    if( m_registrationObject->GetNumberOfActivePoints() > 2 &&
        rms > 0 )  // we never expect perfect match (rms == 0), should we?
        ui->registerPushButton->setEnabled( true );
    else
        ui->registerPushButton->setEnabled( false );
    int numberOfRows;
    disconnect( m_model, SIGNAL( itemChanged( QStandardItem * ) ), this, SLOT( PointDataChanged( QStandardItem * ) ) );
    while( ( numberOfRows = m_model->rowCount() ) > 0 ) m_model->takeRow( numberOfRows - 1 );
    ui->rmsNumberLabel->setText( QString::number( rms ) );
    QColor disabled( "gray" );
    QBrush brush( disabled );
    int activePointsCount = 0;
    for( int idx = 0; idx < m_registrationObject->GetNumberOfPoints(); idx++ )
    {
        m_model->insertRow( idx );
        m_model->setData( m_model->index( idx, 0 ), m_registrationObject->GetPointNames().at( idx ) );
        double fre = 0;
        bool ok    = landmarkTransform->GetFRE( activePointsCount, fre );
        if( m_registrationObject->GetPointEnabledStatus( idx ) == 1 && ok )
        {
            m_model->setData( m_model->index( idx, 1 ), QString::number( ( fre ) ) );
            activePointsCount++;
        }
        else
            m_model->setData( m_model->index( idx, 1 ), "n/a" );
        m_model->item( idx, 0 )->setEditable( true );
        m_model->item( idx, 1 )->setEditable( false );
        for( int j = 0; j < 2; j++ ) ui->pointsTreeView->resizeColumnToContents( j );
        if( m_registrationObject->GetPointEnabledStatus( idx ) != 1 )
        {
            for( int j = 0; j < m_model->columnCount(); j++ )
            {
                QStandardItem * item = m_model->item( idx, j );
                if( item ) item->setForeground( brush );
            }
        }
    }
    connect( m_model, SIGNAL( itemChanged( QStandardItem * ) ), this, SLOT( PointDataChanged( QStandardItem * ) ) );
    if( m_registrationObject->GetNumberOfPoints() > 0 )
    {
        int selectedPointIndex = m_registrationObject->GetSourcePoints()->GetSelectedPointIndex();
        if( selectedPointIndex != PointsObject::InvalidPointIndex &&
            selectedPointIndex < m_registrationObject->GetNumberOfPoints() )
            ui->pointsTreeView->setCurrentIndex( m_model->index( selectedPointIndex, 0 ) );
    }
}

void LandmarkRegistrationObjectSettingsWidget::PointDataChanged( QStandardItem * item )
{
    if( item->column() == 0 )
    {
        m_registrationObject->SetPointLabel( item->row(), item->text().toUtf8().data() );
    }
}

bool LandmarkRegistrationObjectSettingsWidget::ReadTagFile()
{
    Q_ASSERT( m_registrationObject );
    return m_registrationObject->ReadTagFile();
}
