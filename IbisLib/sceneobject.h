/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_SCENEOBJECT_H
#define TAG_SCENEOBJECT_H

#include <vtkSmartPointer.h>

#include <QObject>
#include <QString>
#include <QVector>

#include "serializer.h"
#include "viewinteractor.h"
#include "vtkObject.h"

class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkAssembly;
class vtkProp3D;
class View;
class vtkTransform;
class vtkMatrix4x4;
class QWidget;
class SceneManager;
class vtkEventQtSlotConnect;

/**
 * @class   SceneObject
 * @brief   It is the basic object used to build a scene, all other objects are derived from SceneObject
 *
 *  SceneObject defines common properties of all objects and their basic functionality.
 */

class SceneObject : public QObject, public vtkObject, public ViewInteractor
{
    Q_OBJECT

public:
    static SceneObject * New() { return new SceneObject; }

    vtkTypeMacro( SceneObject, vtkObject );

    SceneObject();
    virtual ~SceneObject();

    /**
     * Read/write properties of the object in xml format.
     * Every object has to override this function.
     */
    virtual void Serialize( Serializer * ser );
    /**
     * Adjust object after loading a scene.
     * Other objects override this function if needed.
     */
    virtual void PostSceneRead();
    /**
     * Export object with its properties in the format proper to the object type.
     * Other objects override this function if they can be exported.
     */
    virtual void Export();
    /**
     * Is it possible to export the given object?
     * Every object has to override this function.
     */
    virtual bool IsExportable() { return false; }

    ///@{
    /** Set/Get the ObjectID */
    vtkSetMacro( ObjectID, int );
    vtkGetMacro( ObjectID, int );
    ///@}

    /** @name Object Name and File Info
     * @brief  Getting and setting object and object file names
     */
    ///@{
    /** Get object name */
    QString GetName() { return this->Name; }
    /** Set object name */
    virtual void SetName( QString name );
    /** Check if it is possible to change object name */
    bool CanChangeName() { return NameChangeable; }
    /** Allow/disallow changing object name */
    virtual void SetNameChangeable( bool c ) { NameChangeable = c; }
    /** Return the name of the file containing the object */
    QString GetDataFileName() { return this->DataFileName; }
    /** Set the name of the file containing the object */
    void SetDataFileName( QString DataFileName );
    /** Return file name including the path */
    QString GetFullFileName() { return this->FullFileName; }
    /** Set file name including the path */
    void SetFullFileName( QString FullFileName ) { this->FullFileName = FullFileName; }
    ///@}

    /** @name Object Transforms
     * @brief  Getting and setting object transforms
     */
    ///@{
    /** Set local transform */
    void SetLocalTransform( vtkTransform * localTransform );
    /** Get local transform */
    vtkTransform * GetLocalTransform();
    /** Get world transform */
    vtkTransform * GetWorldTransform();
    /** Find out if local transform can be set manually */
    bool CanEditTransformManually() { return AllowManualTransformEdit; }
    /** Allow/disallow manual transform change */
    void SetCanEditTransformManually( bool c ) { AllowManualTransformEdit = c; }
    /** Set a flag telling the system  that many modifs have to be done on the
    transforms and we don't want to issue a WorldTransformChanged signal every time. */
    void StartModifyingTransform();
    /** Inform the system that transform modifications are finished. */
    void FinishModifyingTransform();
    ///@}

    /** @name Object and Views
     * @brief  Setting and releasing object in views
     */
    ///@{
    /** Setup object in a specific view */
    virtual void Setup( View * view );
    /** Prepare object before rendering, it may be specific to the given object type */
    virtual void PreDisplaySetup();
    /** Release object from a specific view */
    virtual void Release( View * view );
    /** Release object from all views */
    virtual void ReleaseAllViews();
    ///@}

    /** @name Create Settings Dialogs
     * @brief Setting dialog allows changing objects patameters (color, opacity, etc.).
     */
    ///@{
    /** Create basic settings dialog */
    virtual QWidget * CreateSettingsDialog( QWidget * parent );
    /** Create widgets specific to the object, they are added as tabs to the basic settings dialog. */
    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) {}
    /** Create tab widget that will show object transforms */
    QWidget * CreateTransformEditWidget( QWidget * parent );
    ///@}

    /** @name Show and Hide
     * @brief Showing and hiding objects in views
     */
    ///@{
    /** Hide/show the object and all its children */
    virtual void SetHiddenWithChildren( bool hide );
    /** Hide/show all object's children, leave the object in view */
    virtual void SetHiddenChildren( SceneObject * parent, bool hide );
    /** Check if the object is hidden */
    bool IsHidden() { return this->ObjectHidden; }
    /** Hide/show the object, children remain in view */
    void SetHidden( bool h );
    /** Check if hiding the object is allowed */
    bool IsHidable() { return AllowHiding; }
    /** Allow/disallow hiding of the object */
    virtual void SetHidable( bool h ) { AllowHiding = h; }
    ///@}

    /** @name Children
     * @brief Object position, adding, removing and accessing children snd parent objects.
     */
    ///@{
    /** Add a child to the object */
    void AddChild( SceneObject * child );
    /** Add a child to the object at a specific position */
    void InsertChild( SceneObject * child, int index );
    /** Remove one child */
    void RemoveChild( SceneObject * child );
    /** Get the number of all object's children */
    int GetNumberOfChildren();
    /** Get child at a specific position */
    SceneObject * GetChild( int index );
    /** Get child by name */
    SceneObject * GetChild( const QString & objectName );
    /** Get child's position */
    int GetChildIndex( SceneObject * child );
    /** Get listable child's position */
    int GetChildListableIndex( SceneObject * child );
    /** Get object's position */
    int GetObjectIndex();
    /** Get listable object's position */
    int GetObjectListableIndex();
    /** Get object's parent */
    SceneObject * GetParent() { return this->Parent; }
    /** Check if object can have children */
    bool CanAppendChildren() { return AllowChildren; }
    /** Allow/disallow children */
    void SetCanAppendChildren( bool c ) { AllowChildren = c; }
    /** Check if object can change parent */
    bool CanChangeParent() { return AllowChangeParent; }
    /** Allow/disallow changing parent */
    void SetCanChangeParent( bool c ) { AllowChangeParent = c; }
    /** Check object's descent */
    bool DescendsFrom( SceneObject * obj );
    ///@}

    /** @name Listability
     * @brief Show object name in the object tree.
     */
    ///@{
    /** Get number of the objects that are listed in the objects tree */
    int GetNumberOfListableChildren();
    /** Get a listable child at a specific position */
    SceneObject * GetListableChild( int index );
    /** Check if the object is listed in the objects tree */
    bool IsListable() { return ObjectListable; }
    /** Allow/disallow listing object in the objects tree */
    void SetListable( bool l );
    ///@}

    /** Convert world coordinates to local coordinates */
    void WorldToLocal( double worldPoint[ 3 ], double localPoint[ 3 ] );
    /** Convert local coordinates to world coordinates */
    void LocalToWorld( double localPoint[ 3 ], double worldPoint[ 3 ] );

    /** Check if the object  was user created */
    bool IsUserObject();

    /** Check if it is a system object */
    bool IsManagedBySystem() { return ObjectManagedBySystem; }
    /** Allow/disallow setting object as a system object */
    vtkSetMacro( ObjectManagedBySystem, bool );

    /** Check if the object is controlled by tracker */
    bool IsManagedByTracker() { return ObjectManagedByTracker; }
    /** Allow/disallow managing by tracker */
    vtkSetMacro( ObjectManagedByTracker, bool );

    /** Allow/disallow deleting of the object */
    virtual void SetObjectDeletable( bool d ) { ObjectDeletable = d; }
    /** Check if the object can be deleted */
    bool IsObjectDeletable() { return ObjectDeletable; }

    /** Check if the object is added to the scene */
    bool IsObjectInScene() { return this->Manager != 0; }

    /** Get  pointer to SceneManager */
    vtkGetObjectMacro( Manager, SceneManager );

    /** Try to pick a 3D position only on that particular SceneObject */
    virtual bool Pick( View * v, int x, int y, double pickedPos[ 3 ] ) { return false; }

    ///@{
    /** Set/Get the RenderLayer */
    vtkGetMacro( RenderLayer, int );
    vtkSetMacro( RenderLayer, int );
    ///@}

signals:

    /** @name Signals
     * @brief Signals are emitted when there are changes affecting the scene and other objects.
     */
    ///@{
    void NameChanged();
    void AttributesChanged( SceneObject * );
    void ObjectModified();
    void RemovingFromScene();
    void WorldTransformChangedSignal();
    void FinishedReading();
    ///@}

public slots:

    virtual void MarkModified();
    void NotifyTransformChanged();

protected:
    virtual void ObjectAddedToScene() {}
    virtual void ObjectAboutToBeRemovedFromScene() {}
    virtual void ObjectRemovedFromScene() {}
    virtual void Hide() {}
    virtual void Show() {}

    QString GetSceneDataDirectoryForThisObject( QString baseDir );
    virtual void InternalPostSceneRead() {}
    virtual void WorldTransformChanged();
    /** let subclass react to the change in transform */
    virtual void InternalWorldTransformChanged() {}

    /** @name Object and Object Files
     */
    /** Name of the object to display on the list. */
    QString Name;
    /** Just the name of the file. */
    QString DataFileName;
    /** Name of the data file including full path. */
    QString FullFileName;

    /** @name Transforms
     */
    ///@{
    /** Update WorldTransform after modifying LocalTransform. */
    virtual void UpdateWorldTransform();
    /** When IsModifyingTransform flag is set true, changes to Local Transform accumulate, but WorldTransform remais
     * unchainged. */
    bool IsModifyingTransform;
    /** TransformModified flag is set true when LocalTransform has changed. */
    bool TransformModified;
    ///@}

    /** Connect to VTK Events. */
    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkConnections;

    /** The views vector is used to remember which actors were instanciated
    for every view so we can remove them or add new objects as children */
    ///@{
    typedef std::vector<View *> ViewContainer;
    ViewContainer Views;
    ///@}

    /** @name Scene Hierarchy Management
     */
    ///@{
    typedef QList<SceneObject *> SceneObjectVec;
    /** Vector of children of the object. */
    SceneObjectVec Children;
    /** Parent of the object. */
    SceneObject * Parent;
    /** Allow/disallow children of the object. */
    bool AllowChildren;
    /** Allow/disallow changing parent. */
    bool AllowChangeParent;
    ///@}

    /** @name Other Object Properties
     */
    ///@{
    /** This flag is set when the application controls the object. */
    bool ObjectManagedBySystem;
    /** Is the object visible in the scene?. */
    bool ObjectHidden;
    /** Allow/disallow hiding. */
    bool AllowHiding;
    /** Allow/disallow deleting. */
    bool ObjectDeletable;
    /** Allow/disallow name change. */
    bool NameChangeable;
    /** Is the object listed in the object tree? */
    bool ObjectListable;
    /** Allow/disallow changing transform */
    bool AllowManualTransformEdit;
    /** Is it a tracked object?*/
    bool ObjectManagedByTracker;
    ///@}
    /** This is a hint to determine which layer of renderer we draw on. */
    int RenderLayer;

private:
    void AddToScene( SceneManager * man, int objectId );
    void RemoveFromScene();
    friend class SceneManager;
    SceneManager * Manager;
    int ObjectID;
    // Transforms affecting the object:
    //  LocalTransform(Tl) is a local object-space transform
    //	WorldTransform(Tw) is a concatenation of all transforms affecting the object: Tw = Tp * Tl
    //  Tp is parent transform.
    vtkSmartPointer<vtkTransform> WorldTransform;
    vtkTransform * LocalTransform;
};

ObjectSerializationHeaderMacro( SceneObject );

#endif  // TAG_SCENEOBJECT_H
