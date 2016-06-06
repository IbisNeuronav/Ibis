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
#include "ui_pointpropertieswidget.h"
#include "application.h"
#include "scenemanager.h"
#include "pointerobject.h"
#include "view.h"
#include <QAction>
#include <QMenu>
#include <QPalette>
#include <QContextMenuEvent>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkRenderer.h>

PointPropertiesWidget::PointPropertiesWidget(QWidget *parent) :
    QWidget(parent)
    , m_points(0)
    , ui(new Ui::PointPropertiesWidget)
{
    ui->setupUi(this);
    ui->pointRadiusSpinBox->blockSignals(true);
    ui->pointRadiusSpinBox->setMinimum(1);
    ui->pointRadiusSpinBox->setMaximum(MAX_RADIUS);
    ui->pointRadiusSpinBox->blockSignals(false);
    ui->pointRadius2DSpinBox->blockSignals(true);
    ui->pointRadius2DSpinBox->setMinimum(1);
    ui->pointRadius2DSpinBox->setMaximum(100);
    ui->pointRadius2DSpinBox->blockSignals(false);

    m_current_point_index = -1;
    m_model = new QStandardItemModel(0, 2);
    m_model->setHeaderData(0, Qt::Horizontal, QObject::tr("Label"));
    m_model->setHeaderData(1, Qt::Horizontal, QObject::tr("Position"));

    ui->pointsTreeView->setModel(m_model);
    ui->pointsTreeView->setAllColumnsShowFocus(true);
    ui->pointsTreeView->setItemsExpandable(false);
    ui->pointsTreeView->setRootIsDecorated(false);
    ui->pointsTreeView->setAlternatingRowColors(true);
    ui->pointsTreeView->setSortingEnabled(false);
}

PointPropertiesWidget::~PointPropertiesWidget()
{
    if(m_points)
    {
        this->disconnect();
        m_points->UnRegister( 0 );
    }
    delete ui;
}

void PointPropertiesWidget::UpdateUI()
{
    if (m_points)
    {
        ui->pointRadiusSpinBox->blockSignals(true);
        ui->pointRadiusSpinBox->setValue((int)m_points->Get3DRadius());
        ui->pointRadiusSpinBox->blockSignals(false);
        ui->pointRadius2DSpinBox->blockSignals(true);
        ui->pointRadius2DSpinBox->setValue((int)m_points->Get2DRadius());
        ui->pointRadius2DSpinBox->blockSignals(false);
        ui->labelSizeSpinBox->blockSignals(true);
        ui->labelSizeSpinBox->setValue((int)m_points->GetLabelSize());
        ui->labelSizeSpinBox->blockSignals(false);
        ui->numberOfPointsLineEdit->setText(QString::number(m_points->GetNumberOfPoints()));

        m_current_point_index = m_points->GetSelectedPointIndex();

        double pt[3];
        QString pointLabel;
        int numberOfRows;
        while ((numberOfRows = m_model->rowCount()) > 0)
            m_model->takeRow(numberOfRows-1);
        int numPoints = m_points->GetNumberOfPoints();
        int i;
        for (i = 0; i < numPoints; i++)
        {
            pointLabel = m_points->GetPointLabel(i);
            m_points->GetPointCoordinates(i, pt);
            QString coords = QString("(%1, %2, %3)")
                            .arg(pt[0])
                            .arg(pt[1])
                            .arg(pt[2]);
            m_model->insertRow(i);
            m_model->setData(m_model->index(i, 0), pointLabel);
            m_model->setData(m_model->index(i, 1), coords);

            m_model->item(i, 0)->setEditable(true);
            m_model->item(i, 1)->setEditable(false);
            for (int j = 0; j < 3; j++)
                ui->pointsTreeView->resizeColumnToContents(j);
        }
        if (m_current_point_index >= numPoints)
            m_current_point_index = 0;
        if (numPoints == 0)
            m_current_point_index = -1;
        if (m_current_point_index >= 0)
        {
            ui->pointsTreeView->setCurrentIndex(m_model->index(m_current_point_index,0));
            m_points->SetSelectedPoint(m_current_point_index);
        }
        SceneManager *manager = m_points->GetManager();
        Q_ASSERT( manager );
        PointerObject * pObj = manager->GetNavigationPointerObject();
        ui->showDistanceCheckBox->blockSignals( true );
        if( pObj )
        {
            int numPoints = m_points->GetNumberOfPoints();
            if( numPoints > 0 )
            {
                ui->showDistanceCheckBox->setEnabled(true);
                ui->showDistanceCheckBox->setChecked( m_points->GetComputeDistanceEnabled() );
            }
            else
            {
                ui->showDistanceCheckBox->setEnabled(false);
                ui->showDistanceCheckBox->setChecked(false);
           }
        }
        else
        {
            ui->showDistanceCheckBox->setEnabled(false);
            ui->showDistanceCheckBox->setChecked(false);
        }
        ui->showDistanceCheckBox->blockSignals( false );
    }
}

void PointPropertiesWidget::UpdateSettings()
{
    this->UpdateUI();
}

void PointPropertiesWidget::SetPointsObject(PointsObject *obj)
{
    if(m_points == obj)
    {
        return;
    }

    if(m_points)
    {
        this->disconnect();
        m_points->UnRegister( 0 );
    }

    m_points = obj;

    if(m_points)
    {
        m_points->Register( 0 );
        Q_ASSERT( m_points->GetManager() );
        connect(m_points, SIGNAL(PointsChanged()), this, SLOT(UpdateSettings()));
        connect( m_points, SIGNAL(PointAdded()), this, SLOT(UpdateSettings()) );
        connect( m_points, SIGNAL(PointRemoved(int)), this, SLOT(UpdateSettings()) );
        connect( m_points->GetManager(), SIGNAL( CurrentObjectChanged() ), this, SLOT( ClosingSettingsWidget() ) );
    }
    this->UpdateUI();
}

void PointPropertiesWidget::on_showDistanceCheckBox_toggled( bool show )
{
    Q_ASSERT( m_points );
    SceneManager *manager = m_points->GetManager();
    vtkRenderer *renderer = manager->GetViewRenderer( THREED_VIEW_TYPE );
    m_points->EnableComputeDistance( show );
    manager->EmitShowGenericLabel( show );
    if( show )
    {
        m_points->ComputeDistanceFromSelectedPointToPointerTip( );
        manager->EmitShowGenericLabelText();
    }
}

void PointPropertiesWidget::on_labelSizeSpinBox_valueChanged(int size)
{
    Q_ASSERT( m_points );
    m_points->SetLabelSize((double)size);
    m_points->UpdatePoints();
}

void PointPropertiesWidget::on_pointRadiusSpinBox_valueChanged(int size)
{
    Q_ASSERT( m_points );
    m_points->Set3DRadius((double)size);
    m_points->UpdatePoints();
}

void PointPropertiesWidget::on_pointRadius2DSpinBox_valueChanged(int r)
{
    Q_ASSERT( m_points );
    m_points->Set2DRadius((double)r);
    m_points->UpdatePoints();
}

void PointPropertiesWidget::on_showLabelsCheckBox_toggled(bool show )
{
    Q_ASSERT( m_points );
    m_points->ShowLabels( show );
}

void PointPropertiesWidget::on_pointsTreeView_clicked(QModelIndex idx)
{
    Q_ASSERT( m_points );
    int num = idx.row();
    m_points->SetSelectedPoint(num);
    m_current_point_index = num;
    SceneManager *manager = m_points->GetManager();
    Q_ASSERT( manager );
    manager->EmitShowGenericLabel( m_points->GetComputeDistanceEnabled() );
    if( m_points->GetComputeDistanceEnabled() )
    {
        m_points->ComputeDistanceFromSelectedPointToPointerTip( );
        manager->EmitShowGenericLabelText( );
    }
}

void PointPropertiesWidget::DeletePoint()
{
    Q_ASSERT(m_points);
    QModelIndex currentIndex = ui->pointsTreeView->currentIndex();
    if ( currentIndex.isValid() )
    {
        int index = currentIndex.row();
        if( index >= 0 && index < m_model->rowCount() )
        {
            m_points->RemovePoint( index );
        }
        this->UpdateUI();
    }
}

void PointPropertiesWidget::contextMenuEvent( QContextMenuEvent * event )
{
    if ( !ui->pointsTreeView->hasFocus() )
        return;
    Q_ASSERT(m_points);
    QModelIndex currentIndex = ui->pointsTreeView->currentIndex();
    if (currentIndex.isValid())
    {
        int index = currentIndex.row();
        if( index >= 0 && index < m_model->rowCount())
        {
            QMenu contextMenu;
            contextMenu.addAction( tr("") );
            contextMenu.addAction( tr("Delete"), this, SLOT(DeletePoint()) );
            contextMenu.exec( event->globalPos() );
        }
    }
}


void PointPropertiesWidget::ClosingSettingsWidget()
{
    Q_ASSERT( m_points );
    Q_ASSERT( m_points->GetManager() );
    if( m_points->GetManager()->GetCurrentObject() !=  m_points )
        m_points->OnCloseSettingsWidget();
}
