/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "worldobject.h"

#include "application.h"
#include "polydataobject.h"
#include "scenemanager.h"
#include "worldobjectsettingswidget.h"

WorldObject::WorldObject()
{
    this->Name            = QString( "World" );
    this->ObjectHidden    = false;
    this->AllowHiding     = false;
    this->ObjectDeletable = false;
    this->NameChangeable  = false;
}

WorldObject::~WorldObject() {}

void WorldObject::SetAxesObject( vtkSmartPointer<PolyDataObject> obj )
{
    if( obj == this->m_axesObject ) return;

    this->m_axesObject = obj;
}

void WorldObject::SetAxesHidden( bool h )
{
    if( this->m_axesObject ) this->m_axesObject->SetHidden( h );
}

bool WorldObject::AxesHidden()
{
    if( this->m_axesObject ) return this->m_axesObject->IsHidden();
    return true;
}

void WorldObject::Set3DViewFollowsReferenceVolume( bool f )
{
    Q_ASSERT( this->GetManager() );
    this->GetManager()->Set3DViewFollowingReferenceVolume( f );
}

bool WorldObject::Is3DViewFollowingReferenceVolume()
{
    Q_ASSERT( this->GetManager() );
    return this->GetManager()->Is3DViewFollowingReferenceVolume();
}

void WorldObject::SetCursorVisible( bool v )
{
    Q_ASSERT( this->GetManager() );
    this->GetManager()->SetCursorVisibility( v );
}

bool WorldObject::GetCursorVisible()
{
    Q_ASSERT( this->GetManager() );
    return this->GetManager()->GetCursorVisible();
}

void WorldObject::SetCursorColor( const QColor & c )
{
    Q_ASSERT( this->GetManager() );
    this->GetManager()->SetCursorColor( c );
}

void WorldObject::SetCursorColor( float color[3] )
{
    Q_ASSERT( this->GetManager() );
    QColor col( (int)( color[0] * 255 ), (int)( color[1] * 255 ), (int)( color[2] * 255 ) );
    this->GetManager()->SetCursorColor( col );
}

QColor WorldObject::GetCursorColor()
{
    Q_ASSERT( this->GetManager() );
    return this->GetManager()->GetCursorColor();
}

void WorldObject::SetBackgroundColor( const QColor & c )
{
    Q_ASSERT( this->GetManager() );
    double newColorfloat[3] = { 1, 1, 1 };
    newColorfloat[0]        = double( c.red() ) / 255.0;
    newColorfloat[1]        = double( c.green() ) / 255.0;
    newColorfloat[2]        = double( c.blue() ) / 255.0;
    this->GetManager()->SetViewBackgroundColor( newColorfloat );
    this->GetManager()->SetView3DBackgroundColor( newColorfloat );
}

QColor WorldObject::GetBackgroundColor()
{
    Q_ASSERT( this->GetManager() );
    double color[3];
    this->GetManager()->GetViewBackgroundColor( color );
    QColor ret( (int)( color[0] * 255 ), (int)( color[1] * 255 ), (int)( color[2] * 255 ) );
    return ret;
}

void WorldObject::Set3DBackgroundColor( const QColor & c )
{
    Q_ASSERT( this->GetManager() );
    double newColorfloat[3] = { 1, 1, 1 };
    newColorfloat[0]        = double( c.red() ) / 255.0;
    newColorfloat[1]        = double( c.green() ) / 255.0;
    newColorfloat[2]        = double( c.blue() ) / 255.0;
    this->GetManager()->SetView3DBackgroundColor( newColorfloat );
}

QColor WorldObject::Get3DBackgroundColor()
{
    Q_ASSERT( this->GetManager() );
    double color[3];
    this->GetManager()->GetView3DBackgroundColor( color );
    QColor ret( (int)( color[0] * 255 ), (int)( color[1] * 255 ), (int)( color[2] * 255 ) );
    return ret;
}

void WorldObject::Set3DInteractorStyle( InteractorStyle style )
{
    Q_ASSERT( this->GetManager() );
    this->GetManager()->Set3DInteractorStyle( style );
}

InteractorStyle WorldObject::Get3DInteractorStyle()
{
    Q_ASSERT( this->GetManager() );
    return this->GetManager()->Get3DInteractorStyle();
}

double WorldObject::Get3DCameraViewAngle()
{
    Q_ASSERT( this->GetManager() );
    return this->GetManager()->Get3DCameraViewAngle();
}

void WorldObject::Set3DCameraViewAngle( double angle )
{
    Q_ASSERT( this->GetManager() );
    return this->GetManager()->Set3DCameraViewAngle( angle );
}

void WorldObject::SetUpdateFrequency( double fps ) { Application::GetInstance().SetUpdateFrequency( fps ); }

double WorldObject::GetUpdateFrequency() { return Application::GetInstance().GetUpdateFrequency(); }

QWidget * WorldObject::CreateSettingsDialog( QWidget * parent )
{
    WorldObjectSettingsWidget * res = new WorldObjectSettingsWidget( parent );
    res->setObjectName( "WorldObjectSettingsWidget" );
    res->setAttribute( Qt::WA_DeleteOnClose );
    res->SetWorldObject( this );
    return res;
}
