/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "contoursurfaceplugininterface.h"

#include <QMessageBox>
#include <QString>
#include <QtGui>
#include <QtPlugin>

#include "generatedsurface.h"
#include "ibisapi.h"
#include "imageobject.h"
#include "sceneobject.h"
#include "view.h"

ContourSurfacePluginInterface::ContourSurfacePluginInterface() {}

ContourSurfacePluginInterface::~ContourSurfacePluginInterface() {}

SceneObject * ContourSurfacePluginInterface::CreateObject()
{
    IbisAPI * ibisAPI = GetIbisAPI();
    Q_ASSERT( ibisAPI );
    m_generatedSurface = vtkSmartPointer<GeneratedSurface>::New();
    m_generatedSurface->SetPluginInterface( this );
    connect( ibisAPI, SIGNAL( ObjectRemoved( int ) ), this, SLOT( OnObjectRemoved( int ) ) );
    if( ibisAPI->IsLoadingScene() )
    {
        ibisAPI->AddObject( m_generatedSurface );
        return m_generatedSurface;
    }
    // If we have a current object we build surface now
    SceneObject * obj = ibisAPI->GetCurrentObject();
    if( obj->IsA( "ImageObject" ) )
    {
        ImageObject * image = ImageObject::SafeDownCast( obj );
        double imageRange[2];
        image->GetImageScalarRange( imageRange );
        double contourValue = imageRange[0] + ( imageRange[1] - imageRange[0] ) /
                                                  5.0;  // the best seems to be 20% of max, tested on Colin27
        m_generatedSurface->SetImageObjectID( image->GetObjectID() );
        m_generatedSurface->SetContourValue( contourValue );
        m_generatedSurface->SetGaussianSmoothingFlag( true );
        if( m_generatedSurface->GenerateSurface() )
        {
            QString surfaceName( image->GetName() );
            int n = surfaceName.lastIndexOf( '.' );
            if( n < 0 )
                surfaceName.append( "_surface" );
            else
            {
                surfaceName.replace( n, surfaceName.size() - n, "_surface" );
            }
            surfaceName.append( QString::number( image->GetNumberOfChildren() ) );
            m_generatedSurface->SetName( surfaceName );
            m_generatedSurface->SetScalarsVisible( 0 );
            ibisAPI->AddObject( m_generatedSurface, image );
            ibisAPI->SetCurrentObject( m_generatedSurface );
        }
        return m_generatedSurface;
    }
    QMessageBox::warning( 0, "Error!", "Current object should be an ImageObject" );
    return NULL;
}
bool ContourSurfacePluginInterface::CanBeActivated()
{
    if( this->GetIbisAPI()->GetCurrentObject()->IsA( "ImageObject" ) ) return true;
    return false;
}

void ContourSurfacePluginInterface::OnObjectRemoved( int objID )
{
    if( objID == m_generatedSurface->GetObjectID() )
    {
        // refresh 3D view
        this->GetIbisAPI()->GetMain3DView()->NotifyNeedRender();
    }
}
