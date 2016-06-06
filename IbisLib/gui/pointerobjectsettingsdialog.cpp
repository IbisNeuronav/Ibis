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

PointerObjectSettingsDialog::PointerObjectSettingsDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PointerObjectSettingsDialog)
{
    ui->setupUi(this);
    m_pointer = 0;
    m_pointerPickedPointsObject = 0;
}

PointerObjectSettingsDialog::~PointerObjectSettingsDialog()
{
    if (m_pointer)
        m_pointer->UnRegister(0);
}

void PointerObjectSettingsDialog::SetPointer(PointerObject *ptr)
{
    if (ptr == m_pointer)
        return;

    if (m_pointer)
        m_pointer->UnRegister(0);

    m_pointer = ptr;

    if (m_pointer)
    {
        m_pointer->Register(0);
        ui->navigationCheckBox->blockSignals(true);
        ui->navigationCheckBox->setChecked(m_pointer->GetManager()->GetNavigationState());
        ui->navigationCheckBox->blockSignals(false);
        this->UpdatePointSetsComboBox();
    }
}

void PointerObjectSettingsDialog::SetPointerPickedPointsObject(PointsObject *pts)
{
    if (pts == m_pointerPickedPointsObject)
        return;

    if (m_pointerPickedPointsObject)
        m_pointerPickedPointsObject->UnRegister(0);

    m_pointerPickedPointsObject = pts;

    if (m_pointerPickedPointsObject)
        m_pointerPickedPointsObject->Register(0);
}

void PointerObjectSettingsDialog::SaveTipPosition()
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
        if (delayAddObject)
            m_pointer->ManagerAddPointerPickedPointsObject();
    }
}

void PointerObjectSettingsDialog::EnableNavigation(bool yes)
{
    Q_ASSERT(m_pointer);
    m_pointer->GetManager()->EnablePointerNavigation( yes );
}

void PointerObjectSettingsDialog::NewPointsObject()
{
    Q_ASSERT(m_pointer);
    m_pointer->CreatePointerPickedPointsObject();
    this->SetPointerPickedPointsObject(m_pointer->GetCurrentPointerPickedPointsObject());
    m_pointer->ManagerAddPointerPickedPointsObject();
}

void PointerObjectSettingsDialog::UpdateSettings()
{
    Q_ASSERT(m_pointer);
    ui->navigationCheckBox->blockSignals(true);
    ui->navigationCheckBox->setChecked(m_pointer->GetManager()->GetNavigationState());
    ui->navigationCheckBox->blockSignals(false);
    this->UpdatePointSetsComboBox();
}

void PointerObjectSettingsDialog::PointSetsComboBoxActivated(int index)
{
    Q_ASSERT(m_pointer);
    QList <PointsObject*> PointerPickedPointsObjectList = m_pointer->GetPointerPickedPointsObjects();
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
    QList <PointsObject*> PointerPickedPointsObjectList = m_pointer->GetPointerPickedPointsObjects();
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
