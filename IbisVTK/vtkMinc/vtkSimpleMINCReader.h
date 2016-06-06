/*=========================================================================

  Program:   Visualization Toolkit Bic Extension
  Module:    $RCSfile: vtkSimpleMINCReader.h,v $
  Language:  C++
  Date:      $Date: 2008-06-12 15:19:34 $
  Version:   $Revision: 1.3 $

  Copyright (c) 2002-2008 IPL, BIC, MNI, McGill, Simon Drouin, Eric Benoit
  All rights reserved.
  
=========================================================================*/
// .NAME vtkSimpleMINCReader - wrapper for vtkMINCReader and vtkMINCIcv
// .SECTION Description
// Reads a minc image with a simpler but less powerful interface than
// vtkMINCReader.
// .SECTION See Also
// vtkMINCIcv vtkMINCReader

#ifndef __vtkSimpleMINCReader_h_
#define __vtkSimpleMINCReader_h_

#include <vtkImageReader2.h>
#include "vtkMINCReader.h"
#include "vtkMINCIcv.h"

class vtkSimpleMINCReader : public vtkMINCReader
{

public:

    static vtkSimpleMINCReader * New();
    vtkTypeRevisionMacro(vtkSimpleMINCReader,vtkImageReader2);
    
    void SetOutputDataType( int type );
    void SetValidRange( double min, double max );
    void SetImageRange( double min, double max );

    void PrintSelf(ostream &os, vtkIndent indent);

protected:

    vtkSimpleMINCReader();
    ~vtkSimpleMINCReader();

private:

    vtkSimpleMINCReader(const vtkSimpleMINCReader&);  // Not implemented.
    void operator=(const vtkSimpleMINCReader&);  // Not implemented.
};

#endif



