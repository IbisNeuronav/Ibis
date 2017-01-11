/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __QuadViewWindow_h_
#define __QuadViewWindow_h_

#include <qvariant.h>
#include <qwidget.h>
#include "serializer.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QAbstractButton;
class QSpacerItem;
class QSplitter;
class QLabel;
class QSettings;
class QVTKWidget;
class vtkQtRenderWindow;
class QVTKWidget2;
class QFrame;
class SceneManager;
class View;
class vtkRenderWindowInteractor;

//#define USE_QVTKWIDGET_2

class QuadViewWindow : public QWidget
{
    Q_OBJECT

public:

    QuadViewWindow( QWidget * parent = 0, Qt::WindowFlags fl = 0 );
    virtual ~QuadViewWindow();

    virtual void Serialize( Serializer * ser );
    virtual void SetSceneManager( SceneManager * man );

    void LoadSettings( QSettings & s );
    void SaveSettings( QSettings & s );

    void AddBottomWidget( QWidget * w );
    void RemoveBottomWidget( QWidget * w );
    void SetExpandedView( bool on );
    void SetCurrentViewWindow( int index );

public slots:

    void Detach3DView( QWidget * parent );
    void Attach3DView();
    
    void Win0NeedsRender();
    void Win1NeedsRender();
    void Win2NeedsRender();
    void Win3NeedsRender();
    
    void RenderAll();
    
    void ZoomInButtonClicked();
    void ZoomOutButtonClicked();
    void ResetCameraButtonClicked();
    void ExpandViewButtonClicked();
    void ResetPlanesButtonClicked();
    void ViewFrontButtonClicked();
    void ViewBackButtonClicked();
    void ViewRightButtonClicked();
    void ViewLeftButtonClicked();
    void ViewBottomButtonClicked();
    void ViewTopButtonClicked();

    void OnCursorMoved();
    void OnShowGenericLabelText();

    void OnShowGenericLabel( bool );

protected:

    QAbstractButton * CreateToolButton( QString name, QString iconPath, QString toolTip, const char * callbackSlot );
    void MakeOneView( int index, const char * name );

    void WinNeedsRender( int winIndex );

    // reimplemented from QObject. Filters event sent to children. Used to track focus
    bool eventFilter(QObject *obj, QEvent *event);

    void PlaceCornerText();

    SceneManager * m_sceneManager;
    
    QVBoxLayout * m_generalLayout;
    
    QHBoxLayout * m_buttonBox;
    QAbstractButton * m_expandViewButton;
    QLabel      * m_cursorPosLabel;
    QSpacerItem * m_buttonBoxSpacer;
    QLabel      * m_genericLabel;

    QGridLayout *m_viewWindowsLayout;

    static const QString ViewNames[4];
#ifdef USE_QVTKWIDGET_2
    QVTKWidget2 * m_vtkWindows[4];
#else
    vtkQtRenderWindow * m_vtkWindows[4];
#endif
    QFrame * m_vtkWindowFrames[4];
    QVBoxLayout * m_frameLayouts[4];

    QWidget * m_detachedWidget;

    QFrame * m_bottomWidgetFrame;
    QVBoxLayout * m_bottomWidgetLayout;
    
    int m_currentViewWindow;
    bool m_viewExpanded;
};

ObjectSerializationHeaderMacro( QuadViewWindow );

#endif
