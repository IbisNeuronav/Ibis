/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImageDimensionReorder.h,v $
  Language:  C++
  Date:      $Date: 2004-05-11 21:17:28 $
  Version:   $Revision: 1.3 $
  Thanks to Simon Drouin who developed this class

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDimensionReorder - Change the storage dimension order
// of an image.
// .SECTION Description
// Change the storage dimension order of an image. By default, do nothing.
// To change the order, specify an InputOrder. For example,
//
// vtkImageDimensionReorder * reorder = vtkImageDimensionReorder::New();
// reorder->SetInputOrder( 2, 1, 0 );
//
// The SetInputOrder call tells the class that the input image is in
// z, y, x order. The filter will always reorder to put it in x,y,z order.

#ifndef VTKIMAGEDIMENSIONREORDER_H
#define VTKIMAGEDIMENSIONREORDER_H

#include "vtkSimpleImageToImageFilter.h"

class vtkImageDimensionReorder : public vtkSimpleImageToImageFilter
{
public:
    static vtkImageDimensionReorder * New();
    vtkTypeRevisionMacro( vtkImageDimensionReorder, vtkSimpleImageToImageFilter );
    void PrintSelf( ostream & os, vtkIndent indent );

    vtkSetVector3Macro( InputOrder, int );
    vtkGetVector3Macro( InputOrder, int );

protected:
    vtkImageDimensionReorder();
    ~vtkImageDimensionReorder(){};

    int InputOrder[3];
    int OutputStep[3];

    void ExecuteInformation();
    void SimpleExecute( vtkImageData * inData, vtkImageData * outData );

private:
    vtkImageDimensionReorder( const vtkImageDimensionReorder & );  // Not implemented.
    void operator=( const vtkImageDimensionReorder & );            // Not implemented.
};

#endif
