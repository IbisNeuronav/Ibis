/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "pointpropertieswidget.h"

#include <QTreeWidget>

#include "pointerobject.h"
#include "pointsobject.h"
#include "scenemanager.h"
#include "ui_pointpropertieswidget.h"

PointPropertiesWidget::PointPropertiesWidget( QWidget * parent )
    : QWidget( parent ), m_points( 0 ), ui( new Ui::PointPropertiesWidget )
{
    ui->setupUi( this );
    ui->pointRadiusSpinBox->blockSignals( true );
    ui->pointRadiusSpinBox->setMinimum( 1 );
    ui->pointRadiusSpinBox->setMaximum( PointsObject::MaxRadius );
    ui->pointRadiusSpinBox->blockSignals( false );
    ui->pointRadius2DSpinBox->blockSignals( true );
    ui->pointRadius2DSpinBox->setMinimum( 1 );
    ui->pointRadius2DSpinBox->setMaximum( 100 );
    ui->pointRadius2DSpinBox->blockSignals( false );
}

PointPropertiesWidget::~PointPropertiesWidget()
{
    if( m_points )
    {
        this->disconnect();
    }
    delete ui;
}

void PointPropertiesWidget::UpdateUI()
{
    if( m_points )
    {
        ui->pointRadiusSpinBox->blockSignals( true );
        ui->pointRadiusSpinBox->setValue( (int)m_points->Get3DRadius() );
        ui->pointRadiusSpinBox->blockSignals( false );
        ui->pointRadius2DSpinBox->blockSignals( true );
        ui->pointRadius2DSpinBox->setValue( (int)m_points->Get2DRadius() );
        ui->pointRadius2DSpinBox->blockSignals( false );
        ui->labelSizeSpinBox->blockSignals( true );
        ui->labelSizeSpinBox->setValue( (int)m_points->GetLabelSize() );
        ui->labelSizeSpinBox->blockSignals( false );
        ui->numberOfPointsLineEdit->setText( QString::number( m_points->GetNumberOfPoints() ) );

        ui->pointsTreeWidget->clear();
        QList<QTreeWidgetItem *> items;
        for( int i = 0; i < m_points->GetNumberOfPoints(); ++i )
        {
            double pt[ 3 ];
            m_points->GetPointCoordinates( i, pt );
            QStringList cols;
            cols.append( m_points->GetPointLabel( i ) );
            cols.append( QString( "(%1, %2, %3)" ).arg( pt[ 0 ] ).arg( pt[ 1 ] ).arg( pt[ 2 ] ) );
            QTreeWidgetItem * it = new QTreeWidgetItem( (QTreeWidget *)0, cols );
            it->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
            items.append( it );
        }
        ui->pointsTreeWidget->insertTopLevelItems( 0, items );
        int selectedIndex = m_points->GetSelectedPointIndex();
        if( selectedIndex != PointsObject::InvalidPointIndex )
            ui->pointsTreeWidget->setCurrentItem( items[ selectedIndex ] );

        SceneManager * manager = m_points->GetManager();
        Q_ASSERT( manager );
        PointerObject * pObj = manager->GetNavigationPointerObject();
        ui->showDistanceCheckBox->blockSignals( true );
        if( pObj )
        {
            int numPoints = m_points->GetNumberOfPoints();
            if( numPoints > 0 )
            {
                ui->showDistanceCheckBox->setEnabled( true );
                ui->showDistanceCheckBox->setChecked( m_points->GetComputeDistanceEnabled() );
            }
            else
            {
                ui->showDistanceCheckBox->setEnabled( false );
                ui->showDistanceCheckBox->setChecked( false );
            }
        }
        else
        {
            ui->showDistanceCheckBox->setEnabled( false );
            ui->showDistanceCheckBox->setChecked( false );
        }
        ui->showDistanceCheckBox->blockSignals( false );
    }
}

void PointPropertiesWidget::SetPointsObject( PointsObject * obj )
{
    // simtodo : no need to register, this dialog should
    // be deleted automatically before m_points is deleted
    if( m_points == obj )
    {
        return;
    }

    if( m_points )
    {
        this->disconnect();
    }

    m_points = obj;

    if( m_points )
    {
        Q_ASSERT( m_points->GetManager() );

        // simtodo : can't we have a single signal? check in points object
        connect( m_points, SIGNAL( PointsChanged() ), this, SLOT( UpdateUI() ) );
        connect( m_points, SIGNAL( PointAdded() ), this, SLOT( UpdateUI() ) );
        connect( m_points, SIGNAL( PointRemoved( int ) ), this, SLOT( UpdateUI() ) );
    }
    this->UpdateUI();
}

void PointPropertiesWidget::on_showDistanceCheckBox_toggled( bool show )
{
    Q_ASSERT( m_points );
    SceneManager * manager = m_points->GetManager();
    vtkRenderer * renderer = manager->GetViewRenderer( manager->GetMain3DViewID() );
    m_points->EnableComputeDistance( show );
    manager->EmitShowGenericLabel( show );
    if( show )
    {
        m_points->ComputeDistanceFromSelectedPointToPointerTip();
        manager->EmitShowGenericLabelText();
    }
}

void PointPropertiesWidget::on_labelSizeSpinBox_valueChanged( int size )
{
    Q_ASSERT( m_points );
    m_points->SetLabelSize( (double)size );
}

void PointPropertiesWidget::on_pointRadiusSpinBox_valueChanged( int size )
{
    Q_ASSERT( m_points );
    m_points->Set3DRadius( (double)size );
}

void PointPropertiesWidget::on_pointRadius2DSpinBox_valueChanged( int r )
{
    Q_ASSERT( m_points );
    m_points->Set2DRadius( (double)r );
}

void PointPropertiesWidget::on_showLabelsCheckBox_toggled( bool show )
{
    Q_ASSERT( m_points );
    m_points->ShowLabels( show );
}

void PointPropertiesWidget::on_pointsTreeWidget_itemChanged( QTreeWidgetItem * item, int column )
{
    Q_ASSERT( m_points );
    int index     = ui->pointsTreeWidget->indexOfTopLevelItem( item );
    QString label = item->text( column );
    m_points->SetPointLabel( index, label );
}

void PointPropertiesWidget::on_deletePointButton_clicked()
{
    Q_ASSERT( m_points );
    QTreeWidgetItem * item = ui->pointsTreeWidget->currentItem();
    if( item )
    {
        int index = ui->pointsTreeWidget->indexOfTopLevelItem( item );
        m_points->RemovePoint( index );
    }
}

void PointPropertiesWidget::on_pointsTreeWidget_itemClicked( QTreeWidgetItem * item, int column )
{
    Q_ASSERT( m_points );

    int index = ui->pointsTreeWidget->indexOfTopLevelItem( item );
    m_points->SetSelectedPoint( index );
    m_points->MoveCursorToPoint( index );
    SceneManager * manager = m_points->GetManager();
    Q_ASSERT( manager );
    manager->EmitShowGenericLabel( m_points->GetComputeDistanceEnabled() );
    if( m_points->GetComputeDistanceEnabled() )
    {
        m_points->ComputeDistanceFromSelectedPointToPointerTip();
        manager->EmitShowGenericLabelText();
    }
}

void PointPropertiesWidget::on_pointsTreeWidget_itemDoubleClicked( QTreeWidgetItem * item, int column )
{
    int col = 0;  // can only edit the label column
    ui->pointsTreeWidget->editItem( item, col );
}
