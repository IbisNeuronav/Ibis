/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "sceneobject.h"
#include "view.h"
#include "serializerhelper.h"
#include "vtkTransform.h"
#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkAssembly.h"
#include "vtkRenderer.h"
#include "scenemanager.h"
#include "transformeditwidget.h"
#include "vtkEventQtSlotConnect.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QList>

ObjectSerializationMacro( SceneObject );

SceneObject::SceneObject()
    : ObjectManagedBySystem(false)
{ 
    this->Parent = 0;
    this->AllowChildren = true;
    this->AllowChangeParent = true;
    this->Manager = 0;
    this->ObjectID = SceneManager::InvalidId;
    this->ObjectHidden = false;
    this->AllowHiding = true;
    this->ObjectDeletable = true;
    this->NameChangeable = true;
    this->ObjectListable = true;
    this->ObjectManagedByTracker = false;
    this->AllowManualTransformEdit = true;
    this->LocalTransform = vtkTransform::New();  // by default, we have a vtkTransform that can be manipulated manually. Could be changed in certain object types.
    this->WorldTransform = vtkSmartPointer<vtkTransform>::New();
    this->IsModifyingTransform = false;
    this->TransformModified = false;
    this->RenderLayer = 0;
    this->m_vtkConnections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->m_vtkConnections->Connect( this->LocalTransform, vtkCommand::ModifiedEvent, this, SLOT(NotifyTransformChanged()), 0, 0.0, Qt::DirectConnection );
    this->m_vtkConnections->Connect( this->WorldTransform, vtkCommand::ModifiedEvent, this, SLOT(NotifyTransformChanged()), 0, 0.0, Qt::DirectConnection );
}

SceneObject::~SceneObject() 
{
    if (this->LocalTransform)
        this->LocalTransform->UnRegister( this );
}

void SceneObject::Serialize( Serializer * ser )
{
    vtkMatrix4x4 * local = this->LocalTransform->GetMatrix();
    bool allowChildren = true;
    bool allowChangeParent = true;
    bool objectManagedBySystem = false;
    bool objectHidden = false;
    bool allowHiding = true;
    bool objectDeletable = true;
    bool nameChangeable = true;
    bool objectListable = true;
    bool allowManualTransformEdit = true;

    if( !ser->IsReader())
    {
        allowChildren = this->AllowChildren;
        allowChangeParent = this->AllowChangeParent;
        objectManagedBySystem = this->ObjectManagedBySystem;
        objectHidden = this->ObjectHidden;
        allowHiding = this->AllowHiding;
        objectDeletable = this->ObjectDeletable;
        nameChangeable = this->NameChangeable;
        objectListable = this->ObjectListable;
        allowManualTransformEdit = this->AllowManualTransformEdit;
    }
    ::Serialize( ser, "ObjectName", this->Name );
    ::Serialize( ser, "AllowChildren", allowChildren);
    ::Serialize( ser, "AllowChangeParent", allowChangeParent);
    ::Serialize( ser, "ObjectManagedBySystem", objectManagedBySystem);
    ::Serialize( ser, "ObjectHidden", objectHidden);
    ::Serialize( ser, "AllowHiding", allowHiding);
    ::Serialize( ser, "ObjectDeletable", objectDeletable);
    ::Serialize( ser, "NameChangeable", nameChangeable);
    ::Serialize( ser, "ObjectListable", objectListable);
    ::Serialize( ser, "AllowManualTransformEdit", allowManualTransformEdit);
    ::Serialize( ser, "LocalTransform", local );


    if( ser->IsReader())
    {
        this->SetCanAppendChildren(allowChildren);
        this->SetCanChangeParent(allowChangeParent);
        this->SetObjectManagedBySystem(objectManagedBySystem);
        this->SetHidden(objectHidden);
        this->SetHidable(allowHiding);
        this->SetObjectDeletable(objectDeletable);
        this->SetNameChangeable(nameChangeable);
        this->SetListable(objectListable);
        this->SetCanEditTransformManually(allowManualTransformEdit);
        this->GetLocalTransform()->Update();
        this->UpdateWorldTransform();
    }
}

void SceneObject::PostSceneRead()
{
    this->InternalPostSceneRead();
    for( int i = 0; i < Children.size(); ++i )
        Children[i]->PostSceneRead();
    emit FinishedReading();
}

void SceneObject::Export()
{
}

void SceneObject::SetName( QString name ) 
{ 
    this->Name = name;
    emit NameChanged();
}

void SceneObject::SetDataFileName( QString name ) 
{ 
    this->DataFileName = name;
    if (this->Name.isEmpty())
    {
        this->Name = name;
        emit NameChanged();
    }
}

void SceneObject::SetLocalTransform( vtkTransform * localTransform )
{
    if( localTransform == this->LocalTransform )
        return;

    if( this->LocalTransform )
    {
        this->m_vtkConnections->Disconnect( this->LocalTransform );
        this->LocalTransform->UnRegister( this );
        this->LocalTransform = 0;
    }

    if( localTransform )
    {
        this->LocalTransform = localTransform;
        this->LocalTransform->Register(this);
    }
    else
        this->LocalTransform = vtkTransform::New();

    this->m_vtkConnections->Connect( this->LocalTransform, vtkCommand::ModifiedEvent, this, SLOT(NotifyTransformChanged()), 0, 0.0, Qt::DirectConnection );

    UpdateWorldTransform();
}

vtkTransform * SceneObject::GetLocalTransform( )
{
    return this->LocalTransform;
}

vtkTransform * SceneObject::GetWorldTransform( )
{
    return this->WorldTransform;
}

void SceneObject::NotifyTransformChanged()
{
    if( this->IsModifyingTransform )
        this->TransformModified = true;
    else
    {
        WorldTransformChanged();
    }
}

#include <QDir>

QString SceneObject::GetSceneDataDirectoryForThisObject( QString baseDir )
{
    QString dataDirName = baseDir;
    dataDirName += QString("/%1_%2_data").arg( GetObjectID() ).arg( this->GetClassName() );
    QDir dir;
    if( !dir.exists( dataDirName ))
        dir.mkpath( dataDirName );
    return dataDirName;
}

void SceneObject::WorldTransformChanged()
{
    // give subclasses a chance to react
    this->InternalWorldTransformChanged();

    emit WorldTransformChangedSignal();
    for( int i = 0; i < GetNumberOfChildren(); ++i )
        this->Children[i]->WorldTransformChanged();
    emit ObjectModified();
}

void SceneObject::StartModifyingTransform()
{
    this->IsModifyingTransform = true;
}

void SceneObject::FinishModifyingTransform()
{
    this->IsModifyingTransform = false;
    if( this->TransformModified )
    {
        WorldTransformChanged();
        this->TransformModified = false;
    }
}

// Implement base class behavior: keep track of the views in which the
// object has been setup and watch for modifications (do we need that last part?).
void SceneObject::Setup( View * view )
{
	ViewContainer::iterator it = std::find( Views.begin(), Views.end(), view );
	if( it != Views.end() )
        return;

	view->Register( this );
	Views.push_back( view );

    connect( this, SIGNAL( ObjectModified() ), view, SLOT(NotifyNeedRender()) );
}

void SceneObject::Release( View * view )
{
	ViewContainer::iterator it = std::find( Views.begin(), Views.end(), view );
	if( it == Views.end() )
        return;

	this->disconnect( view );
	view->UnRegister( this );
	this->Views.erase( it );
    view->NotifyNeedRender();
}

void SceneObject::ReleaseAllViews()
{
	while( Views.size() > 0 )
	{
		View * v = Views.back();
		Release( v );
	}
}
    
void SceneObject::PreDisplaySetup() 
{
    SceneObjectVec::iterator it = this->Children.begin();
    for( ; it != this->Children.end(); ++it )
    {
        (*it)->PreDisplaySetup();
    }
}

void SceneObject::AddChild( SceneObject * child )
{
    // Add object to list of children
    if( child )
    {
        this->Children.push_back( child );
        child->Parent = this;
		child->UpdateWorldTransform();
    }
}

void SceneObject::InsertChild( SceneObject * child, int index )
{
    // Add object to list of children
    if( child )
    {
        this->Children.insert( index, child );
        child->Parent = this;
        child->UpdateWorldTransform();
    }
}

void SceneObject::RemoveChild( SceneObject * child )
{
	SceneObjectVec::iterator it = std::find( this->Children.begin(), this->Children.end(), child );
	if( it != this->Children.end() )
	{
		child->Parent = 0;
		this->Children.erase( it );
		child->UpdateWorldTransform();
	}
}

void SceneObject::UpdateWorldTransform()
{
	// reset the list of concatenated transforms
	this->WorldTransform->Identity();

	if( Parent )
        this->WorldTransform->Concatenate( Parent->WorldTransform );
    if( LocalTransform )
        this->WorldTransform->Concatenate( LocalTransform );

    WorldTransformChanged();
}

int SceneObject::GetNumberOfChildren()
{
    return this->Children.size();
}

SceneObject * SceneObject::GetChild( int index )
{
    if( index < (int)this->Children.size() )
    {
        return this->Children[ index ];
    }
    return 0;
}

SceneObject * SceneObject::GetChild( const QString & objectName )
{
    SceneObjectVec::iterator it = this->Children.begin();
    for( ; it != this->Children.end(); ++it )
    {
        if( (*it)->GetName().compare(objectName) == 0 )
        {
            return (*it);
        }
    }
    return 0;
}

int SceneObject::GetChildIndex( SceneObject * child )
{
    return this->Children.indexOf( child );
}


int SceneObject::GetChildListableIndex( SceneObject * child )
{
    int count = 0;
    for( int i = 0; i < GetNumberOfChildren(); ++i )
    {
        if( GetChild( i )->IsListable() )
        {
            if( GetChild( i ) == child )
                return count;
            ++count;
        }
    }
    return -1;
}

int SceneObject::GetObjectIndex()
{
    if( this->GetParent() )
        return this->GetParent()->GetChildIndex( this );
    return 0;  // scene root is unique
}

int SceneObject::GetObjectListableIndex()
{
    if( this->GetParent() )
        return this->GetParent()->GetChildListableIndex( this );
    return 0; // scene root is unique
}

void SceneObject::SetListable( bool l )
{
    if( !this->IsObjectInScene( ) )
        this->ObjectListable = l;
    else
        vtkErrorMacro(<< "Can't set listability for an object already in scene." );
}

bool SceneObject::DescendsFrom( SceneObject * obj )
{
    if( obj == this )
        return true;

    SceneObject * parent = GetParent();
    while( parent )
    {
        if( obj == parent )
            return true;
        parent = parent->GetParent();
    }
    return false;
}

bool SceneObject::IsUserObject()
{
    bool user = !IsManagedByTracker();
    user &= !IsManagedBySystem();
    user &= IsListable();
    return user;
}

int SceneObject::GetNumberOfListableChildren()
{
    int count = 0;
    for( int i = 0; i < GetNumberOfChildren(); ++i )
    {
        if( GetChild(i)->IsListable() )
            ++count;
    }
    return count;
}

SceneObject * SceneObject::GetListableChild( int index )
{
    int count = 0;
    for( int i = 0; i < GetNumberOfChildren(); ++i )
    {
        if( GetChild(i)->IsListable() )
        {
            if( index == count )
                return GetChild(i);
            ++count;
        }
    }
    return 0;
}

void SceneObject::AddToScene( SceneManager * man, int objectId )
{
    this->Manager = man;
    this->ObjectID = objectId;
    this->ObjectAddedToScene();
}

void SceneObject::RemoveFromScene()
{
    this->ObjectRemovedFromScene();
    this->Manager = 0;
    emit RemovingFromScene();
}

void SceneObject::MarkModified()
{
    emit ObjectModified();
}

void SceneObject::SetHiddenWithChildren( bool hide )
{
    this->ObjectHidden = hide;
    if( !this->ObjectHidden )
    {
        Show();
    }
    else
    {
        Hide();
    }
    this->SetHiddenChildren( this, this->ObjectHidden );
    if( this->Manager )
        this->Manager->SetCurrentObject(0);
}

void SceneObject::SetHiddenChildren( SceneObject * parent, bool hide )
{
    for( int i = 0; i < parent->GetNumberOfChildren(); ++i )
    {
        SceneObject * child = parent->GetChild(i);
        if ( child->IsHidable() )
            child->SetHidden( hide );
        this->SetHiddenChildren( child, hide );
    }
}

void SceneObject::SetHidden( bool h )
{
    this->ObjectHidden = h;
    if( this->GetManager() )
    {
        if( this->ObjectHidden )
            this->Hide();
        else
            this->Show();
    }
}

QWidget * SceneObject::CreateSettingsDialog( QWidget * parent )
{ 
    QVector <QWidget*> widgets;
    this->CreateSettingsWidgets( parent, &widgets );
    if (!widgets.isEmpty())
    {
        if (widgets.count() > 1 || (widgets.count() == 1 && !this->IsManagedByTracker()))
        {
            QTabWidget *topWidget = new QTabWidget(parent);
            for (int i = 0; i < widgets.count(); i++ )
                topWidget->insertTab(i, widgets[i], widgets[i]->objectName());
            if (!this->IsManagedByTracker())
                topWidget->insertTab(widgets.count(), this->CreateTransformEditWidget(parent),"Transform");
            return topWidget;
        }
        else
            return widgets.at(0);
    }
    return CreateTransformEditWidget( parent );
}

QWidget * SceneObject::CreateTransformEditWidget( QWidget * parent )
{
    TransformEditWidget * transformEditWidget = new TransformEditWidget( parent );
    transformEditWidget->SetSceneObject( this );
    return transformEditWidget;
}

void SceneObject::WorldToLocal( double worldPoint[3], double localPoint[3] )
{
    if( this->Parent )
    {
        vtkLinearTransform * inverseTrans = this->Parent->GetWorldTransform()->GetLinearInverse();
        inverseTrans->TransformPoint( worldPoint, localPoint );
    }
    else
    {
        localPoint[0] = worldPoint[0];
        localPoint[1] = worldPoint[1];
        localPoint[2] = worldPoint[2];
    }
}

void SceneObject::LocalToWorld( double localPoint[3], double worldPoint[3] )
{
    if( this->Parent )
    {
        vtkLinearTransform * transform = this->Parent->GetWorldTransform();
        transform->TransformPoint( localPoint, worldPoint );
    }
    else
    {
        worldPoint[0] = localPoint[0];
        worldPoint[1] = localPoint[1];
        worldPoint[2] = localPoint[2];
    }
}
