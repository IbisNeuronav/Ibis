#ifndef __TrackerStatusDialog_h_
#define __TrackerStatusDialog_h_

#include <QList>
#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QCustomEvent;
class QComboBox;
class QCheckBox;
class QPushButton;
class vtkQtMatrixDialog;
class SceneManager;

class ToolUI : public QWidget
{
 
Q_OBJECT
            
public:
    
    ToolUI( QWidget * parent = 0 );
    ~ToolUI();
    
    void SetSceneManager( SceneManager * man, int m_toolObjectId );

public slots:
    void UpdateUI();

protected slots:

    void SnapshotButtonClicked();
    void SnapshotMatrixWidgetClosed();
    
protected:

    SceneManager * m_manager;
    int m_toolObjectId;
    
    QHBoxLayout * ToolLayout;
    QPushButton * SnapshotButton;
    QLabel * ToolNameLabel;
    QLabel * ToolStateLabel;
    vtkQtMatrixDialog * SnapshotMatrixWidget;  
};


class TrackerStatusDialog : public QWidget
{
    Q_OBJECT

public:
    
    TrackerStatusDialog( QWidget * parent = 0 );
    ~TrackerStatusDialog();

    void SetSceneManager( SceneManager * man );

public slots:
    
    void OnIbisClockTick();
    void UpdateUI();

    void OnNavigationComboBoxActivated( int );
    void OnNavigationCheckboxToggled( bool );

protected:
    
    void ClearAllTools();

    // GUI elements
    QComboBox * m_pointerToolCombo;
    QLabel * m_pointersLabel;
    QCheckBox * m_navigationCheckBox;

    // GUI for each tool
    QVBoxLayout * m_trackerStatusDialogLayout;
    QList< ToolUI * > m_toolsWidget;

    SceneManager * m_sceneManager;
};

#endif
