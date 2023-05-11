/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "objecttreemodel.h"

#include <QtGui>

#include "imageobject.h"
#include "scenemanager.h"
#include "sceneobject.h"

const QString ObjectTreeMimeData::ObjectTreeMimeType( "Ibis-ObjectPointer" );

ObjectTreeModel::ObjectTreeModel( SceneManager * man, QObject * parent )
    : QAbstractItemModel( parent ), m_sceneManager( man )
{
    connect( m_sceneManager, SIGNAL( StartAddingObject( SceneObject *, int ) ), this,
             SLOT( BeginInserting( SceneObject *, int ) ) );
    connect( m_sceneManager, SIGNAL( FinishAddingObject() ), this, SLOT( EndInserting() ) );
    connect( m_sceneManager, SIGNAL( StartRemovingObject( SceneObject *, int ) ), this,
             SLOT( BeginRemoving( SceneObject *, int ) ) );
    connect( m_sceneManager, SIGNAL( FinishRemovingObject() ), this, SLOT( EndRemoving() ) );
    connect( m_sceneManager, SIGNAL( ObjectAttributesChanged( SceneObject * ) ), this,
             SLOT( UpdateObjectAttributes( SceneObject * ) ) );
    connect( this, SIGNAL( ObjectRenamed( QString, QString ) ), m_sceneManager,
             SLOT( EmitSignalObjectRenamed( QString, QString ) ) );
}

ObjectTreeModel::~ObjectTreeModel() { disconnect( m_sceneManager ); }

int ObjectTreeModel::columnCount( const QModelIndex & /*parent*/ ) const { return 1; }

QVariant ObjectTreeModel::data( const QModelIndex & index, int role ) const
{
    if( !index.isValid() ) return QVariant();

    if( role == Qt::DisplayRole || role == Qt::EditRole )
    {
        SceneObject * item = static_cast<SceneObject *>( index.internalPointer() );
        return QVariant( item->GetName() );
    }
    else if( role == Qt::CheckStateRole )
    {
        if( index.column() == 0 )
        {
            SceneObject * item = static_cast<SceneObject *>( index.internalPointer() );
            if( item->IsHidable() )
            {
                if( item->IsHidden() )
                    return QVariant( Qt::Unchecked );
                else
                    return QVariant( Qt::Checked );
            }
        }
        return QVariant();
    }
    else if( role == Qt::ForegroundRole )
    {
        SceneObject * item = static_cast<SceneObject *>( index.internalPointer() );
        if( m_sceneManager->GetReferenceDataObject() == item ) return QVariant( QBrush( Qt::red ) );
        return QVariant();
    }
    else
    {
        return QVariant();
    }
}

bool ObjectTreeModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
    if( role != Qt::EditRole && role != Qt::CheckStateRole ) return false;

    if( !index.isValid() ) return false;

    SceneObject * item = static_cast<SceneObject *>( index.internalPointer() );
    if( role == Qt::CheckStateRole )
    {
        Qt::CheckState state = static_cast<Qt::CheckState>( value.toUInt() );
        if( state == Qt::Checked )
            item->SetHidden( false );
        else
            item->SetHidden( true );
        emit dataChanged( index, index );
    }
    else
    {
        QString oldName( item->GetName() );
        item->SetName( value.toString() );
        emit dataChanged( index, index );
        emit ObjectRenamed( oldName, value.toString() );
    }

    return true;
}

Qt::ItemFlags ObjectTreeModel::flags( const QModelIndex & index ) const
{
    if( !index.isValid() ) return 0;

    Qt::ItemFlags f    = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    SceneObject * item = static_cast<SceneObject *>( index.internalPointer() );
    if( item->IsHidable() ) f |= Qt::ItemIsUserCheckable;
    if( item->CanChangeName() ) f |= Qt::ItemIsEditable;
    if( item->CanAppendChildren() ) f |= Qt::ItemIsDropEnabled;
    if( item->CanChangeParent() ) f |= Qt::ItemIsDragEnabled;

    return f;
}

QVariant ObjectTreeModel::headerData( int /*section*/, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) return QVariant( QString( "Object Name" ) );

    return QVariant();
}

QModelIndex ObjectTreeModel::index( int row, int column, const QModelIndex & parent ) const
{
    if( !hasIndex( row, column, parent ) ) return QModelIndex();

    if( !parent.isValid() )
    {
        // we want one of the root items
        return createIndex( row, column, m_sceneManager->GetSceneRoot() );
    }

    SceneObject * parentItem = static_cast<SceneObject *>( parent.internalPointer() );
    SceneObject * childItem  = parentItem->GetListableChild( row );
    if( childItem )
        return createIndex( row, column, childItem );
    else
        return QModelIndex();
}

QModelIndex ObjectTreeModel::parent( const QModelIndex & index ) const
{
    int row    = index.row();
    int column = index.column();

    if( !index.isValid() ) return QModelIndex();

    SceneObject * childItem  = static_cast<SceneObject *>( index.internalPointer() );
    SceneObject * parentItem = childItem->GetParent();

    if( !parentItem ) return QModelIndex();

    return createIndex( parentItem->GetObjectListableIndex(), 0, parentItem );
}

int ObjectTreeModel::rowCount( const QModelIndex & parent ) const
{
    if( parent.column() > 0 ) return 0;

    // no parent, we are at root and we have only one root (the world node)
    if( !parent.isValid() )
    {
        return 1;
    }

    SceneObject * parentItem = static_cast<SceneObject *>( parent.internalPointer() );
    return parentItem->GetNumberOfListableChildren();
}

Qt::DropActions ObjectTreeModel::supportedDropActions() const { return Qt::MoveAction; }

QStringList ObjectTreeModel::mimeTypes() const
{
    QStringList l;
    l.push_back( ObjectTreeMimeData::ObjectTreeMimeType );
    return l;
}

QMimeData * ObjectTreeModel::mimeData( const QModelIndexList & indexes ) const
{
    if( indexes.size() > 0 )
    {
        // we take only the first element as we don't support more anyways
        ObjectTreeMimeData * data = new ObjectTreeMimeData( indexes.at( 0 ) );
        return data;
    }
    return 0;
}

bool ObjectTreeModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column,
                                    const QModelIndex & parent )
{
    const ObjectTreeMimeData * treeData = qobject_cast<const ObjectTreeMimeData *>( data );
    if( treeData )
    {
        const QModelIndex & from = treeData->GetIndex();
        SceneObject * objBackup  = IndexToObject( from );
        SceneObject * newParent  = IndexToObject( parent );
        if( objBackup && newParent )
        {
            // No need to move
            if( objBackup->GetParent() == newParent ) return false;

            // Make sure row is valid ( row == -1 if droping directly on the parent )
            if( row == -1 )  // in this case, append to the end
                row = newParent->GetNumberOfChildren();

            // Do the move
            m_sceneManager->ChangeParent( objBackup, newParent, row );
            return true;
        }
    }
    return false;
}

void ObjectTreeModel::BeginInserting( SceneObject * parent, int position )
{
    QModelIndex parentIndex = ObjectToIndex( parent );
    beginInsertRows( parentIndex, position, position );
}

void ObjectTreeModel::EndInserting() { endInsertRows(); }

void ObjectTreeModel::BeginRemoving( SceneObject * parent, int position )
{
    QModelIndex parentIndex = ObjectToIndex( parent );
    beginRemoveRows( parentIndex, position, position );
}

void ObjectTreeModel::EndRemoving() { endRemoveRows(); }

QModelIndex ObjectTreeModel::ObjectToIndex( SceneObject * obj )
{
    if( !obj ) return QModelIndex();
    int row = obj->GetObjectListableIndex();
    if( row != -1 ) return createIndex( row, 0, obj );
    return QModelIndex();
}

SceneObject * ObjectTreeModel::IndexToObject( const QModelIndex & index )
{
    if( index.isValid() )
    {
        SceneObject * item = static_cast<SceneObject *>( index.internalPointer() );
        return item;
    }
    return 0;
}

void ObjectTreeModel::UpdateObjectAttributes( SceneObject * obj )
{
    emit dataChanged( ObjectToIndex( obj ), ObjectToIndex( obj ) );
}
