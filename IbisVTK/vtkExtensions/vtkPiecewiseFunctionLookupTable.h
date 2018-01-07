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

// .NAME vtkPiecewiseFunctionLookupTable - Lookup table built using piecewise function for every rgba component
// .SECTION Description
// vtkPiecewiseFunctionLookupTable is a lookup table that is built using
// 4 different piecewise function (r,g,b,a). It samples the each of the
// functions to build its internal table.
//
// .SECTION See Also
// vtkColorTransferFunction, vtkPiecewiseFunction, vtkLookupTable

#ifndef __vtkPiecewiseFunctionLookupTable_h
#define __vtkPiecewiseFunctionLookupTable_h

#include "vtkLookupTable.h"

class vtkColorTransferFunction;
class vtkPiecewiseFunction;


class vtkPiecewiseFunctionLookupTable : public vtkLookupTable
{

public:

  // Description:
  // Construct with range=[0,1]; and hsv ranges set up for rainbow color table
  // (from red to blue).
  static vtkPiecewiseFunctionLookupTable *New();

  vtkTypeMacro(vtkPiecewiseFunctionLookupTable,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetIntensityFactor( double f );

  // Description:
  // Force the lookup table to regenerate.
  virtual void ForceBuild() override;

  void AddColorPoint( float value, float r, float g, float b );
  void AddAlphaPoint( float value, float alpha );
  void RemoveAllPoints();

  vtkColorTransferFunction * GetColorFunction() { return ColorFunction; }
  vtkPiecewiseFunction * GetAlphaFunction() { return AlphaFunction; }

protected:

    double IntensityFactor;

    vtkColorTransferFunction * ColorFunction;
    vtkPiecewiseFunction * AlphaFunction;

  vtkPiecewiseFunctionLookupTable();
  ~vtkPiecewiseFunctionLookupTable();

private:

  vtkPiecewiseFunctionLookupTable(const vtkPiecewiseFunctionLookupTable&);  // Not implemented.
  void operator=(const vtkPiecewiseFunctionLookupTable&);  // Not implemented.

};


#endif



