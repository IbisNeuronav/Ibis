/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __USManualCalibrationPluginInterface_h_
#define __USManualCalibrationPluginInterface_h_

#include "toolplugininterface.h"

class USManualCalibrationWidget;
class UsProbeObject;
class SceneObject;

class USManualCalibrationPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.USManualCalibrationPluginInterface" )

public:

    USManualCalibrationPluginInterface();
    ~USManualCalibrationPluginInterface();
    virtual QString GetPluginName() override { return QString("USManualCalibration"); }
    bool CanRun() override;
    QString GetMenuEntryString() override { return QString("US Manual Calibration"); }

    virtual QWidget * CreateFloatingWidget() override;
    virtual bool WidgetAboutToClose() override;
    virtual void LoadSettings( QSettings & s ) override;
    virtual void SaveSettings( QSettings & s ) override;

    UsProbeObject * GetCurrentUsProbe();
    void StartPhantomRegistration();

    const double * GetPhantomPoint( int nIndex, int pointIndex );
    SceneObject * GetCalibrationPhantomObject();

protected:

    void ValidateUsProbe();
    void BuildCalibrationPhantomRepresentation();

    int m_calibrationPhantomObjectId;
    int m_phantomRegSourcePointsId;
    int m_phantomRegTargetPointsId;
    int m_landmarkRegistrationObjectId;
    int m_usProbeObjectId;
};

#endif
