/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "linesfactory.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPointData.h"
#include <QtGui>

LinesFactory::LinesFactory()
{
    m_startSegment = true;
    m_color[0] = 255;
    m_color[1] = 255;
    m_color[2] = 255;
    m_color[3] = 255;

    m_pts = vtkSmartPointer<vtkPoints>::New();
    m_lines = vtkSmartPointer<vtkCellArray>::New();
    m_scalars = vtkSmartPointer<vtkUnsignedCharArray>::New();
    m_scalars->SetNumberOfComponents( 4 );
    m_scalars->SetName( "Colors" );

    m_poly = vtkSmartPointer<vtkPolyData>::New();
    m_poly->SetPoints( m_pts.GetPointer() );
    m_poly->SetLines( m_lines.GetPointer() );
    m_poly->GetPointData()->SetScalars( m_scalars.GetPointer());
}

LinesFactory::~LinesFactory()
{
}

void LinesFactory::StartNewSegment()
{
    m_startSegment = true;
}

void LinesFactory::SetColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;
    m_color[3] = a;
}

int LinesFactory::GetNumberOfPoints()
{
    return m_pts->GetNumberOfPoints();
}

void LinesFactory::SetPoint( int index, double pt[3] )
{
    Q_ASSERT( m_pts->GetNumberOfPoints() > index );
    m_pts->SetPoint( index, pt );
    m_poly->Modified();
}

double * LinesFactory::GetPoint( int index )
{
    Q_ASSERT( m_pts->GetNumberOfPoints() > index );
    return m_pts->GetPoint( index );
}

void LinesFactory::AddPoint( double x, double y, double z )
{
    AddPoint( x, y, z, m_color[0], m_color[1], m_color[2], m_color[3] );
}

void LinesFactory::AddPoint( double x, double y, double z, unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
    m_pts->InsertNextPoint( x, y, z );
    m_scalars->InsertNextTuple4( r, g, b, a );
    if( m_startSegment )
        m_startSegment = false;
    else
    {
        m_lines->InsertNextCell( 2 );
        m_lines->InsertCellPoint( m_pts->GetNumberOfPoints() - 2 );
        m_lines->InsertCellPoint( m_pts->GetNumberOfPoints() - 1 );
    }
    m_poly->Modified();
}

void LinesFactory::RemoveLast()
{
    int nbPoints = m_pts->GetNumberOfPoints();
    if( nbPoints > 0 )
    {
        m_pts->SetNumberOfPoints( nbPoints - 1 );
        m_scalars->SetNumberOfTuples( nbPoints - 1 );
        if( nbPoints > 1 )
            m_lines->SetNumberOfCells( m_lines->GetNumberOfCells() - 1 );
    }
    m_poly->Modified();
}

void LinesFactory::Clear()
{
    m_startSegment = true;
    m_pts->Reset();
    m_lines->Reset();
    m_scalars->Reset();
    m_poly->Modified();
}
