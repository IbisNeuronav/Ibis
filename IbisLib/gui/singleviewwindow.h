#ifndef __SingleViewWindow_h_
#define __SingleViewWindow_h_

#include <QVariant>
#include <QWidget>
//Added by qt3to4:
#include <QCloseEvent>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>

class Q3VBoxLayout;
class Q3HBoxLayout;
class QPushButton;
class QSpacerItem;
class vtkQtRenderWindow;
class SceneManager;
class View;

class SingleViewWindow : public QWidget
{
    Q_OBJECT

public:

    SingleViewWindow( QWidget * parent = 0, const char * name = 0, Qt::WFlags fl = 0 );
    virtual ~SingleViewWindow();

    virtual void SetSceneManager( SceneManager * man );
    
public slots:
    
    void WinNeedsRender();
    
    void ResetCameraButtonClicked();

protected:
    
    // Description:
    // Get the chance to release things before the window is closed (and maybe
    // destroyed if the Qt::WDestructiveClose flag is set.
    void closeEvent(  QCloseEvent * e );
    
    SceneManager * m_sceneManager;
    
    Q3VBoxLayout * m_generalLayout;
    
    Q3HBoxLayout * m_buttonBox;
    QPushButton * m_resetCamerasButton;
    QSpacerItem * m_buttonBoxSpacer; 

    vtkQtRenderWindow * m_vtkWindow;
};


#endif
