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
#include "application.h"
#include "imageobject.h"
#include "sceneobject.h"
#include "scenemanager.h"
#include "generatedsurface.h"
#include <QtPlugin>
#include <QString>
#include <QtGui>
#include <QMessageBox>

ContourSurfacePluginInterface::ContourSurfacePluginInterface()
{
    m_generatedSurface = 0;
    SceneManager::InConstructor( "ContourSurfacePluginInterface", 0 );
}

ContourSurfacePluginInterface::~ContourSurfacePluginInterface()
{
    if( m_generatedSurface )
        m_generatedSurface ->Delete();
    SceneManager::InDestructor( "ContourSurfacePluginInterface", 0 );
}

SceneObject *ContourSurfacePluginInterface::CreateObject()
{
    SceneManager *manager = GetSceneManager();
    Q_ASSERT(manager);
    m_generatedSurface = GeneratedSurface::New();
    m_generatedSurface->SetPluginInterface( this );
    if( manager->IsLoadingScene() )
    {
        SceneManager::AddingRegistering( "ContourSurfacePluginInterface", "WorldObject", m_generatedSurface->GetReferenceCount(), m_generatedSurface );
        manager->AddObject(m_generatedSurface);
        return m_generatedSurface;
    }
    // If we have a current object we build surface now
    SceneObject *obj = manager->GetCurrentObject();
    if (obj->IsA("ImageObject"))
    {
        ImageObject *image = ImageObject::SafeDownCast(obj);
        double imageRange[2];
        image->GetImageScalarRange(imageRange);
        double contourValue = imageRange[0]+(imageRange[1]-imageRange[0])/5.0; //the best seems to be 20% of max, tested on Colin27
        m_generatedSurface->SetImageObject(image);
        m_generatedSurface->SetContourValue(contourValue);
        m_generatedSurface->SetGaussianSmoothingFlag(true);
        vtkPolyData *surface = m_generatedSurface->GenerateSurface();
        if (surface)
        {
            QString surfaceName(image->GetName());
            int n = surfaceName.lastIndexOf('.');
            if (n < 0)
                surfaceName.append("_surface");
            else
            {
                surfaceName.replace(n, surfaceName.size()-n, "_surface");
            }
            surfaceName.append(QString::number(image->GetNumberOfChildren()));
            m_generatedSurface->SetName(surfaceName);
            m_generatedSurface->SetScalarsVisible(0);
            SceneManager::AddingRegistering( "ContourSurfacePluginInterface", image->GetName(), m_generatedSurface->GetReferenceCount(), m_generatedSurface );
            manager->AddObject(m_generatedSurface, image);
            manager->SetCurrentObject( m_generatedSurface );
        }
        return m_generatedSurface;
    }
    QMessageBox::warning( 0, "Error!", "Current object should be an ImageObject" );
    return NULL;
}
bool ContourSurfacePluginInterface::CanBeActivated()
{
    if( this->GetSceneManager()->GetCurrentObject()->IsA( "ImageObject" ) )
        return true;
    return false;
}

