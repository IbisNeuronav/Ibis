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

#include "vtkObject.h"
#include <QObject>
#include <QString>
#include <QVector>
#include "serializer.h"

class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkAssembly;
class vtkProp3D;
class View;
class vtkLinearTransform;
class vtkTransform;
class vtkMatrix4x4;
class QWidget;
class SceneManager;
class vtkEventQtSlotConnect;

class SceneObject : public QObject, public vtkObject
{
    
Q_OBJECT

public:
        
    static SceneObject * New() { return new SceneObject; }

    vtkTypeMacro(SceneObject,vtkObject);
    
    SceneObject();
    virtual ~SceneObject();
  
    virtual void Serialize( Serializer * ser );
    virtual void PostSceneRead();
    virtual void Export();
    virtual bool IsExportable() { return false; }

    virtual void ShallowCopy(SceneObject*);

    static const int InvalidObjectId;
    vtkSetMacro( ObjectID, int );
    vtkGetMacro( ObjectID, int );
    QString GetName() { return this->Name; }
    virtual void SetName( QString name );
    bool CanChangeName() { return NameChangeable; }
    virtual void SetNameChangeable( bool c ) { NameChangeable = c; }
    QString GetDataFileName() { return this->DataFileName; }
    void SetDataFileName( QString DataFileName );
    QString GetFullFileName() { return this->FullFileName; }
    void SetFullFileName( QString FullFileName ) {this->FullFileName = FullFileName;}

    void SetLocalTransform( vtkLinearTransform * localTransform );
    vtkGetObjectMacro( LocalTransform, vtkLinearTransform );
    vtkGetObjectMacro( WorldTransform, vtkTransform );
    bool CanEditTransformManually() { return AllowManualTransformEdit; }
    void SetCanEditTransformManually( bool c ) { AllowManualTransformEdit = c; }

    // Next 2 functions used in the case where many modifs have to be done on the
    // transforms and we don't want to issue a WorldTransformChanged signal every time.
    void StartModifyingTransform();
    void FinishModifyingTransform();
    
    virtual bool Setup( View * view );
    virtual void PreDisplaySetup();
    virtual bool Release( View * view );
    virtual void ReleaseAllViews();
    virtual QWidget * CreateSettingsDialog( QWidget * parent );
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) {};
    QWidget * CreateTransformEditWidget( QWidget * parent );

    virtual void SetHiddenWithChildren( bool hide );
    virtual void SetHiddenChildren(SceneObject * parent, bool hide);
    bool IsHidden() { return this->ObjectHidden; }
    void SetHidden( bool h );
    bool IsHidable() { return AllowHiding; }
    virtual void SetHidable( bool h ) { AllowHiding = h; }
        
    void AddChild( SceneObject * child );
    void InsertChild( SceneObject * child, int index );
    void RemoveChild( SceneObject * child );
    int GetNumberOfChildren();
    SceneObject * GetChild( int index );
    SceneObject * GetChild( const QString & objectName );
    int GetChildIndex( SceneObject * child );
    int GetChildListableIndex( SceneObject * child );
    int GetObjectIndex();
    int GetObjectListableIndex();
    SceneObject * GetObject( const QString & objectName );
    SceneObject * GetObjectByID( int id );
    SceneObject * GetParent() { return this->Parent; }
    bool CanAppendChildren() { return AllowChildren; }
    void SetCanAppendChildren( bool c ) { AllowChildren = c; }
    bool CanChangeParent() { return AllowChangeParent; }
    void SetCanChangeParent( bool c ) { AllowChangeParent = c; }
    bool DescendsFrom( SceneObject * obj );

    void WorldToLocal(double worldPoint[3], double localPoint[3] );
    void LocalToWorld(double localPoint[3], double worldPoint[3] );

    int GetNumberOfListableChildren();
    SceneObject * GetListableChild( int index );
    bool IsListable() { return this->ObjectListable; }
    void SetListable( bool l ) { this->ObjectListable = l; }

    bool IsManagedBySystem() { return ObjectManagedBySystem; }
    vtkSetMacro( ObjectManagedBySystem, bool );

    bool IsManagedByTracker() { return ObjectManagedByTracker; }
    vtkSetMacro( ObjectManagedByTracker, bool );

    virtual void SetObjectDeletable( bool d ) { ObjectDeletable = d; }
    bool IsObjectDeletable() { return ObjectDeletable; }

    vtkGetObjectMacro( Manager, SceneManager );

    // For interactive objects. To get callback from the view in subclasses, you need to reimplement
    // any of the functions you need and then register your object with the view in the setup function
    virtual bool OnLeftButtonPressed( View * v, int x, int y, unsigned modifiers ) { return false; }
    virtual bool OnLeftButtonReleased( View * v, int x, int y, unsigned modifiers ) { return false; }
    virtual bool OnRightButtonPressed( View * v, int x, int y, unsigned modifiers ) { return false; }
    virtual bool OnRightButtonReleased( View * v, int x, int y, unsigned modifiers ) { return false; }
    virtual bool OnMiddleButtonPressed( View * v, int x, int y, unsigned modifiers ) { return false; }
    virtual bool OnMiddleButtonReleased( View * v, int x, int y, unsigned modifiers ) { return false; }
    virtual bool OnMouseMoved( View * v, int x, int y, unsigned modifiers ) { return false; }

    vtkGetMacro( RenderLayer, int );
    vtkSetMacro( RenderLayer, int );
      
signals:

    void NameChanged();
    void Modified();
    void RemovingFromScene();
    void WorldTransformChangedSignal();
    void FinishedReading();

public slots:

    virtual void MarkModified();
    void NotifyTransformChanged();
    
protected:

    // Give subclasses a chance to react
    virtual void ObjectAddedToScene() {}
    virtual void ObjectAboutToBeRemovedFromScene() {}
    virtual void ObjectRemovedFromScene() {}
    virtual void Hide() {}
    virtual void Show() {}

    QString GetSceneDataDirectoryForThisObject( QString baseDir );
    virtual void InternalPostSceneRead() {}
    virtual void WorldTransformChanged();
    virtual void InternalWorldTransformChanged() {} // let subclass react to the change in transform
    

    QString Name; // name to display on the list
    QString DataFileName; // just the name of the file
    QString FullFileName; // name of the data file including full path

	// Transforms affecting the object:
	//  LocalTransform(Tl) is a local object-space transform
	//	WorldTransform(Tw) is a concatenation of all transforms affecting the object: Tw = Tp * Tl
	//  Tp is parent transform.
    virtual void UpdateWorldTransform();
    vtkTransform * WorldTransform;
    vtkLinearTransform * LocalTransform;
    bool IsModifyingTransform;
    bool TransformModified;

    vtkEventQtSlotConnect * m_vtkConnections;
    
    // The following vector is used to remember which actors were instanciated
    // for every view so we can remove them or add new objects as children
    typedef std::vector< View* > ViewContainer;
    ViewContainer Views;
    
    // Scene hierarchy management
    SceneObject * Parent;
    typedef QList< SceneObject* > SceneObjectVec;
    SceneObjectVec Children;
    bool AllowChildren;
    bool AllowChangeParent;
    
    bool ObjectManagedBySystem;
    bool ObjectHidden;
    bool AllowHiding;
    bool ObjectDeletable;
    bool NameChangeable;
    bool ObjectListable;
    bool AllowManualTransformEdit;
    bool ObjectManagedByTracker;
    int RenderLayer;  // This is an hint to determine which layer of renderer we draw on

private:

    void AddToScene( SceneManager * man, int objectId );
    void RemoveFromScene();
    friend class SceneManager;
    SceneManager * Manager;
    int ObjectID;
};

ObjectSerializationHeaderMacro( SceneObject );

#endif //TAG_SCENEOBJECT_H
