/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef QUADVIEWWINDOW_H
#define QUADVIEWWINDOW_H

#include <QVTKRenderWidget.h>

#include <QObject>
#include <QVariant>

#include "serializer.h"

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
    QuadViewWindow( QWidget * parent = 0, Qt::WindowFlags fl = Qt::WindowFlags() );
    virtual ~QuadViewWindow();

    /**
     * Read/write properties of QuadViewWindow in xml format.
     */
    virtual void Serialize( Serializer * ser );
    /**
     * Set the SceneManager to do all the real work manipulating views.
     */
    virtual void SetSceneManager( SceneManager * man );

    /**
     * LoadSettings gets from previously loaded QSettings information about selected and/or expanded view and applies
     * it to the view. The current view will get a thin red fraqme.
     */
    void LoadSettings( QSettings & s );
    /**
     * SaveSettings puts in QSettings information about selected and/or expanded view.
     */
    void SaveSettings( QSettings & s );

    /**
     * AddBottomWidget - adds a widget at the bottom of QuadViewWindow. Used by plugins to show and configure additional
     * plugin data.
     */
    void AddBottomWidget( QWidget * w );
    /**
     * RemoveBottomWidget - removes previously added widget from the bottom of QuadViewWindow.
     */
    void RemoveBottomWidget( QWidget * w );
    /**
     * SetExpandedView - expands current view
     *  @param on - if true the current view is expanded and other views are hidden, if false all views show up.
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

    /**
     * Detach3DView used in some plugins to move the 3D view out of the QuadViewWindow.
     * @param parent - not used, m_detachedWidget is used instead.
     */
    void Detach3DView( QWidget * parent );
    /**
     * Attach3DView puts back the 3D view in QuadViewWindow.
     */
    void Attach3DView();

    /** @name Toolbar functions
     * @brief  Zooming, expanding views, selecting object position.
     */
    ///@{
    /** Increase image size in all views. */
    void ZoomInButtonClicked();
    /** Decrease image size in all views. */
    void ZoomOutButtonClicked();
    /** Reset cameras in all views. */
    void ResetCameraButtonClicked();
    /** If the current view is already expanded, it goes back in its place in QuadViewWindow and all four views show up.
     *  Otherwise the selected view is expanded and all other views are hidden.
     */
    void ExpandViewButtonClicked();
    /** Set main cutting planes in initial position. */
    void ResetPlanesButtonClicked();
    /** Show the front side of the object in 3D. */
    void ViewFrontButtonClicked();
    /** Show the back side of the object in 3D. */
    void ViewBackButtonClicked();
    /** Show the right side of the object in 3D. */
    void ViewRightButtonClicked();
    /** Show the left side of the object in 3D. */
    void ViewLeftButtonClicked();
    /** Show the bottom side of the object in 3D. */
    void ViewBottomButtonClicked();
    /** Show the top side of the object in 3D. */
    void ViewTopButtonClicked();
    ///@}

    /** Show current position on the toolbar. */
    void OnCursorMoved();
    /** Set prepared text in a label on the toolbar. */
    void OnShowGenericLabelText();
    /** Show a label on the toolbar. */
    void OnShowGenericLabel( bool );

protected:
    QAbstractButton * CreateToolButton( QWidget * parent, QString name, QString iconPath, QString toolTip,
                                        const char * callbackSlot );
    void MakeOneWidget( int index, const char * name );

    // reimplemented from QObject. Filters event sent to children. Used to track focus
    bool eventFilter( QObject * obj, QEvent * event );

    void PlaceCornerText();

    SceneManager * m_sceneManager;

    QVBoxLayout * m_generalLayout;

    QHBoxLayout * m_buttonBox;
    QAbstractButton * m_expandViewButton;
    QLabel * m_cursorPosLabel;
    QSpacerItem * m_buttonBoxSpacer;
    QLabel * m_genericLabel;

    QGridLayout * m_viewWindowsLayout;

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
