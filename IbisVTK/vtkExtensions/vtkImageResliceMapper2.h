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

#ifndef __vtkImageResliceMapper2_h_
#define __vtkImageResliceMapper2_h_

#include "vtkImageResliceMapper.h"

// Description:
// The purpose of this class is only to be able to set the image reslice object
// being used by the vtkImageResliceMapper. We need to be able to set the reslice
// object to share it with other mappers used in different windows to make sure
// we don't reslices the same volumes multiple times during interaction.
// WARNING : sharing vtkImageResliceToColors between mappers won't work if
// ResampleToScreenPixels is on since different windows have different scale levels.
class vtkImageResliceMapper2 : public vtkImageResliceMapper
{

public:

    static vtkImageResliceMapper2 * New();
    vtkTypeMacro( vtkImageResliceMapper2, vtkImageResliceMapper );
    void PrintSelf( ostream & os, vtkIndent indent );

    void SetImageReslice( vtkImageResliceToColors * reslice );

protected:

    vtkImageResliceMapper2();

private:

  vtkImageResliceMapper2(const vtkImageResliceMapper2 & );  // Not implemented.
  void operator=( const vtkImageResliceMapper & );          // Not implemented.

};

#endif
