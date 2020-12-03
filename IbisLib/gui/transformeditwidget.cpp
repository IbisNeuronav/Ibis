/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "transformeditwidget.h"
#include "ui_transformeditwidget.h"
#include <vtkTransform.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkQtMatrixDialog.h>
#include "vtkMatrix4x4Operators.h"
#include "sceneobject.h"
#include "application.h"

TransformEditWidget::TransformEditWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransformEditWidget),
    m_sceneObject(nullptr),
    m_selfUpdating(false),
    m_matrixDialog(nullptr),
    m_worldMatrixDialog(nullptr)
{
    ui->setupUi(this);
    m_transformModifiedConnection = vtkEventQtSlotConnect::New();
}

TransformEditWidget::~TransformEditWidget()
{
    if( m_matrixDialog )
    {
        m_matrixDialog->close();
        m_matrixDialog = nullptr;
    }
    if( m_worldMatrixDialog )
    {
        m_worldMatrixDialog->close();
        m_worldMatrixDialog = nullptr;
    }

    delete ui;
    m_transformModifiedConnection->Delete();
}

void TransformEditWidget::SetSceneObject( SceneObject * obj )
{
    if( m_sceneObject == obj )
        return;
    if( m_sceneObject )
    {
        m_transformModifiedConnection->Disconnect( m_sceneObject->GetLocalTransform() );
    }
    m_sceneObject = obj;
    if( m_sceneObject )
    {
        m_transformModifiedConnection->Connect( m_sceneObject->GetLocalTransform(), vtkCommand::ModifiedEvent, this, SLOT(UpdateUi()) );
    }

    UpdateUi();
}

// take data in ui and put it in transform
void TransformEditWidget::UpdateTransform()
{
    if( m_sceneObject )
    {
        m_selfUpdating = true;
        //m_sceneObject->StartModifyingTransform();

        vtkMatrix4x4 * mat = m_sceneObject->GetLocalTransform()->GetMatrix();
        double rot[3];
        rot[0] = ui->rotateXSpinBox->value();
        rot[1] = ui->rotateYSpinBox->value();
        rot[2] = ui->rotateZSpinBox->value();
        double trans[3];
        trans[0] = ui->translateXSpinBox->value();
        trans[1] = ui->translateYSpinBox->value();
        trans[2] = ui->translateZSpinBox->value();
        vtkMatrix4x4Operators::TransRotToMatrix( trans, rot, mat );
        m_sceneObject->GetLocalTransform()->Modified();

        //m_sceneObject->FinishModifyingTransform();
        m_selfUpdating = false;
    }
}

// take data in transform and put it in ui
void TransformEditWidget::UpdateUi()
{
    if( m_sceneObject && !m_selfUpdating )
    {
        BlockSpinboxSignals(true);
        EnableSpinBoxes(false);

        vtkTransform * localTransform = m_sceneObject->GetLocalTransform();
        if( !localTransform )
        {
            EnableSpinBoxes( false );
            ui->translateXSpinBox->setValue( 0.0 );
            ui->translateYSpinBox->setValue( 0.0 );
            ui->translateZSpinBox->setValue( 0.0 );
            ui->rotateXSpinBox->setValue( 0.0 );
            ui->rotateYSpinBox->setValue( 0.0 );
            ui->rotateZSpinBox->setValue( 0.0 );
            return;
        }
        else
        {
            if( m_sceneObject->CanEditTransformManually() )
                EnableSpinBoxes( true );

            double t[3] = { 0.0, 0.0, 0.0 };
            double r[3] = { 0.0, 0.0, 0.0 };
            vtkMatrix4x4Operators::MatrixToTransRot( localTransform->GetMatrix(), t, r );
            ui->translateXSpinBox->setValue( t[0] );
            ui->translateYSpinBox->setValue( t[1] );
            ui->translateZSpinBox->setValue( t[2] );
            ui->rotateXSpinBox->setValue( r[0] );
            ui->rotateYSpinBox->setValue( r[1] );
            ui->rotateZSpinBox->setValue( r[2] );
        }
        BlockSpinboxSignals(false);
    }
    else if( !m_selfUpdating )
    {
        EnableSpinBoxes(false);
    }
}

void TransformEditWidget::EditMatrixButtonToggled( bool isOn )
{
    if( !isOn )
    {
        if( m_matrixDialog )
        {
            m_matrixDialog->close();
            m_matrixDialog = nullptr;
        }
    }
    else
    {
        Q_ASSERT_X( m_sceneObject, "TransformEditWidget::EditMatrixButtonToggled", "Can't call this function without setting SceneObject." );

        vtkTransform * t = m_sceneObject->GetLocalTransform();
        if( t )
        {
            bool readOnly = !m_sceneObject->CanEditTransformManually();
            QString dialogTitle = m_sceneObject->GetName();
            dialogTitle += ": Local Matrix";
            m_matrixDialog = new vtkQtMatrixDialog( readOnly, 0 );
            m_matrixDialog->setWindowTitle( dialogTitle );
            m_matrixDialog->setAttribute( Qt::WA_DeleteOnClose );
            m_matrixDialog->SetMatrix( t->GetMatrix() );
            Application::GetInstance().ShowFloatingDock( m_matrixDialog );
            connect( m_matrixDialog, SIGNAL(MatrixModified()), m_sceneObject, SLOT(NotifyTransformChanged()) );
            connect( m_matrixDialog, SIGNAL(MatrixModified()), this, SLOT(UpdateUi()) );
            connect( m_matrixDialog, SIGNAL(destroyed()), this, SLOT(EditMatrixDialogClosed()) );
        }
    }
}

void TransformEditWidget::EditMatrixDialogClosed()
{
    m_matrixDialog = nullptr;
    ui->EditMatrixPushButton->setChecked( false );
}

void TransformEditWidget::WorldMatrixButtonToggled( bool isOn )
{
    if( !isOn )
    {
        if( m_worldMatrixDialog )
        {
            m_worldMatrixDialog->close();
            m_worldMatrixDialog = nullptr;
        }
    }
    else
    {
        Q_ASSERT_X( m_sceneObject, "TransformEditWidget::WorldMatrixButtonToggled", "Can't call this function without setting SceneObject." );

        QString dialogTitle = m_sceneObject->GetName();
        dialogTitle += ": World Matrix";

        m_worldMatrixDialog = new vtkQtMatrixDialog( true, 0 );
        m_worldMatrixDialog->setWindowTitle( dialogTitle );
        m_worldMatrixDialog->setAttribute( Qt::WA_DeleteOnClose );
        m_worldMatrixDialog->SetMatrix( m_sceneObject->GetWorldTransform()->GetMatrix() );
        Application::GetInstance().ShowFloatingDock( m_worldMatrixDialog );
        connect( m_worldMatrixDialog, SIGNAL(destroyed()), this, SLOT(WorldMatrixDialogClosed()) );
    }
}

void TransformEditWidget::WorldMatrixDialogClosed()
{
    m_worldMatrixDialog = nullptr;
    ui->WorldMatrixPushButton->setChecked( false );
}

void TransformEditWidget::SetIdentityButtonClicked()
{
    if( m_sceneObject )
    {
        vtkTransform * transform = vtkTransform::SafeDownCast (m_sceneObject->GetLocalTransform() );
        if( transform )
        {
            transform->Identity();
            this->UpdateUi();
        }
    }
}

void TransformEditWidget::EnableSpinBoxes( bool on )
{
    ui->translateXSpinBox->setEnabled( on );
    ui->translateYSpinBox->setEnabled( on );
    ui->translateZSpinBox->setEnabled( on );
    ui->rotateXSpinBox->setEnabled( on );
    ui->rotateYSpinBox->setEnabled( on );
    ui->rotateZSpinBox->setEnabled( on );
    ui->IdentityPushButton->setEnabled( on );
}

void TransformEditWidget::BlockSpinboxSignals( bool block )
{
    ui->translateXSpinBox->blockSignals( block );
    ui->translateYSpinBox->blockSignals( block );
    ui->translateZSpinBox->blockSignals( block );
    ui->rotateXSpinBox->blockSignals( block );
    ui->rotateYSpinBox->blockSignals( block );
    ui->rotateZSpinBox->blockSignals( block );
}
