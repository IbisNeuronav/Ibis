/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "lookuptablemanager.h"
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunctionLookupTable.h>

LookupTableManager::LookupTableManager()
{
}

void LookupTableManager::CreateLookupTable(const QString tableName, double range[2], vtkSmartPointer<vtkPiecewiseFunctionLookupTable> ct )
{
    ct->SetScaleToLinear();
    ct->SetRange( range );
    ct->RemoveAllPoints();

    if ( tableName == "Hot Metal" )
    {
        ct->AddColorPoint( 0.0,  0.0, 0.0, 0.0 );
        ct->AddColorPoint( 0.25, 0.5, 0.0, 0.0 );
        ct->AddColorPoint( 0.5,  1.0, 0.5, 0.0 );
        ct->AddColorPoint( 0.75, 1.0, 1.0, 0.5 );
        ct->AddColorPoint( 1.0,  1.0, 1.0, 1.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if( tableName == "Hot Metal Neg")
    {
        ct->AddColorPoint( 0.0,  1.0, 1.0, 1.0 );
        ct->AddColorPoint( 0.25, 1.0, 1.0, 0.5 );
        ct->AddColorPoint( 0.5,  1.0, 0.5, 0.0 );
        ct->AddColorPoint( 0.75, 0.5, 0.0, 0.0 );
        ct->AddColorPoint( 1.0,  0.0, 0.0, 0.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if( tableName == "Cold Metal")
    {
        ct->AddColorPoint( 0.0,  0.0, 0.0, 0.0 );
        ct->AddColorPoint( 0.25, 0.0, 0.0, 0.5 );
        ct->AddColorPoint( 0.5,  0.0, 0.5, 1.0 );
        ct->AddColorPoint( 0.75, 0.5, 1.0, 1.0 );
        ct->AddColorPoint( 1.0,  1.0, 1.0, 1.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if( tableName == "Cold Metal Neg")
    {
        ct->AddColorPoint( 0.0,  1.0, 1.0, 1.0 );
        ct->AddColorPoint( 0.25, 0.5, 1.0, 1.0 );
        ct->AddColorPoint( 0.5,  0.0, 0.5, 1.0 );
        ct->AddColorPoint( 0.75, 0.0, 0.0, 0.5 );
        ct->AddColorPoint( 1.0,  0.0, 0.0, 0.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if( tableName == "Contour" )
    {
        ct->AddColorPoint( 0.0,   0.0, 0.0, 0.3 );
        ct->AddColorPoint( 0.166, 0.0, 0.0, 1.0 );
        ct->AddColorPoint( 0.166, 0.0, 0.3, 0.3 );
        ct->AddColorPoint( 0.333, 0.0, 1.0, 1.0 );
        ct->AddColorPoint( 0.333, 0.0, 0.3, 0.0 );
        ct->AddColorPoint( 0.5,   0.0, 1.0, 0.0 );
        ct->AddColorPoint( 0.5,   0.3, 0.3, 0.0 );
        ct->AddColorPoint( 0.666, 1.0, 1.0, 0.0 );
        ct->AddColorPoint( 0.666, 0.3, 0.0, 0.0 );
        ct->AddColorPoint( 0.833, 1.0, 0.0, 0.0 );
        ct->AddColorPoint( 0.833, 0.3, 0.3, 0.3 );
        ct->AddColorPoint( 1.0,   1.0, 1.0, 1.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if ( tableName == "Spectrum" )
    {
        ct->AddColorPoint( 0.00, 0.0000,0.0000,0.0000 );
        ct->AddColorPoint( 0.05, 0.4667,0.0000,0.5333 );
        ct->AddColorPoint( 0.10, 0.5333,0.0000,0.6000 );
        ct->AddColorPoint( 0.15, 0.0000,0.0000,0.6667 );
        ct->AddColorPoint( 0.20, 0.0000,0.0000,0.8667 );
        ct->AddColorPoint( 0.25, 0.0000,0.4667,0.8667 );
        ct->AddColorPoint( 0.30, 0.0000,0.6000,0.8667 );
        ct->AddColorPoint( 0.35, 0.0000,0.6667,0.6667 );
        ct->AddColorPoint( 0.40, 0.0000,0.6667,0.5333 );
        ct->AddColorPoint( 0.45, 0.0000,0.6000,0.0000 );
        ct->AddColorPoint( 0.50, 0.0000,0.7333,0.0000 );
        ct->AddColorPoint( 0.55, 0.0000,0.8667,0.0000 );
        ct->AddColorPoint( 0.60, 0.0000,1.0000,0.0000 );
        ct->AddColorPoint( 0.65, 0.7333,1.0000,0.0000 );
        ct->AddColorPoint( 0.70, 0.9333,0.9333,0.0000 );
        ct->AddColorPoint( 0.75, 1.0000,0.8000,0.0000 );
        ct->AddColorPoint( 0.80, 1.0000,0.6000,0.0000 );
        ct->AddColorPoint( 0.85, 1.0000,0.0000,0.0000 );
        ct->AddColorPoint( 0.90, 0.8667,0.0000,0.0000 );
        ct->AddColorPoint( 0.95, 0.8000,0.0000,0.0000 );
        ct->AddColorPoint( 1.00, 0.8000,0.8000,0.8000 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if ( tableName == "Red" )
    {
        ct->AddColorPoint( 0.0, 0.0, 0.0, 0.0 );
        ct->AddColorPoint( 1.0, 1.0, 0.0, 0.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if ( tableName == "Green" )
    {
        ct->AddColorPoint( 0.0, 0.0, 0.0, 0.0 );
        ct->AddColorPoint( 1.0, 0.0, 1.0, 0.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if ( tableName == "Blue" )
    {
        ct->AddColorPoint( 0.0, 0.0, 0.0, 0.0 );
        ct->AddColorPoint( 1.0, 0.0, 0.0, 1.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if( tableName == "Path Planning" )
    {
        ct->AddColorPoint( 0.0,  0.0, 1.0, 0.0); // green
        //ct->AddColorPoint( 0.1,  0.0, 1.0, 0.0); // green
        ct->AddColorPoint( 0.25,  1.0, 1.0, 0.0); // yellow <-- I would really like to be able to move this point on the GUI)
        ct->AddColorPoint( 0.98, 1.0, 0.0, 0.0); // red
        ct->AddColorPoint( 1.0,  0.5, 0.5, 0.5); // gray (for unprocessed area)
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if( tableName == "Jacobian" )
    {
        ct->AddColorPoint(0.000,0.8,1.0,1.0);
        ct->AddColorPoint(0.125,0.4,0.9,1.0);
        ct->AddColorPoint(0.250,0.0,0.6,1.0);
        ct->AddColorPoint(0.375,0.0,0.2,0.5);
        ct->AddColorPoint(0.500,0.0,0.0,0.0);
        ct->AddColorPoint(0.625,0.5,0.0,0.0);
        ct->AddColorPoint(0.750,1.0,0.4,0.0);
        ct->AddColorPoint(0.825,1.0,0.8,0.4);
        ct->AddColorPoint(1.000,1.0,0.8,0.8);
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }
    else if( tableName == "Mask" )
    {
        ct->AddColorPoint( 0.0, 1.0, 1.0, 1.0 );
        ct->AddColorPoint( 1.0, 1.0, 1.0, 1.0 );
        ct->AddAlphaPoint( 0.0, 0.0 );
        ct->AddAlphaPoint( 1.0, 1.0 );
    }
    else // Gray
    {
        ct->AddColorPoint( 0.0, 0.0, 0.0, 0.0 );
        ct->AddColorPoint( 1.0, 1.0, 1.0, 1.0 );
        ct->AddAlphaPoint(0.0, 1.0);
        ct->AddAlphaPoint(1.0, 1.0);
    }

    ct->Build();
}

int LookupTableManager::GetNumberOfTemplateLookupTables() const
{
    return 13;
}

const QString LookupTableManager::GetTemplateLookupTableName(int index)
{
    switch (index)
    {
    case 0:
        return "Gray";
    case 1:
        return "Hot Metal";
    case 2:
        return "Hot Metal Neg";
    case 3:
        return "Cold Metal";
    case 4:
        return "Cold Metal Neg";
    case 5:
        return "Contour";
    case 6:
        return "Spectrum";
    case 7:
        return "Red";
    case 8:
        return "Green";
    case 9:
        return "Blue";
    case 10:
        return "Path Planning";
    case 11:
        return "Jacobian";
    case 12:
        return "Mask";
    }
    return "Gray";
}

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

void LookupTableManager::CreateLabelLookupTable( vtkSmartPointer<vtkLookupTable> lut )
{
    lut->SetScaleToLinear();
    double newRange[2] = { 0, 255 };  // for label volumes, we assume integer volumes between 0 and 255
    lut->SetRange( newRange );
    lut->SetNumberOfTableValues( 256 );
    lut->Build();
    lut->SetTableValue( 0, 0.0, 0.0, 0.0, 0.0 );  // only value with alpha
    for( int i = 1; i < 256; ++i )
        lut->SetTableValue( i, labelColors[i][0], labelColors[i][1], labelColors[i][2] );
}
