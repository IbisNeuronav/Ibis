/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __LinesFactory_h_
#define __LinesFactory_h_

class vtkPolyData;
class vtkPoints;
class vtkCellArray;
class vtkUnsignedCharArray;
#include <vtkSmartPointer.h>

class LinesFactory
{
public:
    LinesFactory();
    ~LinesFactory();

    vtkSmartPointer<vtkPolyData> GetPolyData() { return m_poly; }
    void StartNewSegment();
    bool IsStartingSegment() { return m_startSegment; }
    void SetColor( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
    int GetNumberOfPoints();
    void SetPoint( int index, double pt[ 3 ] );
    double * GetPoint( int index );
    void AddPoint( double x, double y, double z );
    void AddPoint( double x, double y, double z, unsigned char r, unsigned char g, unsigned char b, unsigned char a );
    void RemoveLast();
    void Clear();

protected:
    bool m_startSegment;
    vtkSmartPointer<vtkPolyData> m_poly;
    vtkSmartPointer<vtkPoints> m_pts;
    vtkSmartPointer<vtkCellArray> m_lines;
    vtkSmartPointer<vtkUnsignedCharArray> m_scalars;
    unsigned char m_color[ 4 ];
};

#endif
