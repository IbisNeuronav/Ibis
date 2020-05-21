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
#include "vtkTransform.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkQtMatrixDialog.h"
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

        vtkMatrix4x4 * mat = vtkMatrix4x4::New();
        double rot[3];
        rot[0] = ui->rotateXSpinBox->value();
        rot[1] = ui->rotateYSpinBox->value();
        rot[2] = ui->rotateZSpinBox->value();
        double trans[3];
        trans[0] = ui->translateXSpinBox->value();
        trans[1] = ui->translateYSpinBox->value();
        trans[2] = ui->translateZSpinBox->value();
        vtkMatrix4x4Operators::TransRotToMatrix( trans, rot, mat );
        m_sceneObject->GetLocalTransform()->SetMatrix( mat );
        m_sceneObject->GetLocalTransform()->Modified();

        //m_sceneObject->FinishModifyingTransform();
        m_selfUpdating = false;
        mat->Delete();
    }
}

// Currently in ibis only some SceneObjects can show TransformEditWidget
// The only transform  modifiable from GUI is LocalTransform
void TransformEditWidget::SetUpdatedMatrix( vtkMatrix4x4 * mat )
{
    if( m_sceneObject )
    {
        m_sceneObject->GetLocalTransform()->SetMatrix( mat );
    }
    UpdateUi();
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
            // m_matrixDialog was not clsed, we have to update it setting actual matrix
//            if( m_matrixDialog )
//            {
//                vtkMatrix4x4 *tmpMat = vtkMatrix4x4::New();
//                tmpMat->DeepCopy( localTransform->GetMatrix() );
//                m_matrixDialog->SetMatrix( tmpMat );
//                tmpMat->Delete();
//            }
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
            vtkMatrix4x4 *tmpMat = vtkMatrix4x4::New();
            tmpMat->DeepCopy( t->GetMatrix() );
            m_matrixDialog->SetMatrix( tmpMat );
            //We cannot use the LocalTransform matrix as it may be a product of concatenation
            //vtkQTMatrixDialog is setting matrix elements, which will not affect the concatenation
            //In order to use the modified matrix, we have to get rid of concatenation, which is done in the call to SetMatrix()
            //by callingTransformEditWidget::SetUpdatedMatrix()
            //Once we modify the matrix, we will set it in the LocalTransform
            tmpMat->Delete();
            Application::GetInstance().ShowFloatingDock( m_matrixDialog );
            connect( m_matrixDialog, SIGNAL(MatrixModified(vtkMatrix4x4 *)), m_sceneObject, SLOT(NotifyTransformChanged()) );
            connect( m_matrixDialog, SIGNAL(MatrixModified(vtkMatrix4x4 *)), this, SLOT(SetUpdatedMatrix(vtkMatrix4x4 *)) );
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
            transform->Modified(); // this patch is added to correct vtk bug in vtkTransform::Identity() and will be removed once vtk is fixed.
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
