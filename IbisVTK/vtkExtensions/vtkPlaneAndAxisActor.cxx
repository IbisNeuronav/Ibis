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

#include "vtkPlaneAndAxisActor.h"
#include "vtkSetGet.h"
#include "vtkObjectFactory.h"
#include "vtkAxes.h"
#include "vtkPolyDataMapper.h"
#include "vtkCaptionActor2D.h"
#include "vtkPolyData.h"
#include "vtkPlaneSource.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkAssembly.h"
#include "vtkProperty.h"
#include "vtkCaptionActor2D.h"


vtkStandardNewMacro(vtkPlaneAndAxisActor);


void vtkPlaneAndAxisActor::PrintSelf( ostream & os, vtkIndent indent )
{
    os << indent << "Size: " << Size << endl << indent << "NumberOfSubdivisions: " << NumberOfSubdivisions << endl;
}


vtkPlaneAndAxisActor::vtkPlaneAndAxisActor()
{
    //==============================
    // parameters of the space
    //==============================
    Size = 1000;
    float halfSize = Size / 2;
    int   NumberOfSubdivisions = 10;

    //==============================
    // axes
    //==============================
    m_axes = vtkAxes::New();
    m_axes->SetScaleFactor( halfSize );
    m_axesMapper = vtkPolyDataMapper::New();
    m_axesMapper->SetInputData( m_axes->GetOutput() );
    m_axesActor = vtkActor::New();
    m_axesActor->GetProperty()->SetLineWidth(2.0);
    m_axesActor->GetProperty()->SetInterpolationToFlat();
    m_axesActor->SetMapper( m_axesMapper );

    //==============================
    // grid
    //==============================

    // Generate points
    vtkPoints * newPoints = vtkPoints::New();
    int numberOfPoints = ( NumberOfSubdivisions + 1 ) * 4;
    float increment = Size / NumberOfSubdivisions;
    newPoints->SetNumberOfPoints( numberOfPoints );
    int i = 0;
    int j = 0;
    double coord1;
    for( i = 0; i < NumberOfSubdivisions + 1; i++ )
    {
        coord1 = -halfSize + i * increment;
        newPoints->SetPoint( 2 * i, coord1, -halfSize, 0.0 ); 
        newPoints->SetPoint( 2 * i + 1, coord1, halfSize, 0.0 ); 
    }
    for( j = 0; j < NumberOfSubdivisions + 1; j++, i++ )
    {
        coord1 = -halfSize + j * increment;
        newPoints->SetPoint( 2 * i, -halfSize, coord1, 0 ); 
        newPoints->SetPoint( 2 * i + 1, halfSize, coord1, 0 );  
    }

    // generate lines
    vtkCellArray * newLines = vtkCellArray::New();
    int sizeLines = newLines->EstimateSize( 2, ( NumberOfSubdivisions + 1 ) * 2 );
    newLines->Allocate( sizeLines );
    for ( i = 0; i < ( NumberOfSubdivisions + 1 ) * 2; i++ )
    {
        newLines->InsertNextCell( 2 );
        newLines->InsertCellPoint( 2 * i );
        newLines->InsertCellPoint( 2 * i + 1 );
    }
    
    // create the poly data
    m_gridPolyData = vtkPolyData::New();
    m_gridPolyData->SetPoints( newPoints );
    m_gridPolyData->SetLines( newLines );
    
    // clean up unused lines and points
    newPoints->Delete();
    newLines->Delete();
    
    // grid mapper
    m_gridMapper = vtkPolyDataMapper::New();
    m_gridMapper->SetInputData( m_gridPolyData );

    // grid actors
    m_gridActor = vtkActor::New();
    m_gridActor->SetMapper( m_gridMapper );
    m_gridActor->GetProperty()->SetRepresentationToWireframe();
    m_gridActor->GetProperty()->SetLineWidth(1.0);
    m_gridActor->GetProperty()->SetInterpolationToFlat();
    m_gridActor->GetProperty()->BackfaceCullingOff();


    //==============================
    // transparent plane
    //==============================
    m_planeSource = vtkPlaneSource::New();
    m_planeSource->SetOrigin( halfSize, -halfSize, 0 );
    m_planeSource->SetPoint1( -halfSize, -halfSize, 0 );
    m_planeSource->SetPoint2( halfSize, halfSize, 0 );
    
    m_planeMapper = vtkPolyDataMapper::New();
    m_planeMapper->SetInputData( m_planeSource->GetOutput() );

    m_planeActor = vtkActor::New();
    m_planeActor->SetMapper( m_planeMapper );
    m_planeActor->GetProperty()->SetRepresentationToSurface ();
    m_planeActor->GetProperty()->SetOpacity( 0.3 );
    m_planeActor->GetProperty()->SetInterpolationToFlat();
    m_planeActor->GetProperty()->BackfaceCullingOff();


    //==============================
    // axis text labels
    //==============================
    m_captionx = vtkCaptionActor2D::New();
    m_captionx->SetCaption( "X" );
    m_captionx->SetAttachmentPoint( halfSize, 0, 0 );
    m_captionx->BorderOff();
    m_captionx->LeaderOff();
    m_captionx->SetWidth( 0.02 );
    m_captionx->SetHeight( 0.02 );

    m_captiony = vtkCaptionActor2D::New();
    m_captiony->SetCaption( "Y" );
    m_captiony->SetAttachmentPoint( 0, halfSize, 0 );
    m_captiony->BorderOff();
    m_captiony->LeaderOff();
    m_captiony->SetWidth( 0.02 );
    m_captiony->SetHeight( 0.02 );

    m_captionz = vtkCaptionActor2D::New();
    m_captionz->SetCaption( "Z" );
    m_captionz->SetAttachmentPoint( 0, 0, halfSize );
    m_captionz->BorderOff();
    m_captionz->LeaderOff();
    m_captionz->SetWidth( 0.02 );
    m_captionz->SetHeight( 0.02 );
}


vtkPlaneAndAxisActor::~vtkPlaneAndAxisActor()
{
    m_axes->Delete();         
    m_axesMapper->Delete();   
    m_axesActor->Delete();    
    m_gridPolyData->Delete(); 
    m_gridMapper->Delete();   
    m_gridActor->Delete();    
    m_planeSource->Delete();  
    m_planeMapper->Delete();  
    m_planeActor->Delete();   
    m_captionx->Delete();     
    m_captiony->Delete();     
    m_captionz->Delete();
}


void vtkPlaneAndAxisActor::SetRenderer( vtkRenderer * ren )
{
    ren->AddViewProp( m_axesActor );
    ren->AddViewProp( m_gridActor );
    ren->AddViewProp( m_planeActor );
    ren->AddViewProp( m_captionx );
    ren->AddViewProp( m_captiony );
    ren->AddViewProp( m_captionz );
}
