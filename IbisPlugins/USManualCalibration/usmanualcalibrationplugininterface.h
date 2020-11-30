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

#define CALIBRATIONPHANTOMFILE @CALIBRATIONPHANTOMFILE@

class USManualCalibrationWidget;
class UsProbeObject;
class SceneObject;

class USManualCalibrationPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.USManualCalibrationPluginInterface" )

public:

    enum PhantomSize { MEDIUMDEPTH = 0, SHALLOWDEPTH = 1 };

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
    SceneObject * GetPhantomWiresObject();

    void SetPhatonSize(int);

protected:

    void ValidateUsProbe();
    void BuildWiresRepresentation();
    void BuildCalibrationPhantomRepresentation();
    void UpdateWiresRepresentation();

    int m_phantomWiresObjectId;
    int m_calibrationPhantomObjectId;
    int m_phantomRegSourcePointsId;
    int m_phantomRegTargetPointsId;
    int m_landmarkRegistrationObjectId;
    int m_usProbeObjectId;

    PhantomSize m_currentPhantomSize;
};

#endif
