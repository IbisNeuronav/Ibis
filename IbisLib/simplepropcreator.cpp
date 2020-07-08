/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "simplepropcreator.h"
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkNew.h>
#include "vtkCircleWithCrossSource.h"

vtkProp3D * SimplePropCreator::CreateLine( double start[3], double end[3], double color[4] )
{
    vtkPoints * pts = vtkPoints::New();
    pts->InsertNextPoint( start );
    pts->InsertNextPoint( end );

    static vtkIdType lineIndex[1][2]= { {0,1} };
    vtkCellArray * line = vtkCellArray::New();
    line->InsertNextCell( 2, lineIndex[0] );

    vtkPolyData * poly = vtkPolyData::New();
    poly->SetPoints( pts );
    pts->Delete();
    poly->SetLines( line );
    line->Delete();

    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
    mapper->SetInputData( poly );

    vtkActor * prop = vtkActor::New();
    prop->SetMapper( mapper );
    prop->GetProperty()->SetColor( color );
    mapper->Delete();

    return prop;
}

vtkProp3D * SimplePropCreator::CreatePath( std::vector< Vec3 > & points, double color[4] )
{
    vtkPoints * pts = vtkPoints::New();
    if( points.size() > 1 )
    {
        for( int i = 0; i < points.size(); ++i )
            pts->InsertNextPoint( points[i].Ref() );
    }

    vtkCellArray * line = vtkCellArray::New();
    if( points.size() > 1 )
    {
        for( int i = 0; i < points.size() - 1; ++i )
        {
            line->InsertNextCell( 2 );
            line->InsertCellPoint( i );
            line->InsertCellPoint( i + 1 );
        }
    }

    vtkPolyData * poly = vtkPolyData::New();
    poly->SetPoints( pts );
    pts->Delete();
    poly->SetLines( line );
    line->Delete();

    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
    mapper->SetInputData( poly );

    vtkActor * prop = vtkActor::New();
    prop->SetMapper( mapper );
    prop->GetProperty()->SetColor( color );
    mapper->Delete();

    return prop;
}

vtkProp3D * SimplePropCreator::CreateSphere( double center[3], double radius, double color[4] )
{
    vtkSphereSource * source = vtkSphereSource::New();
    source->SetCenter( center );
    source->SetRadius( radius );
    source->Update();

    vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
    mapper->SetInputData( source->GetOutput() );
    source->Delete();

    vtkActor * prop = vtkActor::New();
    prop->SetMapper( mapper );
    prop->GetProperty()->SetColor( color );
    mapper->Delete();

    return prop;
}

vtkProp3D * SimplePropCreator::CreateTarget( double center[3], double radius, double color[4] )
{
    vtkNew<vtkCircleWithCrossSource> source;
    source->SetRadius( radius );
    source->SetCenter( center );
    source->SetResolution( 4 );
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection( source->GetOutputPort() );
    vtkActor * actor = vtkActor::New();
    actor->SetMapper( mapper.GetPointer() );
    actor->GetProperty()->SetColor( color );
    return actor;
}
