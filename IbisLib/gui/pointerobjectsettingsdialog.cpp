/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "pointerobjectsettingsdialog.h"
#include "ui_pointerobjectsettingsdialog.h"
#include "pointerobject.h"
#include "pointsobject.h"
#include "scenemanager.h"
#include "pointercalibrationdialog.h"
#include "vtkQtMatrixDialog.h"
#include "vtkTransform.h"
#include "scenemanager.h"

PointerObjectSettingsDialog::PointerObjectSettingsDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PointerObjectSettingsDialog)
{
    ui->setupUi(this);
    m_pointer = 0;
    m_pointerPickedPointsObject = 0;
    m_tipCalibrationWidget = 0;
    m_matrixDialog = 0;
}

PointerObjectSettingsDialog::~PointerObjectSettingsDialog()
{
    if( m_tipCalibrationWidget )
        m_tipCalibrationWidget->close();
    if( m_matrixDialog )
    {
        m_matrixDialog->close();
        m_matrixDialog = 0;
    }
}

void PointerObjectSettingsDialog::SetPointer(PointerObject *ptr)
{
    if (ptr == m_pointer)
        return;

    m_pointer = ptr;

    if (m_pointer)
    {
        this->UpdatePointSetsComboBox();
    }
}

void PointerObjectSettingsDialog::SetPointerPickedPointsObject(vtkSmartPointer<PointsObject> pts)
{
    if (pts == m_pointerPickedPointsObject)
        return;

    m_pointerPickedPointsObject = pts;
}

void PointerObjectSettingsDialog::on_savePositionPushButton_clicked()
{
    bool delayAddObject = false;
    if (!m_pointerPickedPointsObject)
    {
        delayAddObject = true;
        m_pointer->CreatePointerPickedPointsObject();
        this->SetPointerPickedPointsObject(m_pointer->GetCurrentPointerPickedPointsObject());
    }
    if (m_pointerPickedPointsObject)
    {
        double *pos = m_pointer->GetTipPosition();
        int index = m_pointerPickedPointsObject->GetNumberOfPoints();
        m_pointerPickedPointsObject->AddPoint(QString::number(index+1), pos);
        SceneManager *manager = m_pointer->GetManager();
        if (delayAddObject)
			m_pointer->ManagerAddPointerPickedPointsObject();
		if (manager->GetReferenceDataObject()) // we can only move cursor if cut planes are displayed
			m_pointerPickedPointsObject->MoveCursorToPoint(index);
		this->UpdateUI();
    }
}

void PointerObjectSettingsDialog::on_newPointsObjectPushButton_clicked()
{
    Q_ASSERT(m_pointer);
    m_pointer->CreatePointerPickedPointsObject();
    this->SetPointerPickedPointsObject(m_pointer->GetCurrentPointerPickedPointsObject());
    m_pointer->ManagerAddPointerPickedPointsObject();
    this->UpdatePointSetsComboBox();
}

void PointerObjectSettingsDialog::UpdateUI()
{
    Q_ASSERT(m_pointer);
    this->UpdatePointSetsComboBox();
}

void PointerObjectSettingsDialog::on_pointSetsComboBox_activated( int index )
{
    Q_ASSERT(m_pointer);
    QList <vtkSmartPointer<PointsObject> > PointerPickedPointsObjectList = m_pointer->GetPointerPickedPointsObjects();
    if (index >= 0 && index < PointerPickedPointsObjectList.count())
    {
        m_pointerPickedPointsObject = PointerPickedPointsObjectList.value(index);
        m_pointer->SetCurrentPointerPickedPointsObject(m_pointerPickedPointsObject);
    }
}

void PointerObjectSettingsDialog::UpdatePointSetsComboBox()
{
    int currentIndex = 0;
    ui->pointSetsComboBox->clear();
    QList <vtkSmartPointer<PointsObject> > PointerPickedPointsObjectList = m_pointer->GetPointerPickedPointsObjects();
    if (PointerPickedPointsObjectList.count() > 0)
    {
        for (int i = 0; i < PointerPickedPointsObjectList.count(); i++)
        {
            ui->pointSetsComboBox->addItem(PointerPickedPointsObjectList.value(i)->GetName());
            if (m_pointerPickedPointsObject == PointerPickedPointsObjectList.value(i))
                currentIndex = i;
        }
        ui->pointSetsComboBox->blockSignals(true);
        ui->pointSetsComboBox->setCurrentIndex(currentIndex);
        ui->pointSetsComboBox->blockSignals(false);
    }
}

void PointerObjectSettingsDialog::OnTipCalibrationDialogClosed()
{
    m_tipCalibrationWidget = 0;
    ui->calibrateTipButton->blockSignals( true );
    ui->calibrateTipButton->setChecked( false );
    ui->calibrateTipButton->blockSignals( false );
}

void PointerObjectSettingsDialog::on_calibrateTipButton_toggled( bool on )
{
    if( on )
    {
        Q_ASSERT( !m_tipCalibrationWidget );
        m_tipCalibrationWidget = new PointerCalibrationDialog;
        m_tipCalibrationWidget->setAttribute( Qt::WA_DeleteOnClose, true );
        connect( m_tipCalibrationWidget, SIGNAL( destroyed() ), SLOT( OnTipCalibrationDialogClosed() ) );
        m_tipCalibrationWidget->SetPointer( m_pointer );
        m_tipCalibrationWidget->show();
    }
    else
    {
        Q_ASSERT( m_tipCalibrationWidget );
        m_tipCalibrationWidget->close();
    }
}

void PointerObjectSettingsDialog::on_calibrationMatrixPushButton_toggled( bool on )
{
    if( on )
    {
        Q_ASSERT_X( m_pointer, "PointerObjectSettingsDialog::on_calibrationMatrixPushButton_toggled", "Can't call this function without setting PointerObject." );
        Q_ASSERT( !m_matrixDialog );

        QString dialogTitle = m_pointer->GetName();
        dialogTitle += ": Pointer Calibration Matrix";

        m_matrixDialog = new vtkQtMatrixDialog( false, 0 );
        m_matrixDialog->setWindowTitle( dialogTitle );
        m_matrixDialog->setAttribute( Qt::WA_DeleteOnClose );
        m_matrixDialog->SetMatrix( m_pointer->GetCalibrationTransform()->GetMatrix() );
        m_matrixDialog->show();
        connect( m_matrixDialog, SIGNAL(destroyed()), this, SLOT(OnCalibrationMatrixDialogClosed()) );
    }
    else
    {
        if( m_matrixDialog )
        {
            m_matrixDialog->close();
            m_matrixDialog = 0;
        }
    }
}

void PointerObjectSettingsDialog::OnCalibrationMatrixDialogClosed()
{
    m_matrixDialog = 0;
    ui->calibrationMatrixPushButton->setChecked( false );
}
