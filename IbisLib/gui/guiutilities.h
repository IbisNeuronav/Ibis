/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __GuiUtilities_h_
#define __GuiUtilities_h_

#include <QList>
#include <QMap>

#include "ibisapi.h"

class SceneObject;
class QComboBox;

class GuiUtilities
{
public:
    template <class ObjectType>
    static void UpdateSceneObjectComboBox( QComboBox * cb, QList<ObjectType *> allObjs, int currentObjectId )
    {
        cb->blockSignals( true );
        cb->clear();
        for( int i = 0; i < allObjs.size(); ++i )
        {
            cb->addItem( allObjs[ i ]->GetName(), QVariant( allObjs[ i ]->GetObjectID() ) );
            if( allObjs[ i ]->GetObjectID() == currentObjectId ) cb->setCurrentIndex( i );
        }
        if( allObjs.size() == 0 ) cb->addItem( "None", QVariant( IbisAPI::InvalidId ) );
        cb->blockSignals( false );
    }

    template <class ObjectType>
    static void UpdateObjectComboBox( QComboBox * cb, QMap<ObjectType *, int> allObjs, int currentObjectId )
    {
        cb->blockSignals( true );
        cb->clear();
        int index = 0;
        foreach( int id, allObjs.values() )
        {
            cb->addItem( allObjs.key( id )->GetName(), QVariant( id ) );
            if( id == currentObjectId ) cb->setCurrentIndex( index );
            index++;
        }
        if( allObjs.size() == 0 ) cb->addItem( "None", QVariant( IbisAPI::InvalidId ) );
        cb->blockSignals( false );
    }

    static int ObjectIdFromObjectComboBox( QComboBox * cb, int index )
    {
        QVariant v   = cb->itemData( index );
        bool ok      = false;
        int objectId = v.toInt( &ok );
        if( !ok ) objectId = IbisAPI::InvalidId;
        return objectId;
    }

    static int ObjectComboBoxIndexFromObjectId( QComboBox * cb, int id )
    {
        for( int index = 0; index < cb->count(); index++ )
        {
            QVariant v   = cb->itemData( index );
            bool ok      = false;
            int objectId = v.toInt( &ok );
            if( ok && objectId == id ) return index;
        }
        return IbisAPI::InvalidId;
    }
};

#endif
