/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __Tracker_h
#define __Tracker_h

#include <vector>
#include "serializer.h"
#include "vtkObject.h"
#include <qobject.h>
#include <qstring.h>
#include "trackerflags.h"
#include "hardwaremodule.h"

class vtkPOLARISTracker;
class vtkTrackerTool;
class QWidget;
class vtkMatrix4x4;
class SceneManager;

#define BR9600    9600
#define BR14400  14400
#define BR19200  19200
#define BR38400  38400
#define BR57600  57600
#define BR115200 115200

#define DEFAULT_BAUD_RATE BR57600

struct ToolDescription
{
    ToolDescription();
    ToolDescription( const ToolDescription & orig );
    virtual void Serialize( Serializer * serializer );
    ToolType type;           // Passive = 1, Active = 0
    ToolUse use;             // What is this tool used for.
    int active;              // wether the tool has been activated
    int toolPort;            // if active == 1, the tool port, otherwise, -1
    int toolObjectId;        // id given by SceneManager to the object representing the tool in order to avoid access by name
    QString name;            // tool name. just for the ui.
    QString romFileName;
    vtkMatrix4x4 * cachedCalibrationMatrix;
    double cachedCalibrationRMS;
    QString cachedSerialNumber;   // serial number used to recognize active tools when plugged in
};

ObjectSerializationHeaderMacro( ToolDescription );


//====================================================================
// .NAME Tracker
// .SECTION Description
// This class manages the communication between a Qt application 
// and a vtkTracker. It can change the settings of the tracker but
// also display dialogs relevant to it. There is also some serializing
// method available to remember the settings.
//
// The class manages a list of tools (usually all tools that can
// potentially be used with the program built). Tools can be activated,
// (tell the tracking device they are there and should be tracked). When
// a tool is activated, Tracker uses the information in the tool list
// to tell the vtkTrackerTool which settings should be applied to it.
// When it is deactivated, settings are saved back in the list. 
// Active tools are automatically activated when plugged in. If this
// tool has never been activated before, a default name will be given
// to it. Tools in the list are accessed by index.
//====================================================================
class Tracker : public QObject, public vtkObject
{
    
Q_OBJECT
    
public:
    
    static Tracker * New() { return new Tracker; }
    
    Tracker();
    ~Tracker();
        
    // Description:
    // Initialize the tracking system. If IsInitialized() == 1, system is already tracking
    void Initialize();
    int IsInitialized();
    
    void StopTracking();
    
    void LockUpdate();
    void UnlockUpdate();

    void Update();
    double GetUpdateRate();

    QString GetTrackerVersion();
    vtkTrackerTool * GetTool( int toolIndex );
    vtkTrackerTool * GetTool( QString & toolName );
    
    // Description:
    // There may be only one in the current version - 2007-06-04
    int GetUSProbeToolIndex( );

    int GetReferenceToolIndex();
    int SetReferenceToolIndex( int toolIndex );
    int GetNavigationPointerIndex();
    int SetNavigationPointerIndex( int toolIndex );
    int GetCurrentToolIndex();
    void SetCurrentToolIndex( int toolIndex );
    
    int GetNumberOfTools();
    int GetNumberOfActiveTools();
    
    QString GetToolName( int toolIndex );
    void SetToolName( int toolIndex, QString toolName );

    int GetToolObjectId(int toolIndex);
    void SetToolObjectId(int toolIndex, int id);

    int GetToolIndex( QString toolName );
    
    void GetActiveToolTypeAndRom(ToolUse use, QString &type, QString &romFile);
    
    // Description:
    // To calibrate the tip of a pointer, Call StartToolTipCalibration(toolIndex).
    // Every update will calibrate the tool. You can get the calibration vector
    // by getting the last row of the calibration matrix and you can get the 
    // rms on the calibration by calling GetToolLastCalibrationRMS(tool). When
    // calibration is ok, call StopToopTipCalibration(tool).
    void StartToolTipCalibration( int toolIndex );
    double GetToolLastCalibrationRMS( int toolIndex );
    void SetToolLastCalibrationRMS( int toolIndex, double rms );
    void StopToolTipCalibration( int toolIndex );
    int IsToolCalibrating( int toolIndex );
    
    // Description:
    // Return the type of tool at index toolIndex. 0 -> passive, 1 -> active
    ToolType GetToolType( int toolIndex );

    // Description:
    // Get/Set the use of tool at index toolIndex.
    void SetToolUse( int toolIndex, ToolUse use );
    ToolUse GetToolUse( int toolIndex );
        
    // Description:
    // Get the filename (and path) of the rom file to be used for this tool. (this is specific to POLARIS for now )
    QString GetRomFileName( int toolIndex );
    void SetRomFileName( int toolIndex, QString & name );
    
    // Description:
    // The string return contains the text-formated properties
    // of the tool at index toolIndex or an empty string if the 
    // tool doesn't exist.
    QString GetToolDescription( int toolIndex );
    
    // Description:
    // Use this function to make sure a toolname is
    // not already used before adding or renaming a tool, 
    // otherwise, the tool won't be added/renamed.
    int ToolNameExists( QString ToolName );
    
    // Description:
    // Add a new tool with name "name". type is the type
    // of tool to add: 0 for passive, 1 for active. Active tool's
    // properties should be identified automatically by the system.
    // return value is the new tool index.
    int AddNewTool( ToolType type, QString & name );
    
    void RemoveTool( int toolIndex );
    
    // Description:
    // Activate (or deactivate) the tool at index toolIndex. if activate != 0 -> activate, otherwise deactivate
    // This function will set the 'active' flag of the tool. If it is an active tool, it has already
    // been activated by the tracking system. If it is a passive one, it will try to activate it only
    // if the tracking system is initialized. ReallyActivateTool does the job of activating the tool through the tracking device. 
    void ActivateTool( int toolIndex, int activate );
    int IsToolActive( int toolIndex );
    
    // Description:
    // Get the state of the tool at index toolIndex. The state is the one computed on last Tracker update.
    TrackerToolState GetCurrentToolState( int toolIndex );
    
    // Description:
    // Get the calibration matrix for tool at index toolIndex. If the tool is active, you get
    // the real tool calibration matrix in vtkTrackerTool, otherwise, you get the cached matrix
    vtkMatrix4x4 * GetToolCalibrationMatrix( int toolIndex );
    
    void SetToolRomFile( int toolIndex, QString & romFile );
    QString GetToolRomFile( int toolIndex );
    
    // Description:
    // Get the calibration matrix that is pre-multiplied with all transformation returned by
    // the tracking system. This matrix is useful to change axes orientation or if you have
    // a reference system different from the one given by the tracking system.
    vtkMatrix4x4 * GetWorldCalibrationMatrix();
    
    
    QWidget * CreateSettingsDialog( QWidget * parent );
    QWidget * CreateStatusDialog( QWidget * parent );
    
    virtual void Serialize( Serializer * serializer );

    // Description:
    // Set the desired baud rate.  Default: 9600.
    void SetBaudRate(int);
    int GetBaudRate();

    // Description:
    // Give more/less power to the user
    void WriteActivePointerCalibrationMatrix(QString & filename);

    int FindFirstActivePointer();

    // Description
    // check if there is a valid, active reference tool
    bool ValidateReference();

public slots:
    void RenameTool(QString, QString);

signals:


    void Updated();
    void ToolActivated( int );
    void ToolDeactivated( int );
    void ToolUseChanged( int );
    void ToolNameChanged( int, QString );
    void ToolRemoved( int );
    void TrackerInitialized();
    void TrackerStopped();
    void ReferenceToolChanged(bool);
    void NavigationPointerChangedSignal();
    
private:

    void ActivateAllPassiveTools();
    void DeactivateAllPassiveTools();
    
    // Functions that act on subclasses of vtkTracker. Might eventually be put 
    // in subclasses of class Tracker.
    void ReallyActivatePassiveTool( int toolIndex );
    void ReallyDeactivatePassiveTool( int toolIndex );
    
    // Description:
    // Backs-up the calibration matrix of active tools so that
    // when this class is serialized, we save the correct calibration matrix
    void BackupCalibrationMatrices();
    
    // Description:
    // This function look if some new active tools have
    // been plugged into the tracking system.
    void LookForNewActiveTools();
    void LookForDeactivatedActiveTools();
    
    // The object that does the real tracking job
    vtkPOLARISTracker                     * m_tracker;
    QString                             m_trackerVersion;
    
    // Meta information for every tool that is not contained in vtkTracker
    typedef std::vector<ToolDescription> ToolDescriptionVec;
    ToolDescriptionVec     m_toolVec;
    
    int                    m_currentTool;
    int                    m_referenceTool;
    int                    m_navigationPointer; // pointer selected to navigate through the volume, cut planes will be moved to its tip position
    vtkMatrix4x4         * m_cachedWorldCalibrationMatrix;
    int                    m_baudRate;
};

ObjectSerializationHeaderMacro( Tracker );

#endif
