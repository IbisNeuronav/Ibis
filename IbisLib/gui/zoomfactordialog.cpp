/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "zoomfactordialog.h"

#include <qcombobox.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qstring.h>
#include "scenemanager.h"

ZoomFactorDialog::ZoomFactorDialog( QWidget * parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
    setupUi(this);

    m_manager = 0;
    factorComboBox->addItem("5.0");
    factorComboBox->addItem("4.0");
    factorComboBox->addItem("3.0");
    factorComboBox->addItem("2.0");
    factorComboBox->addItem("1.9");
    factorComboBox->addItem("1.8");
    factorComboBox->addItem("1.7");
    factorComboBox->addItem("1.6");
    factorComboBox->addItem("1.5");
    factorComboBox->addItem("1.4");
    factorComboBox->addItem("1.3");
    factorComboBox->addItem("1.2");
    factorComboBox->addItem("1.1");
    factorComboBox->setCurrentIndex(8);
    inRadioButton->setChecked(true);
}

ZoomFactorDialog::~ZoomFactorDialog()
{
}

void ZoomFactorDialog::OkButtonClicked()
{
    double factor;
    factor = factorComboBox->currentText().toDouble();
    if (factor != 0.0)
    {
        if (outRadioButton->isChecked())
            factor = 1.0/factor;
        m_manager->ZoomAllCameras(factor);
    }
    accept();
}

void ZoomFactorDialog::SetSceneManager(SceneManager *man)
{
    m_manager = man;
}


