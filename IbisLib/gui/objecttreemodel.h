/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __ObjectTreeModel_h_
#define __ObjectTreeModel_h_

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QMimeData>
#include <QStringList>

class QContextMenuEvent;
class SceneManager;
class SceneObject;


class ObjectTreeMimeData : public QMimeData
{
    Q_OBJECT

public:

    ObjectTreeMimeData( const QModelIndex & i ) : index( i ) {}
    ~ObjectTreeMimeData() {}
    virtual QStringList formats () const
    {
        QStringList l;
        l.push_back( ObjectTreeMimeType );
        return l;
    }
    virtual bool hasFormat ( const QString & mimeType ) const { return ( mimeType == ObjectTreeMimeType ); }
    const QModelIndex & GetIndex() const { return index; }

    static const QString ObjectTreeMimeType;

protected:

    const QModelIndex & index;
};


class ObjectTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
	
    ObjectTreeModel( SceneManager * man, QObject *parent = NULL );
    ~ObjectTreeModel();

    // QAbstractItemModel implementation
    QVariant data(const QModelIndex &index, int role) const;
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    // Support for drag and drop within the tree
	Qt::DropActions supportedDropActions() const;
    QStringList mimeTypes() const;
    QMimeData * mimeData( const QModelIndexList & indexes ) const;
    bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

    QModelIndex ObjectToIndex( SceneObject * obj );
    SceneObject * IndexToObject( const QModelIndex & index );

signals:
    void ObjectRenamed(QString, QString);

private slots:

    void BeginInserting( SceneObject * parent, int position );
    void EndInserting();
    void BeginRemoving(  SceneObject * parent, int position );
    void EndRemoving();

private:

    SceneManager * m_sceneManager;

};


#endif
