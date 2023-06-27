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

#include "serializer.h"
#include "toolplugininterface.h"

class VertebraRegistrationWidget;
class vtkRenderer;

class PedicleScrewNavigationPluginInterface : public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( IbisPlugin )
    Q_PLUGIN_METADATA( IID "Ibis.PedicleScrewNavigationPluginInterface" )

public:
    vtkTypeMacro( PedicleScrewNavigationPluginInterface, ToolPluginInterface );

    PedicleScrewNavigationPluginInterface();
    ~PedicleScrewNavigationPluginInterface();
    virtual QString GetPluginName() override { return QString( "PedicleScrewNavigation" ); }
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString( "Pedicle Screw Navigation" ); }
    QString GetPluginDescription() override
    {
        QString description;
        description =
            "Pedicle Screw Navigation v. 1.1\n"
            "Author: Houssem Gueziri, PhD.\n"
            "\n"
            "This plugin provides functionalities for navigated spinal instrumentation using intraoperative ultrasound "
            "imaging. It includes:\n"
            "   - Fast rigid registration of preoperative CT images to intraoperative US scans\n"
            "   - Standard pedicle screw navigation using reformatted Axial / Sagittal views\n"
            "   - Screw planning (create screws, load and save screw positions)\n"
            "\n"
            "Reference:\n"
            "Gueziri et al. (2020). Open-source software for ultrasound-based guidance in spinal fusion surgery. "
            "Ultrasound Med Biol, 46(12), pp.3353-68.";
        return description;
    }

    vtkRenderer * GetScrewNavigationAxialRenderer();
    vtkRenderer * GetScrewNavigationSagittalRenderer();

    VertebraRegistrationWidget * GetWidget() { return m_interfaceWidget; }

    QWidget * CreateTab() override;

private:
    VertebraRegistrationWidget * m_interfaceWidget;
};

#endif
