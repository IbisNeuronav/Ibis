/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImageDimensionReorder.cxx,v $
  Language:  C++
  Date:      $Date: 2006-03-28 20:17:03 $
  Version:   $Revision: 1.4 $
  Thanks to Simon Drouin who developed this class

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDimensionReorder.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro( vtkImageDimensionReorder, "$Revision: 1.4 $" );
vtkStandardNewMacro( vtkImageDimensionReorder );

//----------------------------------------------------------------------------
vtkImageDimensionReorder::vtkImageDimensionReorder()
{
    this->InputOrder[ 0 ] = 0;
    this->InputOrder[ 1 ] = 1;
    this->InputOrder[ 2 ] = 2;

    this->OutputStep[ 0 ] = 1;
    this->OutputStep[ 1 ] = 1;
    this->OutputStep[ 2 ] = 1;
}

//----------------------------------------------------------------------------
void vtkImageDimensionReorder::ExecuteInformation()
{
    vtkImageData * inData  = vtkImageData::SafeDownCast( this->GetInput() );
    vtkImageData * outData = this->GetOutput();

    int numScalar = inData->GetNumberOfScalarComponents();

    int inputExtent[ 6 ];
    //    inData->GetWholeExtent( inputExtent );
    inData->GetExtent( inputExtent );  // VTK6

    double inputSpacing[ 3 ];
    inData->GetSpacing( inputSpacing );

    int inputDim[ 3 ];
    int inputStep[ 3 ] = { 1, 1, 1 };
    inputStep[ 0 ]     = numScalar;
    inputStep[ 1 ]     = numScalar;
    inputStep[ 2 ]     = numScalar;

    int i;
    for( i = 0; i < 3; i++ )
    {
        inputDim[ i ] = inputExtent[ 2 * i + 1 ] - inputExtent[ 2 * i ] + 1;
        for( int j = 0; j < i; j++ )
        {
            inputStep[ i ] *= inputDim[ j ];
        }
    }

    int outputExtent[ 6 ]     = { 0, 0, 0, 0, 0, 0 };
    double outputSpacing[ 3 ] = { 1.0, 1.0, 1.0 };
    for( i = 0; i < 3; i++ )
    {
        outputExtent[ 2 * i ]                     = inputExtent[ 2 * this->InputOrder[ i ] ];
        outputExtent[ 2 * i + 1 ]                 = inputExtent[ 2 * this->InputOrder[ i ] + 1 ];
        outputSpacing[ i ]                        = inputSpacing[ this->InputOrder[ i ] ];
        this->OutputStep[ this->InputOrder[ i ] ] = inputStep[ i ];
    }

    //    outData->SetWholeExtent( outputExtent );
    outData->SetExtent( outputExtent );  // VTK6
    outData->SetSpacing( outputSpacing );
    outData->SetOrigin( inData->GetOrigin() );
    // VTK6    outData->SetNumberOfScalarComponents( inData->GetNumberOfScalarComponents() );
    // VTK6    outData->SetScalarType( inData->GetScalarType() );
    // VTK6 should I do
    // VTK6    outData->AllocateScalars(?, ?); //VTK6
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class type>
void vtkImageDimensionReorderExecute( vtkImageDimensionReorder * self, type * inptr, type * outptr, int outputStep[ 3 ],
                                      int outputExtent[ 6 ], int numberOfScalarComponent )
{
    type * inIt[ 3 ];
    inIt[ 0 ]          = inptr;
    inIt[ 1 ]          = inptr;
    inIt[ 2 ]          = inptr;
    type * outIt       = outptr;
    int numberOfSlices = outputExtent[ 5 ] - outputExtent[ 4 ] + 1;
    double percent     = 0;

    for( int i = outputExtent[ 4 ]; i <= outputExtent[ 5 ]; i++ )
    {
        inIt[ 1 ] = inIt[ 2 ];
        for( int j = outputExtent[ 2 ]; j <= outputExtent[ 3 ]; j++ )
        {
            inIt[ 0 ] = inIt[ 1 ];
            for( int k = outputExtent[ 0 ]; k <= outputExtent[ 1 ]; k++ )
            {
                for( int l = 0; l < numberOfScalarComponent; l++ )
                {
                    outIt[ l ] = inIt[ 0 ][ l ];
                }
                outIt += numberOfScalarComponent;
                inIt[ 0 ] += outputStep[ 0 ];
            }
            inIt[ 1 ] += outputStep[ 1 ];
        }
        inIt[ 2 ] += outputStep[ 2 ];
        percent = (double)( i - outputExtent[ 4 ] ) / numberOfSlices * 100.0;
        self->UpdateProgress( percent );
    }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageDimensionReorder::SimpleExecute( vtkImageData * inData, vtkImageData * outData )
{
    int outputExtent[ 6 ] = { 0, 0, 0, 0, 0, 0 };
    // VTK6 outData->GetWholeExtent( outputExtent );
    outData->GetExtent( outputExtent );  // VTK6
    int numScalar = inData->GetNumberOfScalarComponents();

    switch( inData->GetScalarType() )
    {
        vtkTemplateMacro( vtkImageDimensionReorderExecute( this, static_cast<VTK_TT *>( inData->GetScalarPointer() ),
                                                           static_cast<VTK_TT *>( outData->GetScalarPointer() ),
                                                           this->OutputStep, outputExtent, numScalar ) );
        default:
            vtkErrorMacro( << "Execute: Unknown ScalarType" );
            return;
    }
}

void vtkImageDimensionReorder::PrintSelf( ostream & os, vtkIndent indent )
{
    this->Superclass::PrintSelf( os, indent );

    os << indent << "Dimension order: " << this->InputOrder[ 0 ] << " " << this->InputOrder[ 1 ] << " "
       << this->InputOrder[ 2 ] << "\n";
}
