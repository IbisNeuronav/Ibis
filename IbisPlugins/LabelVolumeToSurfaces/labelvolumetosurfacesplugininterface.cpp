/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#include "labelvolumetosurfacesplugininterface.h"
#include "application.h"
#include "imageobject.h"
#include "scenemanager.h"
#include <QtPlugin>
#include <QString>
#include <QtGui>
#include <QApplication>
#include <QMessageBox>
#include "vtkImageAccumulate.h"
#include "vtkPointData.h"
#include "vtkMarchingContourFilter.h"
#include "vtkDiscreteMarchingCubes.h"
#include "vtkTriangleFilter.h"
#include "vtkStripper.h"
#include "vtkWindowedSincPolyDataFilter.h"
#include "vtkImageData.h"
#include "polydataobject.h"

static double labelColors[256][3] = {{0,0,0},
                                        {1,0,0},
                                        {0,1,0},
                                        {0,0,1},
                                        {0,1,1},
                                        {1,0,1},
                                        {1,1,0},
                                        {0.541176,0.168627,0.886275},
                                        {1,0.0784314,0.576471},
                                        {0.678431,1,0.184314},
                                        {0.12549,0.698039,0.666667},
                                        {0.282353,0.819608,0.8},
                                        {0.627451,0.12549,0.941176},
                                        {1,1,1},
                                        {0.4,0,0},
                                        {0.4,0.2,0},
                                        {0.4,0.4,0},
                                        {0.2,0.4,0},
                                        {0,0.4,0},
                                        {0,0.4,0.2},
                                        {0,0.4,0.4},
                                        {0,0.2,0.4},
                                        {0,0,0.4},
                                        {0.2,0,0.4},
                                        {0.4,0,0.4},
                                        {0.4,0,0.2},
                                        {0.4,0,0},
                                        {0.4,0.2,0},
                                        {0.4,0.4,0},
                                        {0.2,0.4,0},
                                        {0,0.4,0},
                                        {0,0.4,0.2},
                                        {0,0.4,0.4},
                                        {0,0.2,0.4},
                                        {0,0,0.4},
                                        {0.2,0,0.4},
                                        {0.4,0,0.4},
                                        {0.4,0,0.2},
                                        {0.4,0,0},
                                        {0.4,0.2,0},
                                        {0.4,0.4,0},
                                        {0.2,0.4,0},
                                        {0,0.4,0},
                                        {0,0.4,0.2},
                                        {0,0.4,0.4},
                                        {0,0.2,0.4},
                                        {0,0,0.4},
                                        {0.2,0,0.4},
                                        {0.4,0,0.4},
                                        {0.4,0,0.2},
                                        {0.701961,0,0},
                                        {0.701961,0.34902,0},
                                        {0.701961,0.701961,0},
                                        {0.34902,0.701961,0},
                                        {0,0.701961,0},
                                        {0,0.701961,0.34902},
                                        {0,0.701961,0.701961},
                                        {0,0.34902,0.701961},
                                        {0,0,0.701961},
                                        {0.34902,0,0.701961},
                                        {0.701961,0,0.701961},
                                        {0.701961,0,0.34902},
                                        {0.4,0,0},
                                        {0.4,0.2,0},
                                        {0.4,0.4,0},
                                        {0.2,0.4,0},
                                        {0,0.4,0},
                                        {0,0.4,0.2},
                                        {0,0.4,0.4},
                                        {0,0.2,0.4},
                                        {0,0,0.4},
                                        {0.2,0,0.4},
                                        {0.4,0,0.4},
                                        {0.4,0,0.2},
                                        {0.54902,0,0},
                                        {0.54902,0.27451,0},
                                        {0.54902,0.54902,0},
                                        {0.27451,0.54902,0},
                                        {0,0.54902,0},
                                        {0,0.54902,0.27451},
                                        {0,0.54902,0.54902},
                                        {0,0.27451,0.54902},
                                        {0,0,0.54902},
                                        {0.27451,0,0.54902},
                                        {0.54902,0,0.54902},
                                        {0.54902,0,0.27451},
                                        {0.701961,0,0},
                                        {0.701961,0.34902,0},
                                        {0.701961,0.701961,0},
                                        {0.34902,0.701961,0},
                                        {0,0.701961,0},
                                        {0,0.701961,0.34902},
                                        {0,0.701961,0.701961},
                                        {0,0.34902,0.701961},
                                        {0,0,0.701961},
                                        {0.34902,0,0.701961},
                                        {0.701961,0,0.701961},
                                        {0.701961,0,0.34902},
                                        {0.85098,0,0},
                                        {0.85098,0.423529,0},
                                        {0.85098,0.85098,0},
                                        {0.423529,0.85098,0},
                                        {0,0.85098,0},
                                        {0,0.85098,0.423529},
                                        {0,0.85098,0.85098},
                                        {0,0.423529,0.85098},
                                        {0,0,0.85098},
                                        {0.423529,0,0.85098},
                                        {0.85098,0,0.85098},
                                        {0.85098,0,0.423529},
                                        {0.4,0,0},
                                        {0.4,0.2,0},
                                        {0.4,0.4,0},
                                        {0.2,0.4,0},
                                        {0,0.4,0},
                                        {0,0.4,0.2},
                                        {0,0.4,0.4},
                                        {0,0.2,0.4},
                                        {0,0,0.4},
                                        {0.2,0,0.4},
                                        {0.4,0,0.4},
                                        {0.4,0,0.2},
                                        {0.47451,0,0},
                                        {0.47451,0.239216,0},
                                        {0.47451,0.47451,0},
                                        {0.239216,0.47451,0},
                                        {0,0.47451,0},
                                        {0,0.47451,0.239216},
                                        {0,0.47451,0.47451},
                                        {0,0.239216,0.47451},
                                        {0,0,0.47451},
                                        {0.239216,0,0.47451},
                                        {0.47451,0,0.47451},
                                        {0.47451,0,0.239216},
                                        {0.54902,0,0},
                                        {0.54902,0.27451,0},
                                        {0.54902,0.54902,0},
                                        {0.27451,0.54902,0},
                                        {0,0.54902,0},
                                        {0,0.54902,0.27451},
                                        {0,0.54902,0.54902},
                                        {0,0.27451,0.54902},
                                        {0,0,0.54902},
                                        {0.27451,0,0.54902},
                                        {0.54902,0,0.54902},
                                        {0.54902,0,0.27451},
                                        {0.623529,0,0},
                                        {0.623529,0.313725,0},
                                        {0.623529,0.623529,0},
                                        {0.313725,0.623529,0},
                                        {0,0.623529,0},
                                        {0,0.623529,0.313725},
                                        {0,0.623529,0.623529},
                                        {0,0.313725,0.623529},
                                        {0,0,0.623529},
                                        {0.313725,0,0.623529},
                                        {0.623529,0,0.623529},
                                        {0.623529,0,0.313725},
                                        {0.701961,0,0},
                                        {0.701961,0.34902,0},
                                        {0.701961,0.701961,0},
                                        {0.34902,0.701961,0},
                                        {0,0.701961,0},
                                        {0,0.701961,0.34902},
                                        {0,0.701961,0.701961},
                                        {0,0.34902,0.701961},
                                        {0,0,0.701961},
                                        {0.34902,0,0.701961},
                                        {0.701961,0,0.701961},
                                        {0.701961,0,0.34902},
                                        {0.776471,0,0},
                                        {0.776471,0.388235,0},
                                        {0.776471,0.776471,0},
                                        {0.388235,0.776471,0},
                                        {0,0.776471,0},
                                        {0,0.776471,0.388235},
                                        {0,0.776471,0.776471},
                                        {0,0.388235,0.776471},
                                        {0,0,0.776471},
                                        {0.388235,0,0.776471},
                                        {0.776471,0,0.776471},
                                        {0.776471,0,0.388235},
                                        {0.85098,0,0},
                                        {0.85098,0.423529,0},
                                        {0.85098,0.85098,0},
                                        {0.423529,0.85098,0},
                                        {0,0.85098,0},
                                        {0,0.85098,0.423529},
                                        {0,0.85098,0.85098},
                                        {0,0.423529,0.85098},
                                        {0,0,0.85098},
                                        {0.423529,0,0.85098},
                                        {0.85098,0,0.85098},
                                        {0.85098,0,0.423529},
                                        {0.92549,0,0},
                                        {0.92549,0.462745,0},
                                        {0.92549,0.92549,0},
                                        {0.462745,0.92549,0},
                                        {0,0.92549,0},
                                        {0,0.92549,0.462745},
                                        {0,0.92549,0.92549},
                                        {0,0.462745,0.92549},
                                        {0,0,0.92549},
                                        {0.462745,0,0.92549},
                                        {0.92549,0,0.92549},
                                        {0.92549,0,0.462745},
                                        {0.4,0,0},
                                        {0.4,0.2,0},
                                        {0.4,0.4,0},
                                        {0.2,0.4,0},
                                        {0,0.4,0},
                                        {0,0.4,0.2},
                                        {0,0.4,0.4},
                                        {0,0.2,0.4},
                                        {0,0,0.4},
                                        {0.2,0,0.4},
                                        {0.4,0,0.4},
                                        {0.4,0,0.2},
                                        {0.439216,0,0},
                                        {0.439216,0.219608,0},
                                        {0.439216,0.439216,0},
                                        {0.219608,0.439216,0},
                                        {0,0.439216,0},
                                        {0,0.439216,0.219608},
                                        {0,0.439216,0.439216},
                                        {0,0.219608,0.439216},
                                        {0,0,0.439216},
                                        {0.219608,0,0.439216},
                                        {0.439216,0,0.439216},
                                        {0.439216,0,0.219608},
                                        {0.47451,0,0},
                                        {0.47451,0.239216,0},
                                        {0.47451,0.47451,0},
                                        {0.239216,0.47451,0},
                                        {0,0.47451,0},
                                        {0,0.47451,0.239216},
                                        {0,0.47451,0.47451},
                                        {0,0.239216,0.47451},
                                        {0,0,0.47451},
                                        {0.239216,0,0.47451},
                                        {0.47451,0,0.47451},
                                        {0.47451,0,0.239216},
                                        {0.513725,0,0},
                                        {0.513725,0.254902,0},
                                        {0.513725,0.513725,0},
                                        {0.254902,0.513725,0},
                                        {0,0.513725,0},
                                        {0,0.513725,0.254902},
                                        {0,0.513725,0.513725},
                                        {0,0.254902,0.513725},
                                        {0,0,0.513725},
                                        {0.254902,0,0.513725},
                                        {0.513725,0,0.513725},
                                        {0.513725,0,0.254902},
                                        {0.54902,0,0},
                                        {0,0,0}};

//Q_EXPORT_STATIC_PLUGIN2( LabelVolumeToSurfaces, LabelVolumeToSurfacesPluginInterface );

LabelVolumeToSurfacesPluginInterface::LabelVolumeToSurfacesPluginInterface()
{
}

LabelVolumeToSurfacesPluginInterface::~LabelVolumeToSurfacesPluginInterface()
{
}

void LabelVolumeToSurfacesPluginInterface::CreateObject()
{
    SceneManager  *manager = m_application->GetSceneManager();
    Q_ASSERT( manager );
    ImageObject * image = ImageObject::SafeDownCast( manager->GetCurrentObject() );
    if( image && image->IsLabelImage() )
    {  
        // Get the range of labels
        double imageRange[2];
        image->GetImageScalarRange( imageRange );
        int minLabel = (int)floor( imageRange[ 0 ] );
        if( minLabel == 0 )
            minLabel = 1;  // Usually, label 0 is a mask in minc files
        int maxLabel = (int)floor( imageRange[ 1 ] );
        int numberOfLabels = maxLabel - minLabel + 1;

        // Compute the histogram to find out which labels really exist in the volume
        vtkImageAccumulate * histogram = vtkImageAccumulate::New();
        histogram->SetInputData( image->GetImage() );
        histogram->SetComponentExtent( 0, maxLabel, 0, 0, 0, 0 );
        histogram->SetComponentOrigin( 0, 0, 0 );
        histogram->SetComponentSpacing( 1, 1, 1 );
        histogram->Update();

        // Setup filters
        vtkDiscreteMarchingCubes * contourExtractor = vtkDiscreteMarchingCubes::New();
        contourExtractor->SetInputData( image->GetImage() );
        vtkTriangleFilter * triangleFilter = vtkTriangleFilter::New();
        triangleFilter->SetInputConnection( contourExtractor->GetOutputPort() );
        vtkStripper * stripper = vtkStripper::New();
        stripper->SetInputConnection( triangleFilter->GetOutputPort() );

        unsigned int smoothingIterations = 15;
        double passBand = 0.001;
        double featureAngle = 120.0;
        vtkWindowedSincPolyDataFilter * smoother = vtkWindowedSincPolyDataFilter::New();
        smoother->SetInputConnection( stripper->GetOutputPort() );
        smoother->SetNumberOfIterations(smoothingIterations);
        smoother->BoundarySmoothingOff();
        smoother->FeatureEdgeSmoothingOff();
        smoother->SetFeatureAngle(featureAngle);
        smoother->SetPassBand(passBand);
        smoother->NonManifoldSmoothingOn();
        smoother->NormalizeCoordinatesOn();

        QProgressDialog * pd = m_application->StartProgress( 100, "Extracting surfaces..." );
        QApplication::processEvents();

        for( int i = minLabel; i <= maxLabel; ++i )
        {
            // skip label if there is no voxel from this label
            double frequency = histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(i);
            if( frequency == 0.0 )
                continue;

            contourExtractor->SetValue( 0, i );

            // Do the processing
            smoother->Update();

            // Setup a PolyDataObject with output and add it to the scene
            vtkPolyData * outCopy = vtkPolyData::New();
            outCopy->DeepCopy( smoother->GetOutput() );
            PolyDataObject * polyDataObj = PolyDataObject::New();
            QString objName = QString("Label %1").arg( i );
            polyDataObj->SetName( objName );
            polyDataObj->SetPolyData( outCopy );
            if( i < 256 )
                polyDataObj->SetColor( labelColors[i] );
            manager->AddObject( polyDataObj, image );

            // cleanup
            outCopy->Delete();
            polyDataObj->Delete();

            int progress = (int)( 100 * i / (double) numberOfLabels );
            m_application->UpdateProgress( pd, progress );
            QApplication::processEvents();
        }

        m_application->StopProgress( pd );

        // Cleanup
        contourExtractor->Delete();
        triangleFilter->Delete();
        stripper->Delete();
        smoother->Delete();
        histogram->Delete();
    }
    else
    {
        QMessageBox::warning( 0, "Error!", "Current object should be a label volume" );
    }
}
