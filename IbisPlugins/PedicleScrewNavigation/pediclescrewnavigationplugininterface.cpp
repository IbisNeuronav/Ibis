/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Author: Houssem-Eddine Gueziri

#include "pediclescrewnavigationplugininterface.h"

#include <QtPlugin>

#include "application.h"
#include "vertebraregistrationwidget.h"

PedicleScrewNavigationPluginInterface::PedicleScrewNavigationPluginInterface() : m_interfaceWidget( nullptr ) {}

PedicleScrewNavigationPluginInterface::~PedicleScrewNavigationPluginInterface() {}

bool PedicleScrewNavigationPluginInterface::CanRun() { return true; }

QWidget * PedicleScrewNavigationPluginInterface::CreateTab()
{
    // Start the window
    m_interfaceWidget = new VertebraRegistrationWidget( 0 );
    m_interfaceWidget->SetPluginInterface( this );
    m_interfaceWidget->setAttribute( Qt::WA_DeleteOnClose, true );

    return m_interfaceWidget;
}

vtkRenderer * PedicleScrewNavigationPluginInterface::GetScrewNavigationAxialRenderer()
{
    if( !m_interfaceWidget ) return nullptr;
    return m_interfaceWidget->GetScrewNavigationAxialRenderer();
}

vtkRenderer * PedicleScrewNavigationPluginInterface::GetScrewNavigationSagittalRenderer()
{
    if( !m_interfaceWidget ) return nullptr;
    return m_interfaceWidget->GetScrewNavigationSagittalRenderer();
}
