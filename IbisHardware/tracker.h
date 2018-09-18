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
#include <QObject>
#include <qstring.h>
#include "trackerflags.h"
#include "hardwaremodule.h"
#include "trackedsceneobject.h"

class vtkPOLARISTracker;
class vtkTrackerTool;
class QWidget;
class vtkMatrix4x4;
class IbisAPI;
class TrackedVideoSource;
class IbisHardwareModule;
class PolyDataObject;

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
    void InstanciateSceneObject();
    void ClearSceneObject();
    ToolType type;           // Passive = 1, Active = 0
    ToolUse use;             // What is this tool used for.
    int active;              // wether the tool has been activated
    int toolPort;            // if active == 1, the tool port, otherwise, -1
    int videoSourceIndex;
    QString name;            // tool name. just for the ui.
    QString romFileName;
    QString cachedSerialNumber;   // serial number used to recognize active tools when plugged in
    TrackedSceneObject * sceneObject;  // Object in the scene if active or holding state if inactive
    PolyDataObject * toolModel; // tool 3D model with origin at origin of the tracker tool
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
    
    vtkTypeMacro(Tracker,vtkObject);

    Tracker();
    ~Tracker();

    void SetHardwareModule( IbisHardwareModule * hw ) { m_hardwareModule = hw; }
    void SetIbisAPI( IbisAPI * api ) { m_ibisAPI = api; }
        
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
    vtkTrackerTool * GetTool( TrackedSceneObject * obj );
    TrackedVideoSource * GetVideoSource( TrackedSceneObject * obj );

    vtkTransform * GetReferenceTransform() { return m_referenceTransform; }
    int GetReferenceToolIndex();
    void SetReferenceToolIndex( int toolIndex );
    int GetCurrentToolIndex();
    void SetCurrentToolIndex( int toolIndex );
    
    int GetNumberOfTools();
    int GetNumberOfActiveTools();
    
    QString GetToolName( int toolIndex );
    void SetToolName( int toolIndex, QString toolName );

    int GetToolObjectId(int toolIndex);
    void SetToolObjectId(int toolIndex, int id);

    int GetToolIndex( QString toolName );
    int GetToolIndex( TrackedSceneObject * obj );

    bool ToolHasVideo( int toolIndex );
    
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
    TrackerToolState GetToolState( int toolIndex );
    
    void SetToolRomFile( int toolIndex, QString & romFile );
    QString GetToolRomFile( int toolIndex );
    
    QWidget * CreateSettingsDialog( QWidget * parent );
    
    virtual void Serialize( Serializer * serializer );

    // Description:
    // Set the desired baud rate.  Default: 9600.
    void SetBaudRate(int);
    int GetBaudRate();

    QString GetTrackerError() { return m_trackerError; }
    void ClearTrackerError() { m_trackerError.clear(); }

    void AddAllToolsToScene();
    void RemoveAllToolsFromScene();

public slots:

    void RenameTool(QString, QString);

signals:

    void ToolListChanged();
    void Updated();
    
private:

    TrackedVideoSource * GetVideoSource( int sourceIndex );

    void InstanciateToolModels();
    void PushTrackerStateToSceneObjects();
    bool ActivateAllPassiveTools();
    void DeactivateAllPassiveTools();
    
    // Functions that act on subclasses of vtkTracker. Might eventually be put 
    // in subclasses of class Tracker.
    bool ReallyActivatePassiveTool( int toolIndex );
    void ReallyDeactivatePassiveTool( int toolIndex );
    
    void AddToolToScene( int toolIndex );
    void RemoveToolFromScene( int toolIndex );

    // Description:
    // This function look if some new active tools have
    // been plugged into the tracking system.
    void LookForNewActiveTools();
    void LookForDeactivatedActiveTools();

    // Stable transform between the reference and tracking system origin (e.g.: Polaris Camera)
    vtkTransform * m_referenceTransform;
    
    // The object that does the real tracking job
    vtkPOLARISTracker   * m_tracker;
    QString               m_trackerVersion;

    IbisAPI             * m_ibisAPI;
    IbisHardwareModule  * m_hardwareModule;
    
    // Meta information for every tool that is not contained in vtkTracker
    typedef std::vector<ToolDescription> ToolDescriptionVec;
    ToolDescriptionVec     m_toolVec;
    
    QString         m_trackerError;
    int             m_currentTool;
    int             m_referenceTool;
    int             m_baudRate;
};

ObjectSerializationHeaderMacro( Tracker );

#endif
