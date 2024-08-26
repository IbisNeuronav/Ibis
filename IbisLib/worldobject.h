/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H

#include <vtkSmartPointer.h>

#include <QColor>
#include <QObject>

#include "ibistypes.h"
#include "sceneobject.h"

class PolyDataObject;

/**
 * @class   WorldObject
 * @brief   It is also called Scene Root, as it is the topmost obect in a scene and all other objects are its children.
 *
 *  There should be only one WorldObject in the scene. Most of its properties are fixed and user cannot change them.
 *  From WorldObject settings one can control the axes, the cursor, camera view angle, background colors,
 *  interaction style and update frequence.
 *
 */
class WorldObject : public SceneObject
{
    Q_OBJECT

public:
    static WorldObject * New() { return new WorldObject; }
    vtkTypeMacro( WorldObject, SceneObject );

    WorldObject();
    virtual ~WorldObject();

    /** Add axes to visualize the current coordinates system.
     *  Initially axes are created when when SceneManager is instantiated.
     */
    void SetAxesObject( vtkSmartPointer<PolyDataObject> obj );
    /** Return current axes object. */
    vtkSmartPointer<PolyDataObject> GetAxesObject() { return m_axesObject; }

    /** @name  Unchangeable parameters
     *  @brief User can't set name or allow hidding or delete this type of object.
     */
    ///@{
    /** Disable setting name. */
    virtual void SetName( QString name ) override {}
    /** Disable changing name. */
    virtual void SetNameChangeable( bool c ) override {}
    /** Disable hiding. */
    virtual void SetHidden( bool h ) {}
    /** Disable hiding. */
    virtual void SetHidable( bool h ) override {}
    /** Disable deleting. */
    virtual void SetObjectDeletable( bool d ) override {}
    ///@}

    /** Hide axes. */
    void SetAxesHidden( bool h );
    /** Check axes visibility. */
    bool AxesHidden();
    /** Decide if main 3D view is following the reference object. */
    void Set3DViewFollowsReferenceVolume( bool f );
    /** Find out if main 3D view is following the reference object. */
    bool Is3DViewFollowingReferenceVolume();
    /** Hide/show cursor. */
    void SetCursorVisible( bool v );
    /** Check cursor visibility. */
    bool GetCursorVisible();
    /** Set cursor color. */
    void SetCursorColor( const QColor & c );
    /** Set cursor color. */
    void SetCursorColor( double[3] );
    /** Get cursor color. */
    QColor GetCursorColor();
    /** Set background color of all views, 3D included. */
    void SetBackgroundColor( const QColor & c );
    /** Get the color of all views, 3D may have different color. */
    QColor GetBackgroundColor();
    /** Set background color of 3D view only. */
    void Set3DBackgroundColor( const QColor & c );
    /** Get the color of 3D view. */
    QColor Get3DBackgroundColor();

    /** Set interactor style of 3D view. */
    void Set3DInteractorStyle( InteractorStyle style );
    /** Get interactor style of 3D view. */
    InteractorStyle Get3DInteractorStyle();

    /** Get camera view angle of 3D view. */
    double Get3DCameraViewAngle();
    /** Set camera view angle of 3D view. */
    void Set3DCameraViewAngle( double angle );

    /** @name  Update
     *   @brief Set/Get update frequency.
     *
     * */
    ///@{
    void SetUpdateFrequency( double fps );
    double GetUpdateFrequency();
    ///@}

    virtual QWidget * CreateSettingsDialog( QWidget * parent ) override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override {}

private:
    vtkSmartPointer<PolyDataObject> m_axesObject;
};

#endif
