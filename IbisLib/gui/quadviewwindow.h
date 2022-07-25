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

#include <QVariant>
#include "serializer.h"
#include <QObject>
#include <QVTKRenderWidget.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QAbstractButton;
class QSpacerItem;
class QSplitter;
class QLabel;
class QSettings;
class QFrame;

class SceneManager;
class vtkRenderWindowInteractor;

/**
 * @class   QuadViewWindow
 * @brief   A widget used to show the four views.
 *
 *  QuadViewWindow controls showing views, zooming, rotation, cursor. There is a unique QuadViewWindow in the app.
 *
 *  @sa SceneManager
 */
class QuadViewWindow : public QWidget
{
    Q_OBJECT

public:

    QuadViewWindow( QWidget * parent = 0, Qt::WindowFlags fl = 0 );
    virtual ~QuadViewWindow();

    virtual void Serialize( Serializer * ser );
    virtual void SetSceneManager( SceneManager * man );

    /**
     * LoadSettings gets from previously loaded QSettings information about selected and/or expanded view.
     */
    void LoadSettings( QSettings & s );
    /**
     * SaveSettings puts in QSettings information about selected and/or expanded view.
     */
    void SaveSettings( QSettings & s );

    /**
     * AddBottomWidget - adds a widget at the bottom of QuadViewWindow.
     */
    void AddBottomWidget( QWidget * w );
    /**
     * RemoveBottomWidget - removes previously added widget from the bottom of QuadViewWindow.
     */
    void RemoveBottomWidget( QWidget * w );
    /**
     * SetExpandedView - expands current view
     *  @param on - if true the current view is expanded and other views are hidden, if false all for views show up.
     */
    void SetExpandedView( bool on );
    /**
     * SetCurrentViewWindow will store the index of the current view
     * @param index  number from 0 to 3
     */
    void SetCurrentViewWindow( int index );
    /**
     * SetShowToolbar - sets toolbar visibility.
     */
    void SetShowToolbar( bool show );

public slots:

    void Detach3DView( QWidget * parent );
    void Attach3DView();
    
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

    QAbstractButton * CreateToolButton( QWidget * parent, QString name, QString iconPath, QString toolTip, const char * callbackSlot );
    void MakeOneWidget( int index, const char * name );

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

    QVTKRenderWidget * m_vtkWidgets[4];
    QFrame * m_vtkWindowFrames[4];
    QVBoxLayout * m_frameLayouts[4];

    QWidget * m_detachedWidget;

    QFrame * m_toolboxFrame;
    QFrame * m_bottomWidgetFrame;
    QVBoxLayout * m_bottomWidgetLayout;
    
    int m_currentViewWindow;
    bool m_viewExpanded;
};

ObjectSerializationHeaderMacro( QuadViewWindow );

#endif
