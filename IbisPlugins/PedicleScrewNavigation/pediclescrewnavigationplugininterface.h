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

#ifndef __PedicleScrewNavigationPluginInterface_h_
#define __PedicleScrewNavigationPluginInterface_h_

#include <QObject>
#include <QWidget>
#include "toolplugininterface.h"
#include "serializer.h"

class VertebraRegistrationWidget;
class vtkRenderer;

class PedicleScrewNavigationPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.PedicleScrewNavigationPluginInterface" )

public:\

    vtkTypeMacro( PedicleScrewNavigationPluginInterface, ToolPluginInterface );

    PedicleScrewNavigationPluginInterface();
    ~PedicleScrewNavigationPluginInterface();
    virtual QString GetPluginName() { return QString("PedicleScrewNavigation"); }
    bool CanRun();
    QString GetMenuEntryString() { return QString("Pedicle Screw Navigation"); }

    vtkRenderer * GetScrewNavigationAxialRenderer();
    vtkRenderer * GetScrewNavigationSagittalRenderer();

    VertebraRegistrationWidget * GetWidget() { return m_interfaceWidget; }

    QWidget * CreateTab();

private:

    VertebraRegistrationWidget * m_interfaceWidget;

};

#endif
