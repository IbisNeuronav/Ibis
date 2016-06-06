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
#include "scenemanager.h"
#include "generatedsurface.h"
#include <QtPlugin>
#include <QString>
#include <QtGui>

//Q_EXPORT_STATIC_PLUGIN2( ContourSurface, ContourSurfacePluginInterface );

ContourSurfacePluginInterface::ContourSurfacePluginInterface()// : m_contourSurfaceWidget(0)
{
}

ContourSurfacePluginInterface::~ContourSurfacePluginInterface()
{
}

void ContourSurfacePluginInterface::CreateObject()
{
    SceneManager *manager = m_application->GetSceneManager();
    Q_ASSERT(manager);
    SceneObject *obj = manager->GetCurrentObject();
    if (obj->IsA("ImageObject"))
    {
        ImageObject *image = ImageObject::SafeDownCast(obj);
        double imageRange[2];
        image->GetImageScalarRange(imageRange);
        double contourValue = imageRange[0]+(imageRange[1]-imageRange[0])/5.0; //the best seems to be 20% of max, tested on Colin27
        GeneratedSurface *generatedSurface = GeneratedSurface::New();
        generatedSurface->SetImageObject(image);
        generatedSurface->SetContourValue(contourValue);
        generatedSurface->SetGaussianSmoothingFlag(true);
        vtkPolyData *surface = generatedSurface->GenerateSurface();
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
            generatedSurface->SetName(surfaceName);
            generatedSurface->SetScalarsVisible(0);
            manager->AddObject(generatedSurface, image);
        }
        generatedSurface->Delete();
    }
}
