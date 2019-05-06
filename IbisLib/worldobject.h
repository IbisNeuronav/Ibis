/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __WorldObject_h_
#define __WorldObject_h_

#include "sceneobject.h"
#include "ibistypes.h"
#include <QColor>
#include "vtkSmartPointer.h"

class PolyDataObject;

class WorldObject : public SceneObject
{

    Q_OBJECT

public:

    static WorldObject * New() { return new WorldObject; }
    vtkTypeMacro( WorldObject, SceneObject );

    WorldObject();
    virtual ~WorldObject();

    void SetAxesObject( vtkSmartPointer<PolyDataObject> obj );
    vtkSmartPointer<PolyDataObject> GetAxesObject() { return m_axesObject; }

    // can't set name or allow hidding or delete this type of object
    virtual void SetName( QString name ) override {}
    virtual void SetNameChangeable( bool c ) override {}
    virtual void SetHidden( bool h ) { }
    virtual void SetHidable( bool h ) override {}
    virtual void SetObjectDeletable( bool d ) override {}

    void SetAxesHidden( bool h );
    bool AxesHidden();
    void Set3DViewFollowsReferenceVolume( bool f );
    bool Is3DViewFollowingReferenceVolume();
    void SetCursorVisible( bool v );
    bool GetCursorVisible();
    void SetCursorColor( const QColor & c );
    void SetCursorColor( double[3] );
    QColor GetCursorColor();
    void SetBackgroundColor( const QColor & c ); // Set background color of all views
    QColor GetBackgroundColor();
    void Set3DBackgroundColor( const QColor & c ); // Set background color of 3D view only
    QColor Get3DBackgroundColor();

    void Set3DInteractorStyle( InteractorStyle style );
    InteractorStyle Get3DInteractorStyle();

    double Get3DCameraViewAngle();
    void Set3DCameraViewAngle( double angle );

    void SetUpdateFrequency( double fps );
    double GetUpdateFrequency();

    virtual QWidget * CreateSettingsDialog( QWidget * parent ) override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) override {}

private:

    vtkSmartPointer<PolyDataObject> m_axesObject;

};

#endif
